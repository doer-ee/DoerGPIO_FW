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

// PWM timer handles
static TIM_HandleTypeDef htim1;
static TIM_HandleTypeDef htim3;

// PWM enabled flags and current periods
static uint8_t pwm_enabled[GPIO_MAPPING_SIZE] = {0};
static uint16_t pwm_periods[GPIO_MAPPING_SIZE] = {0};

// GPIO mode tracking
static uint8_t gpio_modes[GPIO_MAPPING_SIZE] = {0}; // 0 = input, 1 = output

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

void GPIO_Wrapper_CleanupPin(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }
    
    // Disable PWM if it was enabled
    if (pwm_enabled[gpio_num]) {
        GPIO_Wrapper_PWM_Disable(gpio_num);
    }
    
    // Reset pin to basic GPIO state
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // Start with input mode (safe default)
    GPIO_InitStruct.Pull = GPIO_NOPULL;      // No pull resistors
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
    
    // Reset tracking variables
    gpio_modes[gpio_num] = 0; // Input mode
}

void GPIO_Wrapper_SetMode(uint8_t gpio_num, uint8_t mode)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return;
    }

    // Clean up the pin first to ensure consistent state
    GPIO_Wrapper_CleanupPin(gpio_num);
    
    // Now configure the desired mode
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = (mode == GPIO_WRAPPER_MODE_INPUT) ? 
                          GPIO_WRAPPER_MODE_INPUT : GPIO_WRAPPER_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // Default to no pull
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // Default to low speed
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
    
    // Track the mode
    gpio_modes[gpio_num] = (mode == GPIO_WRAPPER_MODE_INPUT) ? 0 : 1;
}

uint8_t GPIO_Wrapper_SetPull(uint8_t gpio_num, uint8_t pull)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }

    // Check if GPIO is in input mode (0 = input, 1 = output)
    if (gpio_modes[gpio_num] != 0) {
        return 0; // Pull resistors can only be set in input mode
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
    return 1; // Success
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

// PWM functions
void GPIO_Wrapper_PWM_Init(void)
{
    // Enable timer clocks
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    // Initialize TIM1 for GPIO 4 (PA8) - TIM1_CH1
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock (assuming HSI 8MHz)
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        // Error handling
        return;
    }
    
    // Initialize TIM3 for GPIO 22 (PA11) - TIM3_CH2 (if available)
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock (assuming HSI 8MHz)
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        // Error handling
        return;
    }
}

uint8_t GPIO_Wrapper_PWM_Enable(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }
    
    // If PWM is already enabled for this pin, just return success
    if (pwm_enabled[gpio_num]) {
        return 1;
    }
    
    // Clean up the pin first to ensure consistent state
    GPIO_Wrapper_CleanupPin(gpio_num);
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    if (gpio_num == 4) { // GPIO 4 (PA8) - TIM1_CH1
        // Stop PWM first in case it was running (ignore errors)
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        
        // Configure PA8 as alternate function
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // Configure PWM channel
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0; // 0% duty cycle initially (safe default)
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        
        // Try to configure channel (ignore errors if already configured)
        HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
        
        // Start PWM (ignore errors if already started)
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;  // Store initial period (1kHz)
        return 1;
    }
    else if (gpio_num == 22) { // GPIO 22 (PA11) - TIM1_CH4
        // Stop PWM first in case it was running (ignore errors)
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
        
        // Configure PA11 as alternate function
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // Configure PWM channel
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0; // 0% duty cycle initially (safe default)
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        
        // Try to configure channel (ignore errors if already configured)
        HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4);
        
        // Start PWM (ignore errors if already started)
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;  // Store initial period (1kHz)
        return 1;
    }
    
    return 0;
}

void GPIO_Wrapper_PWM_Disable(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !pwm_enabled[gpio_num]) {
        return;
    }
    
    if (gpio_num == 4) { // GPIO 4 (PA8) - TIM1_CH1
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        
        // Reconfigure pin as regular GPIO
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
    else if (gpio_num == 22) { // GPIO 22 (PA11) - TIM1_CH4
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
        
        // Reconfigure pin as regular GPIO
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
}

void GPIO_Wrapper_PWM_SetDutyCycle(uint8_t gpio_num, uint8_t duty_cycle)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !pwm_enabled[gpio_num]) {
        return;
    }
    
    // Only GPIO 4 and 22 support PWM (both use TIM1)
    if (gpio_num != 4 && gpio_num != 22) {
        return;
    }
    
    // Clamp duty cycle to 0-100%
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }
    
    // Use the actual timer period (shared between both channels)
    uint16_t current_period = __HAL_TIM_GET_AUTORELOAD(&htim1);
    
    uint16_t pulse_value = (uint16_t)((duty_cycle * (current_period + 1)) / 100);
    
    if (gpio_num == 4) { // GPIO 4 (PA8) - TIM1_CH1
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_value);
    }
    else if (gpio_num == 22) { // GPIO 22 (PA11) - TIM1_CH4
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse_value);
    }
}

void GPIO_Wrapper_PWM_SetFrequency(uint8_t gpio_num, uint32_t frequency_hz)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !pwm_enabled[gpio_num]) {
        return;
    }
    
    // Only GPIO 4 and 22 support PWM (both use TIM1)
    if (gpio_num != 4 && gpio_num != 22) {
        return;
    }
    
    // Clamp frequency to reasonable range (1Hz to 500kHz)
    if (frequency_hz < 1) {
        frequency_hz = 1;
    } else if (frequency_hz > 500000) {
        frequency_hz = 500000;
    }
    
    // Calculate new period based on 1MHz timer clock (prescaler = 7, 8MHz/8 = 1MHz)
    // Period = (1MHz / frequency) - 1
    uint32_t new_period = (1000000 / frequency_hz) - 1;
    
    // Clamp period to 16-bit range
    if (new_period > 65535) {
        new_period = 65535;
    }
    
    // Since GPIO 4 and 22 share TIM1, changing frequency affects both channels
    // Stop both channels if they're enabled
    if (pwm_enabled[4]) {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    }
    if (pwm_enabled[22]) {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
    }
    
    // Update the shared timer period
    __HAL_TIM_SET_AUTORELOAD(&htim1, new_period);
    
    // Reset duty cycle to 0% when frequency changes (safe default)
    if (pwm_enabled[4]) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    }
    if (pwm_enabled[22]) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
    }
    
    // Restart both channels if they were enabled
    if (pwm_enabled[4]) {
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    }
    if (pwm_enabled[22]) {
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    }
}

uint8_t GPIO_Wrapper_PWM_IsEnabled(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }
    return pwm_enabled[gpio_num];
} 