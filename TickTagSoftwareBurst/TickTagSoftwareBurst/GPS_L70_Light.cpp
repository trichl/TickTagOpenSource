#include "GPS_L70_Light.h"

const char* commandOnlyGPGGAEverySecond =		"$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";
const char* commandOnlyGPRMCEverySecond =		"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";
const char* commandGPGGAandGPRMCEverySecond =	"$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n";
const char* commandBaudrate9600Permanently =	"$PQBAUD,W,9600*4B\r\n";

uint8_t gpsConvertTwoDigit2Number(char *digit_char) {
	return 10 * (digit_char[0] - '0') + (digit_char[1] - '0');
}

void gpsRemoveAllChars(char* str, char c) {
	char *pr = str, *pw = str;
	while(*pr) {
		*pw = *pr++;
		pw += (*pw != c);
	}
	*pw = '\0';
}

int32_t gpsParseLatLong(char* d) {
	// (d)ddmm.mmmm
	int32_t latlon;
	gpsRemoveAllChars(d, '.');
	latlon = atol(d);

	// convert lat lon in degrees + seconds into degrees only -> (d)dd will also be the same for converted result
	int32_t deg = latlon / 1000000UL; // = (d)dd (integer division)
	int32_t min = latlon - (deg * 1000000UL); // = mmmmmm
	latlon = (deg * 100000) + (min / 6); // 5 decimal places = 1.11m accuracy

	return latlon;
}

uint32_t gpsGetUTCTimestamp(gps_t *gpsData) {
	if(gpsData == NULL) { return 0; }
	if(gpsData->year == 80) { return 0; }
	uint16_t year = gpsData->year;
	year += 2000;
	uint32_t timestamp = _UNIX_TIMESTAMP(year, gpsData->month, gpsData->day, gpsData->hour, gpsData->minute, gpsData->second);
	return timestamp;
}

