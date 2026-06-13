# Toolchain. Down load at: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
OBJDUMP = arm-none-eabi-objdump
DBG = arm-none-eabi-gdb

# Project name
TARGET = game_embed

# Directories
BUILD_DIR 				= build
RESOURCE_INC_DIR		= app/resource
APP_INC_DIR 			= app
APP_SRC_DIR 			= app
STD_INC_DIR 			= ThirdParty/STM32F4xx_StdPeriph_Driver/inc
STD_SRC_DIR 			= ThirdParty/STM32F4xx_StdPeriph_Driver/src
CMSIS_INC_DIR 			= ThirdParty/CMSIS/inc
SSD1306_INC_DIR			= ThirdParty/SSD1306/inc
SSD1306_SRC_DIR			= ThirdParty/SSD1306/src
STARTUP_DIR 			= startup
SYS_INC_DIR 			= system/inc
SYS_SRC_DIR 			= system/src
RTOS_SRC_DIR			= ThirdParty/FreeRTOS/src
RTOS_INC_DIR			= ThirdParty/FreeRTOS/inc
BSP_LED_INC_DIR			= bsp_drivers/led/inc
BSP_LED_SRC_DIR			= bsp_drivers/led/src
BSP_BUTTON_INC_DIR		= bsp_drivers/button/inc
BSP_BUTTON_SRC_DIR		= bsp_drivers/button/src
BSP_BUZZER_INC_DIR		= bsp_drivers/buzzer/inc
BSP_BUZZER_SRC_DIR		= bsp_drivers/buzzer/src
BSP_SCREEN_INC_DIR 		= bsp_drivers/screen/inc
BSP_SCREEN_SRC_DIR 		= bsp_drivers/screen/src
RESOURCES_DIR 			= app/resources
GAME_OBJ_METEOROID		= app/gameObject/meteoroid
GAME_OBJ_ARCHERY		= app/gameObject/archery
GAME_OBJ_ARROW			= app/gameObject/arrow
GAME_OBJ_BORDER			= app/gameObject/border
GAME_OBJ_BANG			= app/gameObject/bang
CONTAINER				= app/container

# Source files
SOURCES = $(APP_SRC_DIR)/app.c \
		  $(APP_SRC_DIR)/button_task.c \
		  $(APP_SRC_DIR)/screen_task.c \
		  $(APP_SRC_DIR)/buzzer_task.c \
		  $(APP_SRC_DIR)/led_task.c \
          $(STD_SRC_DIR)/stm32f4xx_gpio.c \
		  $(STD_SRC_DIR)/stm32f4xx_rcc.c \
		  $(STD_SRC_DIR)/stm32f4xx_flash.c \
		  $(STD_SRC_DIR)/stm32f4xx_tim.c \
		  $(STD_SRC_DIR)/stm32f4xx_usart.c \
		  $(STD_SRC_DIR)/stm32f4xx_dma.c \
		  $(STD_SRC_DIR)/stm32f4xx_i2c.c \
		  $(STD_SRC_DIR)/misc.c \
		  $(RTOS_SRC_DIR)/croutine.c \
		  $(RTOS_SRC_DIR)/event_groups.c \
		  $(RTOS_SRC_DIR)/heap_4.c \
		  $(RTOS_SRC_DIR)/list.c \
		  $(RTOS_SRC_DIR)/port.c \
		  $(RTOS_SRC_DIR)/queue.c \
		  $(RTOS_SRC_DIR)/stream_buffer.c \
		  $(RTOS_SRC_DIR)/tasks.c \
		  $(RTOS_SRC_DIR)/timers.c \
		  $(SSD1306_SRC_DIR)/fonts.c \
		  $(SSD1306_SRC_DIR)/ssd1306.c \
          $(STARTUP_DIR)/startup_stm32f411re.c \
		  $(SYS_SRC_DIR)/system_init.c \
		  $(SYS_SRC_DIR)/system_it.c \
		  $(SYS_SRC_DIR)/system_log.c \
		  $(SYS_SRC_DIR)/syscalls.c \
		  $(SYS_SRC_DIR)/system_stm32f4xx.c \
		  $(BSP_LED_SRC_DIR)/led.c \
		  $(BSP_BUTTON_SRC_DIR)/button.c \
		  $(BSP_BUZZER_SRC_DIR)/buzzer.c \
		  $(BSP_SCREEN_SRC_DIR)/screen.c \
		  $(RESOURCES_DIR)/bitmap.c \
		  $(GAME_OBJ_METEOROID)/meteoroid.c \
		  $(GAME_OBJ_ARCHERY)/archery.c \
		  $(GAME_OBJ_ARROW)/arrow.c \
		  $(GAME_OBJ_BORDER)/border.c \
		  $(GAME_OBJ_BANG)/bang.c \
		  $(CONTAINER)/ring_buffer.c

# Object files
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))

