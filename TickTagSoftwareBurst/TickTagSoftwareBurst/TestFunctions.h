#ifndef TEST_H_
#define TEST_H_

#include "TickTag.h"
#include "I2C.h"
#include "UART.h"
#include "EEPROM_CAT24M01.h"
#include "GPS_L70_Light.h"

void mockEmptyMemory(uint32_t dataFixLength, uint32_t metaAddressOfAddressPointer);
void mockFullMemory(uint32_t dataFixLength, uint32_t metaAddressOfAddressPointer);
void testMessage();
void testWriteMemory();
void testMemory();
void i2cTest();
void i2cTest2();
void lightSleepUARTTest();
void newParserTest();
void flpTest();
void readVoltage1626Test();
void readVoltage1606Test();


#endif /* TEST_H_ */