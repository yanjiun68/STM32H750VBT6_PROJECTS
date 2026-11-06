# STM32H750 4x4 button matrix: setup and CubeMX configuration

Target: **STM32H750VBT6**, using a 4x4 switch matrix and USART3 output through a CMSIS-DAP probe with a UART bridge.

The `.ioc` and firmware now use CubeMX-owned peripheral initialization in `main.c`, MSP setup in `stm32h7xx_hal_msp.c`, and one generated IRQ handler per peripheral in `stm32h7xx_it.c`. Application processing stays inside preserved user-code blocks. The old manual `gpio.c`, `dma.c`, `usart.c`, and `tim.c` files are excluded from the CMake target and must remain excluded while this layout is used.

## Wiring

| Matrix terminal | STM32 pin | GPIO configuration |
|---|---|---|
| R1 | PB0 | Open-drain output, pull-up, low speed, initial HIGH |
| R2 | PB1 | Open-drain output, pull-up, low speed, initial HIGH |
| R3 | PB3 | Open-drain output, pull-up, low speed, initial HIGH |
| R4 | PB4 | Open-drain output, pull-up, low speed, initial HIGH |
| C1 | PC0 | Input with pull-up |
| C2 | PC1 | Input with pull-up |
| C3 | PC2_C | Input with pull-up, using GPIOC pin 2 |
| C4 | PC3_C | Input with pull-up, using GPIOC pin 3 |

Keep the internal PC2/PC2_C and PC3/PC3_C analog switches closed for digital input. Follow the matrix terminal labels; the physical connector order depends on the module.

The scanner drives one row LOW, reads the four columns, then releases the row. A pressed switch connects its column to the selected row, producing a LOW input.

| Row | C1 | C2 | C3 | C4 |
|---|---:|---:|---:|---:|
| R1 | Key 1 | Key 2 | Key 3 | Key 4 |
| R2 | Key 5 | Key 6 | Key 7 | Key 8 |
| R3 | Key 9 | Key 10 | Key 11 | Key 12 |
| R4 | Key 13 | Key 14 | Key 15 | Key 16 |

An eight-terminal passive switch matrix needs only these eight signal connections. If the buttons include LEDs or the module has powered electronics, follow its schematic for supply, ground, current limiting, and LED control. The current firmware reads switch contacts; it does not drive button illumination. Use 3.3 V logic on these connections.

| STM32 | CMSIS-DAP probe |
|---|---|
| PD8 / USART3_TX | UART RX |
| PD9 / USART3_RX | UART TX; optional for output-only monitoring |
| GND | GND |
| PA13 / SWDIO | SWDIO, for programming/debugging |
| PA14 / SWCLK | SWCLK, for programming/debugging |

Connect the probe to the PC by USB. The probe must provide a UART bridge/virtual COM port; SWD alone does not carry USART messages. Leave SWO trace disconnected because PB3 is used by R3. Connect a probe voltage-reference input to target 3.3 V if required by the probe's instructions.

## CubeMX settings

### System Core and clocks

| Location / setting | Value |
|---|---|
| SYS: Debug | Serial Wire, without trace |
| SYS: Timebase Source | SysTick |
| System clock source | HSI, 64 MHz, PLL disabled |
| SYSCLK / HCLK | 64 MHz / 64 MHz |
| APB1 prescaler / PCLK1 | Divide by 2 / 32 MHz |
| TIM6 timer clock | 64 MHz |
| USART3 kernel clock | D2PCLK1 / PCLK1, 32 MHz |

Use the **timer clock** displayed in the CubeMX clock tree for timer calculations. With the current APB1 divide-by-two setting and default timer clock selection, TIM6 receives twice PCLK1, so it still runs at 64 MHz. HAL calculates the USART baud divider from its separate 32 MHz kernel clock.

### Connectivity: USART3

