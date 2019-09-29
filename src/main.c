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

const GpInitCb gpInitCb = {
    .gpSendCb                       = gpSendCb,
    .gpStopCommandCb                = gpStopCommandCb,
    .gpStartClockWiseCommandCb      = gpStartClockWiseCommandCb,
    .gpStartContrClockWiseCommandCb = gpStartContrClockWiseCommandCb,
    .gpStartAutoSwitcherCommandCb   = gpStartAutoSwitcherCommandCb,
};

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

void rccConfig(void) {
    RCC_PCLK2Config(RCC_HCLK_Div2);
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

    //initSysTic();
    ringBuffInit(&rxRingBuff, RING_BUFF_DEPTH);
    ringBuffInit(&txRingBuff, RING_BUFF_DEPTH);
    servoControlInit(NULL);
    rccConfig();
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
