#ifndef ST7789_H
#define ST7789_H

#include "main.h"

void ST7789_Init(void);
void ST7789_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void ST7789_SendBlockDMA(const uint8_t *pixels, uint32_t nbytes);
int  ST7789_IsBusy(void);

#endif
