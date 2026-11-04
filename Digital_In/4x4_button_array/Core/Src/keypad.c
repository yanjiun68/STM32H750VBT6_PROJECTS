#include "main.h"
#include "keypad.h"
#include <stddef.h>

#define DEBOUNCE_MS 20U

static GPIO_TypeDef *const row_ports[4] = {GPIOB, GPIOB, GPIOB, GPIOB};
static const uint16_t row_pins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_4};
static const uint16_t col_pins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3};
static uint8_t stable[16];
static uint8_t candidate[16];
static uint8_t count[16];
static KeypadEvent queue[KEYPAD_EVENT_QUEUE_CAPACITY];
static volatile uint8_t head, tail;
static volatile uint8_t used;
static uint32_t dropped;

static void queue_event(uint8_t key, uint8_t pressed, uint32_t tick)
{
    if (used >= KEYPAD_EVENT_QUEUE_CAPACITY) { dropped++; return; }
    queue[head] = (KeypadEvent){key, pressed, tick};
    head = (uint8_t)((head + 1U) % KEYPAD_EVENT_QUEUE_CAPACITY);
    used++;
}

void Keypad_Init(void)
{
    for (uint8_t i = 0; i < 16U; ++i) { stable[i] = 0U; candidate[i] = 0U; count[i] = 0U; }
    head = tail = used = 0U;
    dropped = 0U;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4, GPIO_PIN_SET);
}

void Keypad_Scan1ms(uint32_t tick_ms)
{
    for (uint8_t row = 0; row < 4U; ++row)
    {
        HAL_GPIO_WritePin(row_ports[row], row_pins[row], GPIO_PIN_RESET);
        for (volatile uint32_t settle = 0; settle < 32U; ++settle) { __NOP(); }
        for (uint8_t col = 0; col < 4U; ++col)
        {
            uint8_t index = (uint8_t)(row * 4U + col);
            uint8_t raw = (HAL_GPIO_ReadPin(GPIOC, col_pins[col]) == GPIO_PIN_RESET) ? 1U : 0U;
            if (raw != candidate[index]) { candidate[index] = raw; count[index] = 1U; }
            else if (count[index] < DEBOUNCE_MS) { count[index]++; }
            if ((count[index] == DEBOUNCE_MS) && (stable[index] != raw))
            {
                stable[index] = raw;
                queue_event((uint8_t)(index + 1U), raw, tick_ms);
            }
        }
        HAL_GPIO_WritePin(row_ports[row], row_pins[row], GPIO_PIN_SET);
    }
}

uint8_t Keypad_GetEvent(KeypadEvent *event)
{
    if ((event == NULL) || (used == 0U)) return 0U;
    *event = queue[tail];
    tail = (uint8_t)((tail + 1U) % KEYPAD_EVENT_QUEUE_CAPACITY);
    used--;
    return 1U;
}

uint32_t Keypad_GetDroppedEventCount(void) { return dropped; }
