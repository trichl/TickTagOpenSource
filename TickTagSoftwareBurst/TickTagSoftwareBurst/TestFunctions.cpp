#include "TestFunctions.h"

void mockFullMemory(uint32_t dataFixLength, uint32_t metaAddressOfAddressPointer) {
	uint8_t buffer[4] = { 0 };
	uint32_t addressPointer = EEPROM_CAT24M01_MEM_SIZE - (dataFixLength * 4);
	buffer[0] = (addressPointer >> 24); buffer[1] = (addressPointer >> 16); buffer[2] = (addressPointer >> 8); buffer[3] = addressPointer;
	eepromWriteMemory(metaAddressOfAddressPointer, buffer, 4);
}

void mockEmptyMemory(uint32_t dataFixLength, uint32_t metaAddressOfAddressPointer) {
	uint8_t buffer[4] = { 0 };
	uint32_t addressPointer = 0;
	buffer[0] = (addressPointer >> 24); buffer[1] = (addressPointer >> 16); buffer[2] = (addressPointer >> 8); buffer[3] = addressPointer;
	eepromWriteMemory(metaAddressOfAddressPointer, buffer, 4);
}

void readVoltage1606Test() {
	initSecondUARTwith9600(); // + some uA
	println("Hello World this is TickTag..");
	
	while(1) {
		uint16_t voltage = deviceReadSupplyVoltage();
		printf("V: %u mV\n\r", voltage);
		_delay_ms(2000);	
	}
	
	while(1) { ; }
}

void testMessage() {
	initSecondUARTwith9600(); // + some uA
	println("Hello World this is TickTag..");
	println("Hello World this is TickTag..");
	println("Hello World this is TickTag..");
	println("Hello World this is TickTag..");
	uartOff();
	deinitSecondUartPins();
	initSecondUARTwith9600(); // + some uA
	println("Hey there!");
	println("Hey there!");
	println("Hey there!");
	uartOff();
	deinitSecondUartPins();
}

void testWriteMemory() {
	printf("I2C writing\n\r");
	i2cStartWrite(0x50);
	i2cWrite(0x00);
	i2cWrite(0x00);
	i2cWrite(0xAA);
	i2cWrite(0xBB);
	i2cStop();
	_delay_ms(100);
}

void testMemory() {
	uint8_t retVal = 0;
	//testWriteMemory();
	
	printf("I2C starting\n\r");
	
	i2cStartWrite(0x50);
	i2cWrite(0x00);
	i2cWrite(0x00);
	
	i2cStartRead(0x50);
	retVal = i2cRead(1);
	printf("I2C read, res: %02X\n\r", retVal);
	retVal = i2cRead(0);
	printf("I2C read, res: %02X\n\r", retVal);
	i2cStop();
	printf("I2C stopped\n\r");
}

void i2cTest() {
	deviceInitTimer();
	devicePowerMemoryAndI2COn();
	i2cInit();
	
	printf("%lu Test EEPROM\n\r", deviceGetMillis());
	if(!i2cStartRead(EEPROM_CAT24M01_ADDRESS)) {
		printf("%lu fail\n\r", deviceGetMillis());
	}
	else {
		printf("%lu success\n\r", deviceGetMillis());
	}
	i2cStop();
	
	printf("%lu Test phantom device\n\r", deviceGetMillis());
	if(!i2cStartRead(0x42)) {
		printf("%lu fail\n\r", deviceGetMillis());
	}
	else {
		printf("%lu success\n\r", deviceGetMillis());
	}
	i2cStop();
	
	_delay_ms(100);
	printf("%lu millis\n\r", deviceGetMillis());
	
	deviceInitPins();
	deviceInitDownloadPinInterruptInDeepSleep();
	
	deviceDeepSleep();
	_delay_ms(100);
	deviceLedOn();
	printf("HELLO!\n\r");
	while(1) { ; }
}

void i2cTest2() {
	devicePowerMemoryAndI2COn();
	i2cInit();
	
	const uint16_t LEN = 512;
	uint8_t data[LEN] = { 0 };
	
	eepromReadMemory(0, data, LEN);
	for(uint16_t i=0; i<LEN; i++) {
		printf("%02X ", data[i]);
	}
	printf("\n\rDONE\n\r");
	
	for(uint16_t i=0; i<LEN; i++) {	data[i] = i+1; }
	if(!eepromWriteMemory(0, data, LEN)) {
		printf("ERROR\n\r");
	}
	
	eepromReadMemory(0, data, LEN);
	for(uint16_t i=0; i<LEN; i++) {
		printf("%02X ", data[i]);
	}
	printf("\n\r\n\r");
	
	_delay_ms(500);
	printf("DONE\n\r");
}

