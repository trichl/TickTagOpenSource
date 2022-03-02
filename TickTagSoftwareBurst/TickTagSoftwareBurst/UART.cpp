#include "UART.h"

void initPrimaryUARTwith9600() {
	#if defined (__AVR_ATtiny1626__)
	PORTMUX.USARTROUTEA = 0; // normal pin position
	#else
	PORTMUX.CTRLB = 0; // normal pin position
	#endif
	
	USART0.CTRLB |= (USART_TXEN_bm | USART_RXEN_bm); // VERY STRANGE WORKAROUND: if this is not here: re-initialization of uart fails after using secondary uart.. (TX not working)
	
	PORTB.OUTSET = PIN2_bm; // set output HIGH (recommended by datasheet)
	PORTB.DIR |= PIN2_bm; // TX = output
	PORTB.DIR &= ~PIN3_bm; // RX = input
	
	USART0.BAUD = (uint16_t) USART0_BAUD_RATE(9600); // calculate BAUD rate (depending on F_CPU)
	//USART0.CTRLB |= (USART_TXEN_bm | USART_RXEN_bm); // constantly +17uA
}

void initPrimaryUARTwith115200() {
	#if defined (__AVR_ATtiny1626__)
	PORTMUX.USARTROUTEA = 0; // normal pin position
	#else
	PORTMUX.CTRLB = 0; // normal pin position
	#endif
	
	USART0.CTRLB |= (USART_TXEN_bm | USART_RXEN_bm); // VERY STRANGE WORKAROUND: if this is not here: re-initialization of uart fails after using secondary uart.. (TX not working)
	
	PORTB.OUTSET = PIN2_bm; // set output HIGH (recommended by datasheet)
	PORTB.DIR |= PIN2_bm; // TX = output
	PORTB.DIR &= ~PIN3_bm; // RX = input
	
	USART0.BAUD = (uint16_t) USART0_BAUD_RATE(115200); // calculate BAUD rate (depending on F_CPU)
	//USART0.CTRLB |= (USART_TXEN_bm | USART_RXEN_bm); // constantly +17uA
}

void initSecondUARTwith9600() {
	#if defined (__AVR_ATtiny1626__)
		PORTMUX.USARTROUTEA = 1; // normal pin position
	#else
		PORTMUX.CTRLB = 1; // alternative pin position
	#endif
	
	USART0.CTRLB |= (USART_TXEN_bm | USART_RXEN_bm); // VERY STRANGE WORKAROUND: if this is not here: re-initialization of uart fails most of the time
	
	// pins need to be initialized manually, otherwise UART not working
	PORTA.PIN1CTRL = 0; // enable input buffer again
	PORTA.PIN2CTRL = 0; // enable input buffer again
	PORTA.OUTSET = PIN1_bm; // set output HIGH (recommended in datasheet)
	PORTA.DIR |= PIN1_bm; // TX = output
	PORTA.DIR &= ~PIN2_bm; // RX = input
	
	// here: CTRLB = ON -> DOES NOT WORK, needs to be BEFORE PIN change -> has something to do with pin configuration
	
	USART0.BAUD = (uint16_t) USART0_BAUD_RATE(9600); // calculate BAUD rate (depending on F_CPU)
	//USART0.CTRLB |= (USART_TXEN_bm | USART_RXEN_bm); // constantly +17uA
}

uint8_t uartFlush() {
	uint8_t unused = 0;
	while(USART0.STATUS & USART_RXCIF_bm) { unused = USART0.RXDATAL; } // read char from buffer
	return unused;
}

void deinitSecondUartPins() {
	PORTA.DIR = PORTA.DIR & ~PIN1_bm; // TXD2 as INPUT pin, otherwise leakage
	PORTA.DIR = PORTA.DIR & ~PIN2_bm; // RXD2 as INPUT pin, otherwise leakage
	PORTA.PIN1CTRL = (0 << PORT_INVEN_bp) | (0 << PORT_PULLUPEN_bp) | (1 << PORT_ISC2_bp); // not inverted, no pull-up, disable input buffer (can't read input values), IMPORTANT, otherwise +30uA leakage
	PORTA.PIN2CTRL = (0 << PORT_INVEN_bp) | (0 << PORT_PULLUPEN_bp) | (1 << PORT_ISC2_bp); // not inverted, no pull-up, disable input buffer (can't read input values), IMPORTANT, otherwise +30uA leakage	
}

