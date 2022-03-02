// F_CPU in project settings
#include "TickTag.h"
#include "EEPROM_CAT24M01.h"
#include "GPS_L70_Light.h"
#include "I2C.h"
#include "UART.h"
#include "HelperFunctions.h"
#include "TestFunctions.h"
#include "Configuration.h"

// execute following avrdude commands before flashing: cd [Github Folder]\TickTag\TickTagSoftware\avrdude\bin

// ATTINY1616
// no BOD in sleep, sampled:		avrdude -c jtag2updi -P com7 -p t1616 -C ..\etc\avrdude.conf -U fuse1:w:0b00011000:m		(000 = BODLEVEL 1.8V, 1 = BOD SAMP FREQ 125HZ, 10 = BOD SAMPLED, 00 = BOD DISABLED IN SLEEP)
// run @16MHz:						avrdude -c jtag2updi -P com7 -p t1616 -C ..\etc\avrdude.conf -U fuse2:w:0b00000001:m		(0 = OSC REG ACCESS, XXXXX, 01 = RUN AT 16MHZ)
// keep EEPROM after flashing:		avrdude -c jtag2updi -P com7 -p t1616 -C ..\etc\avrdude.conf -U fuse5:w:0b11110111:m
// only 8ms start delay:			avrdude -c jtag2updi -P com7 -p t1616 -C ..\etc\avrdude.conf -U fuse6:w:0b00000100:m		(100 = 8ms)

// ATTINY1626
// no BOD in sleep, sampled:		avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse1:w:0b00011000:m		(000 = BODLEVEL 1.8V, 1 = BOD SAMP FREQ 125HZ, 10 = BOD SAMPLED, 00 = BOD DISABLED IN SLEEP)
// run @16MHz:						avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse2:w:0b00000001:m		(0 = OSC REG ACCESS, XXXXX, 01 = RUN AT 16MHZ)
// keep EEPROM after flashing:		avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse5:w:0b11110111:m
// only 8ms start delay:			avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse6:w:0b00000100:m		(100 = 8ms)

// WARNING: uartRead() and uartReadMultipleLines() has 1ms delay -> only works for 9600 baud, very very close
// WARNING: uartRead and uartReadMultipleLine: changed "if(index > maxLength)" to "if(index >= maxLength)"
// NOTE: 1Hz mode (frequency = 1 .. 5) -> does not stop when eeprom minimum voltage from configuration reached, but when reaching MIN_VOLTAGE_UNDER_LOAD (or time is over) -> might fully empty a battery then
// NOTE: if SENDING jumper set = RXD2 (powers ATTINY alone via UART bridge, without battery connected) -> any current flow in direction of lipo? -> NO
// NOTE: drift test: ~4 seconds per 15min (calculated: = ~16 seconds per hour = ~6min per day), measured again: 6.7 seconds per 30min
// NOTE: drift test 2: OFF at 9:30 (7:30 UTC), ON at 23:59 (21:59 UTC) -> woke up at 23:50:40 (drift: 8min 20s in 14 hours)
// NOTE: flashing new software requires removal of lipo (after flashing: 1.4mA power consumption until hard reset) -> NO, FIXED with new jtag2updi programming software

/** Global objects and variable */
uint8_t state = ST_FIRST_START_HARD_RESET;
uint16_t batteryVoltage;
uint8_t timeoutCounter = 0;
uint8_t timeoutNotEvenTimeCounter = 0;
uint16_t undervoltageUnderLoadCounter = 0;
uint16_t errorCounter = 0;
uint16_t firstTTFinSession = 0;
bool noFixInSessionYet;
bool blinkDuringFirstFix = true;
uint16_t eepromSettingMinVoltage = OPERATION_VOLTAGE_DEFAULT;
uint16_t eepromSettingGPSFrequencySeconds = GPS_FREQUENCY_SECONDS_DEFAULT;
uint8_t eepromSettingMinHDOP = MIN_HDOP_DEFAULT;
uint16_t eepromSettingActivationDelaySeconds = ACTIVATION_DELAY_DEFAULT;
uint8_t eepromSettingTurnOnHour;
uint8_t eepromSettingTurnOnMinute;
uint8_t eepromSettingTurnOffHour;
uint8_t eepromSettingTurnOffMinute;
uint8_t eepromSettingGeofencing;
uint8_t eepromSettingBlink;
uint8_t eepromSettingBurstDuration;
volatile bool downloadInterrupt; // volatile because used in ISR
volatile bool gpio2Interrupt; // volatile because used in ISR
volatile bool rtcInterrupt; // volatile because used in ISR
uint32_t timestampEstimation;
bool activated;
int32_t geofenceLat;
int32_t geofenceLon;

/** Interrupt functions */

ISR(PORTA_PORT_vect) { downloadInterrupt = true; PORTA.INTFLAGS |= PORT_INT5_bm; } // clear interrupt pin flag, must do this to handle next interrupt 
ISR(RTC_CNT_vect) { rtcInterrupt = true; RTC.INTFLAGS = RTC_OVF_bm; } // for standby sleep
ISR(TCA0_OVF_vect) { deviceIncrementTimer(); TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; }  // Clear the interrupt flag (to reset TCA0.CNT)
ISR(PORTC_PORT_vect) { gpio2Interrupt = true; PORTC.INTFLAGS |= PORT_INT1_bm; } // for activation by GPIO2

/** Various helper functions */

void wait(uint16_t seconds) {
	deviceInitInternalRTCInterrupt(seconds);
	deviceStandbySleep();
	RTC.CTRLA = 0; // disable RTC interrupt
}

bool voltageNotOkay() {
	return (batteryVoltage < eepromSettingMinVoltage) || (batteryVoltage > OPERATION_VOLTAGE_MAX_RANGE);
}

/** Night time functions */

bool isNightTime() {
	const uint16_t OFF_MINUTES_OF_DAY = (eepromSettingTurnOffHour * 60) + eepromSettingTurnOffMinute; // 0 ........ 1439
	const uint16_t ON_MINUTES_OF_DAY = (eepromSettingTurnOnHour * 60) + eepromSettingTurnOnMinute; // 0 ........ 1439
	uint32_t minutesOfDay = (timestampEstimation / 60UL) % 1440UL;
	if((eepromSettingTurnOnHour == 0) && (eepromSettingTurnOnMinute == 0) && (eepromSettingTurnOffHour == 23) && (eepromSettingTurnOffMinute == 59)) { return false; } // always on
	if(timestampEstimation == 0) { return false; } // no valid time - can't judge
	
	if(OFF_MINUTES_OF_DAY < ON_MINUTES_OF_DAY) { // ----OFF___ON-------- (tag on over midnight)
		if((minutesOfDay >= OFF_MINUTES_OF_DAY) && (minutesOfDay < ON_MINUTES_OF_DAY)) { // "<" important because interrupt happens at exact minute (e.g. 14:15) -> would go to sleep again
			return true;
		}
	}
	else { // ___ON----------OFF__
		if((minutesOfDay >= OFF_MINUTES_OF_DAY) || (minutesOfDay < ON_MINUTES_OF_DAY)) { // "<" important because interrupt happens at exact minute (e.g. 14:15) -> would go to sleep again
			return true;
		}
	}
	return false;
}

