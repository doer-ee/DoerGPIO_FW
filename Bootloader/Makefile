################################################################################
# Makefile for STM32F070C6 Bootloader
################################################################################

# Target name
TARGET = bootloader

# Build directory
BUILD_DIR = build

# Toolchain
CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# Source files
C_SOURCES = \
Core/Src/main.c

ASM_SOURCES = \
Core/Startup/startup_stm32f070c6tx.s

# Linker script
LDSCRIPT = STM32F070C6TX_BOOTLOADER.ld

# Include paths
C_INCLUDES = \
-ICore/Inc

# Compiler flags
CPU = -mcpu=cortex-m0
MCU = $(CPU) -mthumb -mfloat-abi=soft

# C defines
C_DEFS = \
-DSTM32F070x6

# Optimization
OPT = -Os

# Compiler flags
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += -g3 -gdwarf-2

# Generate dependency info
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# Linker flags
LDFLAGS = $(MCU) -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -static -nostartfiles -nostdlib

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# List of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "Compiling: $<"
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "Assembling: $<"
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "Linking: $@"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "Creating hex: $@"
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "Creating bin: $@"
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR):
	mkdir $@

# Clean
clean:
	-rm -fR $(BUILD_DIR)

# Dependencies
-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
