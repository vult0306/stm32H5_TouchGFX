#include "st7789.h"

extern SPI_HandleTypeDef hspi1;
/* Do TouchGFX sinh ra trong TouchGFXGeneratedHAL.cpp (extern "C") */
extern void DisplayDriver_TransferCompleteCallback(void);
static volatile int lcd_busy = 0;

#define CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_RESET)
#define CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_SET)
#define DC_CMD()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_RESET)
#define DC_DATA()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_SET)

/* SPI cau hinh 16-bit cho pixel; lenh va tham so la 8-bit -> doi qua lai */
static void spi_width(uint32_t datasize)
{
  if (hspi1.Init.DataSize == datasize) return;
  hspi1.Init.DataSize = datasize;
  HAL_SPI_Init(&hspi1);
}

static void wr_cmd(uint8_t cmd)
{
  spi_width(SPI_DATASIZE_8BIT);
  DC_CMD(); CS_LOW();
  HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
  CS_HIGH();
}

static void wr_data(const uint8_t *d, uint16_t n)
{
  if (n == 0) return;
  spi_width(SPI_DATASIZE_8BIT);
  DC_DATA(); CS_LOW();
  HAL_SPI_Transmit(&hspi1, (uint8_t *)d, n, 100);
  CS_HIGH();
}

static void wr_cmd_p(uint8_t cmd, const uint8_t *p, uint16_t n)
{
  wr_cmd(cmd);
  wr_data(p, n);
}

void ST7789_Init(void)
{
  /* Reset cung */
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120);

  wr_cmd(0x01);              /* SWRESET  */  HAL_Delay(120);
  wr_cmd(0x11);              /* SLPOUT   */  HAL_Delay(120);

  { uint8_t v = 0x55; wr_cmd_p(0x3A, &v, 1); }   /* COLMOD = 16 bit/pixel  */
  { uint8_t v = 0x00; wr_cmd_p(0x36, &v, 1); }   /* MADCTL: xoay man o day */

  wr_cmd(0x21);              /* INVON — BAT BUOC voi panel IPS 240x240 */
  wr_cmd(0x13);              /* NORON    */  HAL_Delay(10);
  wr_cmd(0x29);              /* DISPON   */  HAL_Delay(10);

  /* Xoa GRAM ve den truoc khi bat den nen */
  ST7789_SetWindow(0, 0, 240, 240);
  spi_width(SPI_DATASIZE_16BIT);
  DC_DATA(); CS_LOW();
  { static const uint16_t zero[240] = {0};
    for (int i = 0; i < 240; i++)
      HAL_SPI_Transmit(&hspi1, (uint8_t *)zero, 240, 100); }
  CS_HIGH();

  HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_SET);
}

void ST7789_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t x1 = x + w - 1, y1 = y + h - 1;
  uint8_t b[4];

  b[0] = x  >> 8; b[1] = x  & 0xFF;
  b[2] = x1 >> 8; b[3] = x1 & 0xFF;
  wr_cmd_p(0x2A, b, 4);                /* CASET */

  b[0] = y  >> 8; b[1] = y  & 0xFF;
  b[2] = y1 >> 8; b[3] = y1 & 0xFF;
  wr_cmd_p(0x2B, b, 4);                /* RASET */

  wr_cmd(0x2C);                          /* RAMWR */
}

void ST7789_SendBlockDMA(const uint8_t *pixels, uint32_t nbytes)
{
  lcd_busy = 1;
  spi_width(SPI_DATASIZE_16BIT);
  DC_DATA();
  CS_LOW();                                /* CS giu thap suot ca DMA */
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)pixels, nbytes / 2);
  /*                                          ^^^^^^^^^^
     Size dem theo KHUNG du lieu, khong phai byte.
     16 bit/khung -> so khung = so byte / 2. */
}

int ST7789_IsBusy(void) { return lcd_busy; }

/* Goi tu ngat SPI1 (su kien EOT), khong phai tu ngat DMA */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance != SPI1) return;
  CS_HIGH();
  lcd_busy = 0;                            /* XOA TRUOC khi bao TouchGFX */
  DisplayDriver_TransferCompleteCallback();
}
