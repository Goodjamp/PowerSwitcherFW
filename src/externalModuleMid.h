#ifndef __EXTERNAL_MODULE_MID_H__
#define __EXTERNAL_MODULE_MID_H__

#include "stdint.h"

void externalModuleMidInit(void);

void externalModuleMidSet(uint8_t extModuleId,
                          uint8_t midData[],
                          uint8_t size);

void externalModuleMidGet(uint8_t extModuleId,
                          uint8_t midData[],
                          uint8_t size);

#endif /* __EXTERNAL_MODULE_MID_H__ */