bool uartReadMultipleLinesNoFlush(uint8_t lines, char breakChar, char *line, uint8_t maxLength, uint16_t timeoutMs) {
	uint8_t index = 0;
	char c;
	if(lines == 0) { return false; }
	memset(line, 0, maxLength);
	//uartFlush();
	while(1) {
		while(!(USART0.STATUS & USART_RXCIF_bm)) {
			timeoutMs--;
			_delay_ms(1); // WARNING!!! only works for 9600 baud (1.0416ms/byte)
			if(timeoutMs == 0) { return false; }
		}
		c = (char) (USART0.RXDATAL);
		//if(c != '\n' && c != '\r') { // do not add \n and \r
		line[index++] = c;
		if(index > maxLength) {
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

void lightSleepUARTTest() {
	deviceInitTimer(); // for getting current system time (for ttf measurement)
	gps_t gpsData = { 0 };
	
	deviceL70powerOn();
	deviceL70backupOn(); // just in case it has been turned off during night time
	
	char messageBuffer[L70_START_BUFFER_SIZE];
	char *startMessage;
	initPrimaryUARTwith9600(); // init uart to communicate with L70
	
	if(!gpsConfigureBeforeStart(messageBuffer)) {
		printf("ERROR\n\r");
		while(1) { ; }
	}
	while(true) {
		// REQUIRES: ISR(USART0_RXC_vect) { }
		USART0.CTRLB |= (USART_SFDEN_bm | USART_TXEN_bm | USART_RXEN_bm);
		USART0.CTRLA |= USART_RXSIE_bm;
		//uartFlush();
		deviceStandbySleep();
		initPrimaryUARTwith9600();
		//USART0.STATUS |= USART_RXSIF_bm;
		//deviceLedOn();
		
		//deviceSetCPUSpeed(OSC16_PRESCALER_CPU_0_25MHZ); // requres _delay_ms to be changed (half of that)
		//USART0.BAUD = 104;
		
		// wait on message from L70 (max. 2 * 80 Byte*(8+2) * (0.1042ms/bit) = 166.6ms)
		if(!uartReadMultipleLinesNoFlush(2, '\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { // flush and memset done in function
			break;
		}
		
		//deviceSetCPUSpeed(OSC16_PRESCALER_CPU_1MHZ);
		//USART0.BAUD = (uint16_t) USART0_BAUD_RATE(9600); // calculate BAUD rate (depending on F_CPU)
			
		// switch to debug uart
		#if (DEBUGGING == true)
			initSecondUARTwith9600(); // uart to communicate with external programmer
			//strcpy(command, "$GPRMC,144326.00,A,5107.0017,N,11402.3291,W,0.080,323.3,210307,0.0,E,A*25"); // TEST
		#endif
			
		// decode received message 1
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 6)) {
			if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'R')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); }
			if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'G') && (startMessage[4] == 'G')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); }
			printf("%lu: %s\n\r", deviceGetMillis(), startMessage); // print full string
		}
			
		// decode received message 2
		if(strlen(startMessage) > 1) {
			startMessage = strchr(startMessage+1, '$'); // search for next $ after first $
			if((startMessage != NULL) && (strlen(startMessage) > 6)) {
				if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'R')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); }
				if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'G') && (startMessage[4] == 'G')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); }
			}
		}
		
			
		// check if fix found and store if yes
		if(gpsData.fix == 1) {
			// got a fix
			if((gpsData.hdop > 0) && (gpsData.hdop < 40)) { // only store if hdop is good enough
				printf("FIX\n\r");
			}
		}
		
		// check interrupt pin status
		if(~PORTA.IN & PIN5_bm) { // PATH TESTED, download button pressed
			_delay_ms(200); // wait a bit and check again
			if(~PORTA.IN & PIN5_bm) { break; }
		}
			
		// re-init L70 uart
		initPrimaryUARTwith9600();
			
		// shortly blink led
		deviceLedOff();
	}
	printf("FINISHED\n\r");
	deviceL70powerOff();
	uartFlush();
	uartOff();
	deinitSecondUartPins(); // otherwise leakage

	while(1) { ; }
}

