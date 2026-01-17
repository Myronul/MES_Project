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


void uart_init(uint8_t parity);
void uart_send_data(char* restrict data, size_t len);
void uart_receive_data(char* buffer, uint16_t len, uint16_t timeout);


#endif /* INC_UART_H_ */
