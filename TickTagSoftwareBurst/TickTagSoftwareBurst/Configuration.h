#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#define TRACKING_MODE_NORMAL								0xDD
#define TRACKING_MODE_TEST									0xFF

/** ------ Important configuration ------ */
/** Important configuration - general */
#define SOFTWARE_VERSION									201
#define DEBUGGING											true		// false, if true forwards RX from GPS to USB breakout board
#define TRACKING_MODE										TRACKING_MODE_NORMAL // TRACKING_MODE_NORMAL or TRACKING_MODE_TEST
#define DOUBLE_CHECK_EEPROM_AFTER_SETTING					true		// false, double check if written value is correct (saves flash memory)
#define LED_ON_AFTER_GETTING_FIRST_TIME_AND_NIGHT			true		// if getting the very first fix (blinking) and is night time, turn LED on for 4 seconds

/** Important configuration - battery management */		
#define MIN_VOLTAGE_UNDER_LOAD								3050U		// unit: mV, threshold for exiting under load! GPS only works down to 3V
#define FIRST_SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT		60U			// 60U (1min), max. 16383 = 4.55hrs, very first under voltage in tracking just sleep a bit, keeping backup power on											
#define SLEEP_TIME_SECONDS_WHEN_VOLTAGE_INCORRECT			1800U		// 1800U (30min), max. 16383 = 4.55hrs
#define FIRST_START_DELAY_SECONDS							3			// 3, debounce a bit when soldering lipo

/** Important configuration - timeout for both modes */	
#define GET_FIX_TIMEOUT_NOT_EVEN_TIME_SECONDS				120			// 120, timeout triggered when did not even get a valid time (year still 80, hour = 0, minute < 3)
#define GET_FIX_SLEEP_LONGER_AFTER_TIMEOUT					true		// true, when timeout was triggered: sleep for GET_FIX_SLEEP_LONGER_AFTER_TIMEOUT_SECONDS instead of normal GPS frequency
#define GET_FIX_SLEEP_LONGER_AFTER_TIMEOUT_SECONDS			900U		// 900U = 15min, sleep for that time when timeout happened, keep backup power on

/** Important configuration - "sometimes mode" (frequency > 5) */	
#define GET_FIX_TIMEOUT_SECONDS								300			// 300, only when tracking frequency > 5 ("sometimes"), timeout until GPS attempt is canceled
#define WAIT_TIME_FIRST_FIX_SECONDS							30			// 30, only when tracking frequency > 5 ("sometimes"), always wait that time when getting first fix in session to collect orbit data
#define MAX_WAIT_ON_GOOD_HDOP_SECONDS						9			// 9, only when tracking frequency > 5 ("sometimes"), maximum wait time after fix in case HDOP is not good enough

/** Important configuration - "always-on mode" (frequency = 1 .. 5) */	
#define USE_FLP_MODE										true		// true, fitness low power mode (only 1hz), technically works in sometimes mode as well, but observed long fix times -> suitable for 1hz mode only

/** Important configuration - night time */
#define SLEEP_TIME_DURING_NIGHT								120U		// 120U (2min), max. 16383 = 4.55hrs: 1min = 1.09uA, 2min = 1.02uA, 3min = 0.99uA, 4min = 0.98uA, 5min = 0.97uA
#define BACKUP_POWER_ON_DURING_NIGHT_TIME					false		// false, keeping backup power on also at night time

/** Important configuration - geo-fencing */
#define GEOFENCING_LIMIT									300			// 300, 300 = roughly 300 meters, difference between 5 digit lat/lon (1 = 1.11m difference, 10 = 11.1m difference, 100 = 111m difference)
#define GEOFENCING_OR_ERROR_WAIT_TIME_SECONDS				600			// 600, when error detected or when geo-fencing active: wait for that amount of time (backup power on) before tracking again

/** ------ Less important stuff ------ */

/** Data storage defines */
#define DATA_FIX_LENGTH										10			// 10 byte compressed (uncompressed: 4 byte timestamp, 4 byte lat, 4 byte lon)
#define METADATA_PREFIX_LENGTH_IN_MEMORY					16			// meta data before actual GPS fix data
#define METADATA_ADDRESS_OF_ADDRESS_POINTER					0			// meta data address 0 - 3, 4 bytes
#define METADATA_ADDRESS_OF_TTF_SUM							4			// meta data address, 4 - 7, 4 bytes
//#define METADATA_ADDRESS_OF_NO_FIX_CNT					8			// NOT USED ANYMORE, meta data address, 8 - 9, 2 bytes
#define METADATA_ADDRESS_OF_HDOP_SUM						10			// meta data address, 10 - 13, 4 bytes
// 2 BYTES LEFT IN HEADER!

