# -----------------------------------------------------------------------------
# Reproducible GNU Arm build for STM32F411RE
#
# Design note: Make remains the canonical build tool to preserve the project's
# original workflow. Debug and Release use separate directories so changing
# optimization flags can never reuse incompatible object files.
# -----------------------------------------------------------------------------

CROSS_COMPILE ?= arm-none-eabi-
CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE    := $(CROSS_COMPILE)size
OBJDUMP := $(CROSS_COMPILE)objdump
GDB     := $(CROSS_COMPILE)gdb
OPENOCD ?= openocd
HOST_CC ?= gcc

TARGET     := game_embed
BUILD_TYPE ?= Debug
BUILD_DIR  := build/$(BUILD_TYPE)
ELF        := $(BUILD_DIR)/$(TARGET).elf
BIN        := $(BUILD_DIR)/$(TARGET).bin
HEX        := $(BUILD_DIR)/$(TARGET).hex
MAP        := $(BUILD_DIR)/$(TARGET).map
LST        := $(BUILD_DIR)/$(TARGET).lst
HOST_TEST  := build/host/ring_buffer_test
HOST_SAVER_TEST := build/host/screen_saver_test

SOURCES := \
	app/app.c \
	app/game_state.c \
	app/game_session.c \
	app/screen_saver.c \
	app/button_task.c \
	app/screen_task.c \
	app/buzzer_task.c \
	app/led_task.c \
	app/stack_monitor.c \
	app/container/ring_buffer.c \
	app/resources/bitmap.c \
	app/resources/sound.c \
	app/gameObject/meteoroid/meteoroid.c \
	app/gameObject/archery/archery.c \
	app/gameObject/arrow/arrow.c \
	app/gameObject/border/border.c \
	app/gameObject/bang/bang.c \
	bsp_drivers/led/src/led.c \
	bsp_drivers/button/src/button.c \
	bsp_drivers/buzzer/src/buzzer.c \
	bsp_drivers/screen/src/screen.c \
	startup/startup_stm32f411re.c \
	system/src/system_init.c \
	system/src/system_it.c \
	system/src/system_log.c \
	system/src/system_fault.c \
	system/src/syscalls.c \
	system/src/system_stm32f4xx.c \
	system/src/freertos_hooks.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_flash.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_i2c.c \
	ThirdParty/STM32F4xx_StdPeriph_Driver/src/misc.c \
	ThirdParty/FreeRTOS/src/croutine.c \
	ThirdParty/FreeRTOS/src/event_groups.c \
	ThirdParty/FreeRTOS/src/heap_4.c \
	ThirdParty/FreeRTOS/src/list.c \
	ThirdParty/FreeRTOS/src/port.c \
	ThirdParty/FreeRTOS/src/queue.c \
	ThirdParty/FreeRTOS/src/stream_buffer.c \
	ThirdParty/FreeRTOS/src/tasks.c \
	ThirdParty/FreeRTOS/src/timers.c \
	ThirdParty/SSD1306/src/fonts.c \
	ThirdParty/SSD1306/src/ssd1306.c

INCLUDE_DIRS := \
	app \
	app/container \
	app/resources \
	app/gameObject/meteoroid \
	app/gameObject/archery \
	app/gameObject/arrow \
	app/gameObject/border \
	app/gameObject/bang \
	bsp_drivers/led/inc \
	bsp_drivers/button/inc \
	bsp_drivers/buzzer/inc \
	bsp_drivers/screen/inc \
	system/inc \
	ThirdParty/STM32F4xx_StdPeriph_Driver/inc \
	ThirdParty/FreeRTOS/inc \
	ThirdParty/CMSIS/inc \
	ThirdParty/SSD1306/inc

OBJECTS := $(addprefix $(BUILD_DIR)/,$(SOURCES:.c=.o))
DEPS    := $(OBJECTS:.o=.d)
INCLUDES := $(addprefix -I,$(INCLUDE_DIRS))

ARCH_FLAGS := \
	-mcpu=cortex-m4 \
	-mthumb \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16

CPPFLAGS := \
	-DUSE_STDPERIPH_DRIVER \
	-DSTM32F411xE \
	$(INCLUDES)

CFLAGS_COMMON := \
	$(ARCH_FLAGS) \
	-std=gnu11 \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wformat=2 \
	-Wstrict-prototypes \
	-Werror \
	-ffunction-sections \
	-fdata-sections \
	-MMD \
	-MP

ifeq ($(BUILD_TYPE),Debug)
	CFLAGS := $(CFLAGS_COMMON) -Og -g3 -DDEBUG
else ifeq ($(BUILD_TYPE),Release)
	CFLAGS := $(CFLAGS_COMMON) -Os -g1 -flto -DNDEBUG
else
	$(error BUILD_TYPE must be Debug or Release)
endif

LDFLAGS := \
	$(ARCH_FLAGS) \
	--specs=nano.specs \
	-T linker_scripts.ld \
	-Wl,-Map=$(MAP) \
	-Wl,--gc-sections \
	-Wl,--orphan-handling=warn \
	-Wl,--cref \
	-Wl,--fatal-warnings \
	-Wl,--print-memory-usage

ifeq ($(BUILD_TYPE),Release)
	LDFLAGS += -flto
endif

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

all: check-toolchain $(ELF) $(BIN) $(HEX)
	@echo "================================"
	@echo "Build complete: $(BUILD_TYPE)"
	@echo "================================"
	@$(SIZE) $(ELF)

check-toolchain:
	@$(CC) --version | head -n 1

$(ELF): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Linking: $@"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Preserve the source directory tree under build/ to avoid basename collisions.
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN): $(ELF)
	@echo "Creating binary: $@"
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	@echo "Creating Intel HEX: $@"
	$(OBJCOPY) -O ihex $< $@

$(LST): $(ELF)
	@echo "Creating annotated listing: $@"
	$(OBJDUMP) -d -S $< > $@

size: $(ELF)
	@echo "Memory usage:"
	$(SIZE) $(ELF)

objdump: $(ELF)
	$(OBJDUMP) -h -S $(ELF)

listing: $(LST)

# A small host test validates the reusable container without target hardware.
$(HOST_TEST): tests/test_ring_buffer.c app/container/ring_buffer.c app/container/ring_buffer.h
	@mkdir -p $(dir $@)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iapp/container \
		tests/test_ring_buffer.c app/container/ring_buffer.c -o $@

$(HOST_SAVER_TEST): tests/test_screen_saver.c app/screen_saver.c app/screen_saver.h
	@mkdir -p $(dir $@)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iapp \
		tests/test_screen_saver.c app/screen_saver.c -o $@

test: $(HOST_TEST) $(HOST_SAVER_TEST)
	$(HOST_TEST)
	$(HOST_SAVER_TEST)

flash: all
	@echo "Flashing $(ELF) via OpenOCD..."
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(ELF) verify reset exit"

full_flash_erase:
	@echo "WARNING: erasing application and persistent score data..."
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "init; reset halt; stm32f4x mass_erase 0; exit"

openocd_server:
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f4x.cfg

debug: $(ELF)
	# Connect to a separately running `make openocd_server` session.
	$(GDB) -q $(ELF) -ex "target extended-remote :3333" \
		-ex "monitor reset halt"

clean:
	@echo "Cleaning generated build outputs..."
	rm -rf build

rebuild: clean all

-include $(DEPS)

.PHONY: \
	all check-toolchain size objdump listing flash full_flash_erase \
	openocd_server debug clean rebuild test
