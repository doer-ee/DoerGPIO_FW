/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "gpio_wrapper.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

uint8_t uart_tx_init[] = "Hello\r\n";
uint8_t uart_rx_recived[] = "Received\r\n";
#define UART_RX_BUFFER_SIZE 64
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
uint8_t uart_rx_index = 0;
uint8_t version_string[] = "DoerGPIO_HW0.2_SW0.2\r\n";
#define UART_RX_DMA_BUFFER_SIZE 64
uint8_t uart_rx_dma_buffer[UART_RX_DMA_BUFFER_SIZE];
volatile uint16_t uart_rx_dma_index = 0;
static uint8_t i2c_bus_initialized[GPIO_WRAPPER_I2C_BUS_COUNT] = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void UART_IdleLine_Callback(void);
void Process_UART_DMA_Buffer(uint16_t length);
void Process_I2C_Command(uint16_t length);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
#define VECTOR_TABLE_SIZE 48  // Covers 0xC0 bytes (16 + IRQs)
#define APP_VECTOR_TABLE  ((uint32_t*)0x08002000)
#define RAM_VECTOR_TABLE  ((uint32_t*)0x20000000)
void relocate_vector_table_to_ram(void)
{
	for (uint32_t i = 0; i < VECTOR_TABLE_SIZE; i++) {
		RAM_VECTOR_TABLE[i] = APP_VECTOR_TABLE[i];
	}
	__HAL_SYSCFG_REMAPMEMORY_SRAM();
}

