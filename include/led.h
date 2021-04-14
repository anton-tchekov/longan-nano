#ifndef __LED_H__
#define __LED_H__

#include "gd32vf103.h"

#define GPIO_PORT_LED_R  GPIOC
#define GPIO_PORT_LED_G  GPIOA
#define GPIO_PORT_LED_B  GPIOA
#define LED_GPIO_CLK_R   RCU_GPIOC
#define LED_GPIO_CLK_G   RCU_GPIOA
#define LED_GPIO_CLK_B   RCU_GPIOA
#define GPIO_PIN_LED_R   GPIO_PIN_13
#define GPIO_PIN_LED_G   GPIO_PIN_1
#define GPIO_PIN_LED_B   GPIO_PIN_2

#define LED_R_ON() gpio_bit_reset(GPIO_PORT_LED_R, GPIO_PIN_LED_R);
#define LED_G_ON() gpio_bit_reset(GPIO_PORT_LED_G, GPIO_PIN_LED_G);
#define LED_B_ON() gpio_bit_reset(GPIO_PORT_LED_B, GPIO_PIN_LED_B);

#define LED_R_OFF() gpio_bit_set(GPIO_PORT_LED_R, GPIO_PIN_LED_R);
#define LED_G_OFF() gpio_bit_set(GPIO_PORT_LED_G, GPIO_PIN_LED_G);
#define LED_B_OFF() gpio_bit_set(GPIO_PORT_LED_B, GPIO_PIN_LED_B);

void led_init(void);

#endif

