#include "keypad_uart.h"
#include "keypad.h"
#include <stdio.h>
#include <string.h>

/* Place the DMA buffer in D2 SRAM: DMA1 cannot access DTCM. */
static uint8_t tx_buffer[32] __attribute__((section(".dma_buffer"), aligned(32)));
static UART_HandleTypeDef *uart;
static volatile uint8_t busy;

void KeypadUart_Init(UART_HandleTypeDef *huart) { uart = huart; busy = 0U; }

void KeypadUart_Process(void)
{
    KeypadEvent event;
    if ((uart == NULL) || busy || !Keypad_GetEvent(&event)) return;
    int n = snprintf((char *)tx_buffer, sizeof(tx_buffer), "KEY %u %s %lu\r\n",
                     (unsigned)event.key, event.pressed ? "PRESSED" : "RELEASED",
                     (unsigned long)event.tick_ms);
    if ((n <= 0) || ((size_t)n >= sizeof(tx_buffer))) return;
    SCB_CleanDCache_by_Addr((uint32_t *)tx_buffer, (int32_t)((n + 31) & ~31));
    busy = 1U;
    if (HAL_UART_Transmit_DMA(uart, tx_buffer, (uint16_t)n) != HAL_OK) busy = 0U;
}

void KeypadUart_TxComplete(UART_HandleTypeDef *huart)
{ if (huart == uart) busy = 0U; }

void KeypadUart_Error(UART_HandleTypeDef *huart)
{ if (huart == uart) busy = 0U; }