# Include paths
INCLUDES = -I$(APP_INC_DIR) \
		   -I$(RESOURCE_INC_DIR) \
           -I$(STD_INC_DIR) \
		   -I$(RTOS_INC_DIR) \
		   -I$(CMSIS_INC_DIR) \
		   -I$(SSD1306_INC_DIR) \
		   -I$(SYS_INC_DIR) \
		   -I$(BSP_LED_INC_DIR) \
		   -I$(BSP_BUTTON_INC_DIR) \
		   -I$(BSP_BUZZER_INC_DIR) \
		   -I$(BSP_SCREEN_INC_DIR) \
		   -I$(RESOURCES_DIR) \
		   -I$(GAME_OBJ_METEOROID) \
		   -I$(GAME_OBJ_ARCHERY) \
		   -I$(GAME_OBJ_ARROW) \
		   -I$(GAME_OBJ_BORDER) \
		   -I$(GAME_OBJ_BANG) \
		   -I$(CONTAINER)

# Compiler flags
CFLAGS = -mcpu=cortex-m4 \
         -mthumb \
         -mfloat-abi=hard \
		 -mfpu=fpv4-sp-d16 \
         -Wall \
         -Wextra \
         -O0 \
         -g3 \
         -ffunction-sections \
         -fdata-sections \
		 -DUSE_STDPERIPH_DRIVER \
		 -DSTM32F411xE \
         $(INCLUDES)

# Linker flags
LDFLAGS = -mcpu=cortex-m4 \
          -mthumb \
          -mfloat-abi=hard \
		  -mfpu=fpv4-sp-d16 \
          --specs=nano.specs \
          -T linker_scripts.ld \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
          -Wl,--print-memory-usage

# VPATH for source file locations
VPATH = $(APP_SRC_DIR):$(STD_SRC_DIR):$(SSD1306_SRC_DIR):$(STARTUP_DIR):$(SYS_SRC_DIR):$(BSP_LED_SRC_DIR):$(BSP_BUTTON_SRC_DIR):$(BSP_BUZZER_SRC_DIR):$(BSP_SCREEN_SRC_DIR):$(RTOS_SRC_DIR):$(GAME_OBJ_METEOROID): \
		$(RESOURCES_DIR):$(GAME_OBJ_ARCHERY):$(GAME_OBJ_ARROW):$(GAME_OBJ_BORDER):$(GAME_OBJ_BANG):$(CONTAINER)

# Default target
all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex
	@echo "================================"
	@echo "Build complete!"
	@echo "================================"
	@$(SIZE) $(BUILD_DIR)/$(TARGET).elf

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "Linking: $@"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Compile app source files
$(BUILD_DIR)/%.o: $(APP_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile std files
$(BUILD_DIR)/%.o: $(STD_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile SSD1306 files
$(BUILD_DIR)/%.o: $(SSD1306_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile startup files
$(BUILD_DIR)/%.o: $(STARTUP_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile system files
$(BUILD_DIR)/%.o: $(SYS_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile FreeRTOS files
$(BUILD_DIR)/%.o: $(RTOS_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile BSP LED files
$(BUILD_DIR)/%.o: $(BSP_LED_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile BSP BUTTON files
$(BUILD_DIR)/%.o: $(BSP_BUTTON_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile BSP BUZZER files
$(BUILD_DIR)/%.o: $(BSP_BUZZER_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile BSP SCREEN files
$(BUILD_DIR)/%.o: $(BSP_SCREEN_SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GAME METEOROID files
$(BUILD_DIR)/%.o: $(GAME_OBJ_METEOROID)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GAME ARCHERY files
$(BUILD_DIR)/%.o: $(GAME_OBJ_ARCHERY)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GAME ARROW files
$(BUILD_DIR)/%.o: $(GAME_OBJ_ARROW)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GAME BORDER files
$(BUILD_DIR)/%.o: $(GAME_OBJ_BORDER)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GAME BANG files
$(BUILD_DIR)/%.o: $(GAME_OBJ_BANG)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile CONTAINER files
$(BUILD_DIR)/%.o: $(CONTAINER)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GAME RESOURCES files
$(BUILD_DIR)/%.o: $(RESOURCES_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Generate .bin file
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "Creating binary: $@"
	$(OBJCOPY) -O binary $< $@

# Generate .hex file
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "Creating hex: $@"
	$(OBJCOPY) -O ihex $< $@

# Generate .hex file
# $(BUILD_DIR)/$(TARGET).asm: $(BUILD_DIR)/$(TARGET).elf
# 	@echo "Creating asm: $@"
# 	$(OBJCOPY) -d $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).asm

# Clean
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)

# Dump
objdump:
	@echo "Dump final file !"
	$(OBJDUMP) -h $(BUILD_DIR)/$(TARGET).elf

#Erase full flash using openOCD
full_flash_erase:
	@echo "Erasing full flash..."
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init; reset halt; stm32f4x mass_erase 0; exit"

# Flash using OpenOCD. Download at: https://openocd.org/pages/getting-openocd.html
flash: clean all full_flash_erase
	@echo "Flashing via OpenOCD..."
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

openocd_server:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Debug with GDB
debug:
	arm-none-eabi-gdb.exe
# $(DBG) $(BUILD_DIR)/$(TARGET).elf

# Show size
size: $(BUILD_DIR)/$(TARGET).elf
	@echo "Memory usage:"
	$(SIZE) $

# Rebuild
rebuild: clean all

.PHONY: all clean flash flash-openocd debug size rebuild tree