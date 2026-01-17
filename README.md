# Software UART Protocol Implementation via Bit-Banging

A pure software implementation of the UART protocol for STM32 microcontrollers, developed without relying on hardware UART peripherals. This project demonstrates protocol emulation using only GPIO pins, timers, and interrupts.

<img width="725" height="199" alt="image" src="https://github.com/user-attachments/assets/7cfeef6d-d3d4-4b6b-a840-0d326703e417" />


## Project Overview

This implementation recreates the asynchronous UART protocol entirely in software using the bit-banging technique. The protocol operates at 300 baud (extendable) and supports configurable parity checking (None/Odd/Even).

**Key Features:**
- Pure software UART protocol (no hardware UART required)
- Configurable parity: None, Odd, Even
- Baud rate: 300 bps (configurable up to MCU limits)
- Frame format: START | 8-bit DATA | PARITY (optional) | STOP
- Efficient O(1) parity computation algorithm
- Robust FSM-based state management
- Bi-directional communication (TX/RX)

## Hardware Requirements

- **MCU:** STM32F407VET6 (ARM Cortex-M4, 168 MHz)
- **TX Pin:** PA9 (GPIO Output)
- **RX Pin:** PA10 (GPIO Input with falling edge interrupt + pull-up)
- **Timer:** TIM2 configured at 600 Hz (2x baud rate)
- **External Clock:** 8 MHz crystal oscillator
- **USB-TTL Converter:** For PC communication testing

### Pin Configuration

| Pin | Function | Mode |
|-----|----------|------|
| PA9 | UART TX | GPIO Output |
| PA10 | UART RX | GPIO Input (EXTI, Falling Edge, Pull-Up) |

### Timer Configuration

- **Frequency:** 600 Hz (twice the baud rate for optimal sampling)
- **APB1 Clock:** 84 MHz
- **Prescaler:** 4 → 21 MHz
- **Counter Period:** 35,000 → 600 Hz