/** Internal EEPROM functions */

uint16_t getOperationVoltageMinFromTinyEEPROM() {
	uint16_t val = (((uint16_t) TinyEEPROM(EEPROM_OPERATION_VOLTAGE_MIN_ADDRESS)) << 8) | (((uint16_t) TinyEEPROM(EEPROM_OPERATION_VOLTAGE_MIN_ADDRESS + 1)));
	if((val < OPERATION_VOLTAGE_MIN_RANGE) || (val > OPERATION_VOLTAGE_MAX_RANGE)) { return OPERATION_VOLTAGE_DEFAULT; }
	return val;
}

uint16_t getGPSFrequencyFromTinyEEPROM() {
	uint16_t val = (((uint16_t) TinyEEPROM(EEPROM_GPS_FREQUENCY_SECONDS_ADDRESS)) << 8) | (((uint16_t) TinyEEPROM(EEPROM_GPS_FREQUENCY_SECONDS_ADDRESS + 1)));
	if((val < GPS_MIN_FREQUENCY_SECONDS_RANGE) || (val > GPS_MAX_FREQUENCY_SECONDS_RANGE)) { return GPS_FREQUENCY_SECONDS_DEFAULT; }
	return val;
}

uint8_t getMinHDOPFromTinyEEPROM() {
	uint8_t val = (uint8_t) TinyEEPROM(EEPROM_MIN_HDOP_ADDRESS);
	if((val < MIN_HDOP_MIN_RANGE) || (val > MIN_HDOP_MAX_RANGE)) { return MIN_HDOP_DEFAULT; }
	return val;
}

uint16_t getActivationDelayFromTinyEEPROM() {
	uint16_t val = (((uint16_t) TinyEEPROM(EEPROM_ACTIVATION_DELAY_SECONDS_ADDRESS)) << 8) | (((uint16_t) TinyEEPROM(EEPROM_ACTIVATION_DELAY_SECONDS_ADDRESS + 1)));
	if((val < ACTIVATION_MIN_DELAY_SECONDS_RANGE) || (val > ACTIVATION_MAX_DELAY_SECONDS_RANGE)) { return ACTIVATION_DELAY_DEFAULT; }
	return val;	
}

uint8_t getTurnOnHourFromTinyEEPROM() {
	uint8_t val = (uint8_t) TinyEEPROM(EEPROM_HOUR_ON_ADDRESS);
	if(val > 23) { return HOUR_ON_DEFAULT; }
	return val;
}

uint8_t getTurnOnMinuteFromTinyEEPROM() {
	uint8_t val = (uint8_t) TinyEEPROM(EEPROM_MINUTE_ON_ADDRESS);
	if(val > 59) { return MINUTE_ON_DEFAULT; }
	return val;
}

uint8_t getTurnOffHourFromTinyEEPROM() {
	uint8_t val = (uint8_t) TinyEEPROM(EEPROM_HOUR_OFF_ADDRESS);
	if(val > 23) { return HOUR_OFF_DEFAULT; }
	return val;
}

uint8_t getTurnOffMinuteFromTinyEEPROM() {
	uint8_t val = (uint8_t) TinyEEPROM(EEPROM_MINUTE_OFF_ADDRESS);
	if(val > 59) { return MINUTE_OFF_DEFAULT; }
	return val;
}

/** Reading data functions */

bool readMemory() {
	uint8_t buffer[DATA_FIX_LENGTH];
	uint32_t cnt, addressPointer, addressIterator, timestamp, ttfSum, hdopSum, temp;
	int32_t lat, lon;
	
	// meta data output from RAM
	printf("UVs: %u, TOs: %u/%u, ErrorsOrGF: %u, TTFF: %u\n\r", undervoltageUnderLoadCounter, timeoutNotEvenTimeCounter, timeoutCounter, errorCounter, firstTTFinSession);
	
	// read address pointer
	if(!eepromReadMemory(METADATA_ADDRESS_OF_ADDRESS_POINTER, buffer, 4)) { return false; } // read current memory position
	addressPointer = arrayToUint32(buffer);
	if((addressPointer == 0xFFFFFFFF) || (addressPointer > EEPROM_CAT24M01_MEM_SIZE)) { // assuming memory is empty or other issue
		return false;
	}

	// calculate stored fixes
	uint32_t storedFixes = addressPointer - METADATA_PREFIX_LENGTH_IN_MEMORY;
	storedFixes = storedFixes / DATA_FIX_LENGTH;
	
	// read ttf sum
	if(!eepromReadMemory(METADATA_ADDRESS_OF_TTF_SUM, buffer, 4)) { return false; } // read current memory position
	ttfSum = arrayToUint32(buffer);
	if(ttfSum == 0xFFFFFFFF) { ttfSum = 0; } // first time ever reading stuff
	if(storedFixes == 0) { ttfSum = 0; }
	else { ttfSum = ttfSum / storedFixes; } // calculate average ttf value
	
	// read hdop sum
	if(!eepromReadMemory(METADATA_ADDRESS_OF_HDOP_SUM, buffer, 4)) { return false; } // read current memory position
	hdopSum = arrayToUint32(buffer);
	if(hdopSum == 0xFFFFFFFF) { hdopSum = 0; } // first time ever reading stuff
	if(storedFixes == 0) { hdopSum = 0; }
	else { hdopSum = hdopSum / storedFixes; } // calculate average ttf value
	
	// output of metadata from EEPROM
	printf("Fixes: %lu, Avg. TTF: %lu s, Avg. HDOP (x10): %lu\n\r", storedFixes, ttfSum, hdopSum);
	
	// read data over uart until address pointer
	addressIterator = METADATA_PREFIX_LENGTH_IN_MEMORY; // start reading data at first data address
	cnt = 1;
	printf("\n\rcount,timestamp,lat,lon\n\r");
	while(1) {
		if(!eepromReadMemory(addressIterator, buffer, DATA_FIX_LENGTH)) { return false; } // read one dataset in memory
		
		// decompress memory
		temp = buffer[0];
		lat = temp << 17;
		temp = buffer[1];
		lat = lat | (temp << 9);
		temp = buffer[2];
		lat = lat | (temp << 1);
		temp = buffer[3];
		lat = lat | ((temp >> 7) & 0xFF);
		lat -= 9000000L;
		
		lon = (temp & 0b01111111) << 19;
		temp = buffer[4];
		lon = lon | (temp << 11);
		temp = buffer[5];
		lon = lon | (temp << 3);
		temp = buffer[6];
		lon = lon | ((temp >> 5) & 0xFF);
		lon -= 18000000L;
		
		timestamp = (temp & 0b00011111) << 24;
		temp = buffer[7];
		timestamp = timestamp | (temp << 16);
		temp = buffer[8];
		timestamp = timestamp | (temp << 8);
		temp = buffer[9];
		timestamp = timestamp | ((temp) & 0xFF);
		timestamp += COMPRESSION_TIMESTAMP;		
		
		printf("%lu,%lu,%ld.%05ld,%ld.%05ld\n\r", cnt, timestamp, lat / 100000L, labs(lat) % 100000L, lon / 100000L, labs(lon) % 100000L);
		
		addressIterator += DATA_FIX_LENGTH;
		if((addressIterator + DATA_FIX_LENGTH) > addressPointer) { break; } // addressPointer = 131072, addressIterator = 131068 -> would read next 12 byte in iteration
		cnt++;
		
		if((cnt > 200) && (~PORTA.IN & PIN5_bm)) { return true; } // interrupt possible with download button after 200 entries being read out
	}
	return true;
}

