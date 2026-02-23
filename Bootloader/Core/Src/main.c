/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Simple bootloader for STM32F070C6
  *                   Jumps to application at 0x08002000
  ******************************************************************************
  * @attention
  *
  * Simple bootloader that starts at 0x08000000 and redirects to 0x08002000
  * Future enhancement: Add firmware update capability via UART
  *
  ******************************************************************************
  */

/* Basic type definitions */
typedef unsigned int uint32_t;
typedef volatile unsigned int __IO;

#define __IO volatile

/* Application address - where the main firmware is located */
#define APPLICATION_ADDRESS   0x08002000

/* Function pointer type for the application's reset handler */
typedef void (*pFunction)(void);

/**
  * @brief  Inline assembly functions for ARM Cortex-M0
  */
__attribute__((always_inline)) static inline void __disable_irq(void)
{
    __asm volatile ("cpsid i" : : : "memory");
}

__attribute__((always_inline)) static inline void __set_MSP(uint32_t topOfMainStack)
{
    __asm volatile ("MSR msp, %0" : : "r" (topOfMainStack) : );
}

/**
  * @brief  Jump to application
  * @param  None
  * @retval None
  */
void jump_to_application(void)
{
    uint32_t app_stack_pointer;
    pFunction app_reset_handler;

    /* Get the application stack pointer (first entry in the vector table) */
    app_stack_pointer = *(__IO uint32_t*)APPLICATION_ADDRESS;

    /* Get the application reset handler (second entry in the vector table) */
    app_reset_handler = (pFunction)(*(__IO uint32_t*)(APPLICATION_ADDRESS + 4));

    /* Disable all interrupts */
    __disable_irq();

    /* Set the main stack pointer to the application's stack pointer */
    __set_MSP(app_stack_pointer);

    /* Jump to the application */
    app_reset_handler();
}

/**
  * @brief  Main program - bootloader entry point
  * @retval int (never returns)
  */
int main(void)
{
    /* TODO: Add delay here to check for update command on UART */
    /* TODO: If no update command received within timeout, jump to app */

    /* For now, just jump directly to application */
    jump_to_application();

    /* Should never reach here */
    while (1);
}