![System Block Diagram](https://via.placeholder.com/600x400/2d2d2d/ffffff?text=STM32+%E2%86%94+USB-TTL+%E2%86%94+PC)

## Software Architecture

### Core Components

The implementation consists of two main files:
- **uart.h** - Public API interface
- **uart.c** - Complete implementation with FSM logic

### Data Structures

```c
typedef enum STATE {
    IDLE,
    TX,
    EndTX,
    RX,
    EndRX,
    ERR
} STATE;

typedef enum PARITY {
    None,
    Odd,
    Even
} PARITY;

typedef struct UART {
    uint8_t data;
    STATE state;
    PARITY parity;
} UART;
```

### Finite State Machine (FSM)

The protocol operates through a carefully designed FSM with the following states:

- **IDLE:** No communication, TX line HIGH
- **TX:** Transmitting data frame
- **EndTX:** Transmission complete, transitioning to IDLE
- **RX:** Receiving data frame
- **EndRX:** Reception complete, transitioning to IDLE
- **ERR:** Error state (parity/framing error)

![FSM State Diagram](https://via.placeholder.com/700x500/1e1e1e/00ff00?text=IDLE+%E2%86%94+TX+%E2%86%92+EndTX+%7C+IDLE+%E2%86%94+RX+%E2%86%92+EndRX+%7C+ERR+%E2%86%92+IDLE)

### Timing and Sampling

The implementation uses a dual-phase timing system:
- **timeUart = 0:** Beginning of bit period (transmission occurs here)
- **timeUart = 1:** Midpoint of bit period (reception sampling occurs here)

This approach ensures:
- Stable signal levels during receiver sampling
- Maximum noise immunity
- Proper synchronization between TX and RX

### Interrupt Service Routines

**1. Timer ISR (600 Hz periodic)**
- Toggles `timeUart` between 0 and 1
- Calls `uart_com_handle()` to execute FSM logic
- Manages timeout counter for reception

**2. GPIO EXTI ISR (Falling Edge on RX)**
- Detects start bit (HIGH → LOW transition)
- Resets timer counter for synchronization
- Disables further GPIO interrupts during reception
- Initializes reception state

## API Reference

### Initialization

```c
void uart_init(uint8_t parity);
```
Initializes the UART protocol with the specified parity mode.
- **Parameters:** `parity` - 0: None, 1: Odd, 2: Even
- **Returns:** void

### Transmission

```c
void uart_send_data(char* restrict data, size_t len);
```
Transmits multiple bytes sequentially.
- **Parameters:** 
  - `data` - Pointer to data buffer
  - `len` - Number of bytes to transmit
- **Returns:** void

### Reception

```c
void uart_receive_data(char* buffer, uint16_t len, uint16_t timeout);
```
Receives multiple bytes with timeout capability.
- **Parameters:**
  - `buffer` - Pointer to reception buffer
  - `len` - Number of bytes to receive
  - `timeout` - Timeout in seconds
- **Returns:** void

## Parity Detection Algorithm

The implementation includes an optimized O(1) parity computation algorithm that avoids loops:

```c
static inline uint8_t uart_check_parity(uint8_t data) {
    data = data - ((data>>1)&0x55);
    data = (data&0x33) + (data>>2&0x33);
    data = (data + (data>>4)) & 0x0F;
    
    return (data % 2 == 0) ? true : false;
}
```

**Algorithm Logic:**
1. Process bit pairs to count 1s in each pair
2. Progressively sum the counts
3. Return parity (even/odd) in constant time

## Usage Example

```c
#include "uart.h"

int main(void) {
    // System initialization
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    
    // Initialize UART with ODD parity
    uart_init(1);
    
    // Transmit message
    char tx_msg[] = "Hello UART!";
    uart_send_data(tx_msg, sizeof(tx_msg) - 1);
    
    // Receive message with 10 second timeout
    char rx_buffer[20];
    uart_receive_data(rx_buffer, 20, 10);
    
    // Echo back received data
    uart_send_data(rx_buffer, 20);
    
    while(1) {
        // Main loop
    }
}
```

## Testing and Validation

The implementation was tested with:
- PC communication via USB-TTL converter
- Multi-byte transmission and reception
- Continuous character streams
- Parity error detection
- Timeout functionality

**Test Results:**
- ✅ Single character transmission/reception
- ✅ Multi-byte streams (e.g., "aaaa")
- ✅ Parity validation (Odd/Even)
- ✅ Framing error detection
- ✅ Timeout mechanism

![Serial Monitor Test](https://via.placeholder.com/800x300/0a0a0a/00ff00?text=TX%3A+pui+de+piropopircanita+%7C+RX%3A+UART+OK)

## Technical Challenges and Solutions

### Problem: Continuous Transmission Errors

**Symptom:** First character received correctly, subsequent characters corrupted when typing rapidly (e.g., "aaaa").

**Root Cause:** Incorrect timing between stop bit of first frame and start bit of next frame. The implementation was waiting an entire bit period after the stop bit before attempting to receive the next start bit.

**Solution:** Modified the FSM logic to properly handle back-to-back frames according to UART specification, where the start bit immediately follows the stop bit with no gap.

## Project Structure

```
MES_Project/
├── Core/
│   ├── Inc/
│   │   ├── uart.h          # Public API header
│   │   └── main.h
│   └── Src/
│       ├── uart.c          # Complete implementation
│       └── main.c          # Test application
├── Drivers/               # STM32 HAL drivers
├── .gitignore
└── README.md
```

## Future Enhancements

- [ ] Dynamic baud rate configuration (up to MCU-dependent limits)
- [ ] Message queue implementation for automatic background transmission
- [ ] DMA support for improved performance
- [ ] Support for 9-bit data frames
- [ ] Hardware flow control (RTS/CTS)
- [ ] Multi-UART instance support

## Development Tools

- **IDE:** STM32CubeIDE
- **Configurator:** STM32CubeMX
- **Debugger:** ST-LINK/V2
- **Terminal:** Serial terminal (115200 baud for USB-TTL converter)

## Performance Characteristics

- **Baud Rate:** 300 bps (configurable)
- **CPU Overhead:** Interrupt-driven, minimal main loop blocking
- **Timing Accuracy:** ±1% (crystal oscillator dependent)
- **Maximum Throughput:** ~30 bytes/second at 300 baud

## License

This project was developed as part of the Microcontrollers and Embedded Systems (MES) course at the National University of Science and Technology POLITEHNICA Bucharest, Faculty of Electronics, Telecommunications and Information Technology.

**Author:** Miron Andrei-Auraș, ACES Master Program, 2025-2026

## References

- [UART Protocol Specification](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter)
- [STM32F407 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020.pdf)
- [Bit-Banging Techniques](https://en.wikipedia.org/wiki/Bit_banging)

## Contributing

This is an academic project, but suggestions and improvements are welcome. Feel free to open an issue or submit a pull request.

## Acknowledgments

Special thanks to the MES course instructors and the ACES program for providing the opportunity to explore low-level embedded systems programming and protocol implementation.

---

**Repository:** https://github.com/Myronul/MES_Project.git

**Documentation:** Full project documentation available in [MES_Project_Doc.pdf](./MES_Project_Doc.pdf)
