#include "TickTag.h"

//extern volatile static uint32_t millisElapsed;

void deviceInitPins() {
	// OUTPUT = lowest setting
	PORTA.DIR = (INPUT_DIR << PIN0_bp)	// PA0: UPDI
	| (OUTPUT_DIR << PIN1_bp)			// PA1: TXD2
	| (OUTPUT_DIR << PIN2_bp)			// PA2: RXD2
	| (OUTPUT_DIR << PIN3_bp)			// PA3: POWER_L70_BCK
	| (OUTPUT_DIR << PIN4_bp)			// PA4: **UNUSED**
	| (INPUT_DIR << PIN5_bp)			// PA5: DOWNLOAD (NEW: changed from OUTPUT to INPUT)
	| (OUTPUT_DIR << PIN6_bp)			// PA6: TIMER
	| (OUTPUT_DIR << PIN7_bp);			// PA7: **UNUSED**	
	PORTB.DIR = (OUTPUT_DIR << PIN0_bp)	// PB0: SCL
	| (OUTPUT_DIR << PIN1_bp)			// PB1: SDA
	| (OUTPUT_DIR << PIN2_bp)			// PB2: TXD
	| (INPUT_DIR << PIN3_bp)			// PB3: RXD
	| (OUTPUT_DIR << PIN4_bp)			// PB4: POWER_MEMORY
	| (OUTPUT_DIR << PIN5_bp);			// PB5: LED_GREEN
	PORTC.DIR = (OUTPUT_DIR << PIN0_bp)	// PC0: **UNUSED**
	| (OUTPUT_DIR << PIN1_bp)			// PC1: GPIO2 (A/ST)
	| (OUTPUT_DIR << PIN2_bp)			// PC2: GPIO1
	| (OUTPUT_DIR << PIN3_bp);			// PC3: POWER_L70_ON
	
	#if defined (__AVR_ATtiny1626__)
	// doesn't work
	//PORTA.PIN1CTRL = (0 << PORT_INVEN_bp) | (0 << PORT_PULLUPEN_bp) | (1 << PORT_ISC2_bp); // not inverted, no pull-up, disable input buffer (can't read input values), IMPORTANT, otherwise +30uA leakage
	//PORTA.PIN2CTRL = (0 << PORT_INVEN_bp) | (0 << PORT_PULLUPEN_bp) | (1 << PORT_ISC2_bp);
	#endif
	
	// unconnected pins: no pull-up, disable digital input buffer (can't measure input value) -> reduces current by around 1uA per pin in active (not sleeping)
	/*PORTA.PIN4CTRL = UNUSED_PIN_SETTING;
	PORTA.PIN7CTRL = UNUSED_PIN_SETTING;
	PORTC.PIN0CTRL = UNUSED_PIN_SETTING;
	PORTC.PIN1CTRL = UNUSED_PIN_SETTING;
	PORTC.PIN2CTRL = UNUSED_PIN_SETTING;*/
	
	// enable PULL-UPs
	//PORTA.PIN6CTRL |= PORT_PULLUPEN_bm;
	PORTA.PIN5CTRL = PORT_PULLUPEN_bm; // NEW: pullup enabled already in start
	
	// 67uA less @1MHz 3.6V, TCD shall use prescaled clock, not "raw OSC20M", divider is not relevant here, TCD disabled (bit 0)
	#if defined (__AVR_ATtiny1616__)
		TCD0.CTRLA = TCD_CLKSEL_SYSCLK_gc | TCD_CNTPRES_DIV1_gc; // only for ATTINY1616, not for ATTINY1606 or ATTINY1626
	#else
		// don't do it
	#endif
}

void deviceInitGPIO2PinInterruptInDeepSleep() {
	// REQUIRES IN MAIN
	/*
	ISR(PORTC_PORT_vect) { PORTC.INTFLAGS |= PORT_INT1_bm; }
	*/
	PORTC.DIR &= ~PIN1_bm; // set pin as input
	PORTC.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc; // enable pull-up and interrupt
}

void deviceDeactivateGPIO2PinInterruptInDeepSleep() {
	PORTC.PIN1CTRL = 0; // disable pull-up and interrupt
	PORTC.DIR |= PIN1_bm; // set as output
}

void deviceInitDownloadPinInterruptInDeepSleep() {
	// REQUIRES IN MAIN
	/*
	ISR(PORTA_PORT_vect) { PORTA.INTFLAGS |= PORT_INT5_bm; }
	*/
	PORTA.PIN5CTRL |= PORT_ISC_BOTHEDGES_gc; // enable interrupt
}

