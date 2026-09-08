# analog_clock

Đồng hồ kim **TouchGFX AnalogClock** chạy trên board **WeAct Studio STM32H523CET6** với màn **ST7789 240×240 IPS SPI**, giờ lấy từ **RTC** trên chip.

Widget `AnalogClock` là widget dựng sẵn của TouchGFX — kéo thả trong Designer, không viết một dòng code vẽ nào. Project này đo xem cái "miễn phí" đó tốn gì trên một con chip **không có tăng tốc đồ hoạ**. Câu trả lời ngắn: **không tốn RAM, không tốn CPU, tốn flash** — bốn tấm bitmap của widget chiếm **245 528 B**, nặng gấp **2.3×** toàn bộ code của chương trình cộng lại.

📖 **Bản hướng dẫn đầy đủ (song ngữ VI/EN):** <https://vuxtechsolution.com/projects/stm32h5-analog-clock-touchgfx/>
Project trước cùng board: [ST7789 + TouchGFX](../ST7789/README.md) · <https://vuxtechsolution.com/projects/stm32h5-st7789-touchgfx/>
Project khởi động: [H523_Blink](../H523_Blink/README.md) · <https://vuxtechsolution.com/projects/stm32h5-blink-jlink/>

---

## 1. Phần cứng

Giống hệt project [ST7789](../ST7789/README.md) — cùng board, cùng cách nối màn, không đổi một sợi dây nào.

| | |
|---|---|
| MCU | STM32H523CETx — Cortex-M33, LQFP48, 512 KB Flash / 272 KB RAM |
| SYSCLK | 250 MHz (HSE 8 MHz → PLL1) |
| Màn hình | ST7789 240×240 IPS, SPI1 — SCK **PA5**, MOSI **PA7** |
| GPIO màn | CS **PA4** · DC **PB0** · RST **PB1** · BLK **PA1** |
| Đồ hoạ | không có DMA2D / NeoChrom → `Graphics accelerator = None` |
| Nguồn giờ | **RTC** trên chip |
| Debug | SWD — PA13 / PA14, SEGGER J-Link |

---

## 2. Toolchain

| | |
|---|---|
| STM32CubeMX | 6.18.1 — standalone, trên **Ubuntu** |
| STM32CubeIDE | 2.2.0 — GCC 14.3.1 |
| X-CUBE-TOUCHGFX | **4.26.1** |
| TouchGFX Designer | **4.26.1** — chỉ có bản Windows, chạy trong **VM** |

> ⚠️ Bản Designer **phải trùng** bản X-CUBE-TOUCHGFX, nếu không Generator parameters bị reset về mặc định — mất sạch cấu hình partial framebuffer. Chi tiết về shared folder giữa Ubuntu và VM: [ST7789 mục 6](../ST7789/README.md).

---

## 3. Kết quả đo

Tất cả số dưới đây đọc từ `Debug/analog_clock.map` của bản build thật (`-O0 -g3`), **không phải ước lượng**.

### Flash

| Section | Byte | % của 512 KB | Là gì |
|---|---|---|---|
| **IntFlashSection** | **245 528** | **46.8%** | bốn bitmap của đồng hồ |
| `.text` | 97 328 | 18.6% | HAL + TouchGFX + driver + app |
| `.rodata` | 9 900 | 1.9% | hằng số |
| FontFlashSection | 1 194 | 0.2% | font Verdana 22 4bpp |
| `.isr_vector` | 596 | 0.1% | bảng vector |
| `.data` (bản LMA) | 136 | — | biến khởi tạo |
| FontSearchFlashSection | 8 | — | — |
| TextFlashSection | 10 | — | — |

**Tổng ≈ 354 700 B — 67.6% của 512 KB**, còn dư ≈166 KB.

### Bên trong 245 KB đó

| Bitmap | Byte | Tính ra sao |
|---|---|---|
| Mặt đồng hồ 240×240 | **230 400** | 240 × 240 × 4 — ARGB8888, khớp chính xác |
| Kim phút | 7 840 | ARGB8888 — cần alpha để xoay sạch |
| Kim giờ | 5 688 | |
| Kim giây | 1 600 | |

Toàn bộ code (`.text` + `.rodata` + `.isr_vector` + `.data`) là **107 960 B**. Riêng tấm ảnh mặt đồng hồ đã nặng hơn **2.1×**; cả bốn bitmap cộng lại nặng hơn **2.3×**.