void uartOff() {
	while(!((USART0.STATUS & USART_TXCIF_bm) && (USART0.STATUS & USART_DREIF_bm))) { ; } // wait till transmit is completed and data register is empty
	USART0.STATUS |= USART_TXCIF_bm; // no Interrupt call, clearing manual, this flag is automatically cleared when the transmit complete interrupt vector is executed. The flag can also be cleared by writing a '1' to its bit location
	USART0.CTRLB = 0; // uart off, otherwise +~500 uA in standby sleep	
}

bool uartRead(char breakChar, char *line, uint8_t maxLength, uint16_t timeoutMs) {
	uint8_t index = 0;
	char c;
	memset(line, 0, maxLength);
	uartFlush();
	while(1) {
		while(!(USART0.STATUS & USART_RXCIF_bm)) {
			timeoutMs--;
			_delay_ms(1); // WARNING!!! only works for 9600 baud (1.0416ms/byte)
			if(timeoutMs == 0) { return false; }
		}
		c = (char) (USART0.RXDATAL);
		if(c != '\n' && c != '\r') { // do not add \n and \r
			line[index++] = c;
			if(index >= maxLength) {
				index = 0;
			}
		}
		if(c == breakChar) {
			line[index] = '\0'; // zero terminate string
			return true;
		}
	}
	return false;
}

bool uartReadMultipleLines(uint8_t lines, char breakChar, char *line, uint8_t maxLength, uint16_t timeoutMs) {
	uint8_t index = 0;
	char c;
	if(lines == 0) { return false; }
	memset(line, 0, maxLength);
	uartFlush();
	while(1) {
		while(!(USART0.STATUS & USART_RXCIF_bm)) {
			timeoutMs--;
			_delay_ms(1); // WARNING!!! only works for 9600 baud (1.0416ms/byte)
			if(timeoutMs == 0) { return false; }
		}
		c = (char) (USART0.RXDATAL);
		//if(c != '\n' && c != '\r') { // do not add \n and \r
			line[index++] = c;
			if(index >= maxLength) {
				index = 0;
			}
		//}
		if(c == breakChar) {
			lines--;
			if(lines == 0) {
				line[index] = '\0'; // zero terminate string
				return true;				
			}
		}
	}
	return true;
}

void print(const char *in) {
	while(*in != '\0') {
		while(!(USART0.STATUS & USART_DREIF_bm)) { ; }
		USART0.TXDATAL = *in;
		in++;
	}
	while(!(USART0.STATUS & USART_DREIF_bm)) { ; }
}

void println(const char *in) {
	print(in);
	print("\n");
	print("\r");
}

void printU16(uint16_t in) {
	if(in >= 10) {
		printU16(in / 10);
	}
	uint8_t digit = in % 10;
	while(!(USART0.STATUS & USART_DREIF_bm)) { ; }
	USART0.TXDATAL = '0' + digit;
}

void printNum(uint32_t in) {
	if(in >= 10) {
		printNum(in / 10);
	}
	uint8_t digit = in % 10;
	while(!(USART0.STATUS & USART_DREIF_bm)) { ; }
	USART0.TXDATAL = '0' + digit;
}

static int usePrintfFunction(char c, FILE *stream) { // static functions should not be declared in C++ header!
	while(!(USART0.STATUS & USART_DREIF_bm)) { ; }
	USART0.TXDATAL = c;
	return 0;
}

void usePrintf() {
	static FILE mystdout;
	fdev_setup_stream(&mystdout, usePrintfFunction, NULL, _FDEV_SETUP_WRITE);
	stdout = &mystdout;
}