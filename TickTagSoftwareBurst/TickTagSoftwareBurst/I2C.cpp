#include "I2C.h"

void i2cInit() {
	TWI0.MBAUD = (uint8_t)I2C_BAUD(100000); // set MBAUD register for 100kHz
	TWI0.MCTRLA = 1 << TWI_ENABLE_bp // Enable TWI Master: enabled
	| 0 << TWI_QCEN_bp // Quick Command Enable: disabled
	| 0 << TWI_RIEN_bp // Read Interrupt Enable: disabled
	| 1 << TWI_SMEN_bp // Smart Mode Enable: enabled
	| TWI_TIMEOUT_DISABLED_gc // Bus Timeout Disabled
	| 0 << TWI_WIEN_bp; // Write Interrupt Enable: disabled
	TWI0.MCTRLB |= TWI_FLUSH_bm; // Purge MADDR and MDATA
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc ; // Force TWI state machine into IDLE state
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm) ;
}

bool pollRIF() {
	uint16_t timeout = TIMEOUT_NUM_WAIT;
	while(!(TWI0.MSTATUS & TWI_RIF_bm)) {
		 _delay_us(TIMEOUT_US_WAIT);
		 timeout--;
		 if(timeout == 0) {
			 return false; // failed
		 }
	}
	return true;
}

bool pollWIF() {
	uint16_t timeout = TIMEOUT_NUM_WAIT;
	while(!(TWI0.MSTATUS & TWI_WIF_bm)) {
		_delay_us(TIMEOUT_US_WAIT);
		timeout--;
		if(timeout == 0) {
			return false; // failed
		}
	}
	return true;	
}

bool i2cStartRead(uint8_t deviceAddr) {
	deviceAddr = (deviceAddr << 1) | 1;  // lsb = 1 for read
	if((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc) { // verify bus is not busy
		TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);
		TWI0.MADDR = deviceAddr;
		//while(!(TWI0.MSTATUS & TWI_RIF_bm)); // read polling
		if(!pollRIF()) { return false; }
		return true;
	}
	return false; // bus is busy
}

bool i2cStartWrite(uint8_t deviceAddr) {
	deviceAddr = (deviceAddr << 1); // lsb = 0 for write
	if((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc) { // verify bus is not busy
		TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);
		TWI0.MADDR = deviceAddr;
		//while(!(TWI0.MSTATUS & TWI_WIF_bm)); // write polling
		if(!pollWIF()) { return false; }
		return true;
	}
	return false; // bus is busy
}

uint8_t i2cRead(bool ack) { // ACK=1 send ACK ; ACK=0 send NACK
	if((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc) { // verify master owns the bus
		//while(!(TWI0.MSTATUS & TWI_RIF_bm)); // wait until RIF set (ack of receiver)
		if(!pollRIF()) { return 0; }
		uint8_t data = TWI0.MDATA;
		if(ack) { TWI0.MCTRLB &= ~(1<<TWI_ACKACT_bp); } // send ack
		else { TWI0.MCTRLB |= (1<<TWI_ACKACT_bp); } // do not send ack, prepare for stop
		return data;
	}
	return 0; // master does not own the bus
}

bool i2cWrite(uint8_t write_data) {
	uint16_t timeout = TIMEOUT_NUM_WAIT;
	if((TWI0.MSTATUS&TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc) { // verify Master owns the bus
		TWI0.MDATA = write_data;
		while(!((TWI0.MSTATUS & TWI_WIF_bm) | (TWI0.MSTATUS & TWI_RXACK_bm))) { // wait until WIF set and RXACK cleared
			_delay_us(TIMEOUT_US_WAIT);
			timeout--;
			if(timeout == 0) {
				return false; // failed
			}
		}
		return true;
	}
	return false; // master does not own the bus
}

void i2cStop() {
	TWI0.MCTRLB |= TWI_ACKACT_NACK_gc;
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}