volatile static uint32_t millisElapsed; // volatile because access in ISR

void deviceIncrementTimer() {
	millisElapsed++;
}

void deviceInitTimer() {
	// NEEDS IMPLEMENTATION OF THAT FUNCTION IN MAIN:
	/*
	ISR(TCA0_OVF_vect) {
		deviceIncrementTimer();
		TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear the interrupt flag (to reset TCA0.CNT)
	}
	*/
	millisElapsed = 0;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; // enable overflow interrupt
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // set Normal mode
	#if defined (__AVR_ATtiny1626__)
		// don't do it
	#else
		TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTEI_bm); // disable event counting
	#endif
	TCA0.SINGLE.PER = TIMER_TOP;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;
	sei();	
}

uint32_t deviceGetMillis() {
	uint32_t temp;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { temp = millisElapsed; }
	return temp;
}

void deviceStopTimer() {
	cli();
	TCA0.SINGLE.INTCTRL = 0;
	TCA0.SINGLE.CTRLA = 0; // disable timer in case it's running
	sei();	
}

void deviceL70powerOn() {
	//PORTC.DIR |= PIN3_bm; // output
	PORTC.OUTSET = PIN3_bm; // high
}

void deviceL70powerOff() {
	//PORTC.DIR |= PIN3_bm; // output
	PORTC.OUTCLR = PIN3_bm; // low
}

void deviceL70backupOn() {
	//PORTA.DIR |= PIN3_bm; // output
	PORTA.OUTSET = PIN3_bm; // high
}

void deviceL70backupOff() {
	//PORTA.DIR |= PIN3_bm; // output
	PORTA.OUTCLR = PIN3_bm; // low
}

void devicePowerMemoryAndI2COff() {
	////PORTB.DIR |= PIN4_bm; // output
	PORTB.OUTCLR = PIN4_bm; // low
}

void devicePowerMemoryAndI2COn() {
	////PORTB.DIR |= PIN4_bm; // output
	PORTB.OUTSET = PIN4_bm; // high
	_delay_ms(1); // according to datasheet: memory ready after 0.1ms
}

void deviceDisableBODInSleep() {
	// TODO: set registers in one step (setting CCP only once) -> use _PROTECTED_WRITE()?
	CCP = 0xD8; 								// configuration change protection -> 4 instructions time for change
	BOD.CTRLA &= ~(1 << BOD_SLEEP0_bp); 		// set 0 (disabled)
	CCP = 0xD8;									// configuration change protection -> 4 instructions time for change
	BOD.CTRLA &= ~(1 << BOD_SLEEP1_bp); 		// set 0 (disabled)
}

void deviceDeepSleep() {
	deviceStopTimer(); // in case it was running
	
	// disable I2C pull-ups (important, being set by I2C)
	PORTB.PIN0CTRL = 0;
	PORTB.PIN1CTRL = 0;
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN); 		// deep sleep
	power_all_disable();   						// power off ADC, Timer 0 and 1, serial interface
	cli(); 										// disable interrupts for CCP = noInterrupts() function in Arduino
	sleep_enable();								// enable

	//disableBODInSleep();						// disable BOD totally (if not already set by fuses)

	sei();           							// enable interrupts again = interrupts() function in Arduino
	sleep_cpu();            					// stop
	sleep_disable();        					// wake again
	power_all_enable();     					// power everything back on
}

void deviceStandbySleep() {
	deviceStopTimer(); // in case it was running
	
	// disable I2C pull-ups (important, being set by I2C)
	PORTB.PIN0CTRL = 0;
	PORTB.PIN1CTRL = 0;
	
	set_sleep_mode(SLEEP_MODE_STANDBY); 		// standby sleep
	sleep_enable();								// enable
	sleep_cpu();            					// stop
}

