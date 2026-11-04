/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    main.c
 * @brief   STM32H750VBT6 4x4 matrix keypad application (bare metal + HAL).
 *
 * Architecture:
 *  TIM6 ISR -> 1 ms tick -> main loop -> keypad scan/debounce/event queue
 *                                  -> UART DMA event transmission.
 *
 * Requirements:
 *  - Keypad driver modules scan/debounce and queue key transitions.
 *  - GPIO: PB0, PB1, PB3, PB4 open-drain row outputs, initially HIGH.
 *          PC0..PC3 inputs with internal pull-ups.
 *  - USART3 TX DMA enabled; 115200 8N1, PD8 TX / PD9 RX if available.
 *  - TIM6 update interrupt at 1000 Hz.
 *  - UART DMA buffer .dma_buffer is mapped to DMA-accessible D2 SRAM.
 *  - GPIO, DMA, USART3 and TIM6 setup lives in their matching Core modules.
 *
 * WARNING: Do not define HAL_TIM_PeriodElapsedCallback or UART callbacks
 * twice. Merge with any existing callbacks in your CubeMX project.
 ******************************************************************************
 */
/* USER CODE END Header */






#include "main.h"
#include "dma.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "keypad.h"
#include "keypad_uart.h"
#include <stdint.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/** Monotonically increasing millisecond tick, written only by TIM6 ISR.
 *  An aligned uint32_t read/write is atomic on Cortex-M7.
 */
static volatile uint32_t s_keypad_tick_ms = 0U;

/** Number of missed 1 ms scanning deadlines; debugger-watch variable. */
static volatile uint32_t s_scan_deadlines_missed = 0U;
/* USER CODE END PV */

/* USER CODE BEGIN PFP */
static void App_Init(void);
static void App_Process(void);
/* USER CODE END PFP */

/**
 * @brief  Program entry point.
 * @note   Peripheral initialization is implemented in the matching Core
 *         modules and called here before application startup.
 */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  /* USER CODE BEGIN 2 */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART3_UART_Init();
    MX_TIM6_Init();


    /* 4. Initialize application drivers and start periodic scanning. */
    App_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    App_Process();

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief Initialize keypad and UART output, then enable TIM6 interrupts.
 */
static void App_Init(void)
{
    /* Keypad_Init() initializes debounce state, event queue and DWT cycle
     * counter; GPIO clocks and pin modes must already be configured.
     */
    Keypad_Init();

    /* Bind the nonblocking UART event-output module to USART3. */
    KeypadUart_Init(&huart3);

    /* Start 1 ms periodic timer interrupts. */
    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Run one iteration of the main application.
 *
 * Concurrency contract:
 *  - TIM6 ISR ONLY increments s_keypad_tick_ms.
 *  - Main context exclusively owns GPIO scanning, debouncing and event queue.
 *  - UART DMA owns tx_buffer until its completion callback.
 *
 * This approach avoids concurrent keypad GPIO and queue modification.
 */
static void App_Process(void)
{
    static uint32_t last_processed_tick = 0U;
    uint32_t tick_snapshot = s_keypad_tick_ms;

    /* A new timer tick indicates that a scan is due. */
    if (tick_snapshot != last_processed_tick)
    {
        uint32_t elapsed = tick_snapshot - last_processed_tick;

        /* Count scan periods missed while main context was busy.
         * We deliberately do not replay old scans: a replay would sample
         * the current GPIO level repeatedly, not the historical levels.
         */
        if (elapsed > 1U)
        {
            s_scan_deadlines_missed += elapsed - 1U;
        }

        last_processed_tick = tick_snapshot;

        /* Scan all 4 rows, debounce all 16 switches, and enqueue changes.
         * This function must be called only from the main context.
         */
        Keypad_Scan1ms(tick_snapshot);
    }

    /* Transmit one queued event when the UART DMA channel is available.
     * This call is nonblocking; UART transfer continues in hardware.
     */
    KeypadUart_Process();
}

/* USER CODE BEGIN 4 */
/**
 * @brief TIM6 1 ms scheduler callback.
 * @note  Keep this ISR short. Do not scan GPIO or print from here.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        s_keypad_tick_ms++;
    }
}

/**
 * @brief HAL callback when a UART DMA transmit completes.
 * @note  The UART module may reuse its DMA buffer after this callback.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    KeypadUart_TxComplete(huart);
}

/**
 * @brief UART error callback; delegates recovery state to UART module.
 * @note  Production firmware should additionally diagnose persistent
 *        hardware errors and configure appropriate retry policy.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    KeypadUart_Error(huart);
}
/* USER CODE END 4 */

/**
 * @brief  Application error handler.
 * @note   If CubeMX already generated Error_Handler(), retain exactly one
 *         definition and merge its body rather than duplicating it.
 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
