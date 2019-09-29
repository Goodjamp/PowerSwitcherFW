#include "stdint.h"
#include "math.h"
#include "stdio.h"
#include "string.h"

#include "dspAPI.h"
#define  MAX_INT8 ((uint8_t)(0 - 1) >> 1)
#define  MAX_INT16 ((uint16_t)(0 - 1) >> 1)
#define  MAX_INT32 ((uint32_t)(0 - 1) >> 1)



bool dspCalcFIRLPCoeff(double coeffList[], FirFilterConfig config)
{
    double   arg = 0;
    for(uint32_t k = 0; k < config.q; k++) {
        arg = 2 * M_PI * (1 / (config.fs)) * (k - ((double)config.q / 2) + 0.5) * config.df;
        coeffList[k] = sinf(arg) / arg;
    }
    return true;
}

bool dspScalingCoeff32_t(double inCoeff[], int32_t outCoeff[], FirFilterConfig config, int32_t maxValue)
{
    double scalingCoeff = (double)MAX_INT32 / ((float)config.q * (float)maxValue);
    for(uint32_t k = 0; k < config.q; k++) {
        outCoeff[k] = (int32_t)(inCoeff[k] * scalingCoeff);
    }
    return true;
}

bool dspInitFiltrationHandler(FiltrationHandler *handler,
                              FirFilterConfig firFilterConfig,
                              int32_t coeff[],
                              int32_t buff[])
{
    handler->q     = firFilterConfig.q;
    handler->first = buff;
    handler->last  = &buff[handler->q - 1];
    handler->write = handler->first;
    handler->coeff = coeff;
    memset((uint8_t*)(handler->first), 0, (handler->q) * sizeof(uint32_t));
    int32_t temp;
    for(uint32_t k = 0; k < handler->q / 2; k++) {
        temp = handler->coeff[k];
        handler->coeff[k] = handler->coeff[handler->q - 1 - k];
        handler->coeff[handler->q - 1 - k] = temp;
    }
    return true;
}

bool dspFiltrationReset(FiltrationHandler *handler)
{
    handler->write = handler->first;
    memset(handler->first, 0, handler->q);
    return true;
}

int32_t dspFiltration(FiltrationHandler *handler, int32_t value)
{
    *handler->write = value;
    *handler->write++;
    if(handler->write > handler->last) {
        handler->write = handler->first;
    };
    int32_t *valueBuff = handler->write;
    int32_t *coeff = handler->coeff;
    int32_t rez = 0;
    while(valueBuff <= handler->last) {
        rez += (*valueBuff++) * (*coeff++);
    }
    valueBuff = handler->first;
    while(valueBuff < handler->write) {
        rez += (*valueBuff++) * (*coeff++);
    }
    return rez;
}
