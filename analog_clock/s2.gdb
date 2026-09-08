set confirm off
set pagination off
target remote localhost:2331
echo \n=== is flash == my build? ===\n
compare-sections
echo \n=== SPI/DMA handle state ===\n
p/x hspi1.Init.BaudRatePrescaler
p/x hspi1.Init.DataSize
p hspi1.State
p/x hspi1.ErrorCode
p hspi1.Lock
p handle_GPDMA1_Channel0.State
p/x handle_GPDMA1_Channel0.ErrorCode
p/x hspi1.hdmatx
echo \n=== GPDMA1 ch0 regs (LBAR FCR SR CR) ===\n
x/4xw 0x40020050
x/4xw 0x40020060
echo \n=== ch0 TR1 TR2 BR1 SAR DAR LLR ===\n
x/5xw 0x40020090
x/1xw 0x400200CC
echo \n=== NVIC ISER0 ISPR0 IABR0 / ICSR ===\n
p/x *(unsigned int*)0xE000E100
p/x *(unsigned int*)0xE000E200
p/x *(unsigned int*)0xE000E300
p/x *(unsigned int*)0xE000ED04
p $ipsr
