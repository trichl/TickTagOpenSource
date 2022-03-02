#ifndef EEPROM_CAT24M01_h
#define EEPROM_CAT24M01_h

#include <stdint.h>
#include "I2C.h"
#include "UART.h"

#define EEPROM_CAT24M01_ADDRESS					0x50		// first page
#define EEPROM_CAT24M01_ADDRESS_2				0x51 		// second page
#define EEPROM_CAT24M01_MEM_SIZE				131072UL	// 17 bit
#define EEPROM_CAT24M01_PAGE_SIZE				256			// important for write-wrapping
#define EEPROM_CAT24M01_MEM_VAL_UNWRITTEN		0xFF
#define EEPROM_CAT24M01_MAX_WRITE_LENGTH		256			// CAT24M01 supports 256

bool eepromIsAlive();
bool eepromReadMemory(uint32_t adr, uint8_t *result_arr, uint32_t len);
bool eepromWriteMemory(uint32_t adr, uint8_t *value_arr, uint32_t len); // uses writeMemoryInternal
bool eepromBusyWriting();
uint8_t eepromIGetI2CAddress(uint32_t adr);

#endif
