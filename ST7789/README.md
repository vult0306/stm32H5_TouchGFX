# ST7789

Hiển thị đồ hoạ **TouchGFX** trên màn **ST7789 240×240 IPS giao tiếp SPI**, chạy trên board **WeAct Studio STM32H523CET6**.

Điểm cốt lõi: H523 **không có** tăng tốc đồ hoạ (Chrom-ART / NeoChrom), **không có** RAM ngoài, **không có** flash ngoài. Ba ràng buộc đó quyết định gần như mọi lựa chọn kỹ thuật trong project — quan trọng nhất là dùng **partial framebuffer** (14.4 KB) thay vì framebuffer đầy (115.2 KB).

📖 **Bản hướng dẫn đầy đủ (song ngữ VI/EN):** <https://vuxtechsolution.com/projects/stm32h5-st7789-touchgfx/>
Project trước cùng board: [H523_Blink](../H523_Blink/README.md) · <https://vuxtechsolution.com/projects/stm32h5-blink-jlink/>

---

## 1. Phần cứng

### Board

| | |
|---|---|
| MCU | STM32H523CETx — Cortex-M33, LQFP48, 512 KB Flash / 272 KB RAM |
| SYSCLK | 250 MHz (HSE 8 MHz → PLL1) |
| Kernel clock SPI1 | **CLKP (per_ck) = HSI 64 MHz** → prescaler /2 = **SCK 32 MHz** |
| Debug | SWD — PA13 (SWDIO) / PA14 (SWCLK), SEGGER J-Link |
| Đồ hoạ | không có DMA2D / NeoChrom → `Graphics accelerator = None` |

### Nối màn ST7789

Sơ đồ tránh mọi chân đã dùng: PC13 (LED), PC14/15 (LSE), PH0/1 (HSE), PA13/14 (SWD), PB14/15 (USART1).

| Module ST7789 | STM32H523 | Chức năng | Ghi chú |
|---|---|---|---|
| GND | GND | — | Nối trước tiên |
| VCC | 3V3 | — | Module có LDO sẵn |
| SCL | **PA5** | SPI1_SCK | AF5 |
| SDA | **PA7** | SPI1_MOSI | AF5 — màn chỉ nhận, không cần MISO |
| CS | **PA4** | GPIO Output | `LCD_CS`, init High |
| DC | **PB0** | GPIO Output | `LCD_DC` — 0 = lệnh, 1 = dữ liệu |
| RES | **PB1** | GPIO Output | `LCD_RST`, active-low |
| BLK | **PA1** | GPIO Output | `LCD_BLK` — init **Low**, bật ở cuối `ST7789_Init()` |

> SPI ở 32 MHz trên dây dupont cắm breadboard là đang cầu may. Nếu màn nhiễu / sọc / lúc được lúc không: rút SCL và SDA xuống dưới 10 cm, và hạ prescaler xuống /4 (16 MHz). **Bring-up nên bắt đầu ở 16 MHz.**

---

## 2. Toolchain

| | |
|---|---|
| STM32CubeMX | 6.18.1 — standalone, trên **Ubuntu** |
| STM32CubeIDE | 2.2.0 — trên **Ubuntu** |
| X-CUBE-TOUCHGFX | **4.26.1** |
| TouchGFX Designer | **4.26.1** — chỉ có bản Windows, chạy trong **VM** |
| SEGGER J-Link | Software pack |

> ⚠️ Bản Designer **phải trùng** bản X-CUBE-TOUCHGFX. Lệch bản thì Generator parameters bị reset về mặc định — mất sạch cấu hình partial framebuffer.

### Bắt buộc trong VM Windows

Cài **Microsoft Visual C++ Redistributable x64** (`vc_redist.x64.exe`) **trước khi** mở Designer. Thiếu nó (hoặc chỉ có bản x86) thì Designer báo `Failed to load native freetype library! Could not open file.` và không làm được gì. Designer là ứng dụng 64-bit.

---

## 3. Ngân sách RAM — vì sao partial framebuffer

| Chiến lược | RAM | % của 272 KB |
|---|---|---|
| Framebuffer đơn 240×240 RGB565 | 115 200 B | 41% |
| Framebuffer kép | 230 400 B | 83% — **hết đường** |
| **Partial, 3 khối × 4800 B** | **14 400 B** | **5%** |

