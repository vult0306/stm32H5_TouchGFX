# H523_Blink

Project khởi động cho board **WeAct Studio STM32H523CET6**, nạp và debug bằng **SEGGER J-Link**.

Nội dung: nháy LED 1 Hz và in log qua `printf()`. Log có thể đi ra **UART1** hoặc **SEGGER RTT** — chọn bằng cách đổi build configuration, không phải sửa code.

📖 **Bản hướng dẫn đầy đủ (song ngữ VI/EN):** <https://vuxtechsolution.com/projects/stm32h5-blink-jlink/>
Dự án tiếp theo cùng board: [ST7789 + TouchGFX](../ST7789/README.md) · <https://vuxtechsolution.com/projects/stm32h5-st7789-touchgfx/>

---

## 1. Phần cứng

### Board

| | |
|---|---|
| MCU | STM32H523CETx — Cortex-M33, LQFP48, 512 KB Flash / 272 KB RAM |
| SYSCLK | 250 MHz (HSE 8 MHz → PLL1: M=1, N=62, FRACN=4096, P=2) |
| Thạch anh | HSE 8 MHz (PH0/PH1) · LSE 32.768 kHz (PC14/PC15) |
| LED | **PC13**, active-low (ghi `0` = sáng) |
| Nút KEY | PA0, active-low + pull-up (chưa dùng trong project này) |
| Debug | SWD — PA13 (SWDIO) / PA14 (SWCLK) |
| Log UART | **USART1 — PB14 (TX) / PB15 (RX)**, 115200 8-N-1 |

> Chân LED và KEY tham chiếu từ định nghĩa board `blackpill_h523ce` của Zephyr. Vài lô board WeAct có đổi chân — nếu LED không nháy, đối chiếu lại với schematic của đúng lô board bạn cầm.

### Nối J-Link (SWD)

| J-Link 20-pin | Board | Bắt buộc |
|---|---|---|
| 1 — VTref | 3V3 | ✅ J-Link dùng để đo mức logic, **không** cấp nguồn |
| 4 — GND | GND | ✅ |
| 7 — SWDIO | PA13 (DIO) | ✅ |
| 9 — SWCLK | PA14 (CLK) | ✅ |
| 15 — RESET | NRST | Nên nối — cứu khi chip vào low-power |

Cấp nguồn cho board bằng cáp **USB-C** riêng, đừng lấy 3.3V từ J-Link.

### Nối USB-TTL — chỉ cần cho `Debug_UART`

| Board | USB-TTL |
|---|---|
| **PB14** — USART1_TX | RXD |
| **PB15** — USART1_RX | TXD |
| GND | GND |

TX nối vào RX và ngược lại. GND phải nối chung.

---

## 2. Toolchain

| | |
|---|---|
| STM32CubeMX | 6.18.1 — **standalone** |
| STM32CubeIDE | 2.2.0 |
| SEGGER J-Link | Software pack + RTT V7.94e |

Từ **STM32CubeIDE 2.0.0**, ST đã tách CubeMX ra khỏi IDE. Menu `File → New → STM32 Project` không còn tồn tại. Project được tạo và cấu hình bằng CubeMX standalone, rồi import vào IDE.

---

## 3. Cấu hình `.ioc`

| Mục | Giá trị |
|---|---|
| SYS → Debug | Serial Wire |
| SYS → Timebase | SysTick |
| RCC → HSE | Crystal/Ceramic Resonator (8 MHz) |
| RCC → LSE | Crystal/Ceramic Resonator |
| Clock → HCLK | 250 MHz |
| PC13 | GPIO_Output · level **High** · Push Pull · No pull · Low speed · **Label = `LED`** |
| USART1 | Asynchronous · 115200 · 8 bit · None · 1 stop |
| ICACHE | Bật, 1-way |
| Project Manager | Toolchain = **STM32CubeIDE** · **Generate Under Root** ✔ · Structure = Advanced |

`GPIO output level = High` là vì LED active-low — để LED tắt lúc khởi động.

---

## 4. Build configurations

| Configuration | Backend của `printf()` | Define | `Core/RTT` |
|---|---|---|---|
| `Debug_UART` | USART1 @ 115200 (PB14) | `LOG_BACKEND_UART` | Exclude from Build |
| `Debug_RTT` | SEGGER RTT qua SWD | `LOG_BACKEND_RTT`, `RTT_USE_ASM=0` | Biên dịch |

Đổi backend = `chuột phải project → Build Configurations → Set Active`. Code không đổi một dòng nào.

`Debug_RTT` có thêm include path `../Core/RTT`.

### Cấu trúc

