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

// ADC channel mapping for GPIO pins that support ADC
// Returns 0xFF if pin doesn't support ADC
static const uint8_t adc_channel_mapping[] = {
    1,     // GPIO 0  (PA1)  - ADC_IN1
    0xFF,  // GPIO 1  (PB12) - No ADC
    0xFF,  // GPIO 2  (PB7)  - No ADC
    0xFF,  // GPIO 3  (PB6)  - No ADC
    0xFF,  // GPIO 4  (PA8)  - No ADC
    0xFF,  // GPIO 5  (PA12) - No ADC
    0xFF,  // GPIO 6  (PA13) - No ADC
    0xFF,  // GPIO 7  (PF1)  - No ADC
    0xFF,  // GPIO 8  (PB5)  - No ADC
    6,     // GPIO 9  (PA6)  - ADC_IN6
    7,     // GPIO 10 (PA7)  - ADC_IN7
    5,     // GPIO 11 (PA5)  - ADC_IN5
    0xFF,  // GPIO 12 (PF0)  - No ADC
    0xFF,  // GPIO 13 (PA14) - No ADC
    2,     // GPIO 14 (PA2)  - ADC_IN2
    3,     // GPIO 15 (PA3)  - ADC_IN3
    0xFF,  // GPIO 16 (PB8)  - No ADC
    0xFF,  // GPIO 17 (PB15) - No ADC
    9,     // GPIO 18 (PB1)  - ADC_IN9
    0xFF,  // GPIO 19 (PA15) - No ADC
    0xFF,  // GPIO 20 (PB9)  - No ADC
    0xFF,  // GPIO 21 (PB10) - No ADC
    0xFF,  // GPIO 22 (PA11) - No ADC
    0xFF,  // GPIO 23 (PB2)  - No ADC
    0xFF,  // GPIO 24 (PB3)  - No ADC
    0xFF,  // GPIO 25 (PB4)  - No ADC
    8,     // GPIO 26 (PB0)  - ADC_IN8
    0xFF,  // GPIO 27 (PB14) - No ADC
};

// Number of GPIO pins in the mapping
#define GPIO_MAPPING_SIZE (sizeof(gpio_mapping) / sizeof(gpio_mapping[0]))

// PWM timer handles
static TIM_HandleTypeDef htim1;
static TIM_HandleTypeDef htim3;
static TIM_HandleTypeDef htim14;
static TIM_HandleTypeDef htim16;
static TIM_HandleTypeDef htim17;

// ADC register base address (from STM32F070x6 reference manual)
#define ADC1_BASE       (0x40012400UL)
#define ADC1            ((ADC_TypeDef *)ADC1_BASE)

// ADC register bit definitions
#define ADC_CR_ADEN     (1U << 0)   // ADC Enable
#define ADC_CR_ADSTART  (1U << 2)   // ADC Start
#define ADC_CR_ADCAL    (1U << 31)  // ADC Calibration

#define ADC_ISR_ADRDY   (1U << 0)   // ADC Ready
#define ADC_ISR_EOC     (1U << 2)   // End of Conversion
#define ADC_ISR_EOCAL   (1U << 11)  // End of Calibration

#define ADC_CFGR1_RES_12BIT   (0U << 3)  // 12-bit resolution
#define ADC_CFGR1_ALIGN       (1U << 5)  // Right alignment

#define ADC_SMPR_SMP_13_5     (2U << 0)  // 13.5 cycles sampling time

// RCC ADC clock enable bit
#define RCC_APB2ENR_ADC1EN    (1U << 9)

// PWM enabled flags and current periods
static uint8_t pwm_enabled[GPIO_MAPPING_SIZE] = {0};
static uint16_t pwm_periods[GPIO_MAPPING_SIZE] = {0};

// ADC enabled flags
static uint8_t adc_enabled[GPIO_MAPPING_SIZE] = {0};

