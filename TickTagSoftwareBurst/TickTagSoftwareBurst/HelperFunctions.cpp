#include "HelperFunctions.h"

uint16_t arrayToUint16(uint8_t *buffer) {
	uint16_t result;
	result = (((uint16_t) buffer[0]) << 8) | (((uint16_t) buffer[1]));
	return result;
}

uint32_t arrayToUint32(uint8_t *buffer) {
	uint32_t result;
	result = (((uint32_t) buffer[0]) << 24) | (((uint32_t) buffer[1]) << 16) | (((uint32_t) buffer[2]) << 8) | (((uint32_t) buffer[3]));
	return result;
}

int32_t arrayToInt32(uint8_t *buffer) {
	uint32_t result;
	result = (((uint32_t) buffer[0]) << 24) | (((uint32_t) buffer[1]) << 16) | (((uint32_t) buffer[2]) << 8) | (((uint32_t) buffer[3]));
	return ((int32_t) result); // cast to signed
}