int main(void)
{
	__enable_irq();
	relocate_vector_table_to_ram();
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  GPIO_Wrapper_Init();
  GPIO_Wrapper_PWM_Init();
  GPIO_Wrapper_ADC_Init();
  GPIO_Wrapper_I2C_StateInit();
  GPIO_Config_t config = {
      .mode = GPIO_WRAPPER_MODE_OUTPUT,
      .pull = GPIO_WRAPPER_PULL_UP,
      .speed = GPIO_WRAPPER_SPEED_LOW
  };
  GPIO_Wrapper_Configure(18, config);
  /* USER CODE BEGIN 2 */

  // Start UART reception in DMA mode
  HAL_UART_Receive_DMA(&huart1, uart_rx_dma_buffer, UART_RX_DMA_BUFFER_SIZE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//    if(HAL_UART_Receive(&huart1, uart_rx_buffer, 1, 100)==HAL_OK)  // Shorter timeout
//    {
//        // Only process valid data (not 0x00 or 0xFF which might indicate noise)
//        if(uart_rx_buffer[0] != 0x00 && uart_rx_buffer[0] != 0xFF)
//        {
//            HAL_UART_Transmit(&huart1, uart_rx_recived, sizeof(uart_rx_recived)-1,1000);
//        }
//    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 230400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  // Enable UART Idle Line interrupt
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

// Add a function to be called from the UART Idle Line interrupt
void UART_IdleLine_Callback(void) {
    // Get the number of bytes received so far
    uint16_t dma_received = UART_RX_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx);
    
    // Process the buffer up to the received length
    Process_UART_DMA_Buffer(dma_received);
}

// Add function to process UART DMA buffer for commands
void Process_UART_DMA_Buffer(uint16_t length) {
    // Find the stop char 'Z' in the buffer
    uint16_t stop_index = 0;
    bool found_z = false;
    for (; stop_index < length; ++stop_index) {
        if (uart_rx_dma_buffer[stop_index] == 'Z') {
            found_z = true;
            break;
        }
    }
    
    // Only process if we found 'Z' (complete message received)
    if (!found_z) {
        // Message not complete yet, don't process
        return;
    }

    if (uart_rx_dma_buffer[0] == '@') {
        Process_I2C_Command(stop_index);
        memset(uart_rx_dma_buffer, 0, UART_RX_DMA_BUFFER_SIZE);
        HAL_UART_DMAStop(&huart1);
        HAL_UART_Receive_DMA(&huart1, uart_rx_dma_buffer, UART_RX_DMA_BUFFER_SIZE);
        return;
    }

    // Check for "VV" command (version request)
    if (stop_index == 2 && uart_rx_dma_buffer[0] == 'V' && uart_rx_dma_buffer[1] == 'V') {
        HAL_UART_Transmit(&huart1, version_string, sizeof(version_string)-1, 1000);
    } else if (stop_index == 2 && uart_rx_dma_buffer[0] == 'R' && uart_rx_dma_buffer[1] == 'R') {
        NVIC_SystemReset();
    } else {
        // Process buffer in pairs up to stop_index
        char collective_response[1024] = {0}; // A large buffer for all responses
        int current_len = 0;
        for (uint16_t i = 0; i + 1 < stop_index; i += 2) {
            uint8_t io_char = uart_rx_dma_buffer[i];
            uint8_t cmd_char = uart_rx_dma_buffer[i+1];
            // Only process if IO char is in valid range
            if (io_char >= 'a' && io_char <= '|') {
                uint8_t gpio_num = io_char - 'a';
                char response[64];
                int len = 0;
                switch(cmd_char) {
                    case 'I':
                        GPIO_Wrapper_SetMode(gpio_num, GPIO_WRAPPER_MODE_INPUT);
                        len = sprintf(response, "GPIO%d: Input", gpio_num);
                        break;
                    case 'O':
                        GPIO_Wrapper_SetMode(gpio_num, GPIO_WRAPPER_MODE_OUTPUT);
                        len = sprintf(response, "GPIO%d: Output", gpio_num);
                        break;
                    case 'A': {
                        uint8_t result = GPIO_Wrapper_ADC_Enable(gpio_num);
                        if (result) {
                            len = sprintf(response, "GPIO%d: ADC enabled", gpio_num);
                        } else {
                            len = sprintf(response, "GPIO%d: ADC not supported", gpio_num);
                        }
                        break;
                    }
                    case '1':
                        GPIO_Wrapper_Write(gpio_num, 1);
                        len = sprintf(response, "GPIO%d: HIGH", gpio_num);
                        break;
                    case '0':
                        GPIO_Wrapper_Write(gpio_num, 0);
                        len = sprintf(response, "GPIO%d: LOW", gpio_num);
                        break;
                    case '?': {
                        uint8_t value = GPIO_Wrapper_Read(gpio_num);
                        len = sprintf(response, "GPIO%d: %d", gpio_num, value);
                        break;
                    }
                    case 'R': {
                        if (GPIO_Wrapper_ADC_IsEnabled(gpio_num)) {
                            uint16_t adc_value = GPIO_Wrapper_ADC_Read(gpio_num);
                            len = sprintf(response, "GPIO%d: ADC %d", gpio_num, adc_value);
                        } else {
                            len = sprintf(response, "GPIO%d: ADC not enabled", gpio_num);
                        }
                        break;
                    }
                    case 'D': {
                        uint8_t result = GPIO_Wrapper_SetPull(gpio_num, GPIO_WRAPPER_PULL_DOWN);
                        if (result) {
                            len = sprintf(response, "GPIO%d: Pull-down", gpio_num);
                        } else {
                            len = sprintf(response, "GPIO%d: Pull-down skipped (input only)", gpio_num);
                        }
                        break;
                    }
                    case 'U': {
                        uint8_t result = GPIO_Wrapper_SetPull(gpio_num, GPIO_WRAPPER_PULL_UP);
                        if (result) {
                            len = sprintf(response, "GPIO%d: Pull-up", gpio_num);
                        } else {
                            len = sprintf(response, "GPIO%d: Pull-up skipped (input only)", gpio_num);
                        }
                        break;
                    }
                    case 'N': {
                        uint8_t result = GPIO_Wrapper_SetPull(gpio_num, GPIO_WRAPPER_PULL_NONE);
                        if (result) {
                            len = sprintf(response, "GPIO%d: No pull", gpio_num);
                        } else {
                            len = sprintf(response, "GPIO%d: No pull skipped (input only)", gpio_num);
                        }
                        break;
                    }
                    case 'P': {
                        uint8_t result = GPIO_Wrapper_PWM_Enable(gpio_num);
                        if (result) {
                            len = sprintf(response, "GPIO%d: PWM enabled", gpio_num);
                        } else {
                            len = sprintf(response, "GPIO%d: PWM not supported", gpio_num);
                        }
                        break;
                    }
                    case 'F': {
                        // Extract frequency value from next bytes in buffer
                        if (i + 3 < stop_index) { // Need at least 2 more bytes for frequency
                            uint32_t frequency = 0;
                            // Parse frequency from next bytes (up to 6 digits for 500000 max)
                            for (uint8_t j = i + 2; j < stop_index && j < i + 8; j++) {
                                if (uart_rx_dma_buffer[j] >= '0' && uart_rx_dma_buffer[j] <= '9') {
                                    frequency = frequency * 10 + (uart_rx_dma_buffer[j] - '0');
                                } else {
                                    break; // Stop at first non-digit
                                }
                            }
                            if (GPIO_Wrapper_PWM_IsEnabled(gpio_num)) {
                                GPIO_Wrapper_PWM_SetFrequency(gpio_num, frequency);
                                if (gpio_num == 4 || gpio_num == 22) {
                                    len = sprintf(response, "GPIO%d: Freq %luHz (shared GPIO4/22)", gpio_num, frequency);
                                } else {
                                    len = sprintf(response, "GPIO%d: Freq %luHz", gpio_num, frequency);
                                }
                            } else {
                                len = sprintf(response, "GPIO%d: PWM not enabled", gpio_num);
                            }
                        } else {
                            len = sprintf(response, "GPIO%d: Missing freq value", gpio_num);
                        }
                        break;
                    }
                    case 'C': {
                        // Extract duty cycle value from next bytes in buffer
                        if (i + 2 < stop_index) { // Need at least 1 more byte for duty cycle
                            uint8_t duty_cycle = 0;
                            // Parse duty cycle from next bytes (assuming decimal ASCII)
                            for (uint8_t j = i + 2; j < stop_index && j < i + 5; j++) {
                                if (uart_rx_dma_buffer[j] >= '0' && uart_rx_dma_buffer[j] <= '9') {
                                    duty_cycle = duty_cycle * 10 + (uart_rx_dma_buffer[j] - '0');
                                } else {
                                    break; // Stop at first non-digit
                                }
                            }
                            if (GPIO_Wrapper_PWM_IsEnabled(gpio_num)) {
                                GPIO_Wrapper_PWM_SetDutyCycle(gpio_num, duty_cycle);
                                len = sprintf(response, "GPIO%d: Duty %d%%", gpio_num, duty_cycle);
                            } else {
                                len = sprintf(response, "GPIO%d: PWM not enabled", gpio_num);
                            }
                        } else {
                            len = sprintf(response, "GPIO%d: Missing duty value", gpio_num);
                        }
                        break;
                    }
                    default:
                        len = sprintf(response, "GPIO%d: Invalid cmd", gpio_num);
                        break;
                }
                if (len > 0) {
                     if (current_len > 0) {
                        current_len += sprintf(collective_response + current_len, ";");
                    }
                    current_len += sprintf(collective_response + current_len, "%s", response);
                }
            }
        }

        if (current_len > 0) {
            // Add final newline
            current_len += sprintf(collective_response + current_len, "\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)collective_response, current_len, 1000);
        }
    }
    
    // Clear the buffer after processing
    memset(uart_rx_dma_buffer, 0, UART_RX_DMA_BUFFER_SIZE);
    // Stop DMA before restarting to ensure clean restart
    HAL_UART_DMAStop(&huart1);
    // Restart DMA reception for next message
    HAL_UART_Receive_DMA(&huart1, uart_rx_dma_buffer, UART_RX_DMA_BUFFER_SIZE);
}

static int hex_value(uint8_t ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

static bool parse_hex_byte_pair(uint8_t hi, uint8_t lo, uint8_t *out)
{
    int high = hex_value(hi);
    int low = hex_value(lo);

    if (high < 0 || low < 0 || out == NULL) {
        return false;
    }

    *out = (uint8_t)((high << 4) | low);
    return true;
}

static void send_i2c_response(uint8_t bus, const char *message)
{
    char response[160];
    int len = snprintf(response, sizeof(response), "I2C%d: %s\r\n", bus, message);

    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)response, (uint16_t)len, 1000);
    }
}