Framebuffer đơn *lọt*, nhưng đánh đổi 41% RAM để lấy một thứ màn SPI không tận dụng được: SPI vẫn phải đẩy từng byte ra ngoài dù buffer có đầy hay không.

| Thông số | Giá trị | Tính ra sao |
|---|---|---|
| Một dòng | 480 B | 240 px × 2 B |
| Block size | 4800 B | 10 dòng |
| Number of blocks | 3 | một đang truyền, một đang vẽ, một dự phòng |
| Truyền 1 khối | ≈1.2 ms | 4800 × 8 ÷ 32 Mbit/s |
| Vẽ lại toàn màn | ≈29 ms | trần ~34 fps |

> ST khuyến nghị với phần cứng giá rẻ: **đừng vượt quá ~30% diện tích màn mỗi khung.**

---

## 4. Cấu hình `.ioc`

### SPI1

| Tham số | Giá trị | Vì sao |
|---|---|---|
| Mode | Transmit Only Master | màn chỉ nhận |
| Frame Format | Motorola | — |
| **Data Size** | **16 Bits** | **mấu chốt** — xem bên dưới |
| First Bit | MSB First | ST7789 nhận byte cao trước |
| Prescaler | 2 → 32 MBit/s | từ kernel 64 MHz |
| CPOL / CPHA | Low / 1 Edge | mode 0 |
| NSS Signal Type | Software | CS điều khiển bằng GPIO |

**Vì sao 16 bit:** TouchGFX lưu pixel RGB565 dưới dạng `uint16_t` — trong RAM là little-endian, byte thấp trước. ST7789 lại đòi byte cao trước. Để SPI ở 8 bit và đẩy thẳng framebuffer đi thì **mọi màu sẽ sai** (đỏ ra xanh dương). Đặt Data Size = 16 bit là SPI đọc nguyên halfword rồi dịch ra MSB trước — **tự đảo thứ tự byte trên đường dây, không tốn chu kỳ CPU nào.**

### Kernel clock của SPI1

Trên STM32H5, SPI1 **không** lấy clock từ PCLK2 như F4/F1. Nó có mux kernel riêng, mặc định CubeMX trỏ vào **PLL1Q** — thường ngoài dải cho phép và ô hiện đỏ.

`Clock Configuration` → **`SPI1/I2S1 Clock Mux` = `CLKP`** (per_ck = HSI 64 MHz). Kiểm tra `PER Clock Mux = HSI` và HSI được bật trong RCC.

Con số này **độc lập với 250 MHz của CPU**, nên sau này đổi tốc độ CPU cũng không làm hỏng màn hình.

### DMA cho SPI1

| Trường | Giá trị |
|---|---|
| Request | SPI1_TX |
| Channel | GPDMA1 Channel 0 |
| Direction | Memory To Peripheral |
| Mode | **Normal** (không phải Circular) |
| Source / Destination Data Width | **Half Word** / **Half Word** |
| Source / Destination Increment | Enabled / Disabled |

> ⚠️ **Phải bật `SPI1 global interrupt` trong tab NVIC.** Trên STM32H5 (giống H7), khi truyền bằng DMA thì `HAL_SPI_TxCpltCallback()` được gọi từ **ngắt của SPI** (sự kiện EOT), **không phải** ngắt DMA. Quên bật thì DMA chạy xong mà callback không bao giờ nổ — TouchGFX gửi đúng một khối rồi đứng im vĩnh viễn.

### TIM6 — nhịp VSync 60 Hz

Module 8 chân không có chân TE (tearing effect) → không có tín hiệu đồng bộ thật. Thay bằng timer chạy đều.

- `Prescaler = 2499` → 250 MHz ÷ 2500 = 100 kHz
- `Counter Period = 1666` → ≈59.99 Hz
- NVIC → bật **TIM6 global interrupt**

### TouchGFX Generator

