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
uint8_t version_string[] = "DoerGPIO_HW0.2_SW0.1\r\n";
#define UART_RX_DMA_BUFFER_SIZE 64
uint8_t uart_rx_dma_buffer[UART_RX_DMA_BUFFER_SIZE];
volatile uint16_t uart_rx_dma_index = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void UART_IdleLine_Callback(void);
void Process_UART_DMA_Buffer(uint16_t length);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

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

    // Check for "VV" command (version request)
    if (stop_index == 2 && uart_rx_dma_buffer[0] == 'V' && uart_rx_dma_buffer[1] == 'V') {
        HAL_UART_Transmit(&huart1, version_string, sizeof(version_string)-1, 1000);
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