### RAM

| Vùng | Byte | % của 272 KB |
|---|---|---|
| `.bss` | 17 636 | 6.3% |
| heap + stack | 16 896 | 6.1% |
| `.data` | 136 | — |

**Tổng 34 668 B — 12.4%.** Partial framebuffer 3 × 4800 B nằm trong `.bss`.

> **Bài học.** Partial framebuffer đã giải quyết bài toán RAM triệt để — 12% là thoải mái. Nhưng ngay khi dùng widget dựa trên ảnh, ràng buộc **nhảy sang flash**, và nhảy rất mạnh. Trên dòng chip 512 KB, đó là thứ phải theo dõi từ ngày đầu chứ không phải khi build lỗi.

### Cách cắt một nửa

Trong panel `Images` của Designer, đổi mặt đồng hồ từ **ARGB8888** sang **RGB565**: 230 400 → 115 200 B, tiết kiệm **22% flash**. Bốn góc trong suốt sẽ thành màu đặc — nếu widget `Box` phía dưới cùng màu thì nhìn không khác gì. Ba cái kim phải giữ ARGB8888, nhưng chúng chỉ 15 KB.

---

## 4. Cấu hình `.ioc`

Giống project ST7789, trừ RTC là mới. Đọc thẳng file `.ioc` (nó là text) nhanh hơn click qua CubeMX rất nhiều:

```bash
grep -E '^(SPI1\.|TIM6\.|NVIC\.|RCC\.SPI1)' analog_clock.ioc
```

| Khối | Tham số | Giá trị |
|---|---|---|
| SPI1 | Mode / Direction | Master, **Transmit Only** |
| | **Data Size** | **16 Bits** — tự đảo byte RGB565 trên đường dây |
| | Kernel clock mux | **CLKP** → 32 MHz |
| | Prescaler | /2 → **16 MBit/s** |
| GPDMA1 ch0 | SPI1_TX, Normal, **Half Word** / Half Word | |
| TIM6 | PSC 2499 / ARR 1666 | ≈60 Hz — nhịp VSync |
| NVIC | **SPI1 global interrupt** | **bật** |
| | TIM6 global interrupt | bật |
| RTC | HourFormat 24, Async 127 / Sync 255 | nguồn clock **LSI** |
| TouchGFX | Display interface / buffering | Custom · **Partial Buffer 3 × 4800** |
| | Graphics accelerator / RTOS | **None** · No OS |

Hai ô dễ sai:

**① `SPI1 global interrupt`.** Trên STM32H5, khi truyền bằng DMA thì `HAL_SPI_TxCpltCallback()` được gọi từ **ngắt của SPI** (sự kiện EOT), **không phải** ngắt DMA. Quên bật thì TouchGFX đẩy đúng một khối rồi đứng im vĩnh viễn.

**② Tốc độ SPI phải khớp với cấu hình đã chứng minh chạy được.** Đây là lỗi tốn nhiều thời gian nhất của cả project — xem mục 7.4.

---

## 5. Trong TouchGFX Designer

1. `☰ All → Box`. Tên `bg`, X 0 Y 0 W 240 H 240. Chuột phải → **Send to back**.
2. `☰ All → Analog Clock`. Chọn **Preset** (ở đây là `Small Plain Dark`) — nó nạp sẵn mặt số và ba kim.
3. Kéo panel `Properties` xuống mục **Animation → để off**.
4. Tab `Images`: đặt **Section = IntFlashSection** cho cả bốn ảnh — board không có flash ngoài.
5. Bấm **`</>` (Generate Code)** — **không phải** nút ▶ (Run Simulator). Lý do ở mục 7.1.

> **Box nền không bỏ được.** Ảnh mặt đồng hồ có bốn góc trong suốt. Với partial framebuffer, chỗ trong suốt không có gì để vẽ → nội dung khối là rác của khung trước, mà khối lại được tái sử dụng liên tục, nên bốn góc sẽ nhiễu bẩn. **Mọi điểm trên màn phải được phủ kín bởi một widget nào đó.**

> **Tắt Animation.** Bật lên thì mỗi lần đổi giây, ba kim quét mượt qua nhiều khung — TextureMapper chạy hàng chục lần mỗi giây thay vì một. Trên chip không có tăng tốc, đó là thứ chắc chắn không đủ băng thông.

Widget đó thực chất là gì — đọc trong `TouchGFX/generated/gui_generated/src/main_screen/mainViewBase.cpp`:

