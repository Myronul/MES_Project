# MES_Project
Final project at the course of Microcontrollers and Embedded Systems, custom implementation of the UART Protocol on an STM32. The implementation cand be found in
the files: core/inc/uart.h and core/inc/uart.c.

Emulated Protocol Specifications:
The protocol is purely software-implemented, using only periodic (timer) interrupts and external GPIO interrupts on the falling edge. For initial tests, the 
lowest baud rate was used, which is 300 bits per second. Future updates may include a feature for dynamically adjusting the baud rate up to an MCU-dependent limit.
The data frame format is as follows: start bit | 8-bit payload | optional parity bit | stop bit.

Software Architecture:
There are two main ISR functions: Timer and GPIOIntr. The timer-associated function executes automatically every X ms, depending on the baud rate. For reception, 
the GPIOIntr function is triggered by the first falling edge, after which the external interrupt is disabled and acquisition continues using the synchronized timer.

Transmission:
Transmission is handled by the timer ISR, set at 600 Hz (i.e., every 6.66 ms). A doubled frequency is used to acquire data efficiently at the middle of the bit 
period. A function with FSM logic manages the UART buffer transmission state.

Reception:
Upon detecting the first bit (falling edge), the associated ISR is called. The timer counter is reset to 0, and the external interrupt is disabled, so that 
acquisition relies on the timer interrupts synchronized with the incoming signal.

Future Features:
*Implementation of a message queue, allowing automatic push of data that will be transmitted automatically via ISR.
*Implementation of dynamically configurable baud rate.