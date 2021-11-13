/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include "crc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum BootloaderOpcode_t {
  BOOTLOADER_CMD_INVALID = 0x00,
  BOOTLOADER_CMD_ECHO = 0x01,
  BOOTLOADER_CMD_SETSIZE = 0x02,
  BOOTLOADER_CMD_UPDATE = 0x03,
  BOOTLOADER_CMD_CHECK = 0x04,
  BOOTLOADER_CMD_JUMP = 0x05
} BootloaderOpcode;

typedef struct BootloaderCommand_t {
  uint32_t data;
  BootloaderOpcode opcode;
} BootloaderCommand;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define AppPagesNum (127-32)
#define AppFrPageAddr 0x8008000

char const* const BOOTLOADER_MSG_OK = "OK!";
char const* const BOOTLOADER_MSG_ERR = "ERR";

uint32_t const APPLICATION_ADDRESS = 0x08008000UL;

#define BOOTLOADER_BUFFER_SIZE 1024
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool buttonPressed = false;
uint32_t applicationSize = 0;
uint8_t bootloaderBuffer[BOOTLOADER_BUFFER_SIZE] = { };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void deinit_peripherals() {
  HAL_CRC_DeInit(&hcrc);
  HAL_UART_DeInit(&huart2);
  HAL_NVIC_DisableIRQ(B1_EXTI_IRQn);
  HAL_GPIO_DeInit(LD2_GPIO_Port, LD2_Pin);
  HAL_GPIO_DeInit(B1_GPIO_Port, B1_Pin);
  LL_RCC_DeInit();  // We're using LL RCC, so we'll use this function
  HAL_DeInit();

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
}

void jump_to_application(uint32_t const app_address) {
  typedef void (*jumpFunction)(); // helper-typedef
  uint32_t const jumpAddress = *(__IO uint32_t*) (app_address + 4); // Address of application's Reset Handler
  jumpFunction runApplication = (jumpFunction) jumpAddress; // Function we'll use to jump to application

  deinit_peripherals(); // Deinitialization of peripherals and systick
  __set_MSP(*((__IO uint32_t*) app_address)); // Stack pointer setup
  runApplication(); // Jump to application
}

BootloaderCommand get_command(UART_HandleTypeDef* const uart, uint32_t const timeout) {
  BootloaderCommand cmd = { .opcode = BOOTLOADER_CMD_INVALID };
  uint8_t buffer[5] = { 0 };

  HAL_StatusTypeDef status = HAL_UART_Receive(uart, buffer, 5, timeout);
  if(buffer[0]==0x01)
  {
	  volatile int i;
	  UNUSED(i);
  }
  if (status == HAL_OK) {
    cmd.opcode = buffer[0];
    // Assuming big-endian data coming from programmer
    cmd.data = (((uint32_t) buffer[1]) << 24)
               | (((uint32_t) buffer[2]) << 16)
               | (((uint32_t) buffer[3]) << 8)
               | buffer[4];
  }

  return cmd;
}

void respond_ok(UART_HandleTypeDef* const uart) {
  // for a nice, visual effect
  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  HAL_UART_Transmit(uart, (uint8_t*) BOOTLOADER_MSG_OK, 3, HAL_MAX_DELAY);
}

void respond_err(UART_HandleTypeDef* const uart) {
  HAL_UART_Transmit(uart, (uint8_t*) BOOTLOADER_MSG_ERR, 3, HAL_MAX_DELAY);
}

bool erase_application(unsigned firstPageAddr, unsigned NbPages) {
  FLASH_EraseInitTypeDef eraseConfig;
  eraseConfig.TypeErase = FLASH_TYPEERASE_PAGES;
  eraseConfig.Banks=1;
  eraseConfig.PageAddress = firstPageAddr;
  eraseConfig.NbPages = NbPages;


  uint32_t sectorError = 0;
  if (HAL_FLASHEx_Erase(&eraseConfig, &sectorError) != HAL_OK) {
    return false;
  }

  return sectorError == 0xFFFFFFFFU;
}