void gpsDecodeGPRMCandGPGGA(char* d, gps_t *gpsData, uint8_t sentenceType) { // e.g. $GPRMC,013732.000,A,3150.7238,N,11711.7278,E,0.00,0.00,220413,,,A*68
	// GPRMC: everything
		// comma 2: time
		// comma 4: lat
		// comma 5: south/north
		// comma 6: lon
		// comma 7: west/east
		// comma 10: date
		// end: fix y/n (N, A, D)
	// GPGGA: only hdop
		// comma 2: time
		// comma 3: lat
		// comma 4: south/north
		// comma 5: lon
		// comma 6: west/east
		// comma 7: fix status (0, 1, 2)
	uint8_t commaCnt = 0;
	char buffer[32];
	uint8_t bufferPnt = 0;
	memset(buffer, 0, 32);
	if(gpsData == NULL) { return; }
	while(*d) {
		if(*d == '$') { } // do nothing
		else if(*d == ',') {
			commaCnt++;
			if(sentenceType == SENTENCE_TYPE_GPRMC) { // GPRMC
				if(commaCnt == 2) { // time
					gpsData->hour = 0;
					gpsData->minute = 0;
					gpsData->second = 0;
					if(strlen(buffer) >= 6) {
						gpsData->hour = gpsConvertTwoDigit2Number(&buffer[0]);
						gpsData->minute = gpsConvertTwoDigit2Number(&buffer[2]);
						gpsData->second = gpsConvertTwoDigit2Number(&buffer[4]);
						//printf("Time: %u:%u:%u\n\r", gpsData->hour, gpsData->minute, gpsData->second);
						// skip milliseconds
					}
				}
				else if(commaCnt == 4) { // lat
					gpsData->latitude = 0;
					if(strlen(buffer) > 0) {
						gpsData->latitude = gpsParseLatLong(buffer);
						//if(debug) { printf("Lat: %ld\n\r", gpsData->latitude); }
					}
				}
				else if(commaCnt == 5) { // south / north
					if(strlen(buffer) == 1) {
						if((buffer[0] == 'S') || (buffer[0] == 's')) {
							gpsData->latitude *= -1;
						}
						//if(debug) { printf("South: %ld\n\r", gpsData->latitude); }
					}
				}
				else if(commaCnt == 6) { // lon
					gpsData->longitude = 0;
					if(strlen(buffer) > 0) {
						gpsData->longitude = gpsParseLatLong(buffer);
						//if(debug) { printf("Lon: %ld\n\r", gpsData->longitude); }
					}
				}
				else if(commaCnt == 7) { // west / east
					if(strlen(buffer) == 1) {
						if((buffer[0] == 'W') || (buffer[0] == 'w')) {
							gpsData->longitude *= -1;
						}
						//if(debug) { printf("West: %ld\n\r", gpsData->longitude); }
					}
				}
				else if(commaCnt == 10) { // date
					gpsData->day = 0;
					gpsData->month = 0;
					gpsData->year = 0;
					if(strlen(buffer) >= 6) {
						gpsData->day = gpsConvertTwoDigit2Number(&buffer[0]);
						gpsData->month = gpsConvertTwoDigit2Number(&buffer[2]);
						gpsData->year = gpsConvertTwoDigit2Number(&buffer[4]);
						//printf("Date: %u.%u.%u\n\r", gpsData->day, gpsData->month, gpsData->year);
					}
				}
			}
			else { // GPGGA
				// sats not important!
				/*if(commaCnt == 8) { // num of sats
					gpsData->sats_in_use = 0;
					if(strlen(buffer) == 1) { // "2"
						gpsData->sats_in_use = buffer[0] - '0';
					}
					else if(strlen(buffer) == 2) { // "12"
						gpsData->sats_in_use = gpsConvertTwoDigit2Number(&buffer[0]);
					}
					//printf("Sats: %u\n\r", gpsData->sats_in_use);
				}
				else*/
				if(commaCnt == 9) { // hdop, can only parse (__.X or __.XX)
					gpsData->hdop = 0;
					if(strlen(buffer) >= 1) {
						int16_t hdopBig;
						char* afterComma = strchr(buffer, '.'); // "13.21" -> ".21"
						if(afterComma != NULL) {
							if(strlen(afterComma) >= 2) {
								if((strlen(buffer) + 2) >= strlen(afterComma)) {
									buffer[strlen(buffer) + 2 - strlen(afterComma)]  = '\0'; // remove everything behind "13.2..."
								}
								gpsRemoveAllChars(buffer, '.'); // "2.2" to "22", "13.2" to "132"
								hdopBig = atoi(buffer);
								if(hdopBig > 255) { hdopBig = 255; }
								gpsData->hdop = hdopBig;
							}
						}
					}
					//printf("HDOP: %u\n\r", gpsData->hdop);
				}				
			}
			memset(buffer, 0, 32); // reset buffer
			bufferPnt = 0;
		}
		else if(*d == '*') { // do not look at CRC
			if(sentenceType == SENTENCE_TYPE_GPRMC) { // evaluate fix result
				gpsData->fix = 0;
				if(strlen(buffer) > 0) {
					if(buffer[0] == 'N') { gpsData->fix = 0; }
					else if((buffer[0] == 'A') || (buffer[0] == 'D')) { gpsData->fix = 1; }
					//printf("Fix: %u\n\r", gpsData->fix);
				}
			}
			break;
		}
		else {
			buffer[bufferPnt] = *d;
			bufferPnt++;
		}
		d++;
	}
}

