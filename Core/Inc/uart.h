/*
 * uart.h
 *
 *  Created on: Oct 20, 2025
 *      Author: mandr
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include"main.h"
#include <stdbool.h>

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
void uart_send_data(char* restrict data, size_t len);
void uart_receive_data(char* buffer, uint16_t len, uint16_t timeout);


#endif /* INC_UART_H_ */
