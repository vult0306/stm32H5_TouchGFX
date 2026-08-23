#include "st7789.h"

/* Do TouchGFX sinh ra trong TouchGFXGeneratedHAL.cpp */
extern void DisplayDriver_TransferCompleteCallback(void);

int touchgfxDisplayDriverTransmitActive(void)
{
  return ST7789_IsBusy();
}

void touchgfxDisplayDriverTransmitBlock(const uint8_t *pixels,
                                        uint16_t x, uint16_t y,
                                        uint16_t w, uint16_t h)
{
  ST7789_SetWindow(x, y, w, h);
  ST7789_SendBlockDMA(pixels, (uint32_t)w * h * 2);
}
