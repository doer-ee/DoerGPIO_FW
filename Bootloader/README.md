# STM32F070C6 Bootloader

Simple bootloader that runs at 0x08000000 and jumps to the application at 0x08002000.

## Memory Layout

- **Bootloader**: 0x08000000 - 0x08001FFF (8KB)
- **Application**: 0x08002000 - 0x08007FFF (24KB)

## Features

- Minimal bootloader (~1KB)
- Jumps directly to application firmware
- Future: UART firmware update capability

## Building

### Prerequisites

- `arm-none-eabi-gcc` toolchain
- GNU Make

### Build Commands

```bash
cd Bootloader
make all
```

### Build Outputs

- `build/bootloader.elf` - ELF file with debug symbols
- `build/bootloader.bin` - Binary file for flashing
- `build/bootloader.hex` - Intel HEX format
- `build/bootloader.map` - Memory map file

### Clean Build

```bash
make clean
```

## Flashing

### Using STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD -d build/bootloader.bin 0x08000000 -v
```

### Using OpenOCD

```bash
openocd -f interface/stlink.cfg -f target/stm32f0x.cfg \
        -c "program build/bootloader.elf verify reset exit"
```

## Development Roadmap

### Phase 1: Basic Jump (Current)
- [x] Jump from bootloader to application
- [x] Minimal startup code
- [x] Build system

### Phase 2: Firmware Update (Future)
- [ ] UART initialization
- [ ] Command protocol (Update, Go, Version)
- [ ] Binary reception with CRC check
- [ ] Flash erase and write
- [ ] Application validation

### Phase 3: Enhanced Features (Future)
- [ ] Timeout for entering update mode
- [ ] Button/GPIO trigger for update mode
- [ ] LED status indication
- [ ] Encrypted firmware support

## File Structure

```
Bootloader/
├── Core/
│   ├── Inc/              (Headers - empty for now)
│   ├── Src/
│   │   └── main.c        (Bootloader main code)
│   └── Startup/
│       └── startup_stm32f070c6tx.s  (Startup assembly)
├── build/                (Build artifacts)
├── STM32F070C6TX_BOOTLOADER.ld  (Linker script)
├── Makefile
└── README.md
```

## Notes

- The application firmware must be compiled to start at 0x08002000
- Application should relocate vector table to RAM (already done in main firmware)
- Bootloader does NOT require HAL libraries (bare metal)