void gpsDecodeGPRMCandGPGGANew(char* d, gps_t *gpsData, uint8_t sentenceType) { // e.g. $GPRMC,013732.000,A,3150.7238,N,11711.7278,E,0.00,0.00,220413,,,A*68
	// GPRMC: (before: for everything)
		// comma 2: time
		// comma 4: lat
		// comma 5: south/north
		// comma 6: lon
		// comma 7: west/east
		// comma 10: date
		// end: fix y/n (N, A, D)
	// GPGGA: (before: only hdop)
		// comma 2: time
		// comma 3: lat
		// comma 4: south/north
		// comma 5: lon
		// comma 6: west/east
		// comma 7: fix status (0, 1, 2)
	uint8_t commaCnt = 0;
	char buffer[32];
	uint8_t bufferPnt = 0;
	memset(buffer, 0, 32);
	if(gpsData == NULL) { return; }
	while(*d) {
		if(*d == '$') { } // do nothing
		else if(*d == ',') {
			commaCnt++;
			if(commaCnt == 2) { // for both GPGGA and GPRMC: time
				gpsData->hour = 0;
				gpsData->minute = 0;
				gpsData->second = 0;
				if(strlen(buffer) >= 6) {
					gpsData->hour = gpsConvertTwoDigit2Number(&buffer[0]);
					gpsData->minute = gpsConvertTwoDigit2Number(&buffer[2]);
					gpsData->second = gpsConvertTwoDigit2Number(&buffer[4]);
					// skip milliseconds
				}
				#if (DEBUG_DECODER == true)
					printf("%u T:%u:%u:%u\n\r", sentenceType, gpsData->hour, gpsData->minute, gpsData->second);
				#endif
			}
			else if(commaCnt == (3 + sentenceType)) { // lat (GPGGA: 3, GPRMC: 4)
				gpsData->latitude = 0;
				if(strlen(buffer) > 0) {
					gpsData->latitude = gpsParseLatLong(buffer);
				}
				#if (DEBUG_DECODER == true)
					printf("%u LA:%ld\n\r", sentenceType, gpsData->latitude);
				#endif
			}
			else if(commaCnt == (4 + sentenceType)) { // south / north
				if(strlen(buffer) == 1) {
					if((buffer[0] == 'S') || (buffer[0] == 's')) {
						gpsData->latitude *= -1;
					}
				}
				#if (DEBUG_DECODER == true)
					printf("%u S:%ld\n\r", sentenceType, gpsData->latitude);
				#endif
			}
			else if(commaCnt == (5 + sentenceType)) { // lon
				gpsData->longitude = 0;
				if(strlen(buffer) > 0) {
					gpsData->longitude = gpsParseLatLong(buffer);
				}
				#if (DEBUG_DECODER == true)
					printf("%u LO:%ld\n\r", sentenceType, gpsData->longitude);
				#endif
			}
			else if(commaCnt == (6 + sentenceType)) { // west / east
				if(strlen(buffer) == 1) {
					if((buffer[0] == 'W') || (buffer[0] == 'w')) {
						gpsData->longitude *= -1;
					}
				}
				#if (DEBUG_DECODER == true)
					printf("%u W:%ld\n\r", sentenceType, gpsData->longitude);
				#endif
			}
			else if(commaCnt == 7) { // fix
				if(sentenceType == SENTENCE_TYPE_GPGGA) {
					gpsData->fix = 0;
					if(strlen(buffer) > 0) {
						if((buffer[0] == '1') || (buffer[0] == '2')) { gpsData->fix = 1; }
					}
					#if (DEBUG_DECODER == true)
						printf("%u F:%u\n\r", sentenceType, gpsData->fix);
					#endif
				}
			}
			else if(commaCnt == 9) { // hdop, can only parse (__.X or __.XX)
				if(sentenceType == SENTENCE_TYPE_GPGGA) {
					gpsData->hdop = 0;
					if(strlen(buffer) >= 1) {
						int16_t hdopBig;
						char* afterComma = strchr(buffer, '.'); // "13.21" -> ".21"
						if(afterComma != NULL) {
							if(strlen(afterComma) >= 2) {
								if((strlen(buffer) + 2) >= strlen(afterComma)) {
									buffer[strlen(buffer) + 2 - strlen(afterComma)]  = '\0'; // remove everything behind "13.2..."
								}
								gpsRemoveAllChars(buffer, '.'); // "2.2" to "22", "13.2" to "132"
								hdopBig = atoi(buffer);
								if(hdopBig > 255) { hdopBig = 255; }
								gpsData->hdop = hdopBig;
							}
						}
					}
					#if (DEBUG_DECODER == true)
						printf("%u H:%u\n\r", sentenceType, gpsData->hdop);
					#endif
				}
			}			
			else if(commaCnt == 10) { // date
				if(sentenceType == SENTENCE_TYPE_GPRMC) {
					gpsData->day = 0;
					gpsData->month = 0;
					gpsData->year = 0;
					if(strlen(buffer) >= 6) {
						gpsData->day = gpsConvertTwoDigit2Number(&buffer[0]);
						gpsData->month = gpsConvertTwoDigit2Number(&buffer[2]);
						gpsData->year = gpsConvertTwoDigit2Number(&buffer[4]);
					}
					#if (DEBUG_DECODER == true)
						printf("%u D:%u.%u.%u\n\r", sentenceType, gpsData->day, gpsData->month, gpsData->year);
					#endif
				}
			}
			memset(buffer, 0, 32); // reset buffer
			bufferPnt = 0;
		}
		else if(*d == '*') { // do not look at CRC
			if(sentenceType == SENTENCE_TYPE_GPRMC) { // evaluate fix result
				gpsData->fix = 0;
				if(strlen(buffer) > 0) {
					if((buffer[0] == 'A') || (buffer[0] == 'D')) { gpsData->fix = 1; }
				}
				#if (DEBUG_DECODER == true)
					printf("%u F:%u\n\r", sentenceType, gpsData->fix);
				#endif
			}
			break;
		}
		else {
			buffer[bufferPnt] = *d;
			bufferPnt++;
		}
		d++;
	}
}

