/*
 * uart.h
 *
 *  Created on: Oct 20, 2025
 *      Author: mandr
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include"main.h"

typedef enum STATE
{
	IDLE,
	TX,
	EndTX,
	RX,
	EndRX,
	ERR

}STATE;

typedef enum PARITY
{
	None,
	Odd,
	Even

}PARITY;

void uart_init(PARITY parity);
void uart_send_data(uint8_t* restrict const data);

#endif /* INC_UART_H_ */
