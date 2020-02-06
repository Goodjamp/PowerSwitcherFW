#include "externalModuleMid.h"
#include "string.h"

#include "stm32f10x_flash.h"
#include "i2cHandle.h"

#define EXTERNAL_MODULE_MID_FLASH_ADDR      0x0800FC00 // last page of main flash memory
#define MAX_EXT_MODULE_DATA_LEN             20

#define EXTERNAL_MODULE_MID_ADDR_COLDPLATE  EXTERNAL_MODULE_MID_FLASH_ADDR
#define EXTERNAL_MODULE_MID_ADDR_RGB_CAP    (EXTERNAL_MODULE_MID_ADDR_COLDPLATE + MAX_EXT_MODULE_DATA_LEN + 1)


typedef enum ExternalModuleId {
    EXTERNAL_MODULE_COLDPLATE = 0,
    EXTERNAL_MODULE_RGB_CAP   = 1,
    EXTERNAL_MODULE_QUANTITY  = 2
} ExternalModuleId;

#pragma pack (push, 1)
typedef struct {
    uint8_t size;
    uint8_t data[MAX_EXT_MODULE_DATA_LEN];
} MidData;
#pragma pack (pop)

void externalModuleMidInit(void)
{
    i2cHandleUpdateTxData(0, (uint8_t *)EXTERNAL_MODULE_MID_ADDR_COLDPLATE, MAX_EXT_MODULE_DATA_LEN);
    i2cHandleUpdateTxData(1, (uint8_t *)EXTERNAL_MODULE_MID_ADDR_RGB_CAP, MAX_EXT_MODULE_DATA_LEN);
}

void externalModuleMidSet(uint8_t extModuleId, uint8_t midData[], uint8_t size)
{
    if (extModuleId > EXTERNAL_MODULE_QUANTITY
      || size > MAX_EXT_MODULE_DATA_LEN) {
        return;
    }
    union {
        MidData mids[EXTERNAL_MODULE_QUANTITY];
        uint8_t buff[sizeof(MidData) * EXTERNAL_MODULE_QUANTITY];
    } temp;
    memcpy(temp.buff, (void *)EXTERNAL_MODULE_MID_FLASH_ADDR, sizeof(temp.buff));
    FLASH_Unlock();
    if (FLASH_ErasePage(EXTERNAL_MODULE_MID_FLASH_ADDR) != FLASH_COMPLETE) {
        FLASH_Lock();
        return;
    }
    #define FLASH_WAIT_TIMEOUT      1000 // in Ticks
    FLASH_WaitForLastOperation(FLASH_WAIT_TIMEOUT);
    memcpy(temp.mids[extModuleId].data, midData, size);
    temp.mids[extModuleId].size = size;
    for (uint8_t i = 0; i < sizeof(temp.buff); i += sizeof(uint16_t)) {
        if (FLASH_ProgramHalfWord(EXTERNAL_MODULE_MID_FLASH_ADDR + i, *((uint16_t *)&temp.buff[i])) != FLASH_COMPLETE) {
            FLASH_Lock();
            return;
        }
        FLASH_WaitForLastOperation(FLASH_WAIT_TIMEOUT);
    }
    FLASH_Lock();
    for (uint8_t i = 0; i < EXTERNAL_MODULE_QUANTITY; i++) {
        i2cHandleUpdateTxData(i, temp.mids[i].data, sizeof(temp.mids[i].data));
    }
}

void externalModuleMidGet(uint8_t extModuleId, uint8_t midData[], uint8_t size)
{
    if (extModuleId > EXTERNAL_MODULE_QUANTITY) {
        return;
    }
    uint32_t addr = extModuleId == EXTERNAL_MODULE_COLDPLATE
                        ? EXTERNAL_MODULE_MID_ADDR_COLDPLATE
                        : EXTERNAL_MODULE_MID_ADDR_RGB_CAP;
    MidData *midDataStruct = (MidData *)addr;
    memcpy(midData, midDataStruct->data, size);
}