bool flash_and_verify(uint8_t const* const bytes, size_t const amount, uint32_t const offset) {
  if (amount == 0 || bytes == NULL || amount % 4 != 0) {
    return false;
  }

  // Program the flash memory word-by-word
  for (uint32_t bytesCounter = 0; bytesCounter < amount; bytesCounter += 4) {
    uint32_t const programmingData = *(uint32_t*) (&bytes[bytesCounter]);
    uint32_t const programmingAddress = APPLICATION_ADDRESS + offset + bytesCounter;
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, programmingAddress, programmingData) != HAL_OK) {
      return false;
    }

    uint32_t const verificationData = *(uint32_t*) programmingAddress;
    if (verificationData != programmingData) {
      return false;
    }

  }

  return true;
}

bool receive_and_flash_firmware(UART_HandleTypeDef* const uart, uint32_t const firmwareSize) {
  // sanity check - fail loudly if no application size is set
  if (firmwareSize == 0) {
    return false;
  }

  uint32_t bytesProgrammed = 0;

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return false;
  }

  if (!erase_application(AppFrPageAddr, AppPagesNum)) {
    HAL_FLASH_Lock();
    return false;
  }

  // tell the programmer that you're ready to go
  respond_ok(uart);

  while (bytesProgrammed < firmwareSize) {
    // Calculate how much data is left to receive
    uint32_t const bytesLeft = firmwareSize - bytesProgrammed;
    uint32_t const bytesToReceive = (
        bytesLeft > BOOTLOADER_BUFFER_SIZE ? BOOTLOADER_BUFFER_SIZE : bytesLeft);

    // Try receiving the data, return on failure
    if (HAL_UART_Receive(uart, bootloaderBuffer, bytesToReceive, 1000) != HAL_OK) {
      HAL_FLASH_Lock();
      return false;
    }

    if (!flash_and_verify(bootloaderBuffer, bytesToReceive, bytesProgrammed)) {
      HAL_FLASH_Lock();
      return false;
    }

    bytesProgrammed += bytesToReceive;
    respond_ok(uart);
  }

  HAL_FLASH_Lock();
  return true;
}

bool verify_firmware(uint32_t const firmwareSize, uint32_t const expectedChecksum) {
  // sanity check - fail loudly if no application size is set
  if (firmwareSize == 0) {
    return false;
  }

  uint32_t const calculatedChecksum = HAL_CRC_Calculate(&hcrc,
                                                        (uint32_t*) APPLICATION_ADDRESS,
                                                        firmwareSize / 4);
  return (calculatedChecksum == expectedChecksum);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
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
  MX_USART2_UART_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  printf("Hello, this is bootloader. Waiting for firmware.\r\n");
//  HAL_FLASH_Unlock();
//  erase_application(AppFrPageAddr, AppPagesNum);
//  printf("Appliation erassed.\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    BootloaderCommand cmd = get_command(&huart2, 100);
	    switch (cmd.opcode) {
	    case BOOTLOADER_CMD_ECHO: {
	      // Simple echo request, respond with OK to tell that
	      // the bootloader is running.
	      respond_ok(&huart2);
	      break;
	    }
	    case BOOTLOADER_CMD_SETSIZE: {
	      // Set the app size and respond with OK
	      applicationSize = cmd.data;
	      respond_ok(&huart2);
	      break;
	    }
	    case BOOTLOADER_CMD_UPDATE: {
	      if (receive_and_flash_firmware(&huart2, applicationSize)) {
	        respond_ok(&huart2);
	      } else {
	        respond_err(&huart2);
	      }
	      break;
	    }
	    case BOOTLOADER_CMD_CHECK: {
	      if (verify_firmware(applicationSize, cmd.data)) {
	        respond_ok(&huart2);
	      } else {
	        respond_err(&huart2);
	      }
	      break;
	    }
	    case BOOTLOADER_CMD_JUMP: {
	      // Jump directly to the application.
	      respond_ok(&huart2);
	      jump_to_application(APPLICATION_ADDRESS);
	      break;
	    }
	    case BOOTLOADER_CMD_INVALID: {
	      // No command received. We have to handle this case, because
	      // otherwise the bootloader would indefinitely spam ERR
	      // while nothing is happening.
	      break;
	    }
	    default: {
	      // Invalid opcode, respond with error.
	      respond_err(&huart2);
	      break;
	    }
	    }

	    if (buttonPressed) {
	      buttonPressed = false;
	      // jump to application
	      jump_to_application(APPLICATION_ADDRESS);
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
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_16);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(64000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int _write(int file, char* ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t*) ptr, len, HAL_MAX_DELAY);
  return len;
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  if (pin == B1_Pin) {
    buttonPressed = true;
  }
}
/* USER CODE END 4 */

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
