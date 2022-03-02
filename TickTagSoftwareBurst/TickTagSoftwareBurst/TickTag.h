#ifndef TickTag_h
#define TickTag_h

// AVR libs
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/atomic.h>

/** TODO LIST */
// TODO: read temperature, chapter 30.3.2.6
// TODO: tryout disabling ADC0 and see if smaller current during active

/** VCC MEASUREMENT */
#define TIMEBASE_VALUE					(uint8_t)ceil(F_CPU*0.000001) // only for ATTINY1626
#define VCC_DO_NOT_USE_AVERAGE			true	// don't average vcc, just take one measurement
#define VCC_SAMPLES						3		// number of samples
#define VCC_SAMPLE_DELAY_MS				1		// delay between samples

/** TEMPERATURE MEASUREMENT */
#define TEMPSENSE_SAMPDUR				((uint8_t) ceil(F_CPU*0.000032/2)) // only for ATTINY1626, SAMPDUR for TEMPSENSE must be >= 32 µs * f_ADC ~= 32 µs * 1.67 MHz ~= 54

/** CPU SPEED */
// CPU speed after writing OSCCFG fuse to use 20MHz (factory default)
#define OSC20_PRESCALER_CPU_0_315MHZ			CLKCTRL_PDIV_64X_gc		// based on 20MHz fuse setting (not 16MHz) -> 20/64 = 0.3125 MHz
#define OSC20_PRESCALER_CPU_0_625MHZ			CLKCTRL_PDIV_32X_gc		// see above
#define OSC20_PRESCALER_CPU_0_833MHZ			CLKCTRL_PDIV_24X_gc		// see above
#define OSC20_PRESCALER_CPU_1_25MHZ				CLKCTRL_PDIV_16X_gc		// see above
#define OSC20_PRESCALER_CPU_2_5MHZ				CLKCTRL_PDIV_8X_gc		// see above
#define OSC20_PRESCALER_CPU_5MHZ				CLKCTRL_PDIV_4X_gc		// see above
#define OSC20_PRESCALER_CPU_10MHZ				CLKCTRL_PDIV_2X_gc		// see above

// CPU speed after writing OSCCFG fuse to use 16MHz instead of default 20MHz (avrdude -c jtag2updi -P com4 -p t1616 -C ..\etc\avrdude.conf -U fuse2:w:0b00000001:m)
// power consumption values for: 3V, 1616-only (special board), TCD0 fix, BOD fully disabled, while(1)
// _delay_ms needs more power than while(1) (!)
#define OSC16_PRESCALER_CPU_0_25MHZ				CLKCTRL_PDIV_64X_gc		// 329uA
#define OSC16_PRESCALER_CPU_0_5MHZ				CLKCTRL_PDIV_32X_gc		
#define OSC16_PRESCALER_CPU_0_666MHZ			CLKCTRL_PDIV_24X_gc		
#define OSC16_PRESCALER_CPU_1MHZ				CLKCTRL_PDIV_16X_gc		// 556uA (16MHz/16)
#define OSC16_PRESCALER_CPU_1_66MHZ				CLKCTRL_PDIV_10X_gc
#define OSC16_PRESCALER_CPU_2MHZ				CLKCTRL_PDIV_8X_gc		// 859uA (UART @115200 works here and above) - 842uA (real board + sampled BOD 125Hz)
#define OSC16_PRESCALER_CPU_2_66MHZ				CLKCTRL_PDIV_6X_gc		// 1.07mA (default value at start-up )
#define OSC16_PRESCALER_CPU_4MHZ				CLKCTRL_PDIV_4X_gc

/** PIN SETTINGS */
#define UNUSED_PIN_SETTING						(0 << PORT_INVEN_bp) | (0 << PORT_PULLUPEN_bp) | (1 << PORT_ISC2_bp) // not inverted, no pull-up, disable input buffer (can't read input values)
#define OUTPUT_DIR								1
#define INPUT_DIR								0

/** TIMER SETTINGS */
#define TIMER_TOP								(F_CPU / 1000 - 1)		// overflow after 1 ms

// WARNING: RXD2 CAN POWER WHOLE DEVICE!

