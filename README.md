# DoerGPIO Firmware

Firmware for the **STM32F070C6T6** microcontroller that exposes GPIO, PWM, and ADC control over UART. Includes a minimal bootloader with UART-based firmware update support and a Python upload tool.

- **Hardware version:** 0.2
- **Software version:** 0.1
- **MCU:** STM32F070C6T6 (Cortex-M0, 32 KB Flash, 16 KB RAM, LQFP48)

---

## Architecture

The system is split into two independently-built firmware images:

| Region | Address | Size | Description |
|--------|---------|------|-------------|
| Bootloader | `0x08000000` | 8 KB | UART update handler + application launcher |
| Application | `0x08002000` | 24 KB | GPIO/PWM/ADC command processor |

On power-up, the bootloader waits **500 ms** for a trigger byte from the host. If received, it enters firmware update mode. Otherwise it jumps to the application.

---

## Features

- **28 GPIO pins** (GPIOA, GPIOB, GPIOF) with unified numbering
- **12-bit ADC** on 8 pins
- **PWM** on 8 pins across 5 timers (TIM1, TIM3, TIM14, TIM16, TIM17)
- **UART command interface** at 230400 baud, ASCII-based, DMA + idle-line interrupt driven
- **Bootloader** with chunk-based XOR-verified flashing and automatic system reset
- **Python upload tool** with auto device detection and progress reporting

---

## UART Command Protocol

Commands are sent as ASCII character pairs terminated with the letter **`Z`**. Multiple commands can be chained before the terminator. Responses are separated by `;` and end with `\r\n`.

### GPIO Numbering

The first character of each command selects the GPIO pin:

| Char | GPIO # | Char | GPIO # |
|------|--------|------|--------|
| `a`  | 0      | `o`  | 14     |
| `b`  | 1      | `p`  | 15     |
| `c`  | 2      | `q`  | 16     |
| `d`  | 3      | `r`  | 17     |
| `e`  | 4      | `s`  | 18     |
| `f`  | 5      | `t`  | 19     |
| `g`  | 6      | `u`  | 20     |
| `h`  | 7      | `v`  | 21     |
| `i`  | 8      | `w`  | 22     |
| `j`  | 9      | `x`  | 23     |
| `k`  | 10     | `y`  | 24     |
| `l`  | 11     | `z`  | 25     |
| `m`  | 12     | `{`  | 26     |
| `n`  | 13     | `\|` | 27     |

### Commands

| Code | Function | Response |
|------|----------|----------|
| `I` | Set pin as input | `GPIO#: Input` |
| `O` | Set pin as output | `GPIO#: Output` |
| `1` | Set output HIGH | `GPIO#: HIGH` |
| `0` | Set output LOW | `GPIO#: LOW` |
| `?` | Read digital value | `GPIO#: 0` or `GPIO#: 1` |
| `U` | Enable pull-up | `GPIO#: Pull-up` |
| `D` | Enable pull-down | `GPIO#: Pull-down` |
| `N` | Disable pull resistor | `GPIO#: No pull` |
| `A` | Enable ADC | `GPIO#: ADC enabled` |
| `R` | Read ADC (12-bit) | `GPIO#: ADC 2048` |
| `P` | Enable PWM | `GPIO#: PWM enabled` |
| `F<freq>` | Set PWM frequency (Hz) | `GPIO#: Freq 1000` |
| `C<duty>` | Set PWM duty cycle (0–100) | `GPIO#: Duty 50%` |
| `VV` | Read firmware version | `DoerGPIO_HW0.2_SW0.1` |
| `RR` | System reset | *(board resets)* |

### Examples

```
# Set GPIO0 HIGH
a1Z

# Set GPIO0 as input, read it
aIa?Z

# Enable PWM on GPIO9, set 5 kHz, 25% duty
jPjF5000jC25Z

# Read ADC on GPIO10
kAkRZ

# Read firmware version
VVZ
```

---

## GPIO Pin Map