| Nhóm | Tham số | Giá trị |
|---|---|---|
| Display | Interface | **Custom** |
| | Width × Height | 240 × 240 |
| | Framebuffer pixel format | RGB565 (16bpp) |
| | Buffering strategy | **Partial Buffer — GRAM display** |
| | Number of blocks / Block size | **3 / 4800** |
| Driver | Application Tick Source | **Custom** |
| | Graphics accelerator | **None** |
| | Real-Time Operating System | No OS |

> Tài liệu TouchGFX có chỗ liệt kê SPI như một display interface, nhưng trang riêng về SPI nói rõ: *"When using a SPI display interface, the Custom Display Interface must be selected in the TouchGFX Generator."*

---

## 5. Cấu trúc & quyền sở hữu file

Ba công cụ cùng ghi vào một cây thư mục. Biết ai sở hữu file nào là điều kiện để project sống lâu dài.

```
ST7789/
├── ST7789.ioc                          ← CubeMX  (nguồn sự thật phần cứng)
├── Core/
│   ├── Inc/st7789.h                    ← BẠN
│   ├── Src/st7789.c                    ← BẠN  (driver + DMA + callback)
│   └── Src/main.c                      ← CubeMX, trừ vùng USER CODE
├── Drivers/                            ← CubeMX
├── Middlewares/ST/touchgfx/            ← CubeMX  (framework — CHỈ được nằm ở đây)
├── STM32H523CETX_FLASH.ld              ← CubeMX  (+ sửa tay ExtFlashSection)
├── TouchGFX/
│   ├── ST7789.touchgfx                 ← Designer (nguồn sự thật giao diện)
│   ├── ApplicationTemplate.touchgfx.part
│   ├── App/app_touchgfx.c              ← sinh MỘT LẦN, của bạn
│   ├── target/
│   │   ├── TouchGFXHAL.cpp             ← sinh MỘT LẦN, của bạn
│   │   ├── STM32TouchController.cpp    ← sinh MỘT LẦN, của bạn
│   │   ├── ST7789DisplayDriver.c       ← BẠN  (3 hàm cầu nối)
│   │   └── generated/                  ← CubeMX  (ghi đè hoàn toàn)
│   ├── gui/                            ← Designer sinh lớp *Base; lớp con là của bạn
│   ├── generated/                      ← Designer (ghi đè hoàn toàn)
│   └── assets/images/                  ← BẠN đặt PNG vào đây
└── .cproject                           ← CubeMX VÀ Designer đều ghi ⚠️
```

**`.cproject` là nguồn xung đột duy nhất.** Với project TouchGFX, cách lành nhất là **không** tạo build configuration tuỳ chỉnh, và **commit git trước mỗi lần Generate Code ở cả hai bên**.

---

## 6. Shared folder giữa Ubuntu và VM

TouchGFX Designer chỉ có bản Windows (kể cả `tgfx.exe`), nên VM là bắt buộc.

### Quy tắc: mount phải ở **hoặc trên** thư mục chứa `.ioc`

VirtualBox → Settings của VM → `Shared Folders → Add`:

- **Folder Path** = thư mục **cha** của tất cả project, ví dụ `~/gitwork/stm32H5_TouchGFX`
- Tick `Auto-mount` + `Make Permanent`, **bỏ tick** `Read-only`
- Trong Windows, `Map network drive` thành ổ có chữ cái (đừng dùng UNC `\\VBOXSVR\…`)
- Cài **Guest Additions** — thiếu nó thì Designer treo lúc Generate Code

### ⚠️ Đừng share thư mục `TouchGFX/`

File `ApplicationTemplate.touchgfx.part` trỏ **ra ngoài** bằng đường dẫn tương đối:

```json
"ProjectFile":            "../ST7789.ioc",
"TouchGfxPath":           "../Middlewares/ST/touchgfx",
"OptionalComponentsRoot": "../Middlewares/ST/touchgfx_components"
```

Mount đúng tại `TouchGFX/` thì `..` nằm ngoài tầm với của Windows. Designer **vẫn sinh** `gui/` và `generated/`, nhưng:

- không tìm thấy `.ioc` → `touchgfx update_project` hỏng
- không vá được `.cproject` ở thư mục gốc → **ảnh vừa import không vào build**
- tự dựng một **bản sao framework** ở `TouchGFX/Middlewares/`

Bản sao đó không được loại `os/OSWrappers.cpp` và `os/OSWrappers_cmsis.cpp` → build chết với `FreeRTOS.h: No such file`.