void Process_I2C_Command(uint16_t length)
{
    uint8_t bus;
    char opcode[3] = {0};

    if (length < 4 || uart_rx_dma_buffer[0] != '@') {
        send_i2c_response(0, "ERR ARG");
        return;
    }

    if (uart_rx_dma_buffer[1] < '0' || uart_rx_dma_buffer[1] > '9') {
        send_i2c_response(0, "ERR BUS");
        return;
    }

    bus = (uint8_t)(uart_rx_dma_buffer[1] - '0');
    opcode[0] = (char)uart_rx_dma_buffer[2];
    opcode[1] = (char)uart_rx_dma_buffer[3];

    if (bus == 0 || bus > GPIO_WRAPPER_I2C_BUS_COUNT) {
        send_i2c_response(bus, "ERR BUS");
        return;
    }

    if (strcmp(opcode, "IN") == 0) {
        uint8_t i;

        if (length != 4) {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        for (i = 0; i < GPIO_WRAPPER_I2C_BUS_COUNT; ++i) {
            i2c_bus_initialized[i] = 0;
        }

        if (GPIO_Wrapper_I2C_Init(bus) && GPIO_Wrapper_I2C_GetActiveBus() == bus) {
            i2c_bus_initialized[bus - 1] = 1;
            send_i2c_response(bus, "OK INIT");
        } else {
            send_i2c_response(bus, "ERR BUS");
        }
        return;
    }

    if (!i2c_bus_initialized[bus - 1]) {
        send_i2c_response(bus, "ERR NOT_INIT");
        return;
    }

    if (strcmp(opcode, "SC") == 0) {
        uint8_t addresses[GPIO_WRAPPER_I2C_MAX_SCAN_RESULTS] = {0};
        uint8_t count = 0;
        char response[160];
        int offset = 0;
        uint8_t i;

        if (length != 4) {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        if (!GPIO_Wrapper_I2C_Scan(bus, addresses, &count)) {
            send_i2c_response(bus, "ERR NOT_INIT");
            return;
        }

        if (count == 0) {
            send_i2c_response(bus, "SCAN NONE");
            return;
        }

        offset = snprintf(response, sizeof(response), "I2C%d: SCAN", bus);
        for (i = 0; i < count && offset > 0 && offset < (int)sizeof(response); ++i) {
            offset += snprintf(response + offset, sizeof(response) - (size_t)offset, " %02X", addresses[i]);
        }
        if (offset > 0 && offset < (int)sizeof(response)) {
            offset += snprintf(response + offset, sizeof(response) - (size_t)offset, "\r\n");
        }
        if (offset > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)response, (uint16_t)offset, 1000);
        }
        return;
    }

    if (strcmp(opcode, "WR") == 0) {
        uint8_t addr;
        uint8_t payload[GPIO_WRAPPER_I2C_MAX_WRITE_BYTES] = {0};
        uint8_t payload_len = 0;
        uint16_t index;

        if (length < 7 || !parse_hex_byte_pair(uart_rx_dma_buffer[4], uart_rx_dma_buffer[5], &addr) || uart_rx_dma_buffer[6] != ',') {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        if (((length - 7) == 0) || (((length - 7) % 2) != 0)) {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        for (index = 7; index + 1 < length; index += 2) {
            if (payload_len >= GPIO_WRAPPER_I2C_MAX_WRITE_BYTES || !parse_hex_byte_pair(uart_rx_dma_buffer[index], uart_rx_dma_buffer[index + 1], &payload[payload_len])) {
                send_i2c_response(bus, "ERR ARG");
                return;
            }
            payload_len++;
        }

        if (GPIO_Wrapper_I2C_Write(bus, addr, payload, payload_len)) {
            send_i2c_response(bus, "OK WRITE");
        } else {
            send_i2c_response(bus, "ERR NACK");
        }
        return;
    }

    if (strcmp(opcode, "RD") == 0) {
        uint8_t addr;
        uint8_t read_len;
        uint8_t data[GPIO_WRAPPER_I2C_MAX_WRITE_BYTES] = {0};
        char response[160];
        int offset;
        uint8_t i;

        if (length != 9 || !parse_hex_byte_pair(uart_rx_dma_buffer[4], uart_rx_dma_buffer[5], &addr) || uart_rx_dma_buffer[6] != ',' || !parse_hex_byte_pair(uart_rx_dma_buffer[7], uart_rx_dma_buffer[8], &read_len) || read_len > GPIO_WRAPPER_I2C_MAX_WRITE_BYTES) {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        if (!GPIO_Wrapper_I2C_Read(bus, addr, data, read_len)) {
            send_i2c_response(bus, "ERR NACK");
            return;
        }

        offset = snprintf(response, sizeof(response), "I2C%d: RX", bus);
        for (i = 0; i < read_len && offset > 0 && offset < (int)sizeof(response); ++i) {
            offset += snprintf(response + offset, sizeof(response) - (size_t)offset, " %02X", data[i]);
        }
        if (offset > 0 && offset < (int)sizeof(response)) {
            offset += snprintf(response + offset, sizeof(response) - (size_t)offset, "\r\n");
        }
        if (offset > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)response, (uint16_t)offset, 1000);
        }
        return;
    }

    if (strcmp(opcode, "MW") == 0) {
        uint8_t addr;
        uint8_t reg;
        uint8_t payload[GPIO_WRAPPER_I2C_MAX_WRITE_BYTES] = {0};
        uint8_t payload_len = 0;
        uint16_t index;

        if (length < 10 || !parse_hex_byte_pair(uart_rx_dma_buffer[4], uart_rx_dma_buffer[5], &addr) || uart_rx_dma_buffer[6] != ',' || !parse_hex_byte_pair(uart_rx_dma_buffer[7], uart_rx_dma_buffer[8], &reg) || uart_rx_dma_buffer[9] != ',') {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        if (((length - 10) == 0) || (((length - 10) % 2) != 0)) {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        for (index = 10; index + 1 < length; index += 2) {
            if (payload_len >= GPIO_WRAPPER_I2C_MAX_WRITE_BYTES || !parse_hex_byte_pair(uart_rx_dma_buffer[index], uart_rx_dma_buffer[index + 1], &payload[payload_len])) {
                send_i2c_response(bus, "ERR ARG");
                return;
            }
            payload_len++;
        }

        if (GPIO_Wrapper_I2C_WriteReg(bus, addr, reg, payload, payload_len)) {
            send_i2c_response(bus, "OK WRITE_REG");
        } else {
            send_i2c_response(bus, "ERR NACK");
        }
        return;
    }

    if (strcmp(opcode, "MR") == 0) {
        uint8_t addr;
        uint8_t reg;
        uint8_t read_len;
        uint8_t data[GPIO_WRAPPER_I2C_MAX_WRITE_BYTES] = {0};
        char response[160];
        int offset;
        uint8_t i;

        if (length != 12 || !parse_hex_byte_pair(uart_rx_dma_buffer[4], uart_rx_dma_buffer[5], &addr) || uart_rx_dma_buffer[6] != ',' || !parse_hex_byte_pair(uart_rx_dma_buffer[7], uart_rx_dma_buffer[8], &reg) || uart_rx_dma_buffer[9] != ',' || !parse_hex_byte_pair(uart_rx_dma_buffer[10], uart_rx_dma_buffer[11], &read_len) || read_len > GPIO_WRAPPER_I2C_MAX_WRITE_BYTES) {
            send_i2c_response(bus, "ERR ARG");
            return;
        }

        if (!GPIO_Wrapper_I2C_ReadReg(bus, addr, reg, data, read_len)) {
            send_i2c_response(bus, "ERR NACK");
            return;
        }

        offset = snprintf(response, sizeof(response), "I2C%d: RX", bus);
        for (i = 0; i < read_len && offset > 0 && offset < (int)sizeof(response); ++i) {
            offset += snprintf(response + offset, sizeof(response) - (size_t)offset, " %02X", data[i]);
        }
        if (offset > 0 && offset < (int)sizeof(response)) {
            offset += snprintf(response + offset, sizeof(response) - (size_t)offset, "\r\n");
        }
        if (offset > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)response, (uint16_t)offset, 1000);
        }
        return;
    }

    send_i2c_response(bus, "ERR ARG");
}
