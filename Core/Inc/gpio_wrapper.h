#ifndef GPIO_WRAPPER_H
#define GPIO_WRAPPER_H

#include "stm32f0xx_hal.h"
#include <stdint.h>

// GPIO pin mapping structure
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} GPIO_Mapping_t;

// GPIO configuration structure
typedef struct {
    uint8_t mode;        // Input/Output
    uint8_t pull;        // No pull/Pull-up/Pull-down
    uint8_t speed;       // Low/Medium/High speed
} GPIO_Config_t;

// GPIO modes (using HAL definitions)
#define GPIO_WRAPPER_MODE_INPUT      GPIO_MODE_INPUT
#define GPIO_WRAPPER_MODE_OUTPUT     GPIO_MODE_OUTPUT_PP

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
void GPIO_Wrapper_SetMode(uint8_t gpio_num, uint8_t mode);
void GPIO_Wrapper_SetPull(uint8_t gpio_num, uint8_t pull);
void GPIO_Wrapper_SetSpeed(uint8_t gpio_num, uint8_t speed);
void GPIO_Wrapper_Write(uint8_t gpio_num, uint8_t state);
uint8_t GPIO_Wrapper_Read(uint8_t gpio_num);
void GPIO_Wrapper_Toggle(uint8_t gpio_num);

#endif /* GPIO_WRAPPER_H */ 