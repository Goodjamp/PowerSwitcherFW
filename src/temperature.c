#include "temperature.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_TIM.h"
#include "stm32f10x_RCC.h"
#include "stm32f10x.h"



void temperatureImitatorInit(void)
{
    //RCC_ClocksTypeDef clock;
    GPIO_InitTypeDef initGpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    DAC_ENABLE_GPIO;

    initGpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    initGpio.GPIO_Pin   = DAC_PIN;
    initGpio.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(DAC_PORT, &initGpio);

    TIM_OCInitTypeDef       pwmInit;
    TIM_TimeBaseInitTypeDef baseInit;

    //RCC_GetClocksFreq(&clock);
    DAC_ENABLE_TIM;
    TIM_TimeBaseStructInit(&baseInit);
    baseInit.TIM_ClockDivision     = TIM_CKD_DIV1;
    baseInit.TIM_CounterMode       = TIM_CounterMode_Up;
    baseInit.TIM_Period            = DAC_TIM_ARP;
    baseInit.TIM_Prescaler         = DAC_BASE_PSC;
    baseInit.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(DAC_TIM, &baseInit);

    TIM_OCStructInit(&pwmInit);
    pwmInit.TIM_OCMode       = TIM_OCMode_PWM1;
    pwmInit.TIM_OutputState  = TIM_OutputState_Enable;
    pwmInit.TIM_OutputNState = TIM_OutputNState_Enable;
    pwmInit.TIM_OCPolarity   = TIM_OCPolarity_High;
    pwmInit.TIM_OCNPolarity  = TIM_OCNPolarity_High;
    pwmInit.TIM_OCIdleState  = TIM_OCNIdleState_Reset;
    pwmInit.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(DAC_TIM, &pwmInit);
    TIM_OC1PolarityConfig(DAC_TIM, TIM_OCPolarity_Low);
    TIM_OC1PreloadConfig(DAC_TIM, TIM_OCPreload_Enable);
    //TIM_GenerateEvent(DAC_TIM, TIM_EventSource_Update);
    TIM_CtrlPWMOutputs(DAC_TIM, ENABLE);


}

void setTimerCompareValue(uint16_t value)
{
    TIM_SetCompare1(DAC_TIM, value);
    TIM_Cmd(DAC_TIM, ENABLE);
}

void generateTemperatureFlashTable(void)
{
    #define TEMPERATURE_TABLE_FLASH_ADDR    0x0800FC00
    #define TEMPERATURE_TABLE_SIZE          50
    #define TEMPERATURE_STEP                5
    #define TEMPERATURE_LOW_TRESHOLD        -45
    #define BUTTON_PIN                      GPIO_Pin_4
    #define BUTTON_PORT                     GPIOB
    #define LED_PIN                         GPIO_Pin_13
    #define LED_PORT                        GPIOC
    uint32_t compareValue = DAC_TIM_ARP - 1;
    GPIO_InitTypeDef initGpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    initGpio.GPIO_Mode  = GPIO_Mode_IPU;
    initGpio.GPIO_Pin   = BUTTON_PIN;
    initGpio.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(BUTTON_PORT, &initGpio);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    initGpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    initGpio.GPIO_Pin   = LED_PIN;
    initGpio.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(LED_PORT, &initGpio);
    GPIO_WriteBit(LED_PORT, LED_PIN, Bit_SET);

    FLASH_Unlock();
    FLASH_ErasePage(TEMPERATURE_TABLE_FLASH_ADDR);
    for (uint32_t i = 0; i < TEMPERATURE_TABLE_SIZE; i++) {
         while(GPIOB->IDR & BUTTON_PIN) {
            if (compareValue == 0) {
                return;
            }
            setTimerCompareValue(--compareValue);
            for (volatile uint32_t j = 0; j < 600000; j++) {
                if (!(GPIOB->IDR & BUTTON_PIN)) {
                    break;
                }
            };
         };
         FLASH_WaitForLastOperation(1000);
         FLASH_ProgramHalfWord(TEMPERATURE_TABLE_FLASH_ADDR + (i * 2), compareValue);
         while(!(GPIOB->IDR & BUTTON_PIN)) {};
         GPIO_WriteBit(LED_PORT, LED_PIN, Bit_RESET);
         for (volatile uint32_t j = 0; j < 200000; j++) {};
         GPIO_WriteBit(LED_PORT, LED_PIN, Bit_SET);
    }
    FLASH_Lock();
}

void setTemperature(uint16_t pwm)
{
    /*
    temperature += -TEMPERATURE_LOW_TRESHOLD;
    temperature /= TEMPERATURE_STEP;
    uint32_t tableValue = *((uint16_t*)(TEMPERATURE_TABLE_FLASH_ADDR + (temperature * 2)));
*/
    setTimerCompareValue(pwm);
}
