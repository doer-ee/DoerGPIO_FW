#ifndef GPIO_WRAPPER_H
#define GPIO_WRAPPER_H

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_i2c.h"
#include "stm32f0xx_hal_tim.h"
#include <stdint.h>

// GPIO pin mapping structure
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} GPIO_Mapping_t;

// GPIO configuration structure
typedef struct {
    uint8_t mode;        // Input/Output/Analog
    uint8_t pull;        // No pull/Pull-up/Pull-down
    uint8_t speed;       // Low/Medium/High speed
} GPIO_Config_t;

// GPIO modes (using HAL definitions)
#define GPIO_WRAPPER_MODE_INPUT      GPIO_MODE_INPUT
#define GPIO_WRAPPER_MODE_OUTPUT     GPIO_MODE_OUTPUT_PP
#define GPIO_WRAPPER_MODE_ANALOG     GPIO_MODE_ANALOG

// GPIO pull configurations
#define GPIO_WRAPPER_PULL_NONE      GPIO_NOPULL
#define GPIO_WRAPPER_PULL_UP        GPIO_PULLUP
#define GPIO_WRAPPER_PULL_DOWN      GPIO_PULLDOWN

// GPIO speed configurations (using HAL definitions)
#define GPIO_WRAPPER_SPEED_LOW      GPIO_SPEED_FREQ_LOW
#define GPIO_WRAPPER_SPEED_MEDIUM   GPIO_SPEED_FREQ_MEDIUM
#define GPIO_WRAPPER_SPEED_HIGH     GPIO_SPEED_FREQ_HIGH

// Function declarations
void GPIO_Wrapper_Init(void);
void GPIO_Wrapper_Configure(uint8_t gpio_num, GPIO_Config_t config);
void GPIO_Wrapper_CleanupPin(uint8_t gpio_num);
void GPIO_Wrapper_SetMode(uint8_t gpio_num, uint8_t mode);
uint8_t GPIO_Wrapper_SetPull(uint8_t gpio_num, uint8_t pull);
void GPIO_Wrapper_SetSpeed(uint8_t gpio_num, uint8_t speed);
void GPIO_Wrapper_Write(uint8_t gpio_num, uint8_t state);
uint8_t GPIO_Wrapper_Read(uint8_t gpio_num);
void GPIO_Wrapper_Toggle(uint8_t gpio_num);

// PWM function declarations
void GPIO_Wrapper_PWM_Init(void);
uint8_t GPIO_Wrapper_PWM_Enable(uint8_t gpio_num);
void GPIO_Wrapper_PWM_Disable(uint8_t gpio_num);
void GPIO_Wrapper_PWM_SetDutyCycle(uint8_t gpio_num, uint8_t duty_cycle);
void GPIO_Wrapper_PWM_SetFrequency(uint8_t gpio_num, uint32_t frequency_hz);
uint8_t GPIO_Wrapper_PWM_IsEnabled(uint8_t gpio_num);

// ADC function declarations
void GPIO_Wrapper_ADC_Init(void);
uint8_t GPIO_Wrapper_ADC_Enable(uint8_t gpio_num);
void GPIO_Wrapper_ADC_Disable(uint8_t gpio_num);
uint16_t GPIO_Wrapper_ADC_Read(uint8_t gpio_num);
uint8_t GPIO_Wrapper_ADC_IsEnabled(uint8_t gpio_num);

#define GPIO_WRAPPER_I2C_BUS_COUNT        2
#define GPIO_WRAPPER_I2C_MAX_WRITE_BYTES  24
#define GPIO_WRAPPER_I2C_MAX_SCAN_RESULTS 32

typedef struct {
    I2C_HandleTypeDef hi2c;
    uint8_t initialized;
    uint8_t supported;
} GPIO_Wrapper_I2C_Bus_t;

void GPIO_Wrapper_I2C_StateInit(void);
uint8_t GPIO_Wrapper_I2C_GetActiveBus(void);
uint8_t GPIO_Wrapper_I2C_Init(uint8_t bus);
uint8_t GPIO_Wrapper_I2C_Scan(uint8_t bus, uint8_t *addresses, uint8_t *count);
uint8_t GPIO_Wrapper_I2C_Write(uint8_t bus, uint8_t addr, const uint8_t *data, uint8_t len);
uint8_t GPIO_Wrapper_I2C_Read(uint8_t bus, uint8_t addr, uint8_t *data, uint8_t len);
uint8_t GPIO_Wrapper_I2C_WriteReg(uint8_t bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len);
uint8_t GPIO_Wrapper_I2C_ReadReg(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

#endif /* GPIO_WRAPPER_H */ 
