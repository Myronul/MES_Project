/*
 * uart.c
 *
 *  Created on: Oct 20, 2025
 *      Author: mandr
 */

#include"uart.h"

extern TIM_HandleTypeDef htim2;


typedef struct UART
{
	uint8_t data;

	STATE state;
	PARITY parity;

}UART;


static UART bufferUart = {0,0,0}; /*init uart buffer*/
static uint16_t timeUart = 0;

/*flags for TX data*/
static uint8_t flagStartBit = 0;
static uint8_t flagDataTx = 0;
static uint8_t flagParityBit = 0;
static uint8_t bitIndex = 0;

/*flags for RX data*/
static uint8_t flagRxStart = 0;


void uart_init(PARITY parity)
{
	/*
	 * init of the uart cusotm protocol
	 * Input: parity type: 0 none, 1 odd and 2 even
	 * Output: void
	 */

	bufferUart.parity = parity;
	bufferUart.state = IDLE;

	HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_SET);

}


void uart_send_data(uint8_t* restrict const data)
{
	/*
	 * Function to change the state in sending data. The effective sending
	 * will have place in the interrupt function call
	 * Input: pointer to the data of unsigned 8 bit
	 * Output: void
	 */

	bufferUart.data = *data;

	flagStartBit = 0;
	flagDataTx = 0;
	flagParityBit = 0;

	TIM2->CNT = 0; /*reset counter register*/
	timeUart = 0;

	HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_RESET);
	bufferUart.state = TX;

}


static inline void uart_check_parity(void)
{

}



static inline void uart_com_handle(void)
{

	switch(bufferUart.state)
	{
		case(TX):

			if(timeUart == 0 && flagStartBit == 0)
			{
				/*end start bit tx*/

				flagStartBit = 1;
				flagDataTx = 1;
				bitIndex = 0;

			}

			if(flagDataTx == 1)
			{
				/*start data payload transmission*/

				if(bitIndex < 8 && timeUart == 0)
				{
					if((bufferUart.data & (1<<bitIndex)) != 0) /*1*/
					{
						HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_SET);
					}

					else /*0*/
					{
						HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_RESET);
					}

					bitIndex++;

					return; /*the return will make the 8th bit
							  last all cycle until timeUart is 0*/

				}

				/*parity bit tx*/

				if(bitIndex == 8 && timeUart == 0)
				{
					if(bufferUart.parity == None)
					{
						HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_SET);
						bufferUart.state = EndTX;

					}

					if(bufferUart.parity == Even)
					{

					}

					if(bufferUart.parity == Odd)
					{

					}
				}

			}
		break;



		case(EndTX):
			bufferUart.state = IDLE;
		break;



		case(RX):

			if(flagRxStart==1 && timeUart==1)
			{
				/*start bit, wait...*/
				return;
			}

			else
			{
				flagRxStart = 0;
			}

			if(bitIndex < 8 && timeUart == 1)
			{
				/*receive the message at time 1*/

				if(HAL_GPIO_ReadPin(UART_RX_PORT, UART_RX_PIN) == 1)
				{
					/*HIGH -> 1*/
					bufferUart.data = bufferUart.data | (1<<bitIndex);
				}

				else
				{
					/*LOW -> 0*/
					bufferUart.data = bufferUart.data & ~(1<<bitIndex);
				}

				bitIndex++;

				return; /*the return will make the 8th bit last*/
					    /*all cycle until 0 uart time*/
			}

			if(bitIndex == 8 && timeUart == 1)
			{
				/*end rx transmission*/

				if(bufferUart.parity == None)
				{
					/*stop bit*/
					bufferUart.state = EndRX;
				}

				if(bufferUart.parity == Odd)
				{

				}

				if(bufferUart.parity == Even)
				{

				}

			}
	    break;



		case(EndRX):
    	    	__HAL_GPIO_EXTI_CLEAR_IT(UART_RX_PIN); /*clear intr bit flag*/
    	    	bufferUart.state = IDLE;
    	    	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
		break;



		default: break;

	}
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/*
	 * Weak function ISR for the timer callback
	 */

	if(htim->Instance == TIM2)
	{
		timeUart = (timeUart + 1) % 2;
		uart_com_handle();
		//HAL_GPIO_TogglePin(UART_TX_PORT, UART_TX_PIN);
	}

}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	/*
	 * Weak function ISR for the interrupt RX callback
	 */

	if(GPIO_Pin == UART_RX_PIN)
	{
		/*start reception data*/
		HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);

		flagRxStart = 1;
		timeUart = 0;
		bitIndex = 0;
		bufferUart.data = 0; /*init data*/
		TIM2->CNT = 0;

		bufferUart.state = RX;
	}
}


















