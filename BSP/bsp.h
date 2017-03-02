#ifndef BSP_H
#define BSP_H

#include "board.h"
#include "stm32l0xx_hal.h"
#include "cmsis_os.h"

void LteOpen();

void LedSet(uint8_t num,uint8_t state);
void BspInit();
void PirLevelSet(uint8_t x);
void LedTog(uint8_t num);
void LteStop();
int fputc(int ch, FILE *f);
#endif