/*void gpsDecodeGPRMC(char* d, gps_t *gpsData) { // e.g. $GPRMC,013732.000,A,3150.7238,N,11711.7278,E,0.00,0.00,220413,,,A*68
	uint8_t commaCnt = 0;
	char buffer[32];
	uint8_t bufferPnt = 0;
	memset(buffer, 0, 32);
	if(gpsData == NULL) { return; }
	while(*d) {
		if(*d == '$') { } // do nothing
		else if(*d == ',') {
			commaCnt++;
			if(commaCnt == 1) { } // = "GPRMC", do not do anything
			else if(commaCnt == 2) { // time
				gpsData->hour = 0;
				gpsData->minute = 0;
				gpsData->second = 0;
				if(strlen(buffer) >= 6) {
					gpsData->hour = gpsConvertTwoDigit2Number(&buffer[0]);
					gpsData->minute = gpsConvertTwoDigit2Number(&buffer[2]);
					gpsData->second = gpsConvertTwoDigit2Number(&buffer[4]);
					//printf("Time: %u:%u:%u\n\r", gpsData->hour, gpsData->minute, gpsData->second);
					// skip milliseconds
				}
			}
			else if(commaCnt == 4) { // lat
				gpsData->latitude = 0;
				if(strlen(buffer) > 0) {
					gpsData->latitude = gpsParseLatLong(buffer);
					//if(debug) { printf("Lat: %ld\n\r", gpsData->latitude); }
				}
			}
			else if(commaCnt == 5) { // south / north
				if(strlen(buffer) == 1) {
					if((buffer[0] == 'S') || (buffer[0] == 's')) {
						gpsData->latitude *= -1;
					}
					//if(debug) { printf("South: %ld\n\r", gpsData->latitude); }
				}
			}			
			else if(commaCnt == 6) { // lon
				gpsData->longitude = 0;
				if(strlen(buffer) > 0) {
					gpsData->longitude = gpsParseLatLong(buffer);
					//if(debug) { printf("Lon: %ld\n\r", gpsData->longitude); }
				}
			}
			else if(commaCnt == 7) { // west / east
				if(strlen(buffer) == 1) {
					if((buffer[0] == 'W') || (buffer[0] == 'w')) {
						gpsData->longitude *= -1;
					}
					//if(debug) { printf("West: %ld\n\r", gpsData->longitude); }
				}
			}
			else if(commaCnt == 10) { // date
				gpsData->day = 0;
				gpsData->month = 0;
				gpsData->year = 0;
				if(strlen(buffer) >= 6) {
					gpsData->day = gpsConvertTwoDigit2Number(&buffer[0]);
					gpsData->month = gpsConvertTwoDigit2Number(&buffer[2]);
					gpsData->year = gpsConvertTwoDigit2Number(&buffer[4]);
					//printf("Date: %u.%u.%u\n\r", gpsData->day, gpsData->month, gpsData->year);
					// skip milliseconds
				}
			}
			memset(buffer, 0, 32); // reset buffer
			bufferPnt = 0;
		}
		else if(*d == '*') { // do not look at CRC
			gpsData->fix = 0;
			if(strlen(buffer) > 0) {
				if(buffer[0] == 'N') { gpsData->fix = 0; }
				else if((buffer[0] == 'A') || (buffer[0] == 'D')) { gpsData->fix = 1; }
				//printf("Fix: %u\n\r", gpsData->fix);
			}
			break;
		}
		else {
			buffer[bufferPnt] = *d;
			bufferPnt++;
		}
		d++;
	}
}*/

