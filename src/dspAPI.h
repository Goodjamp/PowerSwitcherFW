#ifndef __DSP_API_H__
#define __DSP_API_H__

#include "stdint.h"
#include "stdbool.h"

typedef struct FirFilterConfig {
    double   f0;  //central frequency, Gz
    double   fs;  //sampling rate, Gz
    double   df;  //band with, Gz
    uint32_t q;   // filter order
}FirFilterConfig;

typedef struct FiltrationHandler {
    int32_t *first;  // first element of rezults buff
    int32_t *last;   // last element of rezults buff
    int32_t *write;  // write pointer of buff
    int32_t *coeff;  // filter coefficients
    uint32_t q;       // filter order
} FiltrationHandler;

bool     dspCalcFirLpfCoeff(double coeffList[], FirFilterConfig config);
bool     dspScalingFirFiltrCoeff32_t(double inCoeff[], int32_t outCoeff[], FirFilterConfig config, int32_t maxValue);
bool     dspInitFiltrationHandler(FiltrationHandler *handler,
                                  FirFilterConfig firFilterConfig,
                                  int32_t coeff[],
                                  int32_t buff[]);
bool     dspFiltrationReset(FiltrationHandler *handler);
int32_t dspFiltration(FiltrationHandler *handler, int32_t value);

#endif
