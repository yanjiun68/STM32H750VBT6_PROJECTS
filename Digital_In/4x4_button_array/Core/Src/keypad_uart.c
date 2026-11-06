#include "keypad_uart.h"
#include "keypad.h"
#include <stdio.h>

/* Place the DMA buffer in D2 SRAM: DMA1 cannot access DTCM. */
static uint8_t tx_buffer[32] __attribute__((section(".dma_buffer"), aligned(32)));
static UART_HandleTypeDef *uart;
typedef enum
{
    TX_IDLE,
    TX_BUSY,
    TX_COMPLETE,
    TX_ERROR,
    TX_RECOVERING
} TxState;

/* Callbacks only publish state; main owns formatting and the pending packet. */
static volatile TxState tx_state;
static uint16_t pending_size;

void KeypadUart_Init(UART_HandleTypeDef *huart)
{
    uart = huart;
    pending_size = 0U;
    tx_state = TX_IDLE;
}

void KeypadUart_Process(void)
{
    KeypadEvent event;
    if ((uart == NULL) || (tx_state == TX_BUSY) || (tx_state == TX_RECOVERING)) return;

    if (tx_state == TX_ERROR)
    {
        /* Keep the packet until DMA has stopped, then retry it unchanged.
         * HAL may call the abort completion synchronously, so set state first.
         */
        tx_state = TX_RECOVERING;
        if (HAL_UART_AbortTransmit_IT(uart) != HAL_OK) tx_state = TX_ERROR;
        return;
    }

    if (tx_state == TX_COMPLETE)
    {
        pending_size = 0U;
        tx_state = TX_IDLE;
    }

    if (pending_size == 0U)
    {
        if (!Keypad_GetEvent(&event)) return;
        /* Longest valid packet is 28 bytes, including CRLF. */
        int n = snprintf((char *)tx_buffer, sizeof(tx_buffer), "KEY %u %s %lu\r\n",
                         (unsigned)event.key, event.pressed ? "PRESSED" : "RELEASED",
                         (unsigned long)event.tick_ms);
        if ((n <= 0) || ((size_t)n >= sizeof(tx_buffer))) return;
        pending_size = (uint16_t)n;
    }

    SCB_CleanDCache_by_Addr((uint32_t *)tx_buffer, (int32_t)sizeof(tx_buffer));
    tx_state = TX_BUSY;
    if (HAL_UART_Transmit_DMA(uart, tx_buffer, pending_size) != HAL_OK)
    {
        /* HAL_BUSY/HAL_ERROR must not consume the pending event. */
        tx_state = TX_IDLE;
    }
}

void KeypadUart_TxComplete(UART_HandleTypeDef *huart)
{ if ((huart == uart) && (tx_state == TX_BUSY)) tx_state = TX_COMPLETE; }

void KeypadUart_Error(UART_HandleTypeDef *huart)
{ if ((huart == uart) && (tx_state == TX_BUSY)) tx_state = TX_ERROR; }

void KeypadUart_AbortComplete(UART_HandleTypeDef *huart)
{ if ((huart == uart) && (tx_state == TX_RECOVERING)) tx_state = TX_IDLE; }
