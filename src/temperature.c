#include "temperature.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_TIM.h"
#include "stm32f10x_RCC.h"
#include "stm32f10x.h"

#define DAC_TIM         TIM2
#define DAC_ENABLE_TIM  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 ,ENABLE);
#define DAC_BASE_PSC                   0
#define DAC_TIM_ARP                    10000

#define DAC_PORT        GPIOA
#define DAC_PIN         GPIO_Pin_0
#define DAC_ENABLE_GPIO RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);

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

void setTemperature(int temperature)
{
    TIM_SetCompare1(DAC_TIM, temperature);
    TIM_Cmd(DAC_TIM, ENABLE);
}
