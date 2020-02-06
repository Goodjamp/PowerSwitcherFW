#ifndef __I2C_HANDLE_H__
#define __I2C_HANDLE_H__

#include "stdint.h"

void i2cHandleInit(void);
void i2cHandleUpdateTxData(uint8_t slave, uint8_t data[], uint8_t size);

#endif /* __I2C_HANDLE_H__ */