```cpp
analogClock1.setBackground(BITMAP_..._BACKGROUNDS_SMALL_PLAIN_DARK_ID, 120, 120);
analogClock1.setupSecondHand(BITMAP_..._HANDS_SMALL_SECOND_PLAIN_DARK_ID, 2, 100);
analogClock1.setupMinuteHand(BITMAP_..._HANDS_SMALL_MINUTE_PLAIN_DARK_ID, 10, 87);
analogClock1.setupHourHand (BITMAP_..._HANDS_SMALL_HOUR_PLAIN_DARK_ID,   9,  69);
```

Một widget `Image` làm nền cộng ba `TextureMapper` làm kim. **Bốn ID bitmap** — đó là chỗ 245 KB flash biến mất, và cũng là lý do "đổi kích thước đồng hồ" không phải là sửa một con số mà là xuất lại bốn file PNG.

---

## 6. Code

### 6.1 — Driver dùng lại nguyên vẹn

Project mới do CubeMX sinh ra không có driver màn hình. Chép ba file từ project trước, **không sửa một dòng nào** (đã kiểm: cả ba file hiện giống hệt bản trong `ST7789/`):

```bash
cp ST7789/Core/Inc/st7789.h                   analog_clock/Core/Inc/
cp ST7789/Core/Src/st7789.c                   analog_clock/Core/Src/
cp ST7789/TouchGFX/target/ST7789DisplayDriver.c  analog_clock/TouchGFX/target/
```

Đó là phần thưởng của việc giữ driver tách khỏi ứng dụng: nó chỉ phụ thuộc vào **nhãn GPIO** trong `main.h` (`LCD_CS`, `LCD_DC`, `LCD_RST`, `LCD_BLK`), mà nhãn thì giống nhau giữa hai project.

### 6.2 — Bốn vùng USER CODE trong `main.c`

```c
/* USER CODE BEGIN Includes */
#include "st7789.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 0 */
extern void touchgfxSignalVSync(void);      /* TouchGFX sinh ra */
/* USER CODE END 0 */

/* --- trong MX_SPI1_Init(), chay TRUOC MX_TouchGFX_Init() --- */
/* USER CODE BEGIN SPI1_Init 2 */
  ST7789_Init();
/* USER CODE END SPI1_Init 2 */

/* --- trong main(), SAU MX_TouchGFX_Init() --- */
/* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);            /* nhip VSync 60 Hz */
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6) touchgfxSignalVSync();
}
/* USER CODE END 4 */
```

> ⚠️ **Hard fault.** Đừng start TIM6 **trước** `MX_TouchGFX_Init()`. Ngắt gọi `touchgfxSignalVSync()` → `HAL::getInstance()->vSync()`; trước khi TouchGFX init xong thì `getInstance()` trả con trỏ chưa hợp lệ — hard fault ngay ở tick đầu tiên, **trước cả khi vào `while(1)`**.
>
> Hai dòng phải nằm ở hai chỗ khác nhau vì CubeMX đặt `MX_TouchGFX_Init()` **trước** `USER CODE BEGIN 2` và **không** để vùng USER CODE nào giữa `MX_SPI1_Init()` và nó.

### 6.3 — Đọc giờ từ RTC

`TouchGFX/gui/src/main_screen/mainView.cpp`:

```cpp
void mainView::handleTickEvent()
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    /* GetDate PHAI goi ngay sau GetTime de mo khoa shadow register cua RTC */
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    /* Chi ve lai khi giay thuc su doi: ve lai ca dong ho la dat tren con
       chip nay (khong co tang toc do hoa). */
    if (time.Seconds != lastSecond)
    {
        lastSecond = time.Seconds;
        analogClock1.setTime24Hour(time.Hours, time.Minutes, time.Seconds);
    }
}
```

Hai điểm bắt buộc:

- **`HAL_RTC_GetDate()` phải gọi ngay sau `HAL_RTC_GetTime()`**, kể cả khi không dùng tới ngày. Đọc `RTC_TR` khoá shadow register lại; chỉ có đọc `RTC_DR` mới mở khoá. Bỏ qua nó thì giờ đứng yên sau lần đọc đầu.
- **Lọc theo giây.** `handleTickEvent()` chạy 60 lần/giây. Mỗi lần vẽ lại đồng hồ là ≈240×240 px ≈ 29 ms — gọi `setTime24Hour()` mỗi tick là yêu cầu gấp 60 lần thứ phần cứng làm nổi. Chỉ vẽ khi `time.Seconds` đổi thì tải xuống đúng 1 lần/giây.

