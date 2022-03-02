#include "EEPROM_CAT24M01.h"

bool eepromIsAlive() {
	if(!i2cStartRead(EEPROM_CAT24M01_ADDRESS)) {
		i2cStop();
		return false;
	}
	i2cStop();
	return true;
}

uint8_t eepromGetI2CAddress(uint32_t adr) {
	if(adr < EEPROM_CAT24M01_MEM_SIZE / 2) { // 2^16, use first page
		return EEPROM_CAT24M01_ADDRESS;
	}
	else { // use second page (different I2C address)
		return EEPROM_CAT24M01_ADDRESS_2;
	}	
}

bool eepromReadMemory(uint32_t adr, uint8_t *result_arr, uint32_t len) {
	// read wraps around automatically
	if(adr + len > EEPROM_CAT24M01_MEM_SIZE) {
		return false;
	}
	uint8_t i2cAddress = eepromGetI2CAddress(adr);
	
	if(!i2cStartWrite(i2cAddress)) { return false; }
	if(!i2cWrite((adr >> 8) & 0xff)) { return false; }
	if(!i2cWrite((adr >> 0) & 0xff)) { return false; }
	
	if(!i2cStartRead(i2cAddress)) { return false; }
	uint32_t i = 0;
	while(i < (len - 1)) {
		result_arr[i] = i2cRead(true); // with ack
		i++;
	}
	result_arr[i] = i2cRead(false); // last one: no ack
	i2cStop();
	return true;
}

bool eepromBusyWriting() {
	uint8_t val = 0x0;
	if(!eepromReadMemory(0, &val, 1)) { return true; }
	return false; // ACKed the command
}

bool eepromWriteMemory(uint32_t adr, uint8_t *value_arr, uint32_t len) {
	uint16_t protectionCount = 6000U;
	uint16_t pageStillFits;
	uint16_t oneWriteCycleLen;
	bool finished = false;
	uint32_t arrayIndex = 0;
	uint16_t busyWritingTimeoutMs;
	
	if((adr + len > EEPROM_CAT24M01_MEM_SIZE) || (len == 0)) {
		return false;
	}
	
	while(!finished) {
		// calculate how much to write
		pageStillFits = (uint16_t) (EEPROM_CAT24M01_PAGE_SIZE - (adr % EEPROM_CAT24M01_PAGE_SIZE)); // warning: page size maximum 2^16 = 65536
		if(pageStillFits > EEPROM_CAT24M01_MAX_WRITE_LENGTH) { // page still fits more than 256 Bytes
			if(len <= EEPROM_CAT24M01_MAX_WRITE_LENGTH) { // all remaining data can go into the next 256 Bytes
				oneWriteCycleLen = len;
				finished = true;
			}
			else { // write 256 Bytes, but next iteration necessary
				oneWriteCycleLen = EEPROM_CAT24M01_MAX_WRITE_LENGTH;
			}
		}
		else { // page fits less than 256 Bytes, cannot write full length
			if(len <= pageStillFits) { // all remaining data can go into less than 30 Bytes
				oneWriteCycleLen = len;
				finished = true;
			}
			else { // write until page full, but next iteration necessary
				oneWriteCycleLen = pageStillFits; // warning, assigning 16bit integer to 8bit, should be fine because pageStillFits <= 256
			}
		}
		
		// write
		//printf("WRITE %lu\n\r", adr);
		uint8_t i2cAddress = eepromGetI2CAddress(adr);
		if(!i2cStartWrite(i2cAddress)) { return false; }
		if(!i2cWrite((adr >> 8) & 0xff)) { return false; }
		if(!i2cWrite((adr >> 0) & 0xff)) { return false; }
		for(uint16_t i = 0; i < oneWriteCycleLen; i++) { // append data
			if(!i2cWrite(value_arr[arrayIndex+i])) { return false; }
		}
		i2cStop();
		
		// wait until written
		busyWritingTimeoutMs = 1000;
		while(eepromBusyWriting()) {
			_delay_ms(1);
			busyWritingTimeoutMs--;
			if(busyWritingTimeoutMs == 0) { return false; }
		}
		//printf("TO %d\n\r", busyWritingTimeoutMs);
		
		// decrement for next cycle
		len -= oneWriteCycleLen;
		adr += oneWriteCycleLen;
		arrayIndex += oneWriteCycleLen;
		protectionCount--;
		if(protectionCount == 0) { // timeout
			return false;
		}
	}
	return true;
}