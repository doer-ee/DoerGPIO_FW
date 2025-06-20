#include "gpio_wrapper.h"
#include <stdint.h>

// GPIO pin mapping table
static const GPIO_Mapping_t gpio_mapping[] = {
    {GPIOA, GPIO_PIN_1},    // GPIO 0
    {GPIOB, GPIO_PIN_12},   // GPIO 1
    {GPIOB, GPIO_PIN_7},    // GPIO 2
    {GPIOB, GPIO_PIN_6},    // GPIO 3
    {GPIOA, GPIO_PIN_8},    // GPIO 4
    {GPIOA, GPIO_PIN_12},   // GPIO 5
    {GPIOA, GPIO_PIN_13},   // GPIO 6
    {GPIOF, GPIO_PIN_1},    // GPIO 7
    {GPIOB, GPIO_PIN_5},    // GPIO 8
    {GPIOA, GPIO_PIN_6},    // GPIO 9
    {GPIOA, GPIO_PIN_7},    // GPIO 10
    {GPIOA, GPIO_PIN_5},    // GPIO 11
    {GPIOF, GPIO_PIN_0},    // GPIO 12
    {GPIOA, GPIO_PIN_14},   // GPIO 13
    {GPIOA, GPIO_PIN_2},    // GPIO 14
    {GPIOA, GPIO_PIN_3},    // GPIO 15
    {GPIOB, GPIO_PIN_8},    // GPIO 16
    {GPIOB, GPIO_PIN_15},   // GPIO 17
    {GPIOB, GPIO_PIN_1},    // GPIO 18
    {GPIOA, GPIO_PIN_15},   // GPIO 19
    {GPIOB, GPIO_PIN_9},    // GPIO 20
    {GPIOB, GPIO_PIN_10},   // GPIO 21
    {GPIOA, GPIO_PIN_11},   // GPIO 22
    {GPIOB, GPIO_PIN_2},    // GPIO 23
    {GPIOB, GPIO_PIN_3},    // GPIO 24
    {GPIOB, GPIO_PIN_4},    // GPIO 25
    {GPIOB, GPIO_PIN_0},    // GPIO 26
    {GPIOB, GPIO_PIN_14},   // GPIO 27
};

// Number of GPIO pins in the mapping
#define GPIO_MAPPING_SIZE (sizeof(gpio_mapping) / sizeof(gpio_mapping[0]))

void GPIO_Wrapper_Init(void)
{
    // Enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
}

void GPIO_Wrapper_Configure(uint8_t gpio_num, GPIO_Config_t config)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure GPIO pin
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    
    // Set mode
    GPIO_InitStruct.Mode = (config.mode == GPIO_WRAPPER_MODE_INPUT) ? 
                          GPIO_WRAPPER_MODE_INPUT : GPIO_WRAPPER_MODE_OUTPUT;
    
    // Set pull configuration
    switch (config.pull) {
        case GPIO_WRAPPER_PULL_UP:
            GPIO_InitStruct.Pull = GPIO_WRAPPER_PULL_UP;
            break;
        case GPIO_WRAPPER_PULL_DOWN:
            GPIO_InitStruct.Pull = GPIO_WRAPPER_PULL_DOWN;
            break;
        default:
            GPIO_InitStruct.Pull = GPIO_WRAPPER_PULL_NONE;
            break;
    }
    
    // Set speed
    switch (config.speed) {
        case GPIO_WRAPPER_SPEED_MEDIUM:
            GPIO_InitStruct.Speed = GPIO_WRAPPER_SPEED_MEDIUM;
            break;
        case GPIO_WRAPPER_SPEED_HIGH:
            GPIO_InitStruct.Speed = GPIO_WRAPPER_SPEED_HIGH;
            break;
        default:
            GPIO_InitStruct.Speed = GPIO_WRAPPER_SPEED_LOW;
            break;
    }
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
}

void GPIO_Wrapper_SetMode(uint8_t gpio_num, uint8_t mode)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = (mode == GPIO_WRAPPER_MODE_INPUT) ? 
                          GPIO_WRAPPER_MODE_INPUT : GPIO_WRAPPER_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // Keep existing pull configuration
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // Keep existing speed configuration
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
}

void GPIO_Wrapper_SetPull(uint8_t gpio_num, uint8_t pull)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // Keep existing mode
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // Keep existing speed configuration
    
    // Set pull configuration
    switch (pull) {
        case GPIO_WRAPPER_PULL_UP:
            GPIO_InitStruct.Pull = GPIO_WRAPPER_PULL_UP;
            break;
        case GPIO_WRAPPER_PULL_DOWN:
            GPIO_InitStruct.Pull = GPIO_WRAPPER_PULL_DOWN;
            break;
        default:
            GPIO_InitStruct.Pull = GPIO_WRAPPER_PULL_NONE;
            break;
    }
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
}

void GPIO_Wrapper_SetSpeed(uint8_t gpio_num, uint8_t speed)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // Keep existing mode
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // Keep existing pull configuration
    
    // Set speed
    switch (speed) {
        case GPIO_WRAPPER_SPEED_MEDIUM:
            GPIO_InitStruct.Speed = GPIO_WRAPPER_SPEED_MEDIUM;
            break;
        case GPIO_WRAPPER_SPEED_HIGH:
            GPIO_InitStruct.Speed = GPIO_WRAPPER_SPEED_HIGH;
            break;
        default:
            GPIO_InitStruct.Speed = GPIO_WRAPPER_SPEED_LOW;
            break;
    }
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
}

void GPIO_Wrapper_Write(uint8_t gpio_num, uint8_t state)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }
    
    HAL_GPIO_WritePin(gpio_mapping[gpio_num].port, 
                      gpio_mapping[gpio_num].pin, 
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t GPIO_Wrapper_Read(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }
    
    return (HAL_GPIO_ReadPin(gpio_mapping[gpio_num].port, 
                            gpio_mapping[gpio_num].pin) == GPIO_PIN_SET) ? 1 : 0;
}

void GPIO_Wrapper_Toggle(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }
    
    HAL_GPIO_TogglePin(gpio_mapping[gpio_num].port, gpio_mapping[gpio_num].pin);
} 