void newParserTest() {
	deviceInitTimer(); // for getting current system time (for ttf measurement)
	gps_t gpsData = { 0 };
	
	deviceL70powerOn();
	deviceL70backupOn(); // just in case it has been turned off during night time
	
	char messageBuffer[L70_START_BUFFER_SIZE];
	char *startMessage;
	initPrimaryUARTwith9600(); // init uart to communicate with L70
	
	if(!gpsConfigureBeforeStart(messageBuffer)) {
		printf("ERROR\n\r");
		while(1) { ; }
	}
	printf("$PQFLP,W,1,0*21\r\n"); // FLP
	
	while(true) {
		// wait on message from L70 (max. 2 * 80 Byte*(8+2) * (0.1042ms/bit) = 166.6ms)
		if(!uartReadMultipleLines(2, '\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { // flush and memset done in function
			break;
		}
		initSecondUARTwith9600(); // uart to communicate with external programmer
		
		// decode received message 1
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 6)) {
			// no need to check if startMessage[0] == '$' -> strchr would return NULL otherwise
			if((startMessage[3] == 'R') && (startMessage[4] == 'M') && (startMessage[5] == 'C')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); }
			if((startMessage[3] == 'G') && (startMessage[4] == 'G') && (startMessage[5] == 'A')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); }

			printf("%lu: %s\n\r", deviceGetMillis(), startMessage); // print full string
		}
		
		// decode received message 2
		if(strlen(startMessage) > 1) {
			startMessage = strchr(startMessage+1, '$'); // search for next $ after first $
			if((startMessage != NULL) && (strlen(startMessage) > 6)) {
				if((startMessage[3] == 'R') && (startMessage[4] == 'M') && (startMessage[5] == 'C')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); }
				if((startMessage[3] == 'G') && (startMessage[4] == 'G') && (startMessage[5] == 'A')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); }
			}
		}
		printf("------\n\r");
		_delay_ms(10);
		
		// check if fix found and store if yes
		if(gpsData.fix == 1) {
			// got a fix
			if((gpsData.hdop > 0) && (gpsData.hdop < 40)) { // only store if hdop is good enough
				printf("FIX\n\r");
			}
		}
		
		// check interrupt pin status
		if(~PORTA.IN & PIN5_bm) { // PATH TESTED, download button pressed
			_delay_ms(200); // wait a bit and check again
			if(~PORTA.IN & PIN5_bm) { break; }
		}
		
		// re-init L70 uart
		initPrimaryUARTwith9600();
		
		// shortly blink led
		deviceLedOff();
	}
	printf("FINISHED\n\r");
	deviceL70powerOff();
	uartFlush();
	uartOff();
	deinitSecondUartPins(); // otherwise leakage

	while(1) { ; }	
}


void flpTest() {
	//deviceInitTimer(); // for getting current system time (for ttf measurement)
	gps_t gpsData = { 0 };
	
	deviceL70powerOn();
	deviceL70backupOn(); // just in case it has been turned off during night time
	
	char messageBuffer[L70_START_BUFFER_SIZE];
	char *startMessage;
	initPrimaryUARTwith9600(); // init uart to communicate with L70
	
	if(!gpsConfigureBeforeStart(messageBuffer)) {
		printf("ERROR1\n\r");
		while(1) { ; }
	}
	
	deviceDeepSleep();
	deviceLedOn();
	
	// FLP
	bool flpError = false;
	printf("$PQFLP,W,1,0*21\r\n"); // FLP
	while(1) {
		if(!uartRead('\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { flpError = true; break; } // wait on answer for setting, return false if no message received after 1200ms
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 8)) {
			if((startMessage[0] == '$') && (startMessage[1] == 'P') && (startMessage[2] == 'Q') && (startMessage[3] == 'F') && (startMessage[4] == 'L') && (startMessage[5] == 'P') && (startMessage[6] == ',') && (startMessage[7] == 'W')) {
				break;
			}
			else {
				flpError = true;
				break;
			}
		}
	}
	if(flpError) {
		printf("ERROR2\n\r");
		while(1) { ; }		
	}
	
	while(true) {	
		//deviceSetCPUSpeed(OSC16_PRESCALER_CPU_0_25MHZ); // requres _delay_ms to be changed (half of that)
		//USART0.BAUD = 104;
		
		// wait on message from L70 (max. 2 * 80 Byte*(8+2) * (0.1042ms/bit) = 166.6ms)
		if(!uartReadMultipleLines(2, '\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { // flush and memset done in function
			break;
		}
		
		//deviceSetCPUSpeed(OSC16_PRESCALER_CPU_1MHZ);

		// switch to debug uart
		initSecondUARTwith9600(); // uart to communicate with external programmer
		
		// decode received message 1
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 6)) {
			if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'R')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); }
			if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'G') && (startMessage[4] == 'G')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); }
			printf("%lu: %s\n\r", deviceGetMillis(), startMessage); // print full string
		}
		else {
			printf("NOK %lu: %s(%d)\n\r", deviceGetMillis(), startMessage, strlen(startMessage)); // print full string
		}
		
		// decode received message 2
		if(strlen(startMessage) > 1) {
			startMessage = strchr(startMessage+1, '$'); // search for next $ after first $
			if((startMessage != NULL) && (strlen(startMessage) > 6)) {
				if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'R')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); }
				if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'G') && (startMessage[4] == 'G')) { gpsDecodeGPRMCandGPGGA(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); }
			}
		}
		
		// check if fix found and store if yes
		if(gpsData.fix == 1) {
			// got a fix
			if((gpsData.hdop > 0) && (gpsData.hdop < 40)) { // only store if hdop is good enough
				printf("FIX\n\r");
				//deviceLedOn();
			}
		}
		//_delay_ms(50);
		
		// check interrupt pin status
		if(~PORTA.IN & PIN5_bm) { // PATH TESTED, download button pressed
			_delay_ms(200); // wait a bit and check again
			if(~PORTA.IN & PIN5_bm) { break; }
		}
		
		// re-init L70 uart
		initPrimaryUARTwith9600();
	}
	printf("FINISHED\n\r");
	deviceL70powerOff();
	uartFlush();
	uartOff();
	deinitSecondUartPins(); // otherwise leakage

	while(1) { ; }
}