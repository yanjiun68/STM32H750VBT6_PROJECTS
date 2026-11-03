# STM32H750 LED GPIO and PWM Demo

Author : PEH YAN JIUN 

This project is a small STM32 HAL firmware showcase for controlling LEDs with
GPIO and timer-based PWM. The target configured in CubeMX is an
**STM32H750VBT6** in an LQFP100 package. The application code is in
`Core/Src/main.c` and its pin definitions are in `Core/Inc/main.h`; the other
firmware and build files are generated or configured by STM32CubeMX/CMake.

## Current behavior

After startup, the firmware initializes GPIOA and TIM1, TIM3, and TIM4, then
starts five PWM outputs. Its active loop updates **PB0 (TIM3_CH3)** and
**PB1 (TIM3_CH4)** together through these duty-cycle values:

```text
0%, 10%, 20%, ... 90%, 100%
```

There is a 500 ms delay after each value. The ramp takes 5 seconds from the
first 0% setting to 100%; the 100% setting is held for 500 ms before the loop
restarts at 0%. This produces a repeated brightness ramp on both outputs. The
visible brightness depends on the LED, resistor, wiring, and board.

## LED modes in the source

The current build runs the PB0/PB1 PWM ramp. Other example patterns are left
commented out in `main.c` and do not run unless you select and enable them:

- **PA0–PA7 sequential on/off:** turn each output on in order, then turn each
  off in order, with a 200 ms delay for each pin.
- **PA0–PA7 sequential toggle:** toggle the outputs in forward or reverse
  order with a 500 ms delay, then clear the group.
- **PA0–PA7 group toggle:** toggle all eight outputs together with a 50 ms
  delay.
- **PB0/PB1 duty-cycle alternation:** alternate between 25%/75% and 75%/25%,
  holding each pair for one second.
- **PB0/PB1 synchronized PWM ramp:** the active mode, with both channels at the
  same duty cycle.

To try a different pattern, edit the user-code section in `main.c`: comment out
the active `while (1)` loop and enable the chosen example. Keep the timer start
calls for PWM examples. The `GPIO_OUT0`–`GPIO_OUT7` macros in `main.h` identify
the PA0–PA7 GPIO outputs.

## Pin assignment

The pin functions below are taken from `LED_GPIO.ioc` and the application
source. Connect each external LED with a suitable current-limiting resistor;
check the board schematic and LED polarity before wiring.

| MCU pin | CubeMX function | Firmware use |
| --- | --- | --- |
| PA0 | GPIO output | Digital LED output 0 (`GPIO_OUT0`) |
| PA1 | GPIO output | Digital LED output 1 (`GPIO_OUT1`) |
| PA2 | GPIO output | Digital LED output 2 (`GPIO_OUT2`) |
| PA3 | GPIO output | Digital LED output 3 (`GPIO_OUT3`) |
| PA4 | GPIO output | Digital LED output 4 (`GPIO_OUT4`) |
| PA5 | GPIO output | Digital LED output 5 (`GPIO_OUT5`) |
| PA6 | GPIO output | Digital LED output 6 (`GPIO_OUT6`) |
| PA7 | GPIO output | Digital LED output 7 (`GPIO_OUT7`) |
| PB0 | TIM3_CH3, PWM | PWM output; active ramp channel 1 |
| PB1 | TIM3_CH4, PWM | PWM output; active ramp channel 2 |
| PB6 | TIM4_CH1, PWM | PWM output, started by firmware |
| PB7 | TIM4_CH2, PWM | PWM output, started by firmware |
| PE9 | TIM1_CH1, PWM | PWM output, started by firmware |

The PA0–PA7 pins are configured as push-pull outputs and are reset low during
initialization. PB0, PB1, PB6, PB7, and PE9 are assigned to timer alternate
functions for PWM. TIM3 and TIM4 use a compare value of 0 at initialization;
TIM1_CH1 uses a compare value of 500 with a period of 65535. Only PB0 and PB1
are continuously updated by the active application loop.

## Build

The checked-in CMake presets use the Ninja generator and the
`cmake/gcc-arm-none-eabi.cmake` toolchain file. Install CMake, Ninja, and the
GNU Arm Embedded toolchain, then run from the project root:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

The firmware ELF is produced at `build/Debug/LED_GPIO.elf`. To build the
optimized Release configuration instead:

```sh
cmake --preset Release
cmake --build --preset Release
```

## Flash and run

The repository includes `stm32h750.cfg` for an OpenOCD CMSIS-DAP/SWD setup.
Connect a compatible debug probe to the board's SWD interface and run from the
project root (adjust the OpenOCD executable or probe configuration if needed):

```sh
openocd -f stm32h750.cfg \
  -c "program build/Debug/LED_GPIO.elf verify reset exit"
```

After flashing, reset or power-cycle the target. With LEDs connected to PB0
and PB1, observe the repeated synchronized brightness ramp.

## Resume-project outcomes

The outcomes below are derived from the firmware and CubeMX configuration;
they describe implemented scope, not measured laboratory results:

- Configured **8 digital GPIO outputs** across PA0–PA7.
- Configured **5 timer PWM outputs** using three timers: TIM1, TIM3, and TIM4.
- Implemented a synchronized **0–100% PWM brightness ramp** on two channels,
  in **11 duty-cycle steps** of **10 percentage points**, with **500 ms per
  step**.
- Included **4 alternative LED pattern examples** in source: sequential
  on/off, sequential toggle, group toggle, and alternating PWM duty cycles.
- Used a **64 MHz HSI system clock** configured by the application.

## Project files and editing

- `Core/Src/main.c` — application startup, active LED mode, and HAL
  initialization.
- `Core/Inc/main.h` — GPIO aliases and PWM channel definitions.
- `LED_GPIO.ioc` — STM32CubeMX device, pin, clock, and peripheral setup.
- `cmake/`, `CMakeLists.txt`, and `CMakePresets.json` — generated CMake build
  setup.
- `stm32h750.cfg` — OpenOCD target and debug-probe setup.

For application changes, work in the `USER CODE` sections of `main.c` and
`main.h`. When regenerating with CubeMX, keep user-code preservation enabled so
custom code in those sections is retained.
