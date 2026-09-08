set confirm off
set pagination off
target remote localhost:2331
echo \n=== full hspi1.Init as running on the board ===\n
p/x hspi1.Init
echo \n=== what current source would produce ===\n
p/x SPI_BAUDRATEPRESCALER_4
p/x SPI_MODE_MASTER
p/x SPI_DIRECTION_2LINES_TXONLY
p/x SPI_NSS_PULSE_ENABLE
echo \n=== lcd_busy, sampled 3x ===\n
p lcd_busy
p lcd_busy
p lcd_busy
echo \n=== where is it, sampled ===\n
frame