/** Configuration menue (starting at 18 because structure changed) */
#define EEPROM_OPERATION_VOLTAGE_MIN_ADDRESS				18			// 2 bytes for storing min operation voltage
#define EEPROM_GPS_FREQUENCY_SECONDS_ADDRESS				20			// NEW: 2 bytes instead of 4 bytes for storing GPS frequency in seconds
#define EEPROM_MIN_HDOP_ADDRESS								22			// 1 byte for storing min HDOP
#define EEPROM_ACTIVATION_DELAY_SECONDS_ADDRESS				23			// 2 bytes instead of 4 bytes for storing activation delay in seconds
#define EEPROM_HOUR_ON_ADDRESS								25			// 1 byte for storing turn on hour
#define EEPROM_MINUTE_ON_ADDRESS							26			// 1 byte for storing turn on minute
#define EEPROM_HOUR_OFF_ADDRESS								27			// 1 byte for storing turn off hour
#define EEPROM_MINUTE_OFF_ADDRESS							28			// 1 byte for storing turn off minute
#define EEPROM_GEOFENCING_ADDRESS							29			// 1 byte for storing geofencing on/off
#define EEPROM_BLINK_ADDRESS								30			// 1 byte for storing blinking on/off
#define EEPROM_BURST_DURATION_ADDRESS						31			// 1 byte for storing burst duration

/** Configuration defaults */
#define OPERATION_VOLTAGE_DEFAULT							3300U		// unit: mV, threshold for idle (NOT under load, 100mV voltage drop normal)
#define OPERATION_VOLTAGE_MIN_RANGE							3000U		// unit: mV, threshold for idle (NOT under load, 100mV voltage drop normal)
#define OPERATION_VOLTAGE_MAX_RANGE							4250U		// unit: mV, threshold for idle (NOT under load, 100mV voltage drop normal)

#define GPS_FREQUENCY_SECONDS_DEFAULT						30U			// unit: seconds
#define GPS_MIN_FREQUENCY_SECONDS_RANGE						1U			// unit: seconds
#define GPS_MAX_FREQUENCY_SECONDS_RANGE						16382U		// unit: seconds, 16382UL = 4.55 hrs

#define MIN_HDOP_DEFAULT									30U			// unit: HDOP x 10
#define MIN_HDOP_MIN_RANGE									10U			// unit: HDOP x 10
#define MIN_HDOP_MAX_RANGE									250U		// unit: HDOP x 10

#define ACTIVATION_DELAY_DEFAULT							10U			// unit: seconds
#define ACTIVATION_MIN_DELAY_SECONDS_RANGE					10U			// unit: seconds
#define ACTIVATION_MAX_DELAY_SECONDS_RANGE					16382U		// unit: seconds, 16382UL = 4.55 hrs

#define HOUR_ON_DEFAULT										0			// unit: hours 0 - 23
#define MINUTE_ON_DEFAULT									0			// unit: minutes 0 - 59
#define HOUR_OFF_DEFAULT									23			// unit: hours 0 - 23
#define MINUTE_OFF_DEFAULT									59			// unit: minutes 0 - 59

#define GEOFENCING_DEFAULT									0			// 0 = off, 1 = on

#define BLINK_DEFAULT										0			// 0 = off, 1 = on, short blink during fixes (NOTE: first fix for getting time will blink anyhow)

#define BURST_DURATION_DEFAULT								0			// unit: seconds, 0 = 1 = no burst

/** Enums */
typedef enum { ST_FIRST_START_HARD_RESET = 0, ST_TRACKING = 1, ST_VOLTAGE_INCORRECT = 2, ST_MEMORY_FULL = 3, ST_WAIT_FOR_ACTIVATION = 4, ST_DOWNLOAD = 5 } tracker_state_t;
typedef enum { STORE_MEMORY_RETURN_ERROR = 0, STORE_MEMORY_RETURN_MEMORY_FULL = 1, STORE_MEMORY_RETURN_SUCCESS = 2 } store_memory_return_t;
typedef enum { CONTINOUS_RUNNING = 0, CONTINOUS_UNDERVOLTAGE, CONTINOUS_MEMORY_FULL, CONTINOUS_DOWNLOAD_INTERRUPT, CONTINOUS_INIT_ERROR, CONTINOUS_OTHER_ERROR, CONTINOUS_NIGHT_TIME } continous_result_t;
typedef enum { TRACKING_RUNNING = 0, TRACKING_UNDERVOLTAGE, TRACKING_MEMORY_FULL, TRACKING_DOWNLOAD_INTERRUPT, TRACKING_ERROR, TRACKING_NIGHT_TIME, TRACKING_GOT_FIX_SLEEP, TRACKING_TIMEOUT, TRACKING_GEOFENCING_ACTIVE } tracking_result_t;

#define COMPRESSION_TIMESTAMP								1618428394UL

#endif