| GPIO # | Port | Pin | ADC Channel | PWM Timer |
|--------|------|-----|-------------|-----------|
| 0  | PA | 1  | ADC_IN1 | — |
| 1  | PB | 12 | — | — |
| 2  | PB | 7  | — | — |
| 3  | PB | 6  | — | — |
| 4  | PA | 8  | — | TIM1_CH1 |
| 5  | PA | 4  | ADC_IN4 | — |
| 6  | PB | 13 | — | — |
| 7  | PB | 5  | — | — |
| 8  | PB | 11 | — | — |
| 9  | PA | 6  | ADC_IN6 | TIM3_CH1 / TIM16_CH1 |
| 10 | PA | 7  | ADC_IN7 | TIM14_CH1 / TIM17_CH1 |
| 11 | PA | 5  | ADC_IN5 | — |
| 12 | PF | 0  | — | — |
| 13 | PA | 14 | — | — |
| 14 | PA | 2  | ADC_IN2 | — |
| 15 | PA | 3  | ADC_IN3 | — |
| 16 | PB | 8  | — | TIM16_CH1 |
| 17 | PB | 15 | — | — |
| 18 | PB | 1  | ADC_IN9 | TIM3_CH4 |
| 19 | PA | 15 | — | — |
| 20 | PB | 9  | — | TIM17_CH1 |
| 21 | PB | 10 | — | — |
| 22 | PA | 11 | — | TIM1_CH4 |
| 23 | PB | 4  | — | — |
| 24 | PB | 3  | — | — |
| 25 | PB | 2  | — | — |
| 26 | PB | 0  | ADC_IN8 | TIM3_CH3 |
| 27 | PB | 14 | — | — |

### PWM Timer Sharing

Pins that share a timer also share their frequency setting:

| Timer | Shared Pins |
|-------|-------------|
| TIM1  | GPIO 4, GPIO 22 |
| TIM3  | GPIO 9, GPIO 18, GPIO 26 |

TIM14, TIM16, TIM17 each control a single pin independently.

**PWM specs:**
- Timer clock: 1 MHz (8 MHz HSI / prescaler 8)
- Frequency range: 1 Hz – 500 kHz
- Default frequency: 1 kHz
- Duty cycle: 0–100%

---

## Bootloader

The bootloader lives at `0x08000000` and is built independently in the `Bootloader/` directory.

### Boot sequence

1. Initializes SysTick and UART1 (230400 baud)
2. Waits up to **500 ms** for trigger byte `0x7F`
3. If trigger received → firmware update mode
4. Otherwise → jump to application at `0x08002000`

### Update protocol

```
Host  →  0x7F                         (trigger)
Board →  0x79                         (ACK)
Host  →  4-byte firmware size (LE)
Board →  0x79 (ACK) or 0x1F (NAK)
[For each 64-byte chunk:]
  Host  →  64 bytes + 1 XOR checksum byte
  Board →  0x79 (ACK) or 0x1F (NAK, retry)
Board →  0x79                         (final ACK)
Board →  XOR checksum of entire firmware
Board →  System reset
```

Maximum application size: **120 KB** (validated before flashing begins).

---

## Python Upload Tool

`tools/upload_fw.py` implements the bootloader protocol and handles device detection automatically.

### Requirements

```bash
pip install -r tools/requirements.txt   # pyserial>=3.5
```

### Usage

```bash
python3 tools/upload_fw.py <firmware.bin> [--timeout 60]
```

The tool:
1. Waits for a device with USB VID:PID `1a86:7523` to appear (configurable timeout, default 60 s)
2. Opens the serial port at 230400 baud
3. Sends the trigger and waits for ACK
4. Uploads firmware in 64-byte chunks with XOR verification
5. Confirms final XOR checksum
6. Reports success; board resets automatically

---

## Building

### Prerequisites

- `arm-none-eabi-gcc` (GCC ARM Embedded toolchain)
- STM32CubeIDE (for the main application)
- `make` (for the bootloader)

### Bootloader

```bash
cd Bootloader
make
# Output: build/bootloader.bin, build/bootloader.elf
```

### Application

Open the project in **STM32CubeIDE** and build with the `Debug` configuration, or invoke the IDE's build system directly. Build outputs appear in `Debug/`:

- `DoerGPIO_FW.bin` — raw binary for flashing
- `DoerGPIO_FW.elf` — ELF with debug symbols
- `DoerGPIO_FW.map` — linker map

### Flashing

Flash the bootloader to `0x08000000` and the application to `0x08002000` using a programmer (ST-Link, J-Link, etc.) or use `tools/upload_fw.py` for over-UART updates once the bootloader is installed.

---

## Project Structure

```
DoerGPIO_FW/
├── Bootloader/                   # Standalone bootloader
│   ├── Core/Src/main.c           # Bootloader logic
│   ├── Makefile
│   └── STM32F070C6TX_BOOTLOADER.ld
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   └── gpio_wrapper.h        # GPIO/PWM/ADC API
│   └── Src/
│       ├── main.c                # UART command processor
│       └── gpio_wrapper.c        # GPIO/PWM/ADC implementation
├── Drivers/                      # STM32 HAL + CMSIS
├── tools/
│   ├── upload_fw.py              # Firmware upload tool
│   └── requirements.txt
├── DoerGPIO_FW.ioc               # STM32CubeMX configuration
└── STM32F070C6TX_FLASH.ld        # Application linker script
```