void readMemoryFancy() {
	printf("*START MEMORY*\n\r");
	if(!readMemory()) { printf("EMPTY\n\r"); }
	printf("*END MEMORY*\n\r");	
}

void startMenue() {
	const uint8_t LEN = 32;
	char command[LEN];
		
	initSecondUARTwith9600(); // + some uA
	deviceBlink(5);
	devicePowerMemoryAndI2COn();
	i2cInit();
	blinkDuringFirstFix = true; // blink again when menue was shown

	printf("---TICK-TAG---\n\r");
	readMemoryFancy();
	while(1) {
		//printf("ID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n\r", SIGROW.SERNUM0, SIGROW.SERNUM1, SIGROW.SERNUM2, SIGROW.SERNUM3, SIGROW.SERNUM4, SIGROW.SERNUM5, SIGROW.SERNUM6, SIGROW.SERNUM7, SIGROW.SERNUM8, SIGROW.SERNUM9);
		printf("V%u, ID: %02X-%02X%02X, %umV, %02Xb\n\r", SOFTWARE_VERSION, SIGROW.SERNUM0, SIGROW.SERNUM8, SIGROW.SERNUM9, batteryVoltage, eepromSettingBlink); // saves a lot of flash memory
		printf("SETTINGS:\n\r- Min voltage: %u mV\n\r- Frequency: %u s\n\r- Min HDOP (x10): %u\n\r- Activation delay: %u s\n\r- Geofencing: %u\n\r- Burst duration: %u s\n\r", eepromSettingMinVoltage, eepromSettingGPSFrequencySeconds, eepromSettingMinHDOP, eepromSettingActivationDelaySeconds, eepromSettingGeofencing, eepromSettingBurstDuration);
		if((eepromSettingTurnOnHour == 0) && (eepromSettingTurnOnMinute == 0) && (eepromSettingTurnOffHour == 23) && (eepromSettingTurnOffMinute == 59)) { 
			printf("- No time\n\r");
		}
		else {
			printf("- Time: %u:%02u - %u:%02u UTC\n\r", eepromSettingTurnOnHour, eepromSettingTurnOnMinute, eepromSettingTurnOffHour, eepromSettingTurnOffMinute);
		}
		printf("\n\r0 Read memory\n\r1 Reset memory\n\r2 Set min. voltage\n\r");
		printf("3 Set frequency\n\r4 Set accuracy\n\r5 Set activation delay\n\r");
		printf("6 Set times\n\r7 Toggle geofencing ON/OFF\n\r8 Set burst duration\n\r9 Exit\n\r");
		if(!uartRead('\r', command, LEN, 40000)) {
			printf("Bye! Sleeping until activation (by pressing button again)\n\r");
			break;
		}
		if(command[0] == '0') {
			readMemoryFancy();
		}
		else if(command[0] == '1') {
			uint8_t buffer[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
			printf("... ");
			if(!eepromWriteMemory(METADATA_ADDRESS_OF_ADDRESS_POINTER, buffer, 4)) { printf("ERROR\n\r"); } // reset address pointer
			if(!eepromWriteMemory(METADATA_ADDRESS_OF_TTF_SUM, buffer, 4)) { printf("ERROR\n\r"); } // reset ttf sum
			if(!eepromWriteMemory(METADATA_ADDRESS_OF_HDOP_SUM, buffer, 4)) { printf("ERROR\n\r"); } // reset hdop sum
			timeoutCounter = 0;
			timeoutNotEvenTimeCounter = 0;
			undervoltageUnderLoadCounter = 0;
			errorCounter = 0;
			firstTTFinSession = 0;
			printf("DONE\n\r");
		}
		else if(command[0] == '2') {
			printf("Enter mV (%u..%u):\n\r", OPERATION_VOLTAGE_MIN_RANGE, OPERATION_VOLTAGE_MAX_RANGE);
			//printf("-------------------\n\r");
			if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
			int16_t val = atoi(command);
			uint16_t valUnsigned = val;
			if((valUnsigned < OPERATION_VOLTAGE_MIN_RANGE) || (valUnsigned > OPERATION_VOLTAGE_MAX_RANGE)) {
				printf("ERROR\n\r");
			}
			else {
				// write EEPROM settings
				(TinyEEPROM(EEPROM_OPERATION_VOLTAGE_MIN_ADDRESS)) = (uint8_t) (valUnsigned >> 8);
				(TinyEEPROM(EEPROM_OPERATION_VOLTAGE_MIN_ADDRESS + 1)) = (uint8_t) (valUnsigned);
				
				// read EEPROM settings to check
				#if DOUBLE_CHECK_EEPROM_AFTER_SETTING == true
				_delay_ms(100);
				uint16_t valCheck = getOperationVoltageMinFromTinyEEPROM();
				
				if(valCheck != valUnsigned) { printf("ERROR\n\r"); }
				else {
					printf("OK\n\r");
					eepromSettingMinVoltage = valCheck; // update global variable
				}
				#else
				printf("OK\n\r");
				eepromSettingMinVoltage = valUnsigned; // update global variable
				#endif
			}
		}
		else if(command[0] == '3') {
			printf("Enter GPS frequency in s (%u..%u, 1..5 = stay-on):\n\r", GPS_MIN_FREQUENCY_SECONDS_RANGE, GPS_MAX_FREQUENCY_SECONDS_RANGE);
			//printf("-------------------\n\r");
			if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
			int16_t val = atoi(command);
			uint16_t valUnsigned = val;
			if((valUnsigned < GPS_MIN_FREQUENCY_SECONDS_RANGE) || (valUnsigned > GPS_MAX_FREQUENCY_SECONDS_RANGE)) {
				printf("ERROR\n\r");
			}
			else {
				// write EEPROM settings
				(TinyEEPROM(EEPROM_GPS_FREQUENCY_SECONDS_ADDRESS)) = (uint8_t) (valUnsigned >> 8);
				(TinyEEPROM(EEPROM_GPS_FREQUENCY_SECONDS_ADDRESS + 1)) = (uint8_t) (valUnsigned);
				
				// read EEPROM settings to check
				#if DOUBLE_CHECK_EEPROM_AFTER_SETTING == true
				_delay_ms(100);
				uint16_t valCheck = getGPSFrequencyFromTinyEEPROM();
				
				if(valCheck != valUnsigned) { printf("ERROR\n\r"); }
				else {
					printf("OK\n\r");
					eepromSettingGPSFrequencySeconds = valCheck; // update global variable
				}
				#else
				printf("OK\n\r");
				eepromSettingGPSFrequencySeconds = valUnsigned; // update global variable
				#endif
			}
		}
		else if(command[0] == '4') {
			printf("Enter min HDOP x 10 (%u..%u):\n\r", MIN_HDOP_MIN_RANGE, MIN_HDOP_MAX_RANGE);
			printf("TT tries to achieve HDOP within %u s\n\r", MAX_WAIT_ON_GOOD_HDOP_SECONDS);
			//printf("-------------------\n\r");
			if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
			int16_t val = atoi(command);
			uint16_t valUnsigned = val;
			if((valUnsigned < MIN_HDOP_MIN_RANGE) || (valUnsigned > MIN_HDOP_MAX_RANGE)) {
				printf("ERROR\n\r");
			}
			else {
				// write EEPROM settings
				(TinyEEPROM(EEPROM_MIN_HDOP_ADDRESS)) = (uint8_t) (valUnsigned);
				
				// read EEPROM settings to check
				#if DOUBLE_CHECK_EEPROM_AFTER_SETTING == true
				_delay_ms(100);
				uint8_t valCheck = getMinHDOPFromTinyEEPROM();
				
				if(valCheck != valUnsigned) { printf("ERROR\n\r"); }
				else {
					printf("OK\n\r");
					eepromSettingMinHDOP = valCheck; // update global variable
				}
				#else
				printf("OK\n\r");
				eepromSettingMinHDOP = valUnsigned; // update global variable
				#endif
			}
		}
		else if(command[0] == '5') {
			printf("Enter activation delay in s (%u..%u):\n\r", ACTIVATION_MIN_DELAY_SECONDS_RANGE, ACTIVATION_MAX_DELAY_SECONDS_RANGE);
			//printf("-------------------\n\r");
			if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
			int16_t val = atoi(command);
			uint16_t valUnsigned = val;
			if((valUnsigned < ACTIVATION_MIN_DELAY_SECONDS_RANGE) || (valUnsigned > ACTIVATION_MAX_DELAY_SECONDS_RANGE)) {
				printf("ERROR\n\r");
			}
			else {
				// write EEPROM settings
				(TinyEEPROM(EEPROM_ACTIVATION_DELAY_SECONDS_ADDRESS)) = (uint8_t) (valUnsigned >> 8);
				(TinyEEPROM(EEPROM_ACTIVATION_DELAY_SECONDS_ADDRESS + 1)) = (uint8_t) (valUnsigned);
				
				// read EEPROM settings to check
				#if DOUBLE_CHECK_EEPROM_AFTER_SETTING == true
				_delay_ms(100);
				uint16_t valCheck = getActivationDelayFromTinyEEPROM();
				
				if(valCheck != valUnsigned) { printf("ERROR\n\r"); }
				else {
					printf("OK\n\r");
					eepromSettingActivationDelaySeconds = valCheck; // update global variable
				}
				#else
				printf("OK\n\r");
				eepromSettingActivationDelaySeconds = valUnsigned; // update global variable
				#endif
			}
		}
		else if(command[0] == '6') {
			printf("Enter UTC time to turn ON (format: HHMM):\n\r");
			if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
			int16_t val = atoi(command);
			uint16_t valUnsignedOn = val;
			uint8_t hourOn = valUnsignedOn / 100;
			uint8_t minuteOn = valUnsignedOn % 100;
			if((hourOn > 23) || (minuteOn > 59)) {
				printf("ERROR\n\r");
			}
			else {
				// no valcheck to save flash memory
				printf("Enter UTC time to turn OFF (format: HHMM):\n\r");
				if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
				val = atoi(command);
				uint16_t valUnsignedOff = val;
				uint8_t hourOff = valUnsignedOff / 100;
				uint8_t minuteOff = valUnsignedOff % 100;
				if((hourOff > 23) || (minuteOff > 59) || (valUnsignedOff == valUnsignedOn)) {
					printf("ERROR\n\r");
				}
				else {
					// write EEPROM settings
					(TinyEEPROM(EEPROM_HOUR_ON_ADDRESS)) = (uint8_t) (hourOn);
					(TinyEEPROM(EEPROM_MINUTE_ON_ADDRESS)) = (uint8_t) (minuteOn);
					(TinyEEPROM(EEPROM_HOUR_OFF_ADDRESS)) = (uint8_t) (hourOff);
					(TinyEEPROM(EEPROM_MINUTE_OFF_ADDRESS)) = (uint8_t) (minuteOff);
					
					// read EEPROM settings to check
					printf("OK\n\r");
					eepromSettingTurnOnHour = hourOn; // update global variable
					eepromSettingTurnOnMinute = minuteOn; // update global variable
					eepromSettingTurnOffHour = hourOff; // update global variable
					eepromSettingTurnOffMinute = minuteOff; // update global variable
				}
			}
		}
		else if(command[0] == '7') {
			if(eepromSettingGeofencing > 0) {
				(TinyEEPROM(EEPROM_GEOFENCING_ADDRESS)) = (uint8_t) (0);
				eepromSettingGeofencing = 0;
			}
			else {
				(TinyEEPROM(EEPROM_GEOFENCING_ADDRESS)) = (uint8_t) (1);
				eepromSettingGeofencing = 1;
			}
			//printf("CHANGED: %u\n\r", eepromSettingGeofencing);
		}
		else if(command[0] == '8') {
			printf("Enter duration (0-250 s):\n\r");
			if(!uartRead('\r', command, LEN, 60000)) { printf("TIMEOUT\n\r"); break; }
			int16_t val = atoi(command);
			uint16_t valUnsigned = val;
			if(valUnsigned > 250) {
				printf("ERROR\n\r");
			}
			else {
				// write EEPROM settings
				(TinyEEPROM(EEPROM_BURST_DURATION_ADDRESS)) = (uint8_t) (valUnsigned);
				
				// read EEPROM settings to check
				#if DOUBLE_CHECK_EEPROM_AFTER_SETTING == true
				_delay_ms(100);
				uint8_t valCheck = (uint8_t) TinyEEPROM(EEPROM_BURST_DURATION_ADDRESS);
				
				if(valCheck != valUnsigned) { printf("ERROR\n\r"); }
				else {
					printf("OK\n\r");
					eepromSettingBurstDuration = valCheck; // update global variable
				}
				#else
				printf("OK\n\r");
				eepromSettingBurstDuration = valUnsigned; // update global variable
				#endif
			}
		}
		else if(command[0] == 'b') {
			if(eepromSettingBlink > 0) {
				(TinyEEPROM(EEPROM_BLINK_ADDRESS)) = (uint8_t) (0);
				eepromSettingBlink = 0;
			}
			else {
				(TinyEEPROM(EEPROM_BLINK_ADDRESS)) = (uint8_t) (1);
				eepromSettingBlink = 1;
			}
			//printf("BLINK: %u\n\r", eepromSettingBlink);
		}
		else {
			printf("Bye! Sleeping until activation (by pressing button again)\n\r");
			break;
		}
	}
	devicePowerMemoryAndI2COff();
	_delay_ms(250); // wait until printfs are finished and eeprom is powered off
	uartOff();
	deinitSecondUartPins();
}

bool downloadButtonPressedFor3Seconds() {
	uint8_t cntStart = 30; // 30 = 3 seconds
	while(cntStart > 0) { // check if pin is pulled low long enough
		if(PORTA.IN & PIN5_bm) { return false; } // not long enough pulled low
		_delay_ms(100);
		cntStart--;
	}
	return true;
}

/** Memory storage functions */

uint8_t storeInMemory(gps_t *gpsData, bool writeTtfSum) {
	uint8_t buffer[DATA_FIX_LENGTH];
	int32_t tempSigned;
	uint32_t addressPointer, ttfSum = 0, hdopSum = 0, timestamp, temp;
	
	// read address pointer
	if(!eepromReadMemory(METADATA_ADDRESS_OF_ADDRESS_POINTER, buffer, 4)) { return STORE_MEMORY_RETURN_ERROR; } // read current memory position
	addressPointer = arrayToUint32(buffer);
	if((addressPointer == 0xFFFFFFFF) || (addressPointer < METADATA_PREFIX_LENGTH_IN_MEMORY) || (addressPointer > EEPROM_CAT24M01_MEM_SIZE)) { // first time ever writing stuff or error in memory
		addressPointer = METADATA_PREFIX_LENGTH_IN_MEMORY;
	}
	
	// read ttf sum
	if(writeTtfSum) {
		if(!eepromReadMemory(METADATA_ADDRESS_OF_TTF_SUM, buffer, 4)) { return STORE_MEMORY_RETURN_ERROR; } // read current memory position
		ttfSum = arrayToUint32(buffer);
		if(ttfSum == 0xFFFFFFFF) { ttfSum = 0; } // first time ever writing stuff
	}
	
	// read hdop sum
	if(!eepromReadMemory(METADATA_ADDRESS_OF_HDOP_SUM, buffer, 4)) { return STORE_MEMORY_RETURN_ERROR; } // read current memory position
	hdopSum = arrayToUint32(buffer);
	if(hdopSum == 0xFFFFFFFF) { hdopSum = 0; } // first time ever writing stuff

	// fill data (compression to 10 byte)
	timestamp = gpsGetUTCTimestamp(gpsData);
	if(timestamp < COMPRESSION_TIMESTAMP) { timestamp = 0; }
	else { timestamp -= COMPRESSION_TIMESTAMP; }
		
	tempSigned = gpsData->latitude + 9000000L;
	temp = tempSigned; // 25 bit
	buffer[0] = (temp >> 17) & 0xFF;
	buffer[1] = (temp >> 9) & 0xFF;
	buffer[2] = (temp >> 1) & 0xFF;
	buffer[3] = (temp << 7) & 0xFF;
	tempSigned = gpsData->longitude + 18000000L;
	temp = tempSigned; // 26 bit
	buffer[3] = buffer[3] | ((temp >> 19) & 0xFF);
	buffer[4] = (temp >> 11) & 0xFF;
	buffer[5] = (temp >> 3) & 0xFF;
	buffer[6] = (temp << 5) & 0xFF;
	temp = timestamp; // 29 bit
	buffer[6] = buffer[6] | ((temp >> 24) & 0xFF);
	buffer[7] = (temp >> 16) & 0xFF;
	buffer[8] = (temp >> 8) & 0xFF;
	buffer[9] = (temp) & 0xFF;
	
	if(addressPointer + DATA_FIX_LENGTH > EEPROM_CAT24M01_MEM_SIZE) { return STORE_MEMORY_RETURN_MEMORY_FULL; }
	
	// storing data
	if(!eepromWriteMemory(addressPointer, buffer, DATA_FIX_LENGTH)) { return STORE_MEMORY_RETURN_ERROR; }

	// update address pointer
	addressPointer += DATA_FIX_LENGTH;
	buffer[0] = (addressPointer >> 24); buffer[1] = (addressPointer >> 16); buffer[2] = (addressPointer >> 8); buffer[3] = addressPointer;
	if(!eepromWriteMemory(METADATA_ADDRESS_OF_ADDRESS_POINTER, buffer, 4)) { return STORE_MEMORY_RETURN_ERROR; }
		
	// update ttf sum
	if(writeTtfSum) {
		ttfSum += gpsData->ttfSeconds;
		buffer[0] = (ttfSum >> 24); buffer[1] = (ttfSum >> 16); buffer[2] = (ttfSum >> 8); buffer[3] = ttfSum;
		if(!eepromWriteMemory(METADATA_ADDRESS_OF_TTF_SUM, buffer, 4)) { return STORE_MEMORY_RETURN_ERROR; }
	}
	
	// update hdop sum
	hdopSum += gpsData->hdop;
	buffer[0] = (hdopSum >> 24); buffer[1] = (hdopSum >> 16); buffer[2] = (hdopSum >> 8); buffer[3] = hdopSum;
	if(!eepromWriteMemory(METADATA_ADDRESS_OF_HDOP_SUM, buffer, 4)) { return STORE_MEMORY_RETURN_ERROR; }
	
	/*#if DEBUGGING == true
		printf("%lu %lu %lu\n\r", addressPointer, ttfSum, hdopSum);
		readMemory();
	#endif*/
	return STORE_MEMORY_RETURN_SUCCESS;
}

/** Tracking function */

void trackingMerged() {
	deviceInitTimer(); // for getting current system time (for ttf measurement)
	gps_t gpsData = { 0 };
	bool sampleSometimes = (eepromSettingGPSFrequencySeconds > 5); // 1 .. 5: stay on

	devicePowerMemoryAndI2COn();
	i2cInit();
	
	deviceL70powerOn();
	deviceL70backupOn(); // just in case it has been turned off during night time
	
	tracking_result_t trackingResult = TRACKING_RUNNING;
	char messageBuffer[L70_START_BUFFER_SIZE];
	char *startMessage;
	uint8_t msgsReceived;
	uint16_t startTimeWaitAfterFirstFix = 0;
	initPrimaryUARTwith9600(); // init uart to communicate with L70
	uint32_t secondsTtf;
	uint32_t alwaysOnCounter = 0;
	uint8_t burstCounter = 0;
	
	if(!gpsConfigureBeforeStart(messageBuffer)) {
		trackingResult = TRACKING_ERROR; // see below for error handling
	}
	
	#if (USE_FLP_MODE == true)
		if(!sampleSometimes) { // only in continuous mode, because otherwise hot fixes do take way longer
			printf("$PQFLP,W,1,0*21\r\n"); // sends FLP response, but will be ignored
		}
	#endif
	
	while(trackingResult == TRACKING_RUNNING) {
		msgsReceived = 0;
		// wait on message from L70 (max. 2 * 80 Byte*(8+2) * (0.1042ms/bit) = 166.6ms)
		if(!uartReadMultipleLines(2, '\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { // flush and memset done in function
			trackingResult = TRACKING_ERROR; // see below for error handling
			break;
		}
			
		// switch to debug uart
		#if (DEBUGGING == true)
			initSecondUARTwith9600(); // uart to communicate with external programmer
		#endif
			
		// decode received message 1
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 6)) {
			// no need to check if startMessage[0] == '$' -> strchr would return NULL otherwise
			if((startMessage[3] == 'R') && (startMessage[4] == 'M') && (startMessage[5] == 'C')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); msgsReceived++; }
			else if((startMessage[3] == 'G') && (startMessage[4] == 'G') && (startMessage[5] == 'A')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); msgsReceived++; }
			#if (DEBUGGING == true)
				printf("%lu %s\n\r", deviceGetMillis(), startMessage); // print full string
				//printf("%s\n\r", startMessage); // print full string
			#endif
		}
			
		// decode received message 2
		if(strlen(startMessage) > 1) {
			startMessage = strchr(startMessage+1, '$'); // search for next $ after first $
			if((startMessage != NULL) && (strlen(startMessage) > 6)) {
				if((startMessage[3] == 'R') && (startMessage[4] == 'M') && (startMessage[5] == 'C')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPRMC); msgsReceived++; }
				else if((startMessage[3] == 'G') && (startMessage[4] == 'G') && (startMessage[5] == 'A')) { gpsDecodeGPRMCandGPGGANew(startMessage, &gpsData, SENTENCE_TYPE_GPGGA); msgsReceived++; }
			}
		}
		
		// update current ttf in seconds
		secondsTtf = deviceGetMillis() / 1000;
		
		// early timeout: not even got time (all modes)
		if((secondsTtf > GET_FIX_TIMEOUT_NOT_EVEN_TIME_SECONDS) && (gpsData.year == 80) && (gpsData.hour == 0) && (gpsData.minute < ((GET_FIX_TIMEOUT_NOT_EVEN_TIME_SECONDS / 60) + 1))) {
			trackingResult = TRACKING_TIMEOUT;
			timeoutNotEvenTimeCounter++;
			break;
		}
				
		// check if timeout reached (only in sometimes mode, because continuous mode should not stop)
		if(sampleSometimes && (secondsTtf > GET_FIX_TIMEOUT_SECONDS)) { // upper level timeout  
			trackingResult = TRACKING_TIMEOUT;
			timeoutCounter++;
			break;
		}
		
		// update timestamp and check if it is time to go to bed -> only update if both rmc and gga received, otherwise unlikely case that time updated but date not or vice versa
		if((msgsReceived == 2) && (gpsGetUTCTimestamp(&gpsData) != 0)) { timestampEstimation = gpsGetUTCTimestamp(&gpsData); } // update timestampEstimation in case got time
		else { // could not get time (also means no fix)
			// PATH TESTED
			if(timestampEstimation != 0) { timestampEstimation++; } // had time before (previous day) -> add roughly one second, otherwise logger will run forever on second day when not getting time
		}
			
		// check if fix found and store if yes
		if(gpsData.fix == 1) {
			// got a fix
			if(startTimeWaitAfterFirstFix == 0) { startTimeWaitAfterFirstFix = (uint16_t) secondsTtf; } // for SOMETIMES, first time getting a fix right now
			uint8_t minHDOPTimes10Override = eepromSettingMinHDOP;
			uint16_t maxWaitTime = MAX_WAIT_ON_GOOD_HDOP_SECONDS;
			if(sampleSometimes && noFixInSessionYet) { maxWaitTime = WAIT_TIME_FIRST_FIX_SECONDS; minHDOPTimes10Override = 0; } // very first fix in SOMETIMES mode: always wait for WAIT_TIME_FIRST_FIX_SECONDS to collect orbit data
			
			if((sampleSometimes) || (alwaysOnCounter % ((uint32_t) (eepromSettingGPSFrequencySeconds)) == 0)) { // in ALWAYS-ON: only store when modulo with counter == 0
				if(((gpsData.hdop > 0) && (gpsData.hdop < minHDOPTimes10Override))
					|| (sampleSometimes && ((secondsTtf - startTimeWaitAfterFirstFix) > maxWaitTime))) { // only store if hdop is good enough
					// determine seconds
					gpsData.ttfSeconds = secondsTtf; // downcast from 32 to 16 bit
					if(noFixInSessionYet) { firstTTFinSession = secondsTtf; } // very first fix in session -> store TTFF
					blinkDuringFirstFix = false; // do not blink anymore if eepromSettingBlink is false -> because got a very first fix now
						
					// geofencing
					if(eepromSettingGeofencing) {
						// UNTESTED
						if(geofenceLat == 0) { // very first time getting a valid fix that is good enough -> use as geofence reference
							geofenceLat = gpsData.latitude;
							geofenceLon = gpsData.longitude;
						}
						else {
							int32_t latDiff = labs(geofenceLat - gpsData.latitude);
							int32_t lonDiff = labs(geofenceLon - gpsData.longitude);
							if((latDiff < GEOFENCING_LIMIT) && (lonDiff < GEOFENCING_LIMIT)) { // new fix within fencing limit -> stop
								trackingResult = TRACKING_GEOFENCING_ACTIVE;
								break;
							}
						}
					}

					// store fix
					uint8_t storeResult = storeInMemory(&gpsData, sampleSometimes); // TTF only stored in sometimes mode
					noFixInSessionYet = false; // for SOMETIMES: got very very first fix
					if(storeResult == STORE_MEMORY_RETURN_MEMORY_FULL) { trackingResult = TRACKING_MEMORY_FULL; break; } // UNTESTED
					else if(storeResult == STORE_MEMORY_RETURN_ERROR) { trackingResult = TRACKING_ERROR; break; } // UNTESTED, not doing anything
					else if(sampleSometimes) {
						burstCounter++;
						if(burstCounter >= eepromSettingBurstDuration) {
							trackingResult = TRACKING_GOT_FIX_SLEEP; break;
						}
					}
				}
			}
			alwaysOnCounter++; // for always on mode: only incremented when having a fix, then storing only if modulo == 0 -> will also be incremented if hdop is not good enough
		}
		
		// check if time to sleep already
		if(isNightTime()) {
			#if LED_ON_AFTER_GETTING_FIRST_TIME_AND_NIGHT == true
			if(blinkDuringFirstFix) {
				PORTB.OUTSET = PIN5_bm; // LED on
				_delay_ms(4000); // wait for 4 seconds
				PORTB.OUTCLR = PIN5_bm; // LED off
			}
			#endif
			blinkDuringFirstFix = false; // getting time and it's night -> do not blink anymore
			trackingResult = TRACKING_NIGHT_TIME;
			break;
		}
			
		// check voltage UNDER LOAD and EVERY SECOND, NOT using EEPROM setting, WARNING: 15mA sourced from lipo -> measured voltage drop of 100mV (30mAh lipo)
		batteryVoltage = deviceReadSupplyVoltage();
		if((batteryVoltage < MIN_VOLTAGE_UNDER_LOAD) || (batteryVoltage > OPERATION_VOLTAGE_MAX_RANGE)) {
			trackingResult = TRACKING_UNDERVOLTAGE;
			break;		
		}
			
		// check interrupt pin status
		if(~PORTA.IN & PIN5_bm) { // PATH TESTED, download button pressed
			_delay_ms(200); // wait a bit and check again
			if(~PORTA.IN & PIN5_bm) { trackingResult = TRACKING_DOWNLOAD_INTERRUPT; break; }
		}
			
		// re-init L70 uart
		#if (DEBUGGING == true)
			initPrimaryUARTwith9600();
		#endif
			
		// shortly blink led
		if(eepromSettingBlink || blinkDuringFirstFix) { // not having time -> blink while getting time
			deviceLedOn();
			if(gpsData.year == 80) { _delay_ms(120); } // not having full timestamp yet
			else { _delay_ms(30); } // got time
			deviceLedOff();
		}
	}

	// print result
	#if (DEBUGGING == true)
		// COMMENTED OUT: needs too much flash memory
		//printf("%uT%luF%uH%uT%u\n\r", trackingResult, timestampEstimation, gpsData.fix, gpsData.hdop, gpsData.ttfSeconds);
		//_delay_ms(30); // wait until printf finished
	#endif
	
	// after the run: stop everything
	deviceL70powerOff();
	uartFlush();
	uartOff();
	deinitSecondUartPins(); // otherwise leakage
	devicePowerMemoryAndI2COff();
	
	// why did we stop?
	if(trackingResult == TRACKING_UNDERVOLTAGE) { // very likely the case: start again after some time, stay in tracking, backup power STAYS ON, voltage not under load will be checked when entering ST_TRACKING again
		// PATH TESTED
		undervoltageUnderLoadCounter++;
		if(timestampEstimation != 0) { timestampEstimation += FIRST_SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT; } // if got time before: not a reason to reset it
		deviceInitInternalRTCInterrupt(FIRST_SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT);
	}
	else if(trackingResult == TRACKING_MEMORY_FULL) {
		// PATH TESTED
		state = ST_MEMORY_FULL;
		deviceL70backupOff();
		deviceInitInternalRTCInterrupt(1);
		timestampEstimation = 0;  // reset timestamp
	}
	else if(trackingResult == TRACKING_DOWNLOAD_INTERRUPT) {
		if(downloadButtonPressedFor3Seconds()) { // button pressed for 3 seconds -> enter download mode
			// PATH TESTED
			state = ST_DOWNLOAD;
			deviceL70backupOff();
			deviceInitInternalRTCInterrupt(1); // go into download mode
			timestampEstimation = 0; // don't know about time when menue is entered
		}
		else { // not pressed long enough, but stopped by download interrupt
			// PATH TESTED
			deviceInitInternalRTCInterrupt(1); // start again
			timestampEstimation = 0; // don't know about time when menue is entered
		}
	}
	else if(trackingResult == TRACKING_NIGHT_TIME) {
		// PATH TESTED
		deviceInitInternalRTCInterrupt(1); // start again
		timestampEstimation += 1; // sleeping for one second -> increase sleep time
	}
	else if((trackingResult == TRACKING_GOT_FIX_SLEEP) || (trackingResult == TRACKING_TIMEOUT)) { // only in sometimes mode -> CHANGED, also in always-on mode, but only when not even getting time
		// PATH TESTED
		// simple sleep time: just sleep for duration of GPS frequency setting, not looking on how long the fix took
		uint16_t sleepTime = eepromSettingGPSFrequencySeconds; // casting down from uint32_t to uint16_t
		#if (GET_FIX_SLEEP_LONGER_AFTER_TIMEOUT == true)
			// PATH TESTED
			if(trackingResult == TRACKING_TIMEOUT) { sleepTime = GET_FIX_SLEEP_LONGER_AFTER_TIMEOUT_SECONDS; } // sleep longer time, but keep backup power on
		#endif
		if(sleepTime == 0) { sleepTime = 1; } // just in case
			
		// more sophisticated to get even distribution of fixes, but I think it doesn't work out too well
		/*if(eepromSettingGPSFrequencySeconds > (deviceGetMillis() / 1000)) { // normal situation: fix did not take longer than GPS frequency
			uint32_t sleepTimeLong;
			sleepTimeLong = ((eepromSettingGPSFrequencySeconds * 1000) - deviceGetMillis()) / 1000;
			sleepTime = sleepTimeLong;
		}
		else { sleepTime = 1; } // long fix time with short interval setting (5min until fix, but fix every minute -> restart immediately)*/
			
		deviceInitInternalRTCInterrupt(sleepTime);
		if(timestampEstimation != 0) { timestampEstimation += sleepTime; } // next wake up: roughly know what time it is		
	}
	else { // TRACKING_ERROR or GEOFENCING: start again after some time, stay in tracking, backup power stays on
		// UNTESTED
		errorCounter++; // will also be incremented when doing geo-fencing
		if(timestampEstimation != 0) { timestampEstimation += GEOFENCING_OR_ERROR_WAIT_TIME_SECONDS; } // if got time before: not a reason to reset it
		deviceInitInternalRTCInterrupt(GEOFENCING_OR_ERROR_WAIT_TIME_SECONDS);
	}
}

