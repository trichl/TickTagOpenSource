#ifndef GPS_L70_LIGHT_H_
#define GPS_L70_LIGHT_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h> // atoi
#include "UART.h"
#include "Time.h"
#include "TickTag.h"

#define DEBUG_DECODER					false

#define L70_MAX_COMMAND_LEN				90
#define L70_START_BUFFER_SIZE			(2 * L70_MAX_COMMAND_LEN)

#define SENTENCE_TYPE_GPGGA				0
#define SENTENCE_TYPE_GPRMC				1

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
	uint8_t day;
	uint8_t month;
	uint8_t year;
    //uint16_t thousand;
    int32_t latitude; // 5 decimal places, 1.11m accuracy, range: -85(.)00000 to +85(.)00000
    int32_t longitude; // 5 decimal places, 1.11m accuracy, range: -180(.)00000 to +180(.)00000
	uint8_t fix;
	uint16_t ttfSeconds;
	//uint8_t sats_in_use;
	uint8_t hdop;
    //float altitude;
} gps_t;

typedef enum {
	GPS_RESULT_NO_FIX = 0,
	GPS_RESULT_GOT_FIX,
	GPS_RESULT_INTERRUPTED
} gps_result_t;

void gpsSetBaudrate9600Permanently();
void gpsRemoveAllChars(char* str, char c);
int32_t gpsParseLatLong(char* d);
uint8_t gpsConvertTwoDigit2Number(char *digit_char);
//void gpsDecodeGPGGA(char* d, gps_t *gpsData);
//void gpsDecodeGPRMC(char* d, gps_t *gpsData);
void gpsDecodeGPRMCandGPGGA(char* d, gps_t *gpsData, uint8_t sentenceType);
void gpsDecodeGPRMCandGPGGANew(char* d, gps_t *gpsData, uint8_t sentenceType);
bool gpsConfigureBeforeStart(char *messageBuffer);
//gps_result_t gpsStart(gps_t *gpsData, uint16_t minVoltage, uint8_t minHDOP);
uint32_t gpsGetUTCTimestamp(gps_t *gpsData);

#endif /* GPS_L70_LIGHT_H_ */