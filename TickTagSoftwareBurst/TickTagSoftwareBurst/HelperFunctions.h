#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include <stdint.h>

#define _delay_ms_1mhz(a)		_delay_ms(a)
#define _delay_ms_2mhz(a)		_delay_ms(a*2)

uint16_t arrayToUint16(uint8_t *buffer);
uint32_t arrayToUint32(uint8_t *buffer);
int32_t arrayToInt32(uint8_t *buffer);

#endif