```
H523_Blink/
├── Core/
│   ├── Inc/          # main.h, stm32h5xx_hal_conf.h, stm32h5xx_it.h
│   ├── Src/          # main.c, syscalls.c, ...
│   ├── Startup/      # startup_stm32h523cetx.s
│   └── RTT/          # SEGGER RTT — copy tay vào, xem mục 6
│       ├── SEGGER_RTT.c
│       ├── SEGGER_RTT.h
│       └── SEGGER_RTT_Conf.h
├── Drivers/          # HAL + CMSIS (CubeMX sinh ra)
├── H523_Blink.ioc    # nguồn sự thật cho cấu hình phần cứng
└── STM32H523CETX_FLASH.ld
```

`Core` đã là source folder nên `Core/RTT` được biên dịch tự động — không cần khai báo thêm.

### Cách code chọn backend

Trong `Core/Src/main.c`, vùng `USER CODE`:

```c
int __io_putchar(int ch)
{
#if defined(LOG_BACKEND_RTT)
  SEGGER_RTT_PutChar(0, (char)ch);
#else
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
#endif
  return ch;
}
```

`printf()` của newlib gọi xuống `_write()` trong `Core/Src/syscalls.c`, `_write()` gọi `__io_putchar()`. Đè hàm đó là đủ để chuyển hướng toàn bộ `printf`.

Nếu tạo configuration mới mà quên define backend, build sẽ dừng ngay ở `#error` trong `USER CODE BEGIN PD` — chủ ý, để không âm thầm rơi vào nhánh UART.

---

## 5. Quy trình làm việc

### Sửa cấu hình phần cứng

1. Mở `H523_Blink.ioc` bằng **CubeMX standalone**
2. Sửa, kiểm tra Project Manager vẫn là `STM32CubeIDE` + `Generate Under Root`
3. **GENERATE CODE**
4. Về CubeIDE, chọn project, nhấn **F5** — CubeIDE 2.0 chưa tự refresh cây file

> ⚠️ **Commit git trước mỗi lần generate.** CubeMX ghi đè `.cproject` và **hai build configuration sẽ biến mất**. Sau khi generate, chạy `git diff .cproject` để thấy ngay cái gì mất, rồi khôi phục.

### Build & nạp

`Run → Debug Configurations…` → tab **Debugger**:

| Mục | Giá trị |
|---|---|
| Debug probe | **SEGGER J-LINK** (mặc định là ST-LINK — phải đổi) |
| Interface | SWD |
| Device name | `STM32H523CE` |
| Speed | 4000 kHz (chập chờn thì hạ 1000) |
| Reset behaviour | Connect under reset |

Tab **Main**, để một launch config dùng chung cho cả hai build configuration:

- C/C++ Application: `${config_name:H523_Blink}/H523_Blink.elf`
- Build Configuration: **Use Active**

### Xem log — `Debug_UART`

`Window → Show View → Terminal` → **Open a Terminal** → Serial Terminal → COM port của USB-TTL, **115200 8-N-1**.

### Xem log — `Debug_RTT`

Không cần dây nào ngoài SWD.

- **Cách thường dùng:** bấm Debug trong CubeIDE trước, rồi mở **J-Link RTT Viewer** → Connection = **Existing Session**.
- **Standalone:** Target Device `STM32H523CE`, SWD 4000 kHz, RTT Control Block = Auto Detection.

Output mong đợi:

```
==========================================
  Hello World from STM32H523CET6!
  Backend: SEGGER RTT
  SYSCLK : 250000000 Hz
  Build  : Aug 22 2026 16:40:12
==========================================

Blink #1  ->  LED ON
Blink #2  ->  LED OFF
```

---

## 6. Dựng lại từ đầu

Cần khi clone mới, hoặc sau khi CubeMX xoá mất build configuration.

**1. Copy source SEGGER RTT** (nếu `Core/RTT/` chưa có trong repo)

```bash
mkdir -p Core/RTT
cp ~/gitwork/segger_rtt/SEGGER_RTT_V794e/RTT/SEGGER_RTT.c \
   ~/gitwork/segger_rtt/SEGGER_RTT_V794e/RTT/SEGGER_RTT.h \
   ~/gitwork/segger_rtt/SEGGER_RTT_V794e/Config/SEGGER_RTT_Conf.h \
   Core/RTT/
```

Không copy `SEGGER_RTT_ASM_ARMv7M.S` — xem mục 7.

**2. Import vào CubeIDE** — dùng **đúng** menu này:

```
File → STM32 Project Create/Import
     → Import STM32 Project
     → STM32CubeMX/STM32CubeIDE Project → Next
     → trỏ tới thư mục project → Finish
```

**3. Tạo hai configuration**

`Chuột phải project → Build Configurations → Manage…`

- Rename `Debug` → `Debug_UART`
- New… → `Debug_RTT`, copy settings from `Debug_UART`

**4. Thiết lập từng cái**

`Properties → C/C++ Build → Settings` — chọn đúng **Configuration** ở dropdown trên đầu trước khi sửa bất cứ gì.

