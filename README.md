# Archery Game - STM32F411RE / FreeRTOS

An embedded archery game created to study ARM Cortex-M4 startup, exception
handling, linker scripts, peripheral drivers, and real-time design. The project
keeps its original event-driven gameplay: each game object owns a task or timer
and communicates through queues, semaphores, and event groups.

## What this project demonstrates

- Bare-metal Cortex-M startup, vector table, C runtime initialization, and FPU
- A memory-safe linker layout with a dedicated persistent Flash partition
- FreeRTOS tasks, software timers, priority audio queues, task notifications,
  input queues, semaphores, and event groups
- SSD1306 rendering over timeout-bounded I2C with bus recovery
- UART logging over DMA with concurrency and DMA-error handling
- Retained Cortex-M fault records, stack-overflow and allocation hooks
- Wear-reduced, CRC-protected score/settings journaling in Flash sector 7
- Versioned persistent difficulty, sound and screensaver preferences
- Explicit application lifecycle with pause, display sleep and soft replay
- Separate reproducible Debug and Release firmware outputs

## Runtime architecture

```text
Reset_Handler
  -> SystemInit (100 MHz clock/FPU/VTOR)
  -> C runtime (.data, .bss, constructors)
  -> board and diagnostic initialization
  -> splash screen ("PRESS OK TO START")
  -> object tasks/timers and IPC creation
  -> FreeRTOS scheduler

Button sampler -> short/long press queue -> input router -> lifecycle intent
Game timer -> archery/arrow workers -> collision/bang state machine
Meteor timer -> task notification -> meteoroid worker
Object workers -> coherent game-state snapshot -> screen-owner task -> OLED
Audio producers -> normal/high-priority queues -> TIM3 audio-owner task
```

Only the screen-owner task accesses the SSD1306 framebuffer and I2C peripheral.
Object workers update state under one priority-inheritance mutex, then request a
frame through an event group. The renderer copies a coherent snapshot and
releases the state mutex before drawing or performing the slower I2C transfer.
Software-timer callbacks only publish notifications/events and never render,
write Flash, or block on application mutexes.

At boot the game starts on the splash screen and plays the start melody after
the OLED transfer succeeds. Gameplay timers activate only after a debounced
short OK press. The configured idle timeout enters a 25 FPS screensaver engine
with Bubbles, Starfield and Waves effects. Effects auto-rotate every 15 seconds,
reduce to 10 FPS after one minute, and eventually turn the OLED off while the
idle hook keeps the Cortex-M in `WFI`.

## Hardware

Target: **NUCLEO-F411RE**, STM32F411RETx, 512 KiB Flash, 128 KiB SRAM.

| Signal | NUCLEO-F411RE pin |
|---|---|
| Button OK | PB3 |
| Button Up | PA4 |
| Button Down | PB0 |
| OLED SDA | PB7 |
| OLED SCL | PB6 |
| Life LED | PC7 |
| Buzzer | PA6 |
| Power | 3V3 / 5V / GND as required by the attached hardware |

### Controls

- Startup screen: the start melody plays as soon as this prompt is displayed.
  Short-press **OK** to start. Hold **OK** for 800 ms to enter Settings.
- Settings: **Up** cycles difficulty, **Down** selects the initial screensaver
  effect, hold **Up** to toggle sound, and **OK** saves to the CRC journal.
- Screensaver: short **Up/Down** changes effect density, hold **Up/Down** to
  select the next/previous effect, and **OK** returns to the startup screen.
- During gameplay: **Up/Down** moves the archer, short **OK** fires, and holding
  **OK** pauses. Press **OK** again to resume.
- After prolonged saver inactivity the OLED turns off. The first button release
  wakes to the startup screen without performing a hidden action.
- Game-over screen: press **OK** to perform a software session reset and return
  to the startup screen. The MCU, RTOS tasks and peripherals remain running;
  only per-game objects, pending pipeline signals and difficulty are reset.

## Prerequisites

- GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`, `objcopy`, `gdb`)
- GNU Make
- OpenOCD and an ST-Link driver for flashing/debugging
- Git Bash on Windows, or a POSIX shell on Linux

The final verification was performed with GNU Arm Embedded Toolchain 10.3.1.
Override `CROSS_COMPILE` or `OPENOCD` when tools are not on `PATH`.

## Build

```bash
# Debug: -Og, full symbols, assertions
make BUILD_TYPE=Debug

# Release: -Os, LTO, assertions still route through fatal system checks
make BUILD_TYPE=Release
```

Artifacts are isolated by configuration:

```text
build/Debug/game_embed.{elf,bin,hex,map}
build/Release/game_embed.{elf,bin,hex,map}
```

Useful targets:

```bash
make test
make size BUILD_TYPE=Debug
make listing BUILD_TYPE=Debug
make clean
```

## Flash and debug

```bash
make flash BUILD_TYPE=Release
```

`make flash` programs only the application image. Score and versioned settings
records share the append-only journal in sector 7
(`0x08060000`-`0x0807FFFF`). Legacy `SCOR` records remain compatible. To
intentionally erase application and persistent data:

```bash
make full_flash_erase
```

For source debugging, start these in separate terminals:

```bash
make openocd_server
make debug BUILD_TYPE=Debug
```

## Diagnostics

UART1 logging uses PA9/PA10 at 115200 baud. Every fatal path records a stable reason
code in `.noinit`, then resets. On the next boot, the log reports the active
exception vector, stacked PC/LR/xPSR when available, and Cortex-M fault registers.
Three consecutive failures before the scheduler reaches idle stop the reset loop
and leave the MCU halted for debugger inspection.

The stack monitor reports each task's high-water mark every five seconds.
Production stack sizing should be based on a representative long-duration run,
with margin for worst-case interrupt nesting and library calls.

## Memory contract

- `0x08000000`-`0x0805FFFF`: application image (384 KiB)
- `0x08060000`-`0x0807FFFF`: score/settings journal (128 KiB, Flash sector 7)
- `0x20000000`-`0x2001FFFF`: SRAM (128 KiB)
- Main stack reservation: 4 KiB
- FreeRTOS heap: 48 KiB. This includes measured headroom for newlib-enabled
  task control blocks and the Idle/Timer tasks created at scheduler startup.

Link-time assertions reject application/NVM overlap and insufficient SRAM. Do not
change the Flash partition without updating both the linker script and the
sector number used by the persistent journal.
