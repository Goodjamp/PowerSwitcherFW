#define DAC_TIM         TIM2
#define DAC_ENABLE_TIM  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 ,ENABLE);
#define DAC_BASE_PSC                   0
#define DAC_TIM_ARP                    4000

#define DAC_PORT        GPIOA
#define DAC_PIN         GPIO_Pin_0
#define DAC_ENABLE_GPIO RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
#include "stdint.h"

void temperatureImitatorInit(void);
void setTimerCompareValue(uint16_t value);
void generateTemperatureFlashTable(void);
void setTemperature(uint16_t pwm);
