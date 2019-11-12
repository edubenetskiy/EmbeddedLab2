/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum _ButtonState_enum {
	BUTTON_DOWN = GPIO_PIN_RESET, BUTTON_UP = GPIO_PIN_SET,
} ButtonState;

typedef enum _ButtonEvent_enum {
	EVENT_NONE, EVENT_PRESSED, EVENT_RELEASED,
} ButtonEvent;

typedef enum _BulbColor_enum {
	COLOR_GREEN = GPIO_PIN_13,
	COLOR_YELLOW = GPIO_PIN_14,
	COLOR_RED = GPIO_PIN_15,
} BulbColor;

typedef enum _LightState_enum {
	LIGHT_ON = GPIO_PIN_SET, LIGHT_OFF = GPIO_PIN_RESET,
} LightState;

typedef uint32_t timestamp_t;
typedef uint32_t duration_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
const size_t MAX_NUM_SIGNALS = 64;
const duration_t POLL_INTERVAL_MILLIS = 5;

const duration_t DOT_DURATION = 200;
const duration_t DASH_DURATION = 600;

const duration_t MAX_PAUSE_MILLIS = 3000;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
ButtonState read_button_state();
void set_light(BulbColor color, LightState state);
void turn_light_on(BulbColor color);
void turn_light_off(BulbColor color);
uint32_t get_instant_millis();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
ButtonState read_button_state() {
	return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);
}

ButtonEvent determine_button_event(ButtonState oldState, ButtonState newState) {
	if (oldState == BUTTON_DOWN && newState == BUTTON_UP) {
		return EVENT_RELEASED;
	} else if (oldState == BUTTON_UP && newState == BUTTON_DOWN) {
		return EVENT_PRESSED;
	} else {
		return EVENT_NONE;
	}
}

void set_light(BulbColor color, LightState state) {
	HAL_GPIO_WritePin(GPIOD, color, state);
}

void turn_light_on(BulbColor color) {
	set_light(color, LIGHT_ON);
}

void turn_light_off(BulbColor color) {
	set_light(color, LIGHT_OFF);
}

uint32_t clock_now_millis() {
	return HAL_GetTick();
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

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
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		duration_t signal_durations[MAX_NUM_SIGNALS];

		ButtonState prev_button_state = read_button_state();
		size_t num_signals = 0;
		duration_t cur_duration_millis = 0;

		set_light(COLOR_GREEN, LIGHT_ON);

		while (true) {
			HAL_Delay(POLL_INTERVAL_MILLIS);

			if (cur_duration_millis < UINT32_MAX - POLL_INTERVAL_MILLIS) {
				cur_duration_millis += POLL_INTERVAL_MILLIS;
			}

			ButtonState cur_button_state = read_button_state();
			ButtonEvent button_event = determine_button_event(prev_button_state,
					cur_button_state);

			switch (button_event) {
			case EVENT_PRESSED:
				cur_duration_millis = 0;
				break;

			case EVENT_RELEASED:
				signal_durations[num_signals] = cur_duration_millis;
				num_signals++;

				BulbColor color;
				if (cur_duration_millis < DASH_DURATION) {
					color = COLOR_YELLOW;
				} else {
					color = COLOR_RED;
				}

				set_light(color, LIGHT_ON);
				HAL_Delay(100);
				set_light(color, LIGHT_OFF);

				break;

			case EVENT_NONE:
				break;
			}

			if (cur_button_state == BUTTON_UP && num_signals > 0
					&& cur_duration_millis >= MAX_PAUSE_MILLIS) {
				break;
			}

			if (num_signals >= MAX_NUM_SIGNALS) {
				break;
			}

			prev_button_state = cur_button_state;
		}

		set_light(COLOR_GREEN, LIGHT_OFF);
		set_light(COLOR_RED, LIGHT_ON);
		HAL_Delay(DASH_DURATION);

		for (int signal_index = 0; signal_index < num_signals; ++signal_index) {
			duration_t signal_duration = signal_durations[signal_index];

			set_light(COLOR_GREEN, LIGHT_ON);
			if (signal_duration < DASH_DURATION) {
				HAL_Delay(DOT_DURATION);
			} else {
				HAL_Delay(DASH_DURATION);
			}

			set_light(COLOR_GREEN, LIGHT_OFF);
			HAL_Delay(DASH_DURATION);
		}

		set_light(COLOR_GREEN, LIGHT_ON);
		set_light(COLOR_RED, LIGHT_OFF);
		HAL_Delay(DASH_DURATION);
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
			GPIO_PIN_RESET);

	/*Configure GPIO pin : PC15 */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PD13 PD14 PD15 */
	GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