void deviceInitPins();										// (old: @2.7V: 780uA (1MHz, BOD enabled))
void deviceDeepSleep(); 									// @2.7V: 1.6uA sampled BOD (1kHz, 125Hz should be lower, but needs to be fused) in sleep, 450nA BOD disabled (DEFAULT), 3uA with Power-ON
void deviceStandbySleep();									// (old: @2.7V: 920nA with internal RTC running and BOD disabled)
void deviceInitInternalRTCInterrupt(uint16_t seconds);		// for standby sleep
void deviceInitDownloadPinInterruptInDeepSleep();
void deviceInitGPIO2PinInterruptInDeepSleep();
void deviceDeactivateGPIO2PinInterruptInDeepSleep();
uint16_t deviceReadSupplyVoltage();	// minimum 100kHz CPU clock speed and after prescaler between 50kHz and 1.5MHz (for max resolution), in V*1000 (2700 = 2.7V)

// Read temperature
#if defined (__AVR_ATtiny1626__)
int16_t getInternalTemperature();
#endif
// Measure time
void deviceInitTimer();
uint32_t deviceGetMillis();
void deviceIncrementTimer();
		
// Power management
void deviceL70powerOn();
void deviceL70powerOff();
void deviceL70backupOn();
void deviceL70backupOff();
void devicePowerMemoryAndI2COff();
void devicePowerMemoryAndI2COn();
		
// CPU speeds
void deviceSetCPUSpeed32khz();							// VERY SLOW! @3V: 38uA (update!) -> general calculations will be very very slow
void deviceSetCPUSpeed(uint8_t prescalerDivision);
		
// LED
void deviceLedOn(); 					
void deviceLedOff();
void deviceBlink(uint8_t times);
	
void deviceDisableBODInSleep();

// STRUCTS FOR EEPROM ACCESS
struct TinyEEPROM {
    TinyEEPROM(const int index)
        : index( index )                 {}
    
    // Access/read members.
    uint8_t operator*() const            { return eeprom_read_byte( (uint8_t*) index ); }
    operator uint8_t() const             { return **this; }
    
    // Assignment/write members.
    TinyEEPROM &operator=( const TinyEEPROM &ref ) { return *this = *ref; }
    TinyEEPROM &operator=( uint8_t in )       { return eeprom_write_byte( (uint8_t*) index, in ), *this;  }
    TinyEEPROM &operator +=( uint8_t in )     { return *this = **this + in; }
    TinyEEPROM &operator -=( uint8_t in )     { return *this = **this - in; }
    TinyEEPROM &operator *=( uint8_t in )     { return *this = **this * in; }
    TinyEEPROM &operator /=( uint8_t in )     { return *this = **this / in; }
    TinyEEPROM &operator ^=( uint8_t in )     { return *this = **this ^ in; }
    TinyEEPROM &operator %=( uint8_t in )     { return *this = **this % in; }
    TinyEEPROM &operator &=( uint8_t in )     { return *this = **this & in; }
    TinyEEPROM &operator |=( uint8_t in )     { return *this = **this | in; }
    TinyEEPROM &operator <<=( uint8_t in )    { return *this = **this << in; }
    TinyEEPROM &operator >>=( uint8_t in )    { return *this = **this >> in; }
    
    TinyEEPROM &update( uint8_t in )          { return  in != *this ? *this = in : *this; }
    
    // Prefix increment/decrement
    TinyEEPROM& operator++()                  { return *this += 1; }
    TinyEEPROM& operator--()                  { return *this -= 1; }
    
    // Postfix increment/decrement
    uint8_t operator++ (int){ 
        uint8_t ret = **this;
        return ++(*this), ret;
    }

    uint8_t operator-- (int){ 
        uint8_t ret = **this;
        return --(*this), ret;
    }
    int index; // Index of current EEPROM cell.
};

struct EEPtr{
    EEPtr( const int index )
        : index( index )                {}
        
    operator int() const                { return index; }
    EEPtr &operator=( int in )          { return index = in, *this; }
    
    // Iterator functionality.
    bool operator!=( const EEPtr &ptr ) { return index != ptr.index; }
    TinyEEPROM operator*()                   { return index; }
    
    // Prefix & Postfix increment/decrement
    EEPtr& operator++()                 { return ++index, *this; }
    EEPtr& operator--()                 { return --index, *this; }
    EEPtr operator++ (int)              { return index++; }
    EEPtr operator-- (int)              { return index--; }

    int index; // Index of current EEPROM cell.
};

#endif
