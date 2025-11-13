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
static volatile uint16_t timeUart = 0;

/*flags for TX data*/
static volatile uint8_t flagStartBit = 0;
static volatile uint8_t flagStopBit = 0;
static volatile uint8_t flagDataTx = 0;
static volatile uint8_t flagSendBit = 0;
static volatile uint8_t flagEndBit = 0; /*check end send bit function*/
static volatile uint8_t flagParityBit = 0;

static volatile uint8_t parityBitValue = 0;
static volatile uint8_t bitIndex = 0;

/*flags for RX data*/
static volatile uint8_t flagRxStart = 0;



/*Functions prototypes*/
void uart_init(PARITY parity);
void uart_send_data(char* restrict data, size_t len);
static inline void uart_send_byte(uint8_t data);
static inline uint8_t uart_check_parity(uint8_t data);
static inline void uart_com_handle(void);



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



static inline void uart_send_byte(uint8_t data)
{
	/*
	 * Function to change the state in sending data. The effective sending
	 * will have place in the interrupt function call
	 * Input: data of unsigned 8 bit
	 * Output: void
	 */

	bufferUart.data = data;

	flagStartBit = 0;
	flagDataTx = 0;
	flagSendBit = 0;
    flagEndBit = 0;

    if(bufferUart.parity != None)
    {
    	flagParityBit = uart_check_parity(bufferUart.data);
    }

	TIM2->CNT = 0; /*reset counter register*/
	timeUart = 0;

	HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_RESET);
	bufferUart.state = TX;

}



void uart_send_data(char* restrict data, size_t len)
{
	/*
	 * Function to send a large number of data.
	 * Input: Pointer to the data of 8 bit data type
	 * 		  Length of the data packet
	 * Output: void
	 */


	while(len != 0)
	{
		while(bufferUart.state == TX || bufferUart.state == EndTX); /*wait for idle state*/
		uart_send_byte(*data);
		data++;
		len--;
	}

}



static inline uint8_t uart_check_parity(uint8_t data)
{
	/*
	 * Function that will return the parity of the
	 * data frame response. It will return true for
	 * even parity and false for odd parity in O(1) time
	 * Input: 8 bit data value
	 * Output: uint8_t 0 or 1
	 */


	data = data - ((data>>1)&0x55);
	data = (data&0x33) + (data>>2&0x33);
	data = (data + (data>>4)) & 0x0F;

	if(data % 2 == 0)
	{
		return true; /*1*/
	}

	else
	{
		return false; /*0*/
	}

}


static void uart_send_stop_bit(void)
{
	/*
	 * Static function to send the last bit.
	 * timeUart must be equal to 0, at the
	 * beginning of the new bit. The function
	 * will change the state of the bufferUart
	 * variable.
	 * Input: void
	 * Output: void
	 */

	if(timeUart == 0 && flagStopBit == 0)
	{
		flagStopBit = 1;
		HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, GPIO_PIN_SET);
		return;
	}

	if(timeUart==1 && flagStopBit == 1)
	{
		return; /*make sure to end stop bit*/
	}

	if(timeUart == 0 && flagStopBit == 1)
	{
		flagStopBit = 0;
		bufferUart.state = EndTX;
	}


}


static inline void uart_send_bit(uint8_t bit)
{
	/*
	 * Static function to send a bit of
	 * 1 or 0. TimeUart must be equal to
	 * 0, at the start of a new bit sample
	 * Input: bit value 1 or 0
	 * Output: void
	 */


	if(timeUart == 0 && flagSendBit == 0)
	{
		flagSendBit = 1;
		HAL_GPIO_WritePin(UART_TX_PORT, UART_TX_PIN, bit);
		return;
	}

	if(timeUart==1 && flagSendBit == 1)
	{
		return; /*make sure to end stop bit*/
	}

	if(timeUart == 0 && flagSendBit == 1)
	{
		flagSendBit = 0;
		flagEndBit = 1;
	}

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

				if(bitIndex == 8 && timeUart == 0) /*first iteration is 0*/
				{
					if(bufferUart.parity == None)
					{
						uart_send_stop_bit();
						return;
					}


					if(flagEndBit == 0)
					{
						if(bufferUart.parity == Even && flagParityBit==true)
						{
							uart_send_bit(0);
						}

						if(bufferUart.parity == Even && flagParityBit==false)
						{
							uart_send_bit(1);
						}

						if(bufferUart.parity == Odd && flagParityBit==true)
						{
							uart_send_bit(1);
						}

						if(bufferUart.parity == Odd && flagParityBit==false)
						{
							uart_send_bit(0);
						}
					}

					if(flagEndBit == 1)
					{
						uart_send_stop_bit(); /*iteration of timerUart equal 0*/
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
					if(HAL_GPIO_ReadPin(UART_RX_PORT, UART_RX_PIN) == 1)
					{
						bufferUart.state = EndRX;
					}

					else
					{
						bufferUart.state = ERR;
						return;
					}
				}

				else
				{

					if(flagParityBit == 0) /*parity bit sampling*/
					{
						/*take parity bit value*/
						parityBitValue = HAL_GPIO_ReadPin(UART_RX_PORT, UART_RX_PIN);

						if(((uart_check_parity(bufferUart.data) + parityBitValue) % 2 == 1) && (bufferUart.parity == Even))
						{
							/*correct bit sampling*/
							flagParityBit = 1;
							return;
						}

						if(((uart_check_parity(bufferUart.data) + parityBitValue) % 2 == 0) && (bufferUart.parity == Odd))
						{
							/*correct bit sampling*/
							flagParityBit = 1;
							return;
						}

						/*error if not*/
						bufferUart.state = ERR;
						flagParityBit = 1;
						return;
					}

					if(flagParityBit == 1)
					{
						/*stop bit*/

						if(HAL_GPIO_ReadPin(UART_RX_PORT, UART_RX_PIN) == 1)
						{
							bufferUart.state = EndRX;
						}

						else
						{
							bufferUart.state = ERR;
							return;
						}

					}
				}

			}
	    break;



		case(EndRX):
    	    	__HAL_GPIO_EXTI_CLEAR_IT(UART_RX_PIN); /*clear intr bit flag*/
    	    	bufferUart.state = IDLE;
    	    	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
		break;

		case(ERR):
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


