/*void gpsDecodeGPGGA(char* d, gps_t *gpsData) {
	// $GPGGA,HHMMSS.ss,BBBB.BBBB,b,LLLLL.LLLL,l,Q,NN,D.D,H.H,h,G.G,g,A.A,RRRR*PP
	uint8_t commaCnt = 0;
	char buffer[32];
	uint8_t bufferPnt = 0;
	memset(buffer, 0, 32);
	if(gpsData == NULL) { return; }
	while(*d) {
		if(*d == '$') { } // do nothing
		else if(*d == ',') {
			commaCnt++;
			if(commaCnt == 8) { // num of sats
				gpsData->sats_in_use = 0;
				if(strlen(buffer) == 1) { // "2"
					gpsData->sats_in_use = buffer[0] - '0';
				}
				else if(strlen(buffer) == 2) { // "12"
					gpsData->sats_in_use = gpsConvertTwoDigit2Number(&buffer[0]);
				}
				//printf("Sats: %u\n\r", gpsData->sats_in_use);			
			}
			else if(commaCnt == 9) { // hdop, can only parse (__.X or __.XX)
				gpsData->hdop = 0;
				if(strlen(buffer) >= 1) {
					int16_t hdopBig;
					char* afterComma = strchr(buffer, '.'); // "13.21" -> ".21"
					if(afterComma != NULL) {
						if(strlen(afterComma) >= 2) {
							if((strlen(buffer) + 2) >= strlen(afterComma)) {
								buffer[strlen(buffer) + 2 - strlen(afterComma)]  = '\0'; // remove everything behind "13.2..."
							}
							gpsRemoveAllChars(buffer, '.'); // "2.2" to "22", "13.2" to "132"
							hdopBig = atoi(buffer);
							if(hdopBig > 255) { hdopBig = 255; }
							gpsData->hdop = hdopBig;
						}
					}
				}
				//printf("HDOP: %u\n\r", gpsData->hdop);		
			}
			memset(buffer, 0, 32); // reset buffer
			bufferPnt = 0;
		}
		else if(*d == '*') { // do not look at CRC
			break;
		}
		else {
			buffer[bufferPnt] = *d;
			bufferPnt++;
		}
		d++;
	}
}*/

void gpsSetBaudrate9600Permanently() {
	printf(commandBaudrate9600Permanently); // request only GPGGA and GPRMC messages
	// needs to wait afterwards	
}

bool gpsConfigureBeforeStart(char *messageBuffer) {
	char *startMessage;
	// wait for boot confirmation of L70 (First Msg: $PMTK011,MTKGPS*08, Second Msg: $PMTK010,002*2D)
	while(1) {
		if(!uartRead('\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { return false; } // wait until L70 is booted, should return $PMTK010,00X*2E\r\n
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 8)) {
			if((startMessage[0] == '$') && (startMessage[1] == 'P') && (startMessage[2] == 'M') && (startMessage[3] == 'T') && (startMessage[4] == 'K') && (startMessage[5] == '0') && (startMessage[6] == '1') && (startMessage[7] == '0')) {
				break;
			}
		}
	}
	/*
	if(debug) {
		initSecondUARTwith9600();
		printf("First: %s\n\r\n\r", messageBuffer);
		initPrimaryUARTwith9600();
	}
	*/
	// wait on confirmation of setting
	printf(commandGPGGAandGPRMCEverySecond); // request only GPGGA and GPRMC messages
	while(1) {
		if(!uartRead('\n', messageBuffer, L70_START_BUFFER_SIZE, 3000)) { return false; } // wait on answer for setting, return false if no message received after 1200ms
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 8)) {
			if((startMessage[0] == '$') && (startMessage[1] == 'P') && (startMessage[2] == 'M') && (startMessage[3] == 'T') && (startMessage[4] == 'K') && (startMessage[5] == '0') && (startMessage[6] == '0') && (startMessage[7] == '1')) {
				break;
			}
		}
	}
	/*if(debug) {
		initSecondUARTwith9600();
		printf("Second: %s\n\r", messageBuffer);
		_delay_ms(20);
		initPrimaryUARTwith9600();
	}*/
	return true;
}