**Khắc phục:** `rm -rf TouchGFX/Middlewares`, rồi F5 + Clean + build lại. **Kiểm chứng:** cả ba file `ST7789.touchgfx`, `.part` và `target.config` phải cùng ghi `"TouchGfxPath": "../Middlewares/ST/touchgfx"`.

### Ba bước 30 giây trước lần Generate Code đầu tiên

1. Mở ổ đĩa trong Explorer — phải thấy `ST7789.ioc` cùng `Core/ Drivers/ Middlewares/ TouchGFX/`. Thấy `App/ target/ assets/` là đang đứng **trong** `TouchGFX/` → dừng lại, sửa share.
2. Commit ngay sau khi CubeMX generate, **trước** khi Designer chạy lần đầu.
3. Sau Generate Code chạy `git status`. Thấy `TouchGFX/Middlewares/` xuất hiện là share sai → `git checkout .` rồi sửa, **đừng build**.

### Xác nhận ổ đĩa trỏ đúng chỗ

```bash
VBoxManage showvminfo "win11" --machinereadable | grep -i sharedfolder
```

Chỉ sửa được từ **phía host**: `Devices → Shared Folders → Shared Folders Settings…` → đổi `Folder Path` → khởi động lại VM. Giữ nguyên `Folder Name` thì ổ đĩa đã map vẫn dùng được.

---

## 7. Code

### `Core/Src/st7789.c` — driver

Ba phần: đổi độ rộng khung SPI (lệnh 8-bit / pixel 16-bit), init + cửa sổ GRAM, và truyền DMA.

```c
void ST7789_SendBlockDMA(const uint8_t *pixels, uint32_t nbytes)
{
  lcd_busy = 1;
  spi_width(SPI_DATASIZE_16BIT);
  DC_DATA();
  CS_LOW();                                /* CS giu thap suot ca DMA */
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)pixels, nbytes / 2);
  /*                                          ^^^^^^^^^^
     Size dem theo KHUNG du lieu, khong phai byte. 16 bit/khung. */
}

/* Goi tu ngat SPI1 (su kien EOT), khong phai tu ngat DMA */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance != SPI1) return;
  CS_HIGH();
  lcd_busy = 0;                            /* XOA TRUOC khi bao TouchGFX */
  DisplayDriver_TransferCompleteCallback();
}
```

> ⚠️ **Thứ tự trong callback:** phải đặt `lcd_busy = 0` **trước** khi gọi `DisplayDriver_TransferCompleteCallback()`. Hàm đó có thể gọi ngược lại `ST7789_SendBlockDMA()` ngay lập tức nếu còn khối đang chờ — nếu cờ vẫn còn 1, TouchGFX tưởng SPI đang bận và **dừng chuỗi truyền**. Màn vẽ được vài dải rồi đứng.

Trong lệnh init, `INVON (0x21)` là **bắt buộc** với panel IPS 240×240. Panel TN thì dùng `INVOFF (0x20)`.

Đầu file cần:

```c
extern SPI_HandleTypeDef hspi1;
extern void DisplayDriver_TransferCompleteCallback(void);   /* TouchGFX sinh, extern "C" */
```

### `TouchGFX/target/ST7789DisplayDriver.c` — ba hàm cầu nối

```c
#include "st7789.h"
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
```

> TouchGFX có **hai biến thể** partial framebuffer. Một biến thể cần thêm `touchgfxDisplayDriverShouldTransferBlock(uint16_t bottom)`, biến thể kia không.
> **Đã xác nhận với 4.26.1:** nó sinh biến thể **không** có hàm đó. Trước khi gõ, luôn mở `TouchGFX/target/generated/TouchGFXGeneratedHAL.cpp` — khối comment ở đầu file ghi đúng tên hàm phiên bản của bạn cần.

### `Core/Src/main.c` — thứ tự khởi động

CubeMX đặt `MX_TouchGFX_Init()` **trước** `USER CODE BEGIN 2`, và **không có** vùng USER CODE nào giữa `MX_SPI1_Init()` và `MX_TouchGFX_Init()`. Nên hai dòng khởi động phải tách ra hai chỗ — **và đó không phải chuyện thẩm mỹ.**

| Dòng | Đặt ở đâu | Vì sao |
|---|---|---|
| `ST7789_Init();` | `USER CODE BEGIN SPI1_Init 2` | cuối `MX_SPI1_Init()` — GPIO/GPDMA1/SPI1 đã sẵn sàng, và chạy **trước** `MX_TouchGFX_Init()` |
| `HAL_TIM_Base_Start_IT(&htim6);` | `USER CODE BEGIN 2` | phải là **sau** `MX_TouchGFX_Init()` |

> ⚠️ **Hard fault.** Đừng start TIM6 trước `MX_TouchGFX_Init()`. Ngắt gọi `touchgfxSignalVSync()` → `touchgfx::HAL::getInstance()->vSync()`. Trước khi TouchGFX init xong thì `getInstance()` trả con trỏ chưa hợp lệ — hard fault ngay ở tick đầu tiên, **trước cả khi vào `while(1)`**.

Còn lại:

```c
/* USER CODE BEGIN Includes */
#include "st7789.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 0 */
extern void touchgfxSignalVSync(void);   /* TouchGFX sinh ra */
/* USER CODE END 0 */

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6) touchgfxSignalVSync();
}
/* USER CODE END 4 */
```

Vòng `while` để nguyên — CubeMX đã tự chèn `MX_TouchGFX_Process();`.

---

## 8. Trong TouchGFX Designer

