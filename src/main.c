#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stddef.h"

#include "stm32f10x_conf.h"

#include "servoControl.h"
#include "serviceFunction.h"
#include "ringbuff.h"
#include "generalProtocol.h"
#include "usbHIDInterface.h"
#include "usb_user_setings.h"
#include "temperature.h"
#include "math.h"

#include "i2cHandle.h"
#include "externalModuleMid.h"

#define EP_N               1

RingBuff  rxRingBuff;
RingBuff  txRingBuff;
uint8_t   txBuff[BUFF_SIZE];
uint8_t   rxBuff[BUFF_SIZE];

void gpSendCb(uint8_t buff[], uint32_t size);
void gpStopCommandCb(uint8_t channel);
void gpStartClockWiseCommandCb(uint8_t channel);
void gpStartContrClockWiseCommandCb(uint8_t channel);
void gpStartAutoSwitcherCommandCb(uint8_t channel,
                                  uint16_t offTime,
                                  uint16_t onTime,
                                  uint32_t cnt);
void gpSetTemperatureCommandCb(int temperature);
void gpSetMidCommandCb(uint8_t extModuleId,
                       uint8_t midData[],
                       uint8_t size);
void gpGetMidCommandCb(uint8_t extModuleId,
                       uint8_t midData[],
                       uint8_t size);

const GpInitCb gpInitCb = {
    .gpSendCb                       = gpSendCb,
    .gpStopCommandCb                = gpStopCommandCb,
    .gpStartClockWiseCommandCb      = gpStartClockWiseCommandCb,
    .gpStartContrClockWiseCommandCb = gpStartContrClockWiseCommandCb,
    .gpStartAutoSwitcherCommandCb   = gpStartAutoSwitcherCommandCb,
    .gpSetTemperatureCommandCb      = gpSetTemperatureCommandCb,
    .gpSetMidCommandCb              = gpSetMidCommandCb,
    .gpGetMidCommandCb              = gpGetMidCommandCb,
};

uint16_t temperatureToPwm(int temperature)
{
    #define Thermistor_K2C               (273.15f)
    #define THA                          (0.0006728238f)
    #define THB                          (0.0002910997f)
    #define THC                          (8.412704E-11f)
    #define RESISTOR_DIVIDER_VALUE       10000
    #define SUPPLY_VOLTAGE               3.3f
    #define MAX_PWM_VALUE                0xFA0

    double x, y, rThermistor, u;

    x = (1 / THC) * (THA - (1 / (temperature + Thermistor_K2C)));
    y = sqrt(((THB / (3 * THC)) * (THB / (3 * THC)) * (THB / (3 * THC))) + (x / 2) * (x / 2));
    rThermistor = exp(pow(y - x / 2, 1.0f / 3.0f) - pow(y + x / 2, 1.0f / 3.0f));
    u = SUPPLY_VOLTAGE * (rThermistor /  (rThermistor + RESISTOR_DIVIDER_VALUE));
    return (uint16_t)((double)((MAX_PWM_VALUE * u) / SUPPLY_VOLTAGE));
}

void usbHIDRxCB(uint8_t epNumber, uint8_t numRx, uint8_t *rxData)
{
    pushRingBuff(&rxRingBuff, rxData, numRx);
}

void txCompleteCB(void)
{
}

void gpSendCb(uint8_t buff[], uint32_t size)
{
    pushRingBuff(&txRingBuff, buff, size);
}

void gpStopCommandCb(uint8_t channel)
{
    servoControlStop();
    ringBuffClear(&txRingBuff);
}

void gpStartClockWiseCommandCb(uint8_t channel)
{
    servoControlStart(1, CLOCKWISE);
}

void gpStartContrClockWiseCommandCb(uint8_t channel)
{
    servoControlStart(1, COUNTERCLOCKWISE);
}

void gpStartAutoSwitcherCommandCb(uint8_t channel,
                                  uint16_t offTime,
                                  uint16_t onTime,
                                  uint32_t cnt)
{
    powerSwitcherStart(offTime, onTime);
}

void gpSetTemperatureCommandCb(int temperature)
{
   setTemperature(temperatureToPwm(temperature));
}

void gpSetMidCommandCb(uint8_t extModuleId,
                       uint8_t midData[],
                       uint8_t size)
{
    externalModuleMidSet(extModuleId, midData, size);
}

void gpGetMidCommandCb(uint8_t extModuleId,
                       uint8_t midData[],
                       uint8_t size)
{
    externalModuleMidGet(extModuleId, midData, size);
}

void rccConfig(void) {
    RCC_PCLK2Config(RCC_HCLK_Div2);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
}

int main(void)
{
    uint32_t rxSize;
    uint32_t txSize;
    initTestGpio();
    gpInit(&gpInitCb);
    usbHIDInit();
    usbHIDAddRxCB(usbHIDRxCB);
    usbHIDAddTxCompleteCB(txCompleteCB);
    rccConfig();
    uint32_t cnt = 72000;
    while(cnt-- > 2){}
    rccConfig();


    temperatureImitatorInit();

    //initSysTic();
    ringBuffInit(&rxRingBuff, RING_BUFF_DEPTH);
    ringBuffInit(&txRingBuff, RING_BUFF_DEPTH);
    servoControlInit(NULL);
    rccConfig();
    i2cHandleInit();
    externalModuleMidInit();

    while(1)
    {
        if(popRingBuff(&rxRingBuff, rxBuff, &rxSize)) {
            gpDecode(rxBuff, rxSize);
        };

        if(usbHIDEPIsReadyToTx(EP_01) ) {
            if(popRingBuff(&txRingBuff, txBuff, &txSize)) {
                usbHIDTx(EP_01, txBuff, BUFF_SIZE);
            }
        }
    }
}