/*gps_result_t gpsStart(gps_t *gpsData, uint16_t minVoltage, uint8_t minHDOP) {
	char messageBuffer[L70_START_BUFFER_SIZE];
	gpsData->ttfSeconds = 0;
	char *startMessage;
	uint32_t startTime = deviceGetMillis(); // start measuring ttf time
	uint32_t startTimeWaitAfterFirstFix = 0;
	initPrimaryUARTwith9600(); // init uart to communicate with L70
	uint8_t minVoltageCnt = 0;

	if(!gpsConfigureBeforeStart(messageBuffer)) { return GPS_RESULT_NO_FIX; }
	
	while(1) {
		// wait on message from L70 (max. 2 * 80 Byte*(8+2) * (0.1042ms/bit) = 166.6ms)
		uartReadMultipleLines(2, '\n', messageBuffer, L70_START_BUFFER_SIZE, 1500); // flush and memset done in function
		
		// switch to debug uart
		#if (DEBUGGING == true)
			initSecondUARTwith9600(); // uart to communicate with external programmer
			//strcpy(command, "$GPRMC,144326.00,A,5107.0017,N,11402.3291,W,0.080,323.3,210307,0.0,E,A*25"); // TEST
		#endif
		
		// decode received message 1
		startMessage = strchr(messageBuffer, '$'); // remove possible noise before $
		if((startMessage != NULL) && (strlen(startMessage) > 6)) {
			if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'R')) { gpsDecodeGPRMCandGPGGA(startMessage, gpsData, SENTENCE_TYPE_GPRMC); }
			if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'G') && (startMessage[4] == 'G')) { gpsDecodeGPRMCandGPGGA(startMessage, gpsData, SENTENCE_TYPE_GPGGA); }
			#if (DEBUGGING == true)
				printf("%lu: %s\n\r", deviceGetMillis(), startMessage); // print full string
			#endif
		}
		
		// decode received message 2
		if(strlen(startMessage) > 1) {
			startMessage = strchr(startMessage+1, '$'); // search for next $ after first $
			if((startMessage != NULL) && (strlen(startMessage) > 6)) {
				if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'R')) { gpsDecodeGPRMCandGPGGA(startMessage, gpsData, SENTENCE_TYPE_GPRMC); }
				if((startMessage[0] == '$') && (startMessage[1] == 'G') && (startMessage[2] == 'P') && (startMessage[3] == 'G') && (startMessage[4] == 'G')) { gpsDecodeGPRMCandGPGGA(startMessage, gpsData, SENTENCE_TYPE_GPGGA); }
			}
		}
		
		// check if fix found
		if(gpsData->fix == 1) {
			if(startTimeWaitAfterFirstFix == 0) { startTimeWaitAfterFirstFix = deviceGetMillis(); } // first fix
			if(((gpsData->hdop > 0) && (gpsData->hdop < minHDOP)) || (deviceGetMillis() - startTimeWaitAfterFirstFix > 10000UL)) {
				startTime = deviceGetMillis() - startTime;
				startTime /= 1000;
				gpsData->ttfSeconds = startTime;
				return GPS_RESULT_GOT_FIX;				
			}
		}
		
		// check if timeout reached
		uint32_t seconds = deviceGetMillis() / 1000;
		uint16_t seconds16 = seconds;
		if((seconds16 > GET_FIX_TIMEOUT_SECONDS) // upper level timeout
			|| ((seconds16 > GET_FIX_TIMEOUT_NOT_EVEN_TIME_SECONDS) && (gpsData->year == 80) && (gpsData->hour == 0) && (gpsData->minute < ((GET_FIX_TIMEOUT_NOT_EVEN_TIME_SECONDS / 60) + 1)))) { // not even got time
			gpsData->ttfSeconds = seconds16;
			return GPS_RESULT_NO_FIX;
		}
		
		// check interrupt pin status
		if(~PORTA.IN & PIN5_bm) { // download button pressed
			_delay_ms(200); // wait a bit and check again
			if(~PORTA.IN & PIN5_bm) { return GPS_RESULT_INTERRUPTED; }
		}
		
		// check voltage every 5 seconds
		if(minVoltageCnt > 5) {
			minVoltageCnt = 0; // reset value
			if(deviceReadSupplyVoltage() < minVoltage) {
				return GPS_RESULT_NO_FIX;
			}
		}
		
		// re-init L70 uart
		#if (DEBUGGING == true)
			initPrimaryUARTwith9600();
		#endif
		
		// shortly blink led
		#if (BLINK_DURING_FIX == true)
			deviceLedOn();
			if(gpsData->year == 80) { _delay_ms(120); } // not having time yet
			else { _delay_ms(50); } // got time
			deviceLedOff();
		#endif
		
		minVoltageCnt++;
	}
	return GPS_RESULT_NO_FIX;
}*/