1. `Open Project` → `<ổ>:\ST7789\TouchGFX\ApplicationTemplate.touchgfx.part`
2. Lưu thành project thật → sinh `ST7789.touchgfx`
3. Kéo PNG vào panel `Images`, hoặc chép vào `TouchGFX\assets\images\` rồi refresh
4. Thả widget `Image` lên canvas, X = 0, Y = 0
5. `Generate Code`

| Thiết lập trong panel Images | Giá trị | Vì sao |
|---|---|---|
| Image Format | **RGB565** | khớp framebuffer; ARGB8888 tốn gấp đôi flash + chuyển màu bằng CPU mỗi khung |
| Section | **IntFlashSection** | board không có flash ngoài |

Vì linker script không định nghĩa `ExtFlashSection`, thêm vào `STM32H523CETX_FLASH.ld` ngay sau `.rodata`:

```ld
ExtFlashSection : {
  *(ExtFlashSection ExtFlashSection.*)
  *(IntFlashSection IntFlashSection.*)
  *(FontFlashSection FontFlashSection.*)
  *(TextFlashSection TextFlashSection.*)
  . = ALIGN(4);
} >FLASH
```

> Với ảnh đúng 240×240 trên màn 240×240, dùng widget `Image` **chứ không phải** `ScalableImage`. ScalableImage nội suy từng pixel mỗi lần vẽ — vô ích khi tỉ lệ 1:1, và H523 không có gì gánh hộ.

### `Thumbs.db`

Trình chuyển ảnh quét **mọi** file trong `TouchGFX/assets/images/` và chết khi gặp file lạ:

```
ERROR: assets/images/Thumbs.db:encryptable not supported by image converter
```

Xoá từ Ubuntu: `find . -name 'Thumbs.db*' -print -delete`
Chặn từ gốc trong VM (`cmd`, rồi đăng xuất/đăng nhập lại):

```
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer" /v DisableThumbsDBOnNetworkFolders /t REG_DWORD /d 1 /f
```

Thêm `Thumbs.db` và `desktop.ini` vào `.gitignore`.

---

## 9. Build & chạy

1. Trong CubeIDE chọn project → **F5** (thấy file Designer vừa sinh)
2. Build — lần đầu lâu, TouchGFX có vài trăm file nguồn
3. Debug bằng J-Link (probe = `SEGGER J-LINK`, SWD, `STM32H523CE`)

Thứ tự bạn sẽ thấy nếu mọi thứ đúng: **đèn nền sáng → màn đen tuyền (GRAM đã xoá) → ảnh hiện ra từng dải ngang trong ~30 ms.**

### Tách đôi bài toán khi debug

Sau `ST7789_Init()`, gọi `ST7789_SetWindow(0,0,240,240)` rồi bơm 57 600 halfword `0xF800` ra SPI:

- **Màn đỏ toàn bộ** → phần cứng, SPI, DMA, driver đều đúng; lỗi nằm ở tầng TouchGFX
- **Màn xanh dương** → đảo byte; quay lại mục 4, Data Size phải là 16 bit

### Ngân sách flash

Kiểm tra bằng `Debug/ST7789.map`: cộng `.text` + `.rodata` + section chứa ảnh, trừ khỏi 512 KB.

---

## 10. Ảnh động

Dịch chuyển ảnh chỉ là chép pixel — không nội suy, rẻ hơn xoay cả bậc.

Ảnh 120×120 đi vòng bốn góc phần tư, mỗi chặng 2 s: **120 px ÷ 120 tick = đúng 1 px mỗi nhịp** → không cần số thực, không tích luỹ sai số. Vùng vẽ lại ≈120×121 px ≈ 25% màn ≈ 7.3 ms — trong ngân sách 16.7 ms.

```cpp
void screenView::handleTickEvent()
{
    switch (leg)
    {
        case 0: posX++; break;   /* sang phai */
        case 1: posY++; break;   /* xuong     */
        case 2: posX--; break;   /* sang trai */
        default: posY--; break;  /* len       */
    }

    whale.invalidate();          /* xoa vet o cho cu */
    whale.setXY(posX, posY);
    whale.invalidate();          /* ve o cho moi     */

    if (++step >= TICKS_PER_LEG) { step = 0; leg = (leg + 1) & 3; }
}
```

> ⚠️ **Phải có một widget `Box` 240×240 ở lớp dưới cùng.** Khi ảnh rời khỏi một vùng, TouchGFX phải vẽ lại thứ nằm dưới. Chỗ đó trống thì nội dung khối framebuffer là rác — và với partial framebuffer các khối được tái sử dụng liên tục, nên vệt kéo dài phía sau ảnh hiện rất rõ.

Dùng `setXY()` kẹp giữa hai `invalidate()` chứ không dùng `moveTo()`: tài liệu API của `moveTo()` chỉ ghi "Moves the drawable", không nói nó có tự invalidate vùng cũ hay không.

**Nếu giật:** nâng `Block size` từ 4800 lên 9600/14400 (khối 4800 B chỉ là 10 dòng, nên vùng bẩn cao 121 px bị cắt thành hơn chục lượt vẽ-và-truyền riêng biệt); bật `setFrameRateCompensation(true)`; kiểm tra ảnh đúng là RGB565.

**Xoay quanh tâm:** widget `TextureMapper`. Widget phải lớn hơn ảnh, cạnh ≥ **ảnh × 1.42** (đường chéo), nếu không bốn góc bị cắt. `setOrigo` mới là tâm xoay và toạ độ tính theo **widget**; z-origo phải bằng `setCameraDistance`. Nhớ bật texture mapper RGB565 trong `Designer → Config → Framework Features`.

**Lottie:** TouchGFX không đọc được. Chuyển thành chuỗi PNG rồi dùng `AnimatedImage`; định dạng `L8_ARGB8888` / `L8_RGB888` tiết kiệm một nửa tới ba phần tư flash. Tên khung phải xếp liền nhau (`sand_01 … sand_20`) vì bitmap ID được cấp theo thứ tự tên file **trong toàn project**.

---

## 11. Bẫy đã gặp

**Hard fault ngay khi khởi động, chưa vào `while(1)`.**
TIM6 start trước `MX_TouchGFX_Init()`. Chuyển `HAL_TIM_Base_Start_IT()` xuống `USER CODE BEGIN 2`.

**Vẽ được vài dải rồi đứng im.**
① chưa bật `SPI1 global interrupt` trong NVIC → `HAL_SPI_TxCpltCallback` không bao giờ chạy; ② trong callback đặt `lcd_busy = 0` **sau** khi gọi `DisplayDriver_TransferCompleteCallback()` thay vì **trước**.

**Đỏ hoá xanh dương.**
Đảo byte RGB565 — SPI đang ở Data Size 8 bit. Đặt lại 16 bit, và DMA phải `Half Word` ở cả nguồn lẫn đích.

**Màn đen hoàn toàn, đèn nền vẫn sáng.**
ST7789 chưa ra khỏi sleep, hoặc DC/CS nối sai. Kiểm tra `ST7789_Init()` chạy **trước** `MX_TouchGFX_Init()`. Chạy bài kiểm tra màn đỏ ở mục 9.

**Ảnh âm bản.**
Thừa hoặc thiếu `INVON (0x21)`. IPS 240×240 hầu hết cần; TN thì không.

**Ảnh lệch vài pixel, viền rác ở mép.**
Một số module có offset GRAM. Cộng offset vào `x`/`y` trong `ST7789_SetWindow()` — thử `(0,80)` hoặc `(80,0)` tuỳ hướng MADCTL.

**Ảnh di chuyển để lại vệt kéo dài.**
Màn chưa được phủ kín. Thêm `Box` 240×240 làm lớp dưới cùng.

**CubeMX báo đỏ ô `SPI1/I2S1 Clock Mux`.**
Kernel clock đang lấy từ PLL1Q, giá trị ngoài dải. Đổi sang `CLKP` — mục 4.

**`FreeRTOS.h: No such file` / `cmsis_os.h: No such file`.**
Có bản sao thừa của framework tại `TouchGFX/Middlewares/`. `rm -rf` nó, rồi F5 + Clean + build lại. Nguyên nhân gốc là shared folder trỏ sai — mục 6.

**`implicit declaration of function 'ST7789_Init'`.**
Thiếu `#include "st7789.h"` trong `USER CODE BEGIN Includes` của `main.c`.

