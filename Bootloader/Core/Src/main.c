/**
  ******************************************************************************
  * @file     : main.c
  * @brief    : STM32F070C6 Bootloader with UART firmware update
  *
  * On every boot:
  *   - Waits 500ms on UART1 (PA9/PA10, 230400 baud) for trigger byte 0x7F
  *   - If trigger received: receives new firmware, flashes to 0x08002000, jumps to app
  *   - If no trigger (timeout): jumps directly to application at 0x08002000
  *
  * Update protocol:
  *   1. Host  -> 0x7F              Board -> 0x79 (ACK)
  *   2. Host  -> 4-byte size (LE)  Board -> 0x79 (ACK)
  *   3. Repeat until done:
  *      Host  -> 64-byte chunk + 1-byte XOR checksum
  *      Board -> 0x79 (ACK) or 0x1F (NAK, resend chunk)
  *   4. Board -> 0x79 (final ACK), then jumps to application
  ******************************************************************************
  */

/* --------------------------------------------------------------------------
 * Type definitions (no stdint.h dependency)
 * -------------------------------------------------------------------------- */
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;

/* --------------------------------------------------------------------------
 * Register definitions
 * -------------------------------------------------------------------------- */

/* RCC */
#define RCC_AHBENR     (*(volatile uint32_t*)0x40021014)
#define RCC_APB2ENR    (*(volatile uint32_t*)0x40021018)

/* GPIOA */
#define GPIOA_MODER    (*(volatile uint32_t*)0x48000000)
#define GPIOA_AFRH     (*(volatile uint32_t*)0x48000024)

/* USART1 */
#define USART1_CR1     (*(volatile uint32_t*)0x40013800)
#define USART1_BRR     (*(volatile uint32_t*)0x4001380C)
#define USART1_ISR     (*(volatile uint32_t*)0x4001381C)
#define USART1_ICR     (*(volatile uint32_t*)0x40013820)
#define USART1_RDR     (*(volatile uint32_t*)0x40013824)
#define USART1_TDR     (*(volatile uint32_t*)0x40013828)

/* Flash */
#define FLASH_KEYR     (*(volatile uint32_t*)0x40022004)
#define FLASH_SR       (*(volatile uint32_t*)0x4002200C)
#define FLASH_CR       (*(volatile uint32_t*)0x40022010)
#define FLASH_AR       (*(volatile uint32_t*)0x40022014)

/* SysTick */
#define STK_CTRL       (*(volatile uint32_t*)0xE000E010)
#define STK_LOAD       (*(volatile uint32_t*)0xE000E014)
#define STK_VAL        (*(volatile uint32_t*)0xE000E018)

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */
#define APP_ADDRESS    0x08002000U
#define PAGE_SIZE      1024U
#define MAX_APP_SIZE   (120U * 1024U)  /* 120 KB application area (128 KB device - 8 KB bootloader) */

#define PROTO_TRIGGER  0x7FU
#define PROTO_ACK      0x79U
#define PROTO_NAK      0x1FU
#define CHUNK_SIZE     64U

#define BOOT_WAIT_MS   500U

#define FLASH_KEY1     0x45670123U
#define FLASH_KEY2     0xCDEF89ABU

/* --------------------------------------------------------------------------
 * SysTick
 * -------------------------------------------------------------------------- */
static volatile uint32_t tick_ms = 0;

/* Overrides weak Default_Handler from startup file */
void SysTick_Handler(void)
{
    tick_ms++;
}

static void systick_init(void)
{
    STK_LOAD = 7999U;   /* 8 MHz / 1000 Hz - 1 */
    STK_VAL  = 0U;
    STK_CTRL = 0x07U;   /* Enable | TickInt | use CPU clock */
}

static uint32_t millis(void)
{
    return tick_ms;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) < ms);
}

/* --------------------------------------------------------------------------
 * UART1 — PA9 TX, PA10 RX, 230400 baud at 8 MHz HSI
 * -------------------------------------------------------------------------- */
static void uart_init(void)
{
    /* Enable clocks */
    RCC_AHBENR  |= (1U << 17);   /* IOPAEN  */
    RCC_APB2ENR |= (1U << 14);   /* USART1EN */

    /* PA9 and PA10: alternate-function mode (MODER = 10) */
    GPIOA_MODER &= ~((3U << 18) | (3U << 20));
    GPIOA_MODER |=  ((2U << 18) | (2U << 20));

    /* AF1 (USART1): PA9 in AFRH[7:4], PA10 in AFRH[11:8] */
    GPIOA_AFRH &= ~((0xFU << 4) | (0xFU << 8));
    GPIOA_AFRH |=  ((1U   << 4) | (1U   << 8));

    /* Baud rate: 8 000 000 / 230 400 = 34.7 -> 35 (0.8% error, within spec) */
    USART1_BRR = 35U;

    /* UE=1 RE=1 TE=1 */
    USART1_CR1 = (1U << 0) | (1U << 2) | (1U << 3);
}

static void uart_send(uint8_t byte)
{
    while (!(USART1_ISR & (1U << 7)));  /* wait TXE */
    USART1_TDR = byte;
}

/**
 * @brief  Receive one byte with a millisecond timeout.
 * @retval 1 on success, 0 on timeout.
 */
static int uart_recv(uint8_t *b, uint32_t timeout_ms)
{
    uint32_t start = millis();

    /* Clear overrun error if set */
    if (USART1_ISR & (1U << 3))
        USART1_ICR = (1U << 3);

    while (!(USART1_ISR & (1U << 5))) {   /* wait RXNE */
        if ((millis() - start) >= timeout_ms)
            return 0;
    }
    *b = (uint8_t)(USART1_RDR & 0xFFU);
    return 1;
}

