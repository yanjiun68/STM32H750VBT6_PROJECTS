# 4×4 Button Array Project Structure

CubeMX owns peripheral handles and initialization in `Core/Src/main.c`, MSP setup in `stm32h7xx_hal_msp.c`, and interrupt handlers in `stm32h7xx_it.c`. The application functions and loop call live in preserved user-code sections of `main.c`; `keypad.c` and `keypad_uart.c` implement scanning, debounce, the event queue, and UART DMA delivery/recovery.

The legacy `gpio.c`, `dma.c`, `usart.c`, and `tim.c` modules and their headers remain on disk for reference, but those sources are excluded from the CMake target. Do not compile them alongside the generated setup: they define duplicate peripheral handles and MSP functions.

```text
4x4_button_array/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── gpio.h
│   │   ├── dma.h
│   │   ├── usart.h
│   │   ├── tim.h
│   │   ├── keypad.h
│   │   ├── keypad_uart.h
│   │   ├── stm32h7xx_it.h
│   │   └── stm32h7xx_hal_conf.h
│   └── Src/
│       ├── main.c
│       ├── gpio.c
│       ├── dma.c
│       ├── usart.c
│       ├── tim.c
│       ├── keypad.c
│       ├── keypad_uart.c
│       ├── stm32h7xx_it.c
│       ├── stm32h7xx_hal_msp.c
│       ├── system_stm32h7xx.c
│       ├── syscalls.c
│       └── sysmem.c
├── Drivers/
│   ├── CMSIS/
│   └── STM32H7xx_HAL_Driver/
├── cmake/
│   ├── gcc-arm-none-eabi.cmake
│   ├── starm-clang.cmake
│   └── stm32cubemx/CMakeLists.txt
├── CMakeLists.txt
├── CMakePresets.json
├── STM32H750xx_FLASH.ld
├── startup_stm32h750xx.s
├── 4x4_button_array.ioc
├── tests/
│   ├── mocks/stm32h7xx_hal.h
│   └── test_keypad.c
├── cubemx_strm.md
└── stm32h750.cfg
```

