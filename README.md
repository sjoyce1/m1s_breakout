# Sipeed M1s Dock - Vertical Breakout / Brickbat (60 FPS)

A C game engine written for the **Sipeed M1s Dock** (Bouffalo Lab BL808 RISC-V SoC) using the **MaixHAL / `bflb_mcu_sdk`** framework.

---

## 🎮 Game Features & Architecture

- **Orientation & Display**: Tailored for the **280x240 SPI LCD** (ST7789V / NV3041A) with ultra-fast dirty-rectangle rendering targeting steady **60 FPS**.
- **Playfield Layout**:
  - **Left Side**: 5 columns x 10 rows (50 bricks) with column-specific colors and progressive scores (Red = 50, Orange = 40, Yellow = 30, Green = 20, Cyan = 10).
  - **Right Side**: Vertical player paddle ($X = 266$).
  - **Goal Line**: If the ball escapes past the right boundary, a life is lost.
- **Hybrid Controls**:
  1. **GPIO Buttons**: Onboard `GPIO 22` (Up) & `GPIO 23` (Down) with internal pull-up and 8-sample debounce.
  2. **DVP Camera Vision Tracking**: Real-time luminance centroid extraction over a camera slice buffer from the GC0328 sensor. Follows bright markers / hands / flashlights with exponential moving average (EMA) smoothing.
- **Autonomous Attract Mode (Auto-Pilot)**:
  - Activates automatically after **10 seconds** of inactivity.
  - An authentic arcade AI tracks the ball's Y-coordinate with simulated tracking lag and randomized offset jitter.
  - Flashes a retro `* AUTO-PILOT *` HUD banner.
  - Instantly relinquishes control back to the player the moment a GPIO button is pressed or optical gesture motion is detected.

---

## 📂 Project Structure

```
m1s_breakout/
├── CMakeLists.txt              # CMake build configuration for bflb_mcu_sdk
├── Makefile                    # Standard Makefile build wrapper
├── proj.conf                   # BL808 board & peripheral enablement
├── README.md                   # Documentation & build instructions
├── include/
│   ├── game_config.h           # Screen dimensions, pinouts, brick grid, physics constants
│   ├── display_driver.h        # ST7789 SPI LCD driver API & 5x7 font renderer
│   ├── camera_vision.h         # DVP camera DMA capture & bright centroid tracking
│   ├── input_ctrl.h            # GPIO polling, idle timer, hybrid arbitration
│   └── breakout_game.h         # Game state machine, physics, dirty-rect blitter
└── src/
    ├── main.c                  # System boot, hardware initialization, 60 FPS master loop
    ├── display_driver.c        # SPI DMA transfers, fast block fill, glyph rendering
    ├── camera_vision.c         # GC0328 I2C init, DVP slice buffer, centroid math
    ├── input_ctrl.c            # Debounce, idle timeout, priority arbitration
    └── breakout_game.c         # Vertical breakout ball/paddle mechanics, attract AI
```

---

## 🔌 Hardware Pin Mapping (M1s Dock)

| Peripheral | Function / Pin | BL808 GPIO |
| :--- | :--- | :--- |
| **Buttons** | Button Up | `GPIO 22` (Active LOW, Pull-Up) |
| | Button Down | `GPIO 23` (Active LOW, Pull-Up) |
| **SPI LCD** | MOSI (SDA) | `GPIO 3` (SPI0 MOSI) |
| | SCLK (SCL) | `GPIO 2` (SPI0 SCLK) |
| | CS | `GPIO 12` |
| | DC (Data/Command)| `GPIO 13` |
| | RESET | `GPIO 11` |
| | Backlight | `GPIO 14` |
| **DVP Camera** | I2C SCL / SDA | `I2C0` (GC0328 @ `0x21`) |
| | DVP Data / DMA | `CAM0` DVP QQVGA Interface |

---

## 🛠️ Build & Flashing Instructions

### Prerequisites
1. Ensure the Bouffalo Lab MCU SDK (`bflb_mcu_sdk`) and RISC-V GCC toolchain (`riscv64-unknown-elf-gcc`) are installed.
2. Ensure `bflb-mcu-tool` (or Bouffalo Flash Cube) is available.

### Compilation
From the `m1s_breakout` directory:
```bash
# Export SDK path if located elsewhere:
export BL_SDK_BASE=/path/to/bflb_mcu_sdk

# Build firmware for BL808 D0 core:
make CHIP=bl808 BOARD=bl808_m1s_dock CPU_ID=d0 -j8
```

### Flashing to M1s Dock
Connect your M1s Dock via the UART/Burn USB port:
```bash
bflb-mcu-tool --chip=bl808 --port=/dev/ttyUSB0 --baudrate=2000000 --firmware=build/build_out/m1s_breakout_bl808.bin
```
*(On Windows, replace `/dev/ttyUSB0` with the appropriate `COMx` port).*