> **Tên lớp theo tên màn hình.** Màn hình tên `main` thì lớp là `mainView` / `mainViewBase` / `mainPresenter`. Đổi tên màn hình trong Designer là đổi tên lớp — một lý do để đặt tên ngay từ đầu và đừng đổi.

### 6.4 — Bước trung gian nếu chưa muốn động tới RTC

Nếu muốn chứng minh phần vẽ trước, đếm giây bằng tick rồi hẵng cắm RTC vào. Nếu bản này chạy, mọi lỗi về sau nằm ở RTC chứ không phải ở đồ hoạ:

```cpp
void mainView::handleTickEvent()
{
    if (++tickCount < 60) return;             /* 60 tick @ 60 Hz = 1 giay */
    tickCount = 0;
    if (++s >= 60) { s = 0; if (++m >= 60) { m = 0; h = (h + 1) % 24; } }
    analogClock1.setTime24Hour(h, m, s);
}
```

---

## 7. Bốn lỗi đã gặp

Không có cái nào là lỗi lập trình. Cả bốn đều là lỗi **cấu hình hoặc môi trường** — và đó mới là loại tốn thời gian nhất.

### 7.1 — Simulator không build được trên shared folder

```
Compiling gui/src/main_screen/mainView.cpp
mkdir: cannot create directory `build': No such file or directory
generated/simulator/gcc/Makefile:196: recipe for target
'build/MINGW32_NT-6.2/gui/src/main_screen/mainView.o' failed
```

`mkdir build` tạo thư mục **trong thư mục hiện tại** — nó không thể thiếu thư mục cha. Báo "No such file or directory" nghĩa là **chính thư mục hiện tại không giải được**. Đường dẫn `build/MINGW32_NT-6.2/` cho biết đây là MSYS/MinGW đóng gói kèm Designer, mà MSYS hay giải ngược ổ đĩa mạng đã map về dạng UNC `\\VBOXSVR\…` — UNC không dùng được làm thư mục hiện tại.

**Không cần sửa.** Bấm `</>` (Generate Code) chứ không phải ▶ (Run Simulator); simulator không liên quan gì tới firmware chạy trên board.

Với đúng thí nghiệm này, simulator còn **có hại**: nó chạy trên PC x86 đủ mạnh để vẽ TextureMapper 240×240 ở 60 fps không chớp mắt. Bạn sẽ thấy kim quét mượt như lụa và kết luận sai. Câu hỏi *"widget này tốn bao nhiêu trên H523"* là câu hỏi simulator được thiết kế để **không** trả lời.

### 7.2 — `undefined reference`, kèm một dấu vết đánh lạc hướng

```
undefined reference to `touchgfxDisplayDriverTransmitActive'
(touchgfxDisplayDriverTransmitActive): Unknown destination type (ARM/Thumb)
  in ./TouchGFX/target/generated/TouchGFXGeneratedHAL.o
dangerous relocation: unsupported relocation
```

Nguyên nhân đơn giản: **project mới chưa có driver** (mục 6.1). Hai dòng `Unknown destination type (ARM/Thumb)` và `dangerous relocation` là **hệ quả**, không phải lỗi riêng — linker gặp symbol không tồn tại nên không biết nó là ARM hay Thumb, không giải nổi relocation của lệnh `BL`. Chúng trông giống lỗi cấu hình toolchain và rất dễ kéo bạn đi sai hướng cả tiếng đồng hồ.

### 7.3 — Thiếu `SPI1 global interrupt`

Vẽ được vài dải rồi đứng im. Tìm ra bằng cách đọc trực tiếp danh sách NVIC trong `.ioc`:

```bash
grep '^NVIC\.' analog_clock.ioc
```

### 7.4 — Màn đen: SPI chạy nhanh gấp đôi mức đã chứng minh

Build sạch, nạp xong, **đèn nền sáng, màn hoàn toàn đen**. Không có thông báo lỗi nào ở đâu cả.

Bước quyết định: **nạp lại project ST7789 cũ lên đúng board đó** — nó vẫn chạy bình thường. Vậy dây, màn hình và driver đều tốt; khác biệt nằm trong cấu hình. Rồi so hai file `.ioc`:

| | ST7789 (chạy được) | analog_clock (màn đen) |
|---|---|---|
| SPI1 Clock Mux | đặt tường minh | **không đặt → PLL1Q** |
| Kernel clock | 32 MHz | 125 MHz |
| Prescaler | /2 | /4 |
| **SCK thật** | **16 MHz** | **31.25 MHz** |

Màn hình chưa bao giờ chạy ở 31 MHz. Ở tốc độ đó, trên dây dupont cắm breadboard, ST7789 không chốt đúng các byte lệnh trong `ST7789_Init()` — nó không nhận được `SLPOUT` và `DISPON` nên ở lại chế độ ngủ. Nhưng `LCD_BLK` là **GPIO thuần, không đi qua SPI**, nên đèn nền vẫn sáng bình thường. Đó là lý do triệu chứng trông giống "chết hẳn" chứ không giống "sai tốc độ".

**Sửa:** đặt `SPI1/I2S1 Clock Mux = CLKP` để tốc độ màn hình **tách khỏi PLL1**, rồi chọn prescaler cho ra ≈16 MHz. Cấu hình hiện tại: kernel 32 MHz, prescaler /2 → **16.0 MBit/s**. Nếu không tách khỏi PLL1 thì mỗi lần đổi HCLK là tốc độ SPI đổi theo — đúng như vừa xảy ra.

---

## 8. Phương pháp chẩn đoán

Lỗi 7.4 mất nhiều thời gian nhất, và cách tìm ra nó đáng mang sang mọi project khác.

1. **Tìm một bản chạy được.** Nạp lại project cũ lên đúng board đó. Nếu nó chạy, bạn vừa loại sạch phần cứng, dây và driver ra khỏi danh sách nghi ngờ — bằng một thao tác 60 giây.
2. **Diff cấu hình, đừng suy luận.** Hai project gần giống nhau, một chạy một không → câu trả lời nằm trong phần khác nhau. `.ioc` là text.
3. **Tìm điểm cắt đôi.** Đèn nền là điểm cắt hoàn hảo: nó bật ở dòng cuối của `ST7789_Init()` và không đi qua SPI. Sáng = code đã chạy tới đó. Tối = chưa bao giờ tới.

### Bài test màn đỏ

Khi đèn nền sáng mà màn đen, đây là cách tách phần cứng ra khỏi TouchGFX. Dán tạm ngay sau `ST7789_Init()`:

```c
/* USER CODE BEGIN SPI1_Init 2 */
{
  static uint16_t line[240];
  for (int i = 0; i < 240; i++) line[i] = 0xF800;   /* RGB565 = do */
  ST7789_SetWindow(0, 0, 240, 240);                 /* ket thuc bang RAMWR */
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  HAL_SPI_Init(&hspi1);
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
  for (int y = 0; y < 240; y++)
    HAL_SPI_Transmit(&hspi1, (uint8_t *)line, 240, 100);
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
  while (1) { }                                     /* dung lai de nhin */
}
```

| Thấy gì | Kết luận |
|---|---|
| Đỏ toàn màn | dây, SPI, GPIO, driver đều đúng → lỗi ở tầng TouchGFX |
| Xanh dương | đảo byte — kiểm lại `SPI_DATASIZE_16BIT` |
| Đen, đèn nền sáng | lệnh không tới panel — sai dây DC/CS, hoặc SPI quá nhanh (7.4) |
| Đèn nền tối | code chưa bao giờ tới `ST7789_Init()` — hard fault hoặc `Error_Handler()` |

---

## 9. Khi nào nên dùng widget dựng sẵn

| **Nên dùng khi…** | **Đừng dùng khi…** |
|---|---|
| Đồng hồ chiếm trọn màn hình và kích thước đã chốt | Đồng hồ không chiếm trọn màn — preset là 240×240, layout khác là phải tự xuất ảnh, mất hết cái tiện |
| Mặt số cần texture, gradient, vân kim loại — Canvas không vẽ được | Kích thước hoặc màu sắc còn có thể đổi |
| Còn dư flash và cần bản chạy trong hôm nay | Flash đã chật — 246 KB là gần một nửa của 512 KB |
| Làm bàn thử: tách *"RTC và chuỗi tick có đúng không"* khỏi *"vẽ có đẹp không"* mà không tốn dòng code nào | Cần kim quét mượt liên tục thay vì nhảy từng giây |

Đối chiếu với cách tự vẽ:

| | AnalogClock dựng sẵn | Canvas `Line` + `Circle` |
|---|---|---|
| Flash cho tài nguyên | 246 KB | 0 B |
| Thời gian tới bản chạy đầu tiên | ~15 phút | vài giờ |
| Đổi kích thước / màu kim | xuất lại 4 PNG | sửa 1 hằng số |
| Mặt số có texture, gradient | được | không |
| Chi phí vẽ mỗi giây | 3 TextureMapper | 2 Line + 1 Circle |
| Cần buffer riêng | không | CanvasWidgetRenderer |

**Cách giữ cả hai:** bọc đồng hồ trong một container của riêng bạn — `ClockFace` — và để View chỉ gọi `clockFace.setTime(h, m, s)`. Bên trong là `AnalogClock` dựng sẵn hay Canvas tự vẽ, phần còn lại của project không biết và không cần biết. Sáu tháng sau đổi ý thì đó là **một file**, không phải làm lại.

---

## 10. Việc còn để ngỏ

**RTC đang chạy từ LSI, và sai số prescaler.** `.ioc` đặt `RTCFreq_Value = 32000` (LSI), nhưng prescaler vẫn để mặc định `AsynchPrediv = 127` / `SynchPrediv = 255` — bộ số dành cho thạch anh 32 768 Hz. Nhịp thật là 32 000 ÷ (128 × 256) ≈ **0.977 Hz**, tức đồng hồ **chạy chậm ≈2.3%** (≈34 phút mỗi ngày) *trước khi* tính tới sai số vốn đã lớn của LSI.

Hai cách sửa, nên làm cả hai:
- `SynchPrediv = 249` → 32 000 ÷ (128 × 250) = đúng 1 Hz với LSI.
- Chuyển nguồn RTC sang **LSE 32.768 kHz** (board có sẵn thạch anh ở PC14/PC15) rồi giữ 127/255. Đây mới là cách cho sai số dùng được lâu dài.

**Giờ khởi tạo là cố định.** `MX_RTC_Init()` đặt cứng 00:00:00 ngày 08/09/26 và **ghi đè mỗi lần reset**. Muốn giữ giờ qua reset thì phải kiểm tra backup register trong `USER CODE BEGIN Check_RTC_BKUP` và bỏ qua `HAL_RTC_SetTime()` nếu RTC đã được đặt trước đó.

**Linker script chưa khai báo các section của TouchGFX.** `STM32H523CETX_FLASH.ld` không có khối `ExtFlashSection` như project ST7789. Hiện `IntFlashSection` / `FontFlashSection` / `TextFlashSection` vẫn vào đúng FLASH vì linker tự đặt orphan section — build chạy, nhưng vị trí là do mặc định của linker chứ không phải do mình chỉ định. Thêm khối tường minh sau `.rodata` sẽ chắc chắn hơn:

```ld
ExtFlashSection : {
  *(ExtFlashSection ExtFlashSection.*)
  *(IntFlashSection IntFlashSection.*)
  *(FontFlashSection FontFlashSection.*)
  *(TextFlashSection TextFlashSection.*)
  . = ALIGN(4);
} >FLASH
```

---

## Tham khảo

- [Bản hướng dẫn đầy đủ, song ngữ VI/EN](https://vuxtechsolution.com/projects/stm32h5-analog-clock-touchgfx/)
- [TouchGFX — AnalogClock widget](https://support.touchgfx.com/docs/development/ui-development/ui-components/miscellaneous/analog-clock)
- [TouchGFX — Framebuffer Strategies](https://support.touchgfx.com/docs/development/ui-development/scenarios/framebuffer-strategies)
- [TouchGFX — Running on Low Cost Hardware](https://support.touchgfx.com/docs/development/ui-development/scenarios/lowcost-hardware)
- [RM0481 — STM32H523 reference manual](https://www.st.com/resource/en/reference_manual/rm0481-stm32h52333xx-stm32h56263xx-and-stm32h573xx-armbased-32bit-mcus-stmicroelectronics.pdf) — RTC và mux kernel clock của SPI

---

Mọi con số flash và RAM trên trang này đọc từ `Debug/analog_clock.map` của bản build thật trên STM32H523CET6, TouchGFX 4.26.1, CubeIDE 2.2.0, GCC 14.3.1, `-O0 -g3`. Build ở `-Os` sẽ cho `.text` nhỏ hơn đáng kể, nhưng **phần bitmap thì không đổi** — đó chính là điểm mấu chốt.