/* --------------------------------------------------------------------------
 * Flash programming
 * -------------------------------------------------------------------------- */
static void flash_unlock(void)
{
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
}

static void flash_lock(void)
{
    FLASH_CR |= (1U << 7);
}

static void flash_wait(void)
{
    while (FLASH_SR & (1U << 0));   /* poll BSY */
}

static int flash_erase_page(uint32_t addr)
{
    flash_wait();
    FLASH_CR |= (1U << 1);   /* PER */
    FLASH_AR  = addr;
    FLASH_CR |= (1U << 6);   /* STRT */
    flash_wait();

    uint32_t err = FLASH_SR & ((1U << 2) | (1U << 4));  /* PGERR | WRPERR */
    FLASH_SR  = err;                   /* clear error flags (write-1-to-clear) */
    FLASH_CR &= ~(1U << 1);            /* clear PER */
    return (err == 0U);
}

static int flash_write_halfword(uint32_t addr, uint16_t data)
{
    flash_wait();
    FLASH_CR |= (1U << 0);             /* PG */
    *((volatile uint16_t*)addr) = data;
    flash_wait();

    uint32_t err = FLASH_SR & (1U << 2);  /* PGERR */
    FLASH_SR  = err;
    FLASH_CR &= ~(1U << 0);            /* clear PG */
    return (err == 0U);
}

/* --------------------------------------------------------------------------
 * Firmware update handler
 * -------------------------------------------------------------------------- */
static int do_update(void)
{
    uint8_t  buf[CHUNK_SIZE];
    uint8_t  b;
    uint32_t fw_size    = 0U;
    uint32_t bytes_done = 0U;
    uint32_t write_addr = APP_ADDRESS;
    uint32_t erase_addr = APP_ADDRESS;  /* tracks next page to erase */

    /* ACK the trigger */
    uart_send(PROTO_ACK);

    /* Receive firmware size — 4 bytes little-endian */
    for (int i = 0; i < 4; i++) {
        if (!uart_recv(&b, 2000U)) return 0;
        fw_size |= ((uint32_t)b << (i * 8));
    }

    if (fw_size == 0U || fw_size > MAX_APP_SIZE) {
        uart_send(PROTO_NAK);   /* tell host the size is rejected */
        return 0;
    }

    uart_send(PROTO_ACK);

    flash_unlock();

    while (bytes_done < fw_size) {
        uint32_t chunk = fw_size - bytes_done;
        if (chunk > CHUNK_SIZE) chunk = CHUNK_SIZE;

        /* Receive chunk payload */
        for (uint32_t i = 0U; i < chunk; i++) {
            if (!uart_recv(&buf[i], 2000U)) {
                flash_lock();
                return 0;
            }
        }

        /* Pad remainder of chunk with 0xFF (erased flash value) */
        for (uint32_t i = chunk; i < CHUNK_SIZE; i++) buf[i] = 0xFFU;

        /* Receive XOR checksum */
        if (!uart_recv(&b, 2000U)) {
            flash_lock();
            return 0;
        }

        /* Verify checksum */
        uint8_t crc = 0U;
        for (uint32_t i = 0U; i < CHUNK_SIZE; i++) crc ^= buf[i];
        if (crc != b) {
            uart_send(PROTO_NAK);
            continue;  /* ask host to resend this chunk */
        }

        /* Erase flash page(s) just before first write into each page */
        while (erase_addr < (write_addr + CHUNK_SIZE)) {
            if (!flash_erase_page(erase_addr)) {
                uart_send(PROTO_NAK);
                flash_lock();
                return 0;
            }
            erase_addr += PAGE_SIZE;
        }

        /* Write chunk as 16-bit halfwords */
        for (uint32_t i = 0U; i < CHUNK_SIZE; i += 2U) {
            uint16_t hw = (uint16_t)buf[i] | ((uint16_t)buf[i + 1U] << 8U);
            if (!flash_write_halfword(write_addr + i, hw)) {
                uart_send(PROTO_NAK);
                flash_lock();
                return 0;
            }
        }

        bytes_done += chunk;
        write_addr += CHUNK_SIZE;
        uart_send(PROTO_ACK);
    }

    flash_lock();
    uart_send(PROTO_ACK);   /* final ACK — upload complete */
    return 1;
}

/* --------------------------------------------------------------------------
 * Jump to application
 * -------------------------------------------------------------------------- */
typedef void (*AppFunc)(void);

static void boot_disable_irq(void)
{
    __asm volatile ("cpsid i" : : : "memory");
}

static void boot_set_msp(uint32_t sp)
{
    __asm volatile ("MSR msp, %0" : : "r" (sp) : );
}

static void jump_to_app(void)
{
    uint32_t sp    = *((volatile uint32_t*) APP_ADDRESS);
    AppFunc  reset = (AppFunc)(*((volatile uint32_t*)(APP_ADDRESS + 4U)));

    boot_disable_irq();
    STK_CTRL = 0U;     /* disable SysTick */
    boot_set_msp(sp);
    reset();
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */
int main(void)
{
    systick_init();
    uart_init();

    uint8_t trigger;
    if (uart_recv(&trigger, BOOT_WAIT_MS) && trigger == PROTO_TRIGGER) {
        do_update();
    }

    /* Small drain delay so the last UART byte is fully transmitted */
    delay_ms(50U);

    jump_to_app();

    while (1);
}
