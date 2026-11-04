# 4×4 Button Array Project Structure

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
└── stm32h750.cfg
```