void changeGPSBaudrate() {
	_delay_ms(1000);
	deviceSetCPUSpeed(OSC16_PRESCALER_CPU_2MHZ);
	_delay_ms(8000);
	
	deviceL70powerOn();
	deviceL70backupOn(); // just in case it has been turned off during night time
	
	_delay_ms(4000);
	
	initPrimaryUARTwith115200(); // init uart to communicate with L70

	gpsSetBaudrate9600Permanently();

	while(1) {
		deviceBlink(3);
		_delay_ms(400);
	}
}

/** State machine */

int main(void) {
	while(1) {
		// Do on every wake up
		RTC.CTRLA = 0; // disable RTC interrupt
		if(state == ST_FIRST_START_HARD_RESET) { deviceSetCPUSpeed(OSC16_PRESCALER_CPU_1MHZ);  } // 1MHz = 701uA @while(1), do this BEFORE reading supply voltage
		batteryVoltage = deviceReadSupplyVoltage(); // perform as first thing in every state

		/** STATE: FIRST START HARD RESET */
		if(state == ST_FIRST_START_HARD_RESET) { // will be re-entered after data download
			deviceInitPins(); // reset all pins
			deviceInitDownloadPinInterruptInDeepSleep(); // will wake up MCU in ALL STATES!
			wait(FIRST_START_DELAY_SECONDS); // wait a bit until everything stable
			usePrintf(); // use printf function for uart communication
			
			// get settings from eeprom
			eepromSettingMinVoltage = getOperationVoltageMinFromTinyEEPROM();
			eepromSettingGPSFrequencySeconds = getGPSFrequencyFromTinyEEPROM();
			eepromSettingMinHDOP = getMinHDOPFromTinyEEPROM();
			eepromSettingActivationDelaySeconds = getActivationDelayFromTinyEEPROM();
			eepromSettingTurnOnHour = getTurnOnHourFromTinyEEPROM();
			eepromSettingTurnOnMinute = getTurnOnMinuteFromTinyEEPROM();
			eepromSettingTurnOffHour = getTurnOffHourFromTinyEEPROM();
			eepromSettingTurnOffMinute = getTurnOffMinuteFromTinyEEPROM();
			eepromSettingGeofencing = (uint8_t) TinyEEPROM(EEPROM_GEOFENCING_ADDRESS);
			if(eepromSettingGeofencing > 1) { eepromSettingGeofencing = GEOFENCING_DEFAULT; }
			eepromSettingBlink = (uint8_t) TinyEEPROM(EEPROM_BLINK_ADDRESS);
			if(eepromSettingBlink > 1) { eepromSettingBlink = BLINK_DEFAULT; }
			eepromSettingBurstDuration = (uint8_t) TinyEEPROM(EEPROM_BURST_DURATION_ADDRESS);
			if(eepromSettingBurstDuration == 0xFF) { eepromSettingBurstDuration = BURST_DURATION_DEFAULT; }
			
			// (re)set all global variables
			noFixInSessionYet = true;
			downloadInterrupt = false; // volatile because used in ISR
			rtcInterrupt = false; // volatile because used in ISR
			timestampEstimation = 0;
			activated = false;
			geofenceLat = 0;
			geofenceLon = 0;
			
			#if TRACKING_MODE == TRACKING_MODE_TEST
				//lightSleepUARTTest();
				//flpTest();
				//newParserTest();
				//readVoltage1626Test();
				//readVoltage1606Test();
				changeGPSBaudrate();
			#endif
			
			if(voltageNotOkay()) {
				state = ST_VOLTAGE_INCORRECT; // move to special state
				deviceBlink(2); // indicate low battery right from start
				deviceInitInternalRTCInterrupt(FIRST_SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT); // first undervoltage -> only short sleep
			}
			else { // do not check if memory full, very unlikely 
				/*devicePowerMemoryAndI2COn();
				i2cInit();
				mockEmptyMemory(DATA_FIX_LENGTH, METADATA_ADDRESS_OF_ADDRESS_POINTER);
				devicePowerMemoryAndI2COff();*/
				deviceInitGPIO2PinInterruptInDeepSleep(); // can also be activated by GPIO2
				state = ST_WAIT_FOR_ACTIVATION; // wait for activation
			}
		}
		/** STATE: WAIT FOR ACTIVATION */
		else if(state == ST_WAIT_FOR_ACTIVATION) { // download pin interrupt active, GPIO2 pin interrupt active (!!)
			// no voltage check here, because ultra low power state already and check will be executed immediately in ST_TRACKING
			if(downloadInterrupt || gpio2Interrupt) { // can be woken up by both
				_delay_ms(700); // wait a bit and check again
				if((~PORTA.IN & PIN5_bm) || (~PORTC.IN & PIN1_bm)) { // one of the buttons still pressed		
					downloadInterrupt = false;
					activated = true;
					
					// blink to indicate start
					deviceBlink(7); // indicate start
					uint8_t blinkBatteryStatus = (batteryVoltage / 200) - 14; // 1 time when V = 3.0V, 7 times when V = 4.2V
					_delay_ms(700);
					deviceBlink(blinkBatteryStatus); // 0 = doesn't blink at all
					
					state = ST_TRACKING; // move to tracking state
					//deviceL70backupOn(); // DO NOT turn on here, because it already consumes the backup current -> will be turned on in the tracking functions
					deviceDeactivateGPIO2PinInterruptInDeepSleep(); // GPIO2 shall not work afterwards
					deviceInitInternalRTCInterrupt(eepromSettingActivationDelaySeconds); // go into tracking after whatever seconds
					
					// mock timestampestimation (temporary for testing only)
					//timestampEstimation = 1623110325UL;
				}
			}
		}
		/** STATE: TRACKING */
		else if(state == ST_TRACKING) { // download pin interrupt active
			if(voltageNotOkay()) { // checking voltage WITHOUT LOAD!
				state = ST_VOLTAGE_INCORRECT;
				deviceL70backupOff();
				deviceInitInternalRTCInterrupt(FIRST_SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT); // first under voltage -> only short sleep
				timestampEstimation = 0; // don't know about time
			}
			else if(downloadInterrupt) { // woke up by download interrupt (can also happen when not yet activated)
				if(downloadButtonPressedFor3Seconds()) { // button pressed for 3 seconds -> enter download mode
					state = ST_DOWNLOAD;
					deviceL70backupOff();
					deviceInitInternalRTCInterrupt(1); // go into download mode
					timestampEstimation = 0; // don't know about time when menue is entered
				}
				else { // not pressed long enough, but woken up by download interrupt
					deviceInitInternalRTCInterrupt(1); // start again after one second
					timestampEstimation = 0; // don't know about time, interrupt can happen randomly
				}
			}
			else if(isNightTime()) { // TESTED PATH
				// having a valid time here!
				noFixInSessionYet = true; // used to collect more orbit data when doing first fix in session -> resetting it so that in next session first fix again collects more orbit data
				deviceInitInternalRTCInterrupt(SLEEP_TIME_DURING_NIGHT); // go into nighttime mode
				timestampEstimation += SLEEP_TIME_DURING_NIGHT; // having time here because isNightTime returns true
				// keep it on for very first day to keep orbit data? -> no, because after getting time will sleep immediately
				#if	(BACKUP_POWER_ON_DURING_NIGHT_TIME == false)
					deviceL70backupOff(); // WARNING: backup power goes OFF here but staying in tracking -> turns on again in trackingMerged
				#endif
			}
			else { // normal RTC wake up
				#if TRACKING_MODE != TRACKING_MODE_TEST
					trackingMerged();
				#endif
				// here at the end backup power normally is ON! (exception: state transition)
			}
		}
		/** STATE: VOLTAGE INCORRECT */
		else if(state == ST_VOLTAGE_INCORRECT) { // download pin interrupt active, RTC interrupt active, backup pin OFF
			if(voltageNotOkay()) {
				deviceInitInternalRTCInterrupt(SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT);
			}
			else { // woke up by RTC or by download button and voltage okay again
				bool enterDownloadMode = false;
				if(downloadInterrupt) { enterDownloadMode = downloadButtonPressedFor3Seconds(); } // if woke up by download button -> check if pressed for 3 more seconds
				
				// if voltage okay and woken up by whatever reason -> should leave ST_VOLTAGE_INCORRECT
				if(enterDownloadMode) { // highest priority: someone pressed download button (long enough)
					state = ST_DOWNLOAD;
					deviceInitInternalRTCInterrupt(1); // go into download mode
				}
				else if(activated) { // not pressed long enough or "normal" wake up by RTC, but already activated -> go back to tracking
					state = ST_TRACKING; // move to tracking state
					deviceInitInternalRTCInterrupt(1); // go back into tracking after 1 second
					//deviceL70backupOn(); // DO NOT turn on here -> will be turned on in the tracking functions				
				}
				else { // very unusual: voltage not okay in first start state -> transit to here before activation
					deviceInitGPIO2PinInterruptInDeepSleep(); // otherwise cannot be activated with GPIO2 when in under voltage immediately after start
					state = ST_WAIT_FOR_ACTIVATION; // wait for activation
				}
			}		
		}
		/** STATE: MEMORY FULL */	
		else if(state == ST_MEMORY_FULL) { // no voltage check because minimal power consumption, download pin interrupt active
			if(downloadInterrupt) { // don't wait long here and check again status because when memory full entering activation state afterwards isn't a problem
				downloadInterrupt = false;
				state = ST_DOWNLOAD;
				deviceInitInternalRTCInterrupt(1); // go into download mode in 1 second
			}
		}
		/** STATE: DOWNLOAD */
		else if(state == ST_DOWNLOAD) { // no voltage check because minimal power consumption, download pin interrupt active
			startMenue();
			state = ST_FIRST_START_HARD_RESET; // RESTARTING TAG!
			deviceInitInternalRTCInterrupt(1); // go into start state again in 1 second
		}
		
		rtcInterrupt = false; // reset RTC interrupt flag
		downloadInterrupt = false; // reset download interrupt flag
		gpio2Interrupt = false; // reset gpio2 interrupt flag
		deviceStandbySleep(); // go to sleep until an interrupt happens
	}
}