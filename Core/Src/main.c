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
#include "string.h"
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

typedef struct code_chart_entry {
	uint8_t symbol;
	char *code;
} code_chart_entry_t;
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
const duration_t PAUSE_BETWEEN_LETTERS = 300;

const duration_t MAX_PAUSE_MILLIS = 3000;

const code_chart_entry_t MORSE_CODE_CHART[] = {
	{'A', ".-"},
	{'B', "-..."},
	{'C', "-.-."},
	{'D', "-.."},
	{'E', "."},
	{'F', "..-."},
	{'G', "--."},
	{'H', "...."},
	{'I', ".."},
	{'J', ".---"},
	{'K', "-.-"},
	{'L', ".-.."},
	{'M', "--"},
	{'N', "-."},
	{'O', "---"},
	{'P', ".--."},
	{'Q', "--.-"},
	{'R', ".-."},
	{'S', "..."},
	{'T', "-"},
	{'U', "..-"},
	{'V', "...-"},
	{'W', ".--"},
	{'X', "-..-"},
	{'Y', "-.--"},
	{'Z', "--.."},
	{'.', ".-.-.-"},
	{',', "--..--"},
	{'?', "..--.."},
	{'-', "-....-"},
	{'=', "-...-"},
	{':', "---..."},
	{';', "-.-.-."},
	{'(', "-.--."},
	{')', "-.--.-"},
	{'/', "-..-."},
	{'"', ".-..-."},
	{'$', "...-.-"},
	{'\'', ".----."},
	{'_', "..--.-"},
	{'+', ".-.-."},
	{'@', ".--.-."},
	{'0', "-----"},
	{'1', ".----"},
	{'2', "..---"},
	{'3', "...--"},
	{'4', "....-"},
	{'5', "....."},
	{'6', "-...."},
	{'7', "--..."},
	{'8', "---.."},
	{'9', "----."},
	{'\0', ""}, // end of array marker
};
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

HAL_StatusTypeDef read_char(uint8_t *received_char) {
	return HAL_UART_Receive(&huart1, received_char, sizeof(*received_char),
	UINT32_MAX);
}

void print_char(uint8_t symbol) {
	HAL_UART_Transmit(&huart1, &symbol, sizeof(symbol), UINT32_MAX);
}

void toggle_interrupts() {
	// TODO: Check interrupts and turn them on/off
}

uint8_t decode_morse(uint8_t *morse_string) {
	for (size_t index = 0; MORSE_CODE_CHART[index].symbol != '\0'; index++) {
		if (strcmp(MORSE_CODE_CHART[index].code, (char*) morse_string) == 0) {
			return MORSE_CODE_CHART[index].symbol;
		}
	}
	return '\0';
}

uint8_t capitalize(uint8_t ascii_character) {
	if ('a' <= ascii_character && ascii_character <= 'z') {
		ascii_character += 'X' - 'x';
	}
	return ascii_character;
}

uint8_t* encode_morse(uint8_t ascii_character) {
	ascii_character = capitalize(ascii_character);
	for (size_t index = 0; MORSE_CODE_CHART[index].symbol != '\0'; index++) {
		if (MORSE_CODE_CHART[index].symbol == ascii_character) {
			return (uint8_t*) MORSE_CODE_CHART[index].code;
		}
	}
	return NULL;
}

void blink_morse_codepoint(uint8_t morse_codepoint) {
	size_t letter_duration = 0;

	switch (morse_codepoint) {
	case '.':
		letter_duration = DOT_DURATION;
		break;
	case '-':
		letter_duration = DASH_DURATION;
		break;
	default:
		// Unreachable. The alphabet contains nothing but “.” and “-”.
		Error_Handler();
	}

	turn_light_on(COLOR_GREEN);
	HAL_Delay(letter_duration);
	turn_light_off(COLOR_GREEN);
	HAL_Delay(PAUSE_BETWEEN_LETTERS);
}

void blink_morse_string(uint8_t *morse_code) {
	for (int index = 0; morse_code[index] != '\0'; ++index) {
		blink_morse_codepoint(morse_code[index]);
	}
}

void blink_as_morse(uint8_t ascii_character) {
	uint8_t* morse_code = encode_morse(ascii_character);
	blink_morse_string(morse_code);
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
		uint8_t received_char;

		HAL_StatusTypeDef read_result = read_char(&received_char);

		switch (read_result) {
		case HAL_OK:
			print_char(received_char);
			if (received_char == '+') {
				toggle_interrupts();
			} else {
				print_char(received_char);
				blink_as_morse(received_char);
			}
			break;

		case HAL_ERROR:
		case HAL_BUSY:
		case HAL_TIMEOUT:
			break;
		}
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
