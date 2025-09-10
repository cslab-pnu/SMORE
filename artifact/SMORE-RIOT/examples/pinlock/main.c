/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

/*
 * Copyright (c) 2022, University of Rochester
 *
 * Modified for the Randezvous project.
 */

/* Includes ------------------------------------------------------------------*/
//#include "main.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "timex.h"
#include "ztimer.h"
#include "hashes.h"
#include "hashes/sha256.h"
#include "hashes/sha2xx_common.h"
//#include "mbedtls/sha256.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LED_NUMBERS  3U
#define LED_1_INIT() LED_RED_INIT(LOGIC_LED_OFF)
#define LED_2_INIT() LED_GREEN_INIT(LOGIC_LED_OFF)
#define LED_3_INIT() LED_BLUE_INIT(LOGIC_LED_OFF)
#define LED_1_ON()   LED_RED_ON()
#define LED_1_OFF()  LED_RED_OFF()
#define LED_2_ON()   LED_GREEN_ON()
#define LED_2_OFF()  LED_GREEN_OFF()
#define LED_3_ON()   LED_BLUE_ON()
#define LED_3_OFF()  LED_BLUE_OFF()

#define PINRXBUFFSIZE 5
#define PINBUFFSIZE (COUNTOF(pin))
#define COUNTOF(__BUFFER__) (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#define DWT_CYCCNT (*(uint32_t *) 0xe0001004)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

char PinRxBuffer[PINRXBUFFSIZE] = {0};
char key[32] = {0};
char key_in[32] = {0};
char pin[5] = "1995";

/* Private function prototypes -----------------------------------------------*/
static void print(char *, int len);
/* Private functions ---------------------------------------------------------*/

static void delay(void) {
	uint32_t loops = coreclk() / 20;
	for (volatile uint32_t i = 0; i < loops; i++) {  }
}

void lock(void)
{
	LED0_TOGGLE;
}

void unlock(void)
{
	LED0_TOGGLE;
}

void rx_from_uart(uint32_t size)
{
	if (size > PINRXBUFFSIZE) {
		while(1) {}
	}

	strcpy(PinRxBuffer, pin);
	//scanf("%s", PinRxBuffer);
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	int len;
	int unlock_count = 0;

	unsigned int one = 1;
	unsigned int exp;
	unsigned int ms;
	static char locked[] = "System Locked\n";
	static char enter[] = "Enter Pin:\n";
	static char unlocked[] = "Unlocked\n";
	static char incorrect[] = "Incorrect Pin\n";
	static char waiting[] = "Waiting...\n";
	static char lockout[] = "System Lockout\n";

	uint32_t t;

	//printf("Start to run %s\n", BENCHMARK_NAME);
	printf("Start to run\n");
	rx_from_uart(5);
	enable_dwt();
	//t = RTC_GetTick();

	//Program_Init();
	mbedtls_sha256(pin, PINBUFFSIZE, key, 0);
	while (1) {
		lock();
		print(locked, sizeof(locked));
		unsigned int failures = 0;
		// In Locked State
		while (1) {
			print(enter, sizeof(enter));
			//rx_from_uart(5);
			// Hash password received from UART
			//sha256(PinRxBuffer, PINRXBUFFSIZE, key_in);
			mbedtls_sha256(PinRxBuffer, PINRXBUFFSIZE, key_in, 0);
			
			unsigned int i;
			for (i = 0; i < 32; i++) {
				if (key[i] != key_in[i]) {
					break;
				}
			}
			if (i == 32) {
				print(unlocked, sizeof(unlocked));
				unlock_count++;
				if (unlock_count >= 50) {
					goto out;
				}
				break;
			}

			failures++; // Increment number of failures
			print(incorrect, sizeof(incorrect));
			if (failures > 500 && failures <= 510) {
				// Essentially 2^failures
				exp = one << failures;
				// After 5 tries, start waiting around 5 secs
				// and then doubles
				ms = 78 * exp;
				print(waiting, sizeof(waiting));
				delay();
				//ztimer_sleep(ZTIMER_USEC, 5*US_PER_SEC);
				//SDK_DelayAtLeastUs(ms, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
			} else if (failures > 1000) {
				print(lockout, sizeof(lockout));
				while (1) {
				}
			}

		}

		unlock();
		// Wait for lock command
		/*while (1) {
			rx_from_uart(2);
			if (PinRxBuffer[0] == '0') {
				break;
			}
		}*/
		lock();
	}

out:
	t = DWT_CYCCNT;
	printf("Elapsed time: %u cycles\n", t);

	return 0;
}


// Call all initializations for program


static void print(char * str, int len)
{
	printf("%s", str);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t * file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1) {
	}
}
#endif

/******************END OF FILE****/