void deviceInitInternalRTCInterrupt(uint16_t seconds) {
	// needs implementation of interrupt routine and can only wakeup from standby sleep (RTC running):
	//ISR(RTC_CNT_vect) { RTC.INTFLAGS = RTC_OVF_bm; }
	// accuracy depends on PRESCALER -> smaller = better, but then max seconds smaller
	// currently (256 prescaler = 4Hz CNT updates): min. 0.25s - max. 2^16 / 4 = 16384 seconds = 4.55hrs
	cli();												// disable global interrupts
	while(RTC.STATUS > 0) {} 							// wait for all register to be synchronized
	RTC.CNT = 0;										// reset counter value
	while(RTC.STATUS > 0) {}							// wait until CNT is reset
	RTC.PER = seconds * 4; 								// 16 bit wide maximum value, sets seconds between wakes (maximum value of cnt)
	RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; 					// running at 1.024Hz
	RTC.CTRLA = RTC_PRESCALER_DIV256_gc 				// prescaler, e.g. set to 256 -> 1024 / 256 = incrementing CNT with 4Hz
		| 1 << RTC_RTCEN_bp         					// enable RTC
		| 1 << RTC_RUNSTDBY_bp;     					// run in standby, increases standby power consumption
	RTC.INTCTRL = 1 << RTC_OVF_bp; 						// overflow interrupt
	sei();												// enable global interrupts			
}

#if defined (__AVR_ATtiny1626__)
int16_t getInternalTemperature() {
	uint16_t adc_reading;
	uint16_t temperature_in_K;
	int16_t temperature_in_degC;
	
	ADC0.CTRLA = ADC_ENABLE_bm;
	ADC0.CTRLB = ADC_PRESC_DIV2_gc; // fCLK_ADC = 3.333333/2 MHz
	ADC0.CTRLC = ADC_REFSEL_1024MV_gc | (TIMEBASE_VALUE << ADC_TIMEBASE_gp);
	ADC0.CTRLE = TEMPSENSE_SAMPDUR;

	ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc; // ADC Internal Temperature Sensor
	ADC0.COMMAND = ADC_MODE_SINGLE_12BIT_gc; // Single 12-bit mode

	int8_t sigrow_offset = SIGROW.TEMPSENSE1; // Read signed offset from signature row
	uint8_t sigrow_gain = SIGROW.TEMPSENSE0; // Read unsigned gain/slope from signature row

	ADC0.COMMAND |= ADC_START_IMMEDIATE_gc; // Start ADC conversion
	while(!(ADC0.INTFLAGS & ADC_RESRDY_bm)); // Wait until conversion is done

	// Calibration compensation as explained in the data sheet
	adc_reading = ADC0.RESULT >> 2; // 10-bit MSb of ADC result with 1.024V internal reference
	uint32_t temp = adc_reading - sigrow_offset;
	
	temp *= sigrow_gain; // Result might overflow 16-bit variable (10-bit + 8-bit)
	temp += 0x80; // Add 256/2 to get correct integer rounding on division below
	temp >>= 8; // Divide result by 256 to get processed temperature in Kelvin
	temperature_in_K = temp;
	temperature_in_degC = temperature_in_K - 273;
	
	ADC0.CTRLA = 0; // disable ADC
	return temperature_in_degC;
}
#endif