| Configuration | MCU/MPU GCC Compiler → Define symbols | → Include paths |
|---|---|---|
| `Debug_UART` | `LOG_BACKEND_UART` | — |
| `Debug_RTT` | `LOG_BACKEND_RTT`, `RTT_USE_ASM=0` | `../Core/RTT` |

**5. Loại RTT khỏi bản UART**

Set active = `Debug_UART`, rồi `chuột phải Core/RTT → Resource Configurations → Exclude from Build…` → tick `Debug_UART`, bỏ tick `Debug_RTT`.

---

## 7. Bẫy đã gặp

**Nút Build (cái búa) bị làm mờ sau khi import.**
Project mất C/C++ nature vì import bằng `File → Import → General → Existing Projects into Workspace` hoặc `Open Projects from File System`. Hai đường đó import thành *general project*. Phải dùng `File → STM32 Project Create/Import`. Kiểm tra nhanh: thư mục project phải có file ẩn `.cproject`, và `Core`/`Drivers` phải hiện icon thư mục kèm chữ **C** màu tím.

**Link lỗi `undefined reference to SEGGER_RTT_ASM_WriteSkipNoLock`.**
`SEGGER_RTT.h` tự bật `RTT_USE_ASM=1` khi thấy Cortex-M33 và chuyển sang gọi hàm assembly. Nhưng `SEGGER_RTT_ASM_ARMv7M.S` **không include** `SEGGER_RTT.h` — nó chỉ sinh code khi `RTT_USE_ASM` được define trên dòng lệnh **assembler**, nên file assemble ra rỗng. Cách gọn: define `RTT_USE_ASM=0` và bỏ hẳn file `.S`. Bản C thuần vẫn nhanh hơn UART rất nhiều.

**Thiết lập rơi nhầm configuration.**
Trong `Properties → C/C++ Build → Settings` có dropdown **Configuration** ở trên đầu. Quên đổi nó là thiết lập rơi vào config đang active, và không có gì cảnh báo.

**RTT Viewer không có `STM32H523CE` trong danh sách device.**
Bản J-Link software cũ hơn con chip. Dùng **Existing Session** (ô device bị vô hiệu), hoặc chọn `Cortex-M33` (hãng *Unspecified*) + RTT Control Block = **Search Range** `0x20000000 0x44000` (272 KB SRAM của H523). Muốn triệt để thì update J-Link software.

**Code biến mất sau khi generate lại.**
Chỉ viết giữa các cặp `/* USER CODE BEGIN x */` … `/* USER CODE END x */`. Mọi thứ ngoài đó bị CubeMX xoá. Khôi phục: `chuột phải file → Compare With → Local History…`

**`printf("%f")` in ra rác.**
Newlib-nano tắt hỗ trợ float. Bật lại bằng flag `-u _printf_float` ở `MCU/MPU GCC **Linker** → Miscellaneous → Other flags`. Đây là flag của **linker**, đặt ở Compiler thì gcc nhận rồi bỏ qua, không có tác dụng gì.

---

## 8. Phụ lục — SWV / ITM (chưa dùng)

CubeIDE cũng có SWV/ITM Console và nó chạy được với J-Link, nhưng cần thêm dây **PB3 (TRACESWO) → J-Link chân 13**, cộng thêm build configuration thứ ba với define `LOG_BACKEND_SWO`, `ITM_SendChar()` trong `__io_putchar()`, và bật `DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_CLKEN` bằng tay (vì J-Link DLL chưa biết STM32H523 nên không tự chạy trình tự trace-init).

Với nhu cầu `printf` thuần thì RTT hơn hẳn: nhanh hơn, không tốn chân, không tốn dây, không phụ thuộc khai báo core clock đúng hay sai. SWV chỉ đáng bật khi cần **Exception Trace**, **Data Trace** hoặc **Statistical Profiling** — những thứ RTT không làm được.

---

## Tham khảo

- [Zephyr — Black Pill STM32H523](https://docs.zephyrproject.org/latest/boards/weact/blackpill_h523ce/doc/index.html) — chân LED, KEY, HSE
- [WeActStudio.STM32H523CoreBoard](https://github.com/WeActStudio/WeActStudio.STM32H523CoreBoard/) — schematic
- [STM32CubeIDE 2.0.0 workflow tutorial](https://community.st.com/t5/stm32-mcus/stm32cubeide-2-0-0-workflow-tutorial/ta-p/864831) — quy trình import mới
- [What's new in STM32CubeIDE 2.0.0](https://community.st.com/t5/developer-news/what-s-new-in-stm32cubeide-2-0-0/ba-p/856658) — tách CubeMX
- [J-Link RTT Viewer — SEGGER KB](https://kb.segger.com/J-Link_RTT_Viewer)