**`implicit declaration of function 'DisplayDriver_TransferCompleteCallback'`.**
Thêm `extern void DisplayDriver_TransferCompleteCallback(void);` ở đầu `st7789.c`. Khai báo trùng với `ST7789DisplayDriver.c` là bình thường — chúng chỉ là declaration.

**`undefined reference to touchgfxDisplayDriverShouldTransferBlock`.**
Phiên bản của bạn dùng biến thể có hàm đó. Bỏ comment trong `ST7789DisplayDriver.c`, `return 1`.

**`Generate Assets: not supported by image converter`.**
File lạ trong `assets/images/` — thường là `Thumbs.db`. Mục 8.

**`Failed to load native freetype library`.**
Thiếu Visual C++ Redistributable **x64** trong VM. Mục 2.

**Cấu hình TouchGFX Generator tự reset về mặc định.**
CubeMX vừa nâng X-CUBE-TOUCHGFX lên bản khác với Designer. Ghim cả hai về cùng số phiên bản.

**Sọc ngang, nhiễu, lúc được lúc không.**
Tín hiệu SPI. Hạ prescaler xuống 16/32, rút ngắn dây SCL/SDA, thêm tụ 100 nF sát chân nguồn module.

---

## 12. Điều chưa kiểm chứng

Không có tài liệu chính thức nào của ST liệt kê **đích danh STM32H523** trong danh sách thiết bị của X-CUBE-TOUCHGFX. Cortex-M33 và STM32H5 nói chung thì được hỗ trợ, và đã có người chạy TouchGFX + màn SPI trên STM32H563. Trước khi bắt đầu project mới, luôn tự xác nhận bằng cách mở CubeMX → `Select Components` (Alt+O) và tìm `X-CUBE-TOUCHGFX` cho đúng con chip.

---

## Tham khảo

- [TouchGFX — SPI Display Interface](https://support.touchgfx.com/docs/development/board-bring-up/how-to/06-display-spi)
- [TouchGFX — Framebuffer Strategies](https://support.touchgfx.com/docs/development/ui-development/scenarios/framebuffer-strategies)
- [TouchGFX — Running on Low Cost Hardware](https://support.touchgfx.com/docs/development/ui-development/scenarios/lowcost-hardware)
- [TouchGFX — Enabling TouchGFX Generator](https://support.touchgfx.com/docs/development/board-bring-up/how-to/01-enable-touchgfx-generator)
- [RM0481 — STM32H523 reference manual](https://www.st.com/resource/en/reference_manual/rm0481-stm32h52333xx-stm32h56263xx-and-stm32h573xx-armbased-32bit-mcus-stmicroelectronics.pdf) — mux kernel clock của SPI