// GPIO mode tracking: 0 = input, 1 = output, 2 = analog
static uint8_t gpio_modes[GPIO_MAPPING_SIZE] = {0};

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
    
    // Disable ADC if it was enabled
    if (adc_enabled[gpio_num]) {
        GPIO_Wrapper_ADC_Disable(gpio_num);
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
    
    if (mode == GPIO_WRAPPER_MODE_ANALOG) {
        GPIO_InitStruct.Mode = GPIO_WRAPPER_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;  // No pull resistors in analog mode
        gpio_modes[gpio_num] = 2; // Analog mode
    }
    else if (mode == GPIO_WRAPPER_MODE_INPUT) {
        GPIO_InitStruct.Mode = GPIO_WRAPPER_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;  // Default to no pull
        gpio_modes[gpio_num] = 0; // Input mode
    }
    else {
        GPIO_InitStruct.Mode = GPIO_WRAPPER_MODE_OUTPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;  // Default to no pull
        gpio_modes[gpio_num] = 1; // Output mode
    }
    
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // Default to low speed
    
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
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
    __HAL_RCC_TIM14_CLK_ENABLE();
    __HAL_RCC_TIM16_CLK_ENABLE();
    __HAL_RCC_TIM17_CLK_ENABLE();
    
    // Initialize TIM1 for GPIO 4 (PA8) and GPIO 22 (PA11)
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock (assuming HSI 8MHz)
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim1);
    
    // Initialize TIM3 for GPIO 9 (PA6), GPIO 10 (PA7), GPIO 18 (PB1), GPIO 26 (PB0)
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim3);
    
    // Initialize TIM14 for GPIO 10 (PA7)
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim14);
    
    // TIM15 not available on STM32F070C6
    
    // Initialize TIM16 for GPIO 9 (PA6), GPIO 16 (PB8)
    htim16.Instance = TIM16;
    htim16.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim16);
    
    // Initialize TIM17 for GPIO 10 (PA7), GPIO 20 (PB9)
    htim17.Instance = TIM17;
    htim17.Init.Prescaler = 7;   // 8MHz / 8 = 1MHz timer clock
    htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim17.Init.Period = 999;    // 1MHz / 1000 = 1kHz PWM frequency
    htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim17);
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
    // TIM3 PWM pins
    else if (gpio_num == 9) { // GPIO 9 (PA6) - TIM3_CH1
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;
        return 1;
    }
    else if (gpio_num == 10) { // GPIO 10 (PA7) - TIM14_CH1
        HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
        GPIO_InitStruct.Pin = GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF4_TIM14;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;
        return 1;
    }
    else if (gpio_num == 18) { // GPIO 18 (PB1) - TIM3_CH4
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;
        return 1;
    }
    else if (gpio_num == 26) { // GPIO 26 (PB0) - TIM3_CH3
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;
        return 1;
    }
    // TIM16 PWM pins (alternative timers for some pins)
    else if (gpio_num == 16) { // GPIO 16 (PB8) - TIM16_CH1
        HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM16;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;
        return 1;
    }
    // TIM17 PWM pins (alternative timers for some pins)
    else if (gpio_num == 20) { // GPIO 20 (PB9) - TIM17_CH1
        HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF0_TIM17;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim17, &sConfigOC, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
        
        pwm_enabled[gpio_num] = 1;
        pwm_periods[gpio_num] = 999;
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
    // TIM3 pins
    else if (gpio_num == 9) { // GPIO 9 (PA6) - TIM3_CH1
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
    else if (gpio_num == 10) { // GPIO 10 (PA7) - TIM14_CH1
        HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
    else if (gpio_num == 18) { // GPIO 18 (PB1) - TIM3_CH4
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
    else if (gpio_num == 26) { // GPIO 26 (PB0) - TIM3_CH3
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
    // TIM16 pins
    else if (gpio_num == 16) { // GPIO 16 (PB8) - TIM16_CH1
        HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
    // TIM17 pins
    else if (gpio_num == 20) { // GPIO 20 (PB9) - TIM17_CH1
        HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        pwm_enabled[gpio_num] = 0;
        pwm_periods[gpio_num] = 0;
    }
}

void GPIO_Wrapper_PWM_SetDutyCycle(uint8_t gpio_num, uint8_t duty_cycle)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !pwm_enabled[gpio_num]) {
        return;
    }
    
    // Clamp duty cycle to 0-100%
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }
    
    uint16_t pulse_value;
    
    // TIM1 pins (GPIO 4, 22)
    if (gpio_num == 4 || gpio_num == 22) {
        uint16_t current_period = __HAL_TIM_GET_AUTORELOAD(&htim1);
        pulse_value = (uint16_t)((duty_cycle * (current_period + 1)) / 100);
        
        if (gpio_num == 4) { // GPIO 4 (PA8) - TIM1_CH1
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_value);
        }
        else if (gpio_num == 22) { // GPIO 22 (PA11) - TIM1_CH4
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse_value);
        }
    }
    // TIM3 pins (GPIO 9, 18, 26)
    else if (gpio_num == 9 || gpio_num == 18 || gpio_num == 26) {
        uint16_t current_period = __HAL_TIM_GET_AUTORELOAD(&htim3);
        pulse_value = (uint16_t)((duty_cycle * (current_period + 1)) / 100);
        
        if (gpio_num == 9) { // GPIO 9 (PA6) - TIM3_CH1
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_value);
        }
        else if (gpio_num == 18) { // GPIO 18 (PB1) - TIM3_CH4
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pulse_value);
        }
        else if (gpio_num == 26) { // GPIO 26 (PB0) - TIM3_CH3
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pulse_value);
        }
    }
    // TIM14 pins (GPIO 10)
    else if (gpio_num == 10) {
        uint16_t current_period = __HAL_TIM_GET_AUTORELOAD(&htim14);
        pulse_value = (uint16_t)((duty_cycle * (current_period + 1)) / 100);
        __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pulse_value);
    }
    // TIM16 pins (GPIO 16)
    else if (gpio_num == 16) {
        uint16_t current_period = __HAL_TIM_GET_AUTORELOAD(&htim16);
        pulse_value = (uint16_t)((duty_cycle * (current_period + 1)) / 100);
        __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, pulse_value);
    }
    // TIM17 pins (GPIO 20)
    else if (gpio_num == 20) {
        uint16_t current_period = __HAL_TIM_GET_AUTORELOAD(&htim17);
        pulse_value = (uint16_t)((duty_cycle * (current_period + 1)) / 100);
        __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, pulse_value);
    }
}