| Setting | Value |
|---|---|
| Mode | Asynchronous |
| TX / RX pins | PD8 / PD9, AF7 |
| Baud rate | 115200 |
| Word length / parity / stop bits | 8 bits / None / 1 |
| Direction | Transmit and receive |
| Hardware flow control | None |
| Oversampling | 16 |
| Clock prescaler | Divide by 1 |
| FIFO mode | Disabled |

Enable **USART3 global interrupt** in NVIC Settings. Normal-mode UART DMA requires the UART transmit-complete interrupt as well as the DMA interrupt to complete the HAL transmission sequence.

### USART3: DMA Settings

Add the following TX request:

| Setting | Value |
|---|---|
| Request | USART3_TX |
| Stream | DMA1 Stream0 |
| Direction | Memory to peripheral |
| Mode | Normal |
| Peripheral increment | Disabled |
| Memory increment | Enabled |
| Peripheral / memory data width | Byte / Byte |
| DMA priority | Low |
| FIFO | Disabled |

Enable **DMA1 Stream0 global interrupt**. RX DMA is unnecessary for the current event-output application.

### Timers: TIM6

Activate TIM6 and configure:

| Setting | Value |
|---|---|
| Prescaler (PSC) | 6399 |
| Counter mode | Up |
| Counter period / auto-reload (ARR) | 9 |
| Auto-reload preload | Disabled |
| TIM6/DAC global interrupt | Enabled |

The update period is:

```text
Period = (PSC + 1) × (ARR + 1) / TIM6 clock
       = 6400 × 10 / 64,000,000
       = 0.001 seconds = 1 ms
```

| Interval at a 64 MHz timer clock | PSC | ARR |
|---|---:|---:|
| Default: 1.024 ms | 0 | 65535 |
| Required keypad interval: 1 ms | 6399 | 9 |
| Example only: 1 second | 63999 | 999 |

PSC = 0 means divide by 1. ARR = 65535 means 65,536 counts, because counting includes zero. These defaults do **not** give one second at 64 MHz.

Keep the keypad timer at 1 ms: the current debounce logic and event timestamps assume that each timer interrupt represents one millisecond.

### NVIC priorities

Use priority grouping with four preemption-priority bits and match the existing source:

| Interrupt | Preemption priority | Subpriority |
|---|---:|---:|
| TIM6/DAC | 5 | 0 |
| DMA1 Stream0 | 5 | 0 |
| USART3 | 5 | 0 |

Retain the generated SysTick handler and HAL tick support.

## Implemented firmware integration

1. **D2 SRAM1 is enabled before the UART DMA buffer is accessed.** The linker places `.dma_buffer` at `0x30000000`; preserved `SysInit` code enables its clock:

   ```c
   __HAL_RCC_D2SRAM1_CLK_ENABLE();
   ```

2. `.dma_buffer` retains its 32-byte alignment in DMA-accessible D2 SRAM. The UART module cleans its cache line before DMA reads it. DMA1 cannot access DTCM.
3. Generated initialization runs once: GPIO, DMA, TIM6, USART3, then `App_Init()`. DMA must precede USART3; TIM6 is initialized here but starts only in `App_Init()` using `HAL_TIM_Base_Start_IT(&htim6)`.
4. `App_Init()` and `App_Process()` definitions are in `USER CODE BEGIN 0`, and the main-loop processing call is in `USER CODE BEGIN 3`. TIM6's callback increments the scan tick; the main loop scans the matrix and services the UART queue. Missed periods are counted instead of replaying scans.
5. GPIO setup releases the open-drain rows HIGH and enables column pull-ups. Preserved code explicitly closes the PC2_C/PC3_C analog switches for digital input.
6. **UART retains one pending packet until transmission completes.** `HAL_BUSY` or `HAL_ERROR` on start leaves it pending. A transfer error schedules a nonblocking `HAL_UART_AbortTransmit_IT()` from the main loop; the pending packet is retried after the abort-complete callback. Preserve the timer elapsed, UART TX complete, UART error, and UART abort-transmit complete callbacks.
7. The queue now holds **64 events**, allowing a burst of all 16 presses followed by all 16 releases without overflowing an initially empty queue.

