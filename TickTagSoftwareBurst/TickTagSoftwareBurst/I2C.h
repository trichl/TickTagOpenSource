#ifndef I2C_H_
#define I2C_H_

// AVR libs
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define I2C_BAUD(F_SCL)						((((float)F_CPU / (float)F_SCL)) - 10) // IMPORTANT: needs to by >= 0 to work -> F_CPU = 1MHz -> max. 100kHz
#define TIMEOUT_US_WAIT						10
#define TIMEOUT_NUM_WAIT					5000 // ~50ms, 2 bytes

bool pollRIF();
bool pollWIF();

void i2cInit();
bool i2cStartRead(uint8_t deviceAddr);
bool i2cStartWrite(uint8_t deviceAddr);
uint8_t i2cRead(bool ack);
bool i2cWrite(uint8_t write_data);
void i2cStop();

#endif /* I2C_H_ */