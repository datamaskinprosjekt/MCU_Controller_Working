#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DDR_SPI DDRB
#define DD_MISO DDB4

#define INPUT_PIN PINC
#define INPUT_PORT PORTC
#define INPUT_DDR DDRC
#define SWA PC4
#define SWB PC3
#define SWX PC2
#define SW_BITMASK ((1 << SWA) | (1 << SWB) | (1 << SWX))

#define CS PD2

#define REFS_BM 0b11000000

#define ADMUX_BM 0b00001111
#define ADMUX_HORIZONTAL 0b0000
#define ADMUX_VERTICAL   0b0001

void setup_test () {
	DDRD |= (1<<PD5);
}

void setup_spi () {
	DDR_SPI |= (1 << DD_MISO);
	SPCR |= (1 << SPE);
	DDRD &= ~(1<<CS);
	
	
}

void setup_input () {
	INPUT_DDR &= ~SW_BITMASK;
	INPUT_PORT |= SW_BITMASK;
}

void setup_interrupt(){
	EICRA |= (0b10);
	EIMSK |= (1<<INT0);
}

void setup_adc () {
	// Disable power saving
	PRR &= 1 << PRADC;
	
	// Set VCC voltage ref
	ADMUX &= ~REFS_BM;
	ADMUX |= (1 << REFS0);
	
	// Fill high byte register with 8 highest bits (needed for 8-bit resolution read)
	ADMUX |= (1 << ADLAR);
	
	// Enable ADC
	ADCSRA |= (1 << ADEN);
}

uint8_t read_adc(uint8_t adc_mux) {
	ADMUX = (ADMUX & ~ADMUX_BM) | adc_mux; // select input
	ADCSRA |= (1 << ADSC);                 // start conversion
	while (ADCSRA & (1 << ADSC));          // wait until finished
	return ADCH;
}

// Read the three buttons and return them in one uint8_t,
// where A, B, X are given by bit 0, 1, 2. Active high.
uint8_t read_buttons () {
	return
		((~INPUT_PIN & (1 << SWA)) >> (SWA-0)) |
		((~INPUT_PIN & (1 << SWB)) >> (SWB-1)) |
		((~INPUT_PIN & (1 << SWX)) >> (SWX-2)) 
	;
}

void wait_for_spi () {
	while (~SPSR & (1 << SPIF));
}

void wait_for_cs () {
	while(!(PIND & (1 << PD2)));
}


int main(void) {
	setup_spi();
	setup_adc();
	setup_input();
	
	setup_interrupt();
	
	sei();
	while (1) {
		wait_for_cs();
		wait_for_spi();
		switch (SPDR) {
		case 0:
			SPDR = read_buttons();
			_delay_us(25);
			break;
		case 1:
			SPDR = read_adc(ADMUX_HORIZONTAL);
			_delay_us(25);
			break;
		case 2:
			SPDR = read_adc(ADMUX_VERTICAL);
			_delay_us(25);
			break;
		default:
			_delay_us(25);
			break;
		}
	}
}



ISR (INT0_vect){
	SPCR &= ~(1 << SPE);
	_delay_us(1);
	SPCR |= (1 << SPE);
}