In Project Manager, retain **Keep User Code when re-generating**, with peripheral initializers in `main.c`. Do not add manual peripheral initialization calls or duplicate IRQ handlers to user-code blocks. Keep the custom CMake entries for `keypad.c` and `keypad_uart.c`, the legacy peripheral modules excluded, and the linker section for `.dma_buffer`.

## Remaining hardware and delivery limits

- A UART completion confirms local transmission, not reception by the PC. A retry after an interrupted transfer can leave a partial line or duplicate data on the PC. The existing text protocol has no acknowledgment or deduplication.
- Excess transitions are still dropped if the 64-event queue fills; `Keypad_GetDroppedEventCount()` exposes that count. No finite queue guarantees delivery during an indefinite output interruption.
- A matrix without isolation diodes can produce ghost keys during some simultaneous presses. The scanner does not suppress ambiguous combinations. Use suitable matrix hardware for reliable independent multi-key operation.
- The tests below simulate switch contacts and UART callbacks. Actual wiring, cable settling, LED circuitry, and probe reception still require a board test.

## Verification

Build firmware with the installed ARM toolchain:

```bash
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

Host regression tests compile the real keypad and UART modules with simulated GPIO/DMA. They cover contact bounce, held keys, all 16 key positions, queue overflow/wrap, failed DMA starts, transfer errors, and synchronous/asynchronous callbacks:

```bash
gcc -std=c11 -Wall -Wextra -Werror -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Itests/mocks -ICore/Inc \
  tests/test_keypad.c Core/Src/keypad.c Core/Src/keypad_uart.c \
  -o /tmp/keypad-host-tests
ASAN_OPTIONS=detect_leaks=0 /tmp/keypad-host-tests
```

Leak detection is disabled because LeakSanitizer cannot run under the execution environment's process tracing; address and undefined-behavior checks remain enabled.

## Expected behavior and PC monitoring

All 16 switches are scanned once per millisecond. A transition requires 20 consecutive consistent samples before it is queued. With an empty output queue, expect roughly 20 ms after contact bounce settles plus about 1–3 ms of UART transmission and additional USB/PC buffering. This is event reporting: holding a button does not repeatedly send its state.

On Linux, list serial ports and open the probe's UART port:

```bash
python3 -m serial.tools.list_ports
screen /dev/ttyACM0 115200
```

Replace `/dev/ttyACM0` with the reported device, such as `/dev/ttyUSB0`. Use **115200 baud, 8N1, no flow control**. `python3 -m serial.tools.list_ports` requires pyserial; `screen` must also be installed.

Pressing and releasing R1/C1 should produce lines such as:

```text
KEY 1 PRESSED 1234
KEY 1 RELEASED 1456
```

The final number is the detection timestamp in milliseconds since the scan timer started. A quiet terminal while no button changes is expected. Exit `screen` with **Ctrl+A**, then **K**, then **Y**.

If no serial device appears, check the USB cable and probe UART-bridge support. If the port opens without button output, check the SRAM1 clock fix, flashed firmware, running/debugger state, TX-to-RX connection, common ground, and interrupt configuration. During testing, watch `s_scan_deadlines_missed` and `Keypad_GetDroppedEventCount()` for missed scan periods and queue overflow.

## References

- [STM32H750 datasheet: pin functions and PC2_C/PC3_C analog switches](https://www.st.com/resource/en/datasheet/stm32h750vb.pdf)
- [ST AN4891: memory architecture and SRAM clock requirements](https://www.st.com/resource/en/application_note/dm00306681-stm32h72x-stm32h73x-and-singlecore-stm32h74x75x-system-architecture-and-performance-stmicroelectronics.pdf)
- [ST UART guide: UART/DMA configuration and interrupts](https://wiki.st.com/stm32mcu/wiki/Getting_started_with_UART)
- [Arm CMSIS-DAP: optional UART communication support](https://arm-software.github.io/CMSIS-DAP/latest/index.html)