uint16_t deviceReadSupplyVoltage() {
	#if defined (__AVR_ATtiny1626__)
		uint32_t res;
	
		ADC0.CTRLA = ADC_ENABLE_bm; // enable ADC
		//ADC0.CTRLB = ADC_PRESC_DIV2_gc; // DEFAULT setting
		ADC0.CTRLC = VREF_AC0REFSEL_1V024_gc | (TIMEBASE_VALUE << ADC_TIMEBASE0_bp); // Vref = 1.024V
		ADC0.CTRLE = 100; // sample duration ((100 * 2) / F_CPU seconds), 1 MHz = 0.2ms
		ADC0.MUXPOS = ADC_MUXPOS_DAC_gc; // using DAC as MUX voltage, ADC_MUXPOS_VDDDIV10_gc doesn't work
		ADC0.COMMAND = ADC_MODE_SINGLE_12BIT_gc; // single mode with 12 bit
		ADC0.COMMAND |= ADC_START_IMMEDIATE_gc; // start conversion

		while(true) {
			if(ADC0.INTFLAGS & ADC_RESRDY_bm) { // wait until measurement done
				res = (uint32_t) ADC0.RESULT; // get raw adc result
				if(res == 0) { res = 1; }
				res = (4096UL * 1024UL) / res; // convert result to mV
				break;
			}
		}
		ADC0.CTRLA = 0; // disable ADC
		return ((uint16_t) res);
	#else
		// the App Note AN2447 uses Atmel Start to configure Vref but we'll do it explicitly in our code
		VREF.CTRLA = VREF_ADC0REFSEL_1V1_gc;  			// set the Vref to 1.1V

		// the following section is directly taken from Microchip App Note AN2447 page 13     
		ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc; 	   		// ADC internal reference, the Vbg
    
		// take care! ADC needs certain peripheral clock speed (50kHz - 1.5MHz after ADC_PRESC_..) for maximum resolution, here CLOCK = 1MHz divided by 4 = 0.25MHz = within range of 50kHz - 1.5MHz
		ADC0.CTRLC = ADC_PRESC_DIV4_gc					// CLK_PER divided by certain value
			| ADC_REFSEL_VDDREF_gc						// Vdd (Vcc) be ADC reference
			| 0 << ADC_SAMPCAP_bp;						// Sample Capacitance Selection: disabled
		ADC0.CTRLA = 1 << ADC_ENABLE_bp					// ADC Enable: enabled
			| 1 << ADC_FREERUN_bp						// ADC Free run mode: enabled
			| ADC_RESSEL_10BIT_gc;						// 10-bit mode
		//ADC0.CTRLD = ADC_INITDLY_DLY256_gc;			// init delay 256 CLK_ADC cycles (important!)
		ADC0.COMMAND |= 1;								// start running ADC
		_delay_ms(5);									// REALLY?! <- YES, important to wait for first measurement!
	
		#if (VCC_DO_NOT_USE_AVERAGE == true)
			uint32_t vcc, temp;
			while(true) {
				if(ADC0.INTFLAGS) {
					temp = (uint32_t) ADC0.RES;
					if(temp == 0) { temp = 1; }
					vcc = (1024UL * 1100UL) / temp;
					break;
				}
			}
		#else
			// read some samples and calculate average
			uint32_t vcc = 0, temp = 0;
			for(uint8_t i=0; i<VCC_SAMPLES; i++) {
				if(ADC0.INTFLAGS) {							// if an ADC result is ready
					temp = (uint32_t) ADC0.RES;				// e.g. 450 for 2.5V
					if(temp == 0) { temp = 1; }
					vcc += (1024UL * 1100UL) / temp; 		// 1100 = 1.1V
					_delay_ms(VCC_SAMPLE_DELAY_MS);
				}		
			}
			vcc /= VCC_SAMPLES;								// build integer average
		#endif
		// disable ADC again
		ADC0.CTRLA = 0 << ADC_ENABLE_bp					// ADC Enable: enabled (seems okay) -> no, changed that to 0! should be disabled!
			| 0 << ADC_FREERUN_bp						// ADC Free run mode: disabled (important for sleep)
			| ADC_RESSEL_10BIT_gc;						// 10-bit mode
			 
		#if defined (__AVR_ATtiny1606__)
			if(vcc >= 60) { vcc = vcc - 60; } // 1606 ADC seems to measure voltage higher than expected (only tested with one board)
		#endif
		          
		return (uint16_t) vcc;
	#endif
}

void deviceSetCPUSpeed32khz() {
	// after settings speed to 32KHz all delays are wrong by a factor of 1MHz / 32kHz = 31UL, use _delay_ms(Helper::to32KHz(1000));
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | 0 << CLKCTRL_PEN_bp); // set the main clock prescaler divisor to 2X and disable the Main clock prescaler (with PEN = 0, means CLK_PER = CLK_MAIN (prescaler disabled))
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCULP32K_gc | 0 << CLKCTRL_CLKOUT_bp); // set the main clock to internal 32kHz oscillator, clock out disabled
	_PROTECTED_WRITE(CLKCTRL.OSC20MCTRLA, 0x00); // ensure 20MHz isn't forced on
	while(CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm) { ; } // wait until clock changed	
}

void deviceSetCPUSpeed(uint8_t prescalerDivision) {
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, prescalerDivision | 1 << CLKCTRL_PEN_bp); // first bit = prescaler, always enabled
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, 0); // use internal 16/20MHz oscillator
	while(CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm) { ; } // wait until clock changed
}

void deviceLedOn() {
	//PORTB.DIR |= PIN5_bm; // output
	PORTB.OUTSET = PIN5_bm; // on
}

void deviceLedOff() {
	//PORTB.DIR |= PIN5_bm; // output, important, otherwise leakage in sleep
	PORTB.OUTCLR = PIN5_bm; // off
}

void deviceBlink(uint8_t times) {
	while(times > 0) {
		deviceLedOn();
		_delay_ms(200);
		deviceLedOff();
		if(times != 1) { _delay_ms(200); }
		times--;
	}
}






