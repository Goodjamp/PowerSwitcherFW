#include "i2cHandle.h"

#include "stm32f10x_rcc.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#define I2C_SLAVE           I2C1
#define I2C_CLOCK_SPEED     100000 // 100 kHz
#define I2C_ADDRESS_1       0b10100000
#define I2C_ADDRESS_2       0b10100010

#define I2C_PIN_SDA         GPIO_Pin_9
#define I2C_PIN_SCL         GPIO_Pin_8

typedef enum SlaveId {
    SLAVE_ID_0  = 0,
    SLAVE_ID_1  = 1,
    SLAVE_COUNT = 2,
} SlaveId;

static struct {
    #define MAX_BUFF_DATA_LEN   20
    uint8_t rxBuff[MAX_BUFF_DATA_LEN];
    uint8_t txBuff[MAX_BUFF_DATA_LEN];
} slaveStates[SLAVE_COUNT];

void i2cHandleInit(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,  ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    /** init gpio as alternate */
    GPIO_InitTypeDef gpioInitStruct;
    gpioInitStruct.GPIO_Pin = I2C_PIN_SDA;
    gpioInitStruct.GPIO_Speed = GPIO_Speed_10MHz;
    gpioInitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &gpioInitStruct);
    gpioInitStruct.GPIO_Pin = I2C_PIN_SCL;
    GPIO_Init(GPIOB, &gpioInitStruct);
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
    /** configure i2c */
    I2C_InitTypeDef initStruct;
    I2C_StructInit(&initStruct);
    initStruct.I2C_Ack = I2C_Ack_Enable;
    initStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    initStruct.I2C_ClockSpeed = I2C_CLOCK_SPEED;
    initStruct.I2C_Mode = I2C_Mode_I2C;
    initStruct.I2C_OwnAddress1 = I2C_ADDRESS_1;
    initStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_DualAddressCmd(I2C_SLAVE,  ENABLE);
    I2C_OwnAddress2Config(I2C_SLAVE, I2C_ADDRESS_2);
    I2C_StretchClockCmd(I2C_SLAVE, ENABLE);
    I2C_Init(I2C_SLAVE, &initStruct);
    /** Enable i2c */
    I2C_Cmd(I2C_SLAVE, ENABLE);
    /** Enable interrupts */
    I2C_ITConfig(I2C_SLAVE, I2C_IT_BUF, ENABLE);
    I2C_ITConfig(I2C_SLAVE, I2C_IT_EVT, ENABLE);
    I2C_ITConfig(I2C_SLAVE, I2C_IT_ERR, ENABLE);

    /** hardcode test data */
    memset(slaveStates[0].rxBuff, 0xff, sizeof(slaveStates[0].rxBuff));
    memset(slaveStates[0].txBuff, 0xff, sizeof(slaveStates[0].txBuff));
    for (uint8_t i = 0; i < sizeof(slaveStates[1].rxBuff); i++) {
        slaveStates[1].rxBuff[i] = i;
    }
    for (uint8_t i = 0; i < sizeof(slaveStates[1].txBuff); i++) {
        slaveStates[1].txBuff[i] = i;
    }
    //memset(slaveStates[1].rxBuff, 0xff, sizeof(slaveStates[1].rxBuff));
    //memset(slaveStates[1].txBuff, 0xff, sizeof(slaveStates[1].txBuff));
    NVIC_DisableIRQ(I2C1_EV_IRQn);
    NVIC_SetPriority(I2C1_EV_IRQn, 2);
    NVIC_EnableIRQ(I2C1_EV_IRQn);
}

static uint8_t memAddress = 0;
static uint8_t receiveCnt = 0;
static SlaveId activeSlave = 0;

void I2C1_EV_IRQHandler(void)
{
    /*read status registers*/
    uint16_t isr1 = I2C_ReadRegister(I2C_SLAVE, I2C_Register_SR1);
    uint16_t isr2 = I2C_ReadRegister(I2C_SLAVE, I2C_Register_SR2);
    /*check if address match*/
    if ((isr1 & I2C_SR1_ADDR) == I2C_SR1_ADDR) {
        memAddress = 0;
        /*check which slave address match */
        activeSlave = ((isr2 & I2C_SR2_DUALF) == I2C_SR2_DUALF) ? SLAVE_ID_1 : SLAVE_ID_0;
        /*clear address match interrupt*/
        while ((I2C_SLAVE->SR1 & I2C_SR1_ADDR) == I2C_SR1_ADDR) {}
    } /*check stop flag*/
    else if ((isr1 & I2C_SR1_STOPF) == I2C_SR1_STOPF) {
        receiveCnt = 0;
        /*clear stop flag*/
        //I2C_SLAVE->CR1 |= 0x1;
        I2C_Cmd(I2C_SLAVE, ENABLE);
     }
    /*check slave transfer
    Interrupt flag clears after write value to Data register
    */
    if ((isr1 & I2C_SR1_TXE) == I2C_SR1_TXE) {
        if (memAddress > sizeof(slaveStates[activeSlave].txBuff)) {
            I2C_SendData(I2C_SLAVE, 0);
        }
        I2C_SendData(I2C_SLAVE, slaveStates[activeSlave].txBuff[memAddress++]);
    }
    /*check slave receive
    Interrupt flag clears after read value to Data register
    */
    else if ((isr1 & I2C_SR1_RXNE) == I2C_SR1_RXNE) {
        if (receiveCnt == 0) {
            memAddress = I2C_ReceiveData(I2C_SLAVE);
        } else {
            slaveStates[activeSlave].rxBuff[memAddress++] = I2C_ReceiveData(I2C_SLAVE);
        }
        receiveCnt++;
    }
}

void I2C1_ER_IRQHandler(void)
{
       /* Read SR1 register to get I2C error */
    if ((I2C_ReadRegister(I2C_SLAVE, I2C_Register_SR1) & 0xFF00) != 0x00){
        /* Clears error flags */
        I2C_SLAVE->SR1 &= 0x00FF;
    }

    if (I2C_GetFlagStatus(I2C_SLAVE, I2C_FLAG_BUSY)) {
        I2C_ClearFlag(I2C_SLAVE, I2C_FLAG_BUSY);
    }

}
