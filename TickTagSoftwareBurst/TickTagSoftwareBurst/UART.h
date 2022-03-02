#ifndef UART_H_
#define UART_H_

// AVR libs
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#define USART0_BAUD_RATE(BAUD_RATE)				((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5) // IMPORTANT: result needs to be >= 64 in order to work!

void initPrimaryUARTwith9600();
void initPrimaryUARTwith115200(); // at least 2 MHz

void initSecondUARTwith9600(); // 9600: asynchronous by default, NOT @32kHz, should run @0.25MHz as well (BAUD register = 104.666 > 64, okay!)
//void initSecondUARTwith115200(); // asynchronous by default, WARNING: requires at least 2MHz CPU speed, see USART0_BAUD_RATE calculation comment, works with ESP32 down until 2.15V (nice)

void deinitSecondUartPins();

void uartOff();

bool uartRead(char breakChar, char *line, uint8_t maxLength, uint16_t timeoutMs);
bool uartReadMultipleLines(uint8_t lines, char breakChar, char *line, uint8_t maxLength, uint16_t timeoutMs);
uint8_t uartFlush();

void print(const char *in);
void println(const char *in);
void printNum(uint32_t in); // WARNING: recursive
void printU16(uint16_t in); // WARNING: recursive
void usePrintf(); // AVOID! printf needs 1500 bytes in flash!


#endif /* UART_H_ */