void GPIO_Wrapper_PWM_SetFrequency(uint8_t gpio_num, uint32_t frequency_hz)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !pwm_enabled[gpio_num]) {
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
    
    // Handle TIM1 pins (GPIO 4, 22)
    if (gpio_num == 4 || gpio_num == 22) {
        // Stop both TIM1 channels if they're enabled
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
    // Handle TIM3 pins (GPIO 9, 18, 26)
    else if (gpio_num == 9 || gpio_num == 18 || gpio_num == 26) {
        // Stop all TIM3 channels if they're enabled
        if (pwm_enabled[9]) {
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        }
        if (pwm_enabled[18]) {
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
        }
        if (pwm_enabled[26]) {
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
        }
        
        // Update the shared timer period
        __HAL_TIM_SET_AUTORELOAD(&htim3, new_period);
        
        // Reset duty cycle to 0% when frequency changes (safe default)
        if (pwm_enabled[9]) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
        }
        if (pwm_enabled[18]) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
        }
        if (pwm_enabled[26]) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
        }
        
        // Restart all channels if they were enabled
        if (pwm_enabled[9]) {
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
        }
        if (pwm_enabled[18]) {
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
        }
        if (pwm_enabled[26]) {
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
        }
    }
    // Handle TIM14 pins (GPIO 10)
    else if (gpio_num == 10) {
        HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
        __HAL_TIM_SET_AUTORELOAD(&htim14, new_period);
        __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
        HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
    }
    // Handle TIM16 pins (GPIO 16)
    else if (gpio_num == 16) {
        HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
        __HAL_TIM_SET_AUTORELOAD(&htim16, new_period);
        __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
        HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
    }
    // Handle TIM17 pins (GPIO 20)
    else if (gpio_num == 20) {
        HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);
        __HAL_TIM_SET_AUTORELOAD(&htim17, new_period);
        __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, 0);
        HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
    }
}

uint8_t GPIO_Wrapper_PWM_IsEnabled(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }
    return pwm_enabled[gpio_num];
}

// ADC Implementation using direct register access
void GPIO_Wrapper_ADC_Init(void)
{
    // Enable ADC clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    
    // Configure ADC
    ADC1->CFGR1 = ADC_CFGR1_RES_12BIT;  // 12-bit resolution, right aligned
    ADC1->SMPR = ADC_SMPR_SMP_13_5;     // 13.5 cycles sampling time
    
    // Run ADC calibration
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);    // Wait for calibration to complete
    while (!(ADC1->ISR & ADC_ISR_EOCAL)); // Wait for end of calibration
    
    // Enable ADC
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)); // Wait for ADC ready
}

uint8_t GPIO_Wrapper_ADC_Enable(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }
    
    // Check if pin supports ADC
    if (adc_channel_mapping[gpio_num] == 0xFF) {
        return 0; // Pin doesn't support ADC
    }
    
    // Clean up the pin first
    GPIO_Wrapper_CleanupPin(gpio_num);
    
    // Configure pin as analog input
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
    
    // Mark as ADC enabled and in analog mode
    adc_enabled[gpio_num] = 1;
    gpio_modes[gpio_num] = 2; // Analog mode
    
    return 1; // Success
}

void GPIO_Wrapper_ADC_Disable(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !adc_enabled[gpio_num]) {
        return;
    }
    
    // Reset pin to input mode
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_mapping[gpio_num].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(gpio_mapping[gpio_num].port, &GPIO_InitStruct);
    
    // Reset tracking variables
    adc_enabled[gpio_num] = 0;
    gpio_modes[gpio_num] = 0; // Input mode
}

uint16_t GPIO_Wrapper_ADC_Read(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE || !adc_enabled[gpio_num]) {
        return 0;
    }
    
    // Check if pin supports ADC
    if (adc_channel_mapping[gpio_num] == 0xFF) {
        return 0;
    }
    
    uint8_t channel = adc_channel_mapping[gpio_num];
    
    // Select ADC channel
    ADC1->CHSELR = (1U << channel);
    
    // Start conversion
    ADC1->CR |= ADC_CR_ADSTART;
    
    // Wait for conversion to complete
    uint32_t timeout = 10000;
    while (!(ADC1->ISR & ADC_ISR_EOC) && timeout--) {
        // Wait for end of conversion
    }
    
    if (timeout == 0) {
        return 0; // Timeout error
    }
    
    // Read ADC value
    uint16_t adc_value = (uint16_t)ADC1->DR;
    
    // Clear EOC flag
    ADC1->ISR |= ADC_ISR_EOC;
    
    return adc_value;
}

uint8_t GPIO_Wrapper_ADC_IsEnabled(uint8_t gpio_num)
{
    if (gpio_num >= GPIO_MAPPING_SIZE) {
        return 0;
    }
    return adc_enabled[gpio_num];
} 