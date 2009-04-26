#include <avr\io.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h> 		// for sine


#include "uart.h"

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

//SPI pins
#define DD_MOSI    PINB5
#define DD_MISO	   PINB6
#define DD_SCK     PINB7
#define SS	   	   PINB4
#define DDR_SPI    DDRB

//button debouncing states and constants
#define nopress		 0
#define maybepress 	 1
#define press		 2
#define maybenopress 3

uint8_t pushstate1 = nopress;		//previos button state
uint8_t pushstate2 = nopress;		//next button state
uint8_t pushstate3 = nopress;		//play/pause button state
uint8_t pushstate4 = nopress;		//stop button state
uint8_t prev = 0, next = 0, play = 1, stop = 0;

#define t1	200
//serial transmission handshaking stuff
#define rts PIND3
#define cts PIND4

//Prototypes
void SPI_MasterTransmit(unsigned int);	//transmit a 16bit value ovr SPI
void SPI_MasterInit(void);							//initialise the SPI port
void init(void);												//microcon initialise

// RXC ISR variables								
volatile char r_buffer[3072];	//ring buffer 
volatile unsigned int rpos = 0;  //keeps track of write position
volatile unsigned int wpos = 0;  //keeps track of read position


uint8_t count = 0;
volatile int16_t time = 0;
volatile int delay = 0;
volatile uint16_t dac_value = 0;
uint16_t buff_size = 0;

//*****************************************************************************

ISR (USART0_RX_vect) {

	r_buffer[wpos] = UDR0;
	wpos = (wpos+1)&0xBff;  //3072 counter
}

//**********************************************************
//timer 2 overflow ISR
ISR (TIMER2_COMPA_vect) {
      
	//Decrement the time if not already zero
	if(rpos != wpos  &&  play==1) {
		dac_value = (0x4000 | (r_buffer[rpos] + 0x800 - 80));
		SPI_MasterTransmit(dac_value);

		rpos = (rpos+1)&0xBff;  //3072 counter
	
	}
	time++;
	if (delay > 0) delay--;
}


//ISR (TIMER0_COMPA_vect){
	//timer0 compare match
	//count++;
	

//}
//*****************************************************************************

int main(void) {

	init();

	//start the SPI interface
	SPI_MasterInit();
	fprintf(stdout, "SPI up and running\n\r\n\rStarting DAC stuff...\n\r\n\r");

	//endless loop
	while (1){
		if (rpos <= wpos) {
			buff_size = wpos - rpos;
		} else if (rpos > wpos) {
			buff_size = wpos + 3072 - rpos;
		}

		if (buff_size < 512  && play==1 && delay == 0) {
			//PORTD &= ~(1<<rts);			//drive RTS low
			//while (PIND & (1<<cts));	//wait for CTS low
			while (! (UCSR0A | (1<<UDRE0))){}	
  			UDR0 = 0x01;						//command to request more data
			delay = 400;						//230/8000 second delay
			//PORTD |= (1<<rts);
		} else {
			if (prev == 1) {
				while (! (UCSR0A | (1<<UDRE0))){}
  				UDR0 = 0x03;						//command to request previous song
				rpos = wpos;						//reset buffer
				prev = 0;
			}
			if (next == 1) {
				while (! (UCSR0A | (1<<UDRE0))){}
  				UDR0 = 0x02;						//command to request next song
				rpos = wpos;						//reset buffer
				next = 0;
			}
			if (stop == 1) {
				while (! (UCSR0A | (1<<UDRE0))){}
  				UDR0 = 0x04;						//command to request next song
				rpos = wpos;						//reset buffer
			}
		}

		if ((time>>3) >= 30) {
			task1();
			PORTD = ~PORTD;
			time = 0;
		}
		//SPI_MasterTransmit(dac_value);
		//PORTC = ~PORTC;
	}
}

void task1 () {

	switch (pushstate1) {
		case (nopress):
			if (~PINC & (1<<PINC4)) pushstate1 = maybepress;
			else pushstate1 = nopress;
			break;
		case (maybepress):
			if (~PINC & (1<<PINC4)) {
				pushstate1 = press;
				prev = 1;
			}
			else pushstate1 = nopress;
			break;
		case (press):
			if (~PINC & (1<<PINC4)) pushstate1 = press;
			else pushstate1 = maybenopress;
			break;
		case (maybenopress):
			if (~PINC & (1<<PINC4)) pushstate1 = press;
			else {
				pushstate1 = nopress;
				//prev = 0;
			}
			break;
	}
	switch (pushstate2) {
		case (nopress):
			if (~PINC & (1<<PINC6)) pushstate2 = maybepress;
			else pushstate2 = nopress;
			break;
		case (maybepress):
			if (~PINC & (1<<PINC6)) {
				pushstate2 = press;
				next = 1;
			}
			else pushstate2 = nopress;
			break;
		case (press):
			if (~PINC & (1<<PINC6)) pushstate2 = press;
			else pushstate2 = maybenopress;
			break;
		case (maybenopress):
			if (~PINC & (1<<PINC6)) pushstate2 = press;
			else {
				pushstate2 = nopress;
				//next = 0;
			}
			break;
	}
	switch (pushstate3) {
		case (nopress):
			if (~PINC & (1<<PINC2)) pushstate3 = maybepress;
			else pushstate3 = nopress;
			break;
		case (maybepress):
			if (~PINC & (1<<PINC2)) {
				pushstate3 = press;
				play ^= 0x01;
				stop = 0;
			}
			else pushstate3 = nopress;
			break;
		case (press):
			if (~PINC & (1<<PINC2)) pushstate3 = press;
			else pushstate3 = maybenopress;
			break;
		case (maybenopress):
			if (~PINC & (1<<PINC2)) pushstate3 = press;
			else {
				pushstate3 = nopress;
				//next = 0;
			}
			break;
	}
	switch (pushstate4) {
		case (nopress):
			if (~PINC & (1<<PINC0)) pushstate4 = maybepress;
			else pushstate4 = nopress;
			break;
		case (maybepress):
			if (~PINC & (1<<PINC0)) {
				pushstate4 = press;
				play = 0;
				stop = 1;
			}
			else pushstate4 = nopress;
			break;
		case (press):
			if (~PINC & (1<<PINC0)) pushstate4 = press;
			else pushstate4 = maybenopress;
			break;
		case (maybenopress):
			if (~PINC & (1<<PINC0)) pushstate4 = press;
			else {
				pushstate4 = nopress;
				//next = 0;
			}
			break;
	}
}

void SPI_MasterInit(void){
	// Set SS, MOSI and SCK output, all others input
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<SS);

	// Enable SPI, Master, set clock rate fck/4
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(0<<CPHA);

	//Set double data rate for fck/2 operation
	SPSR = (1<<SPI2X);
}

void SPI_MasterTransmit(unsigned int data){
	
	//Slave select goes low
	PORTB &= ~(1<<SS);	

	/* Transmit higher byte */
	SPDR = (char)(data>>8);

	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));

	//read received higher byte
	//rData = (SPDR << 8);

	/* Transmit the lower byte */
	SPDR = (char)(data & 0xff);

	while(!(SPSR & (1<<SPIF)));
	//read the received byte into a variable
	//rData = rData + SPDR;

	//Slave select goes high
	PORTB |= (1<<SS);
}

void init(void) {
/*
	for (int i=0; i<1024; i++)
    {
   		r_buffer[i] = ((char)(127.0 * sin(6.283*((float)i)/8))) + 127 ;
    } 

	
	//set up timer 0 for 1 mSec timebase 
	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR 
	OCR0A = 249;  		//set the compare re to 250 time ticks
	//set prescalar to divide by 64 
	TCCR0B= (1<<CS02)|(1<<CS00); //0b00001011;	
	// turn on clear-on-match
	TCCR0A= (1<<WGM01) ;
*/	
	//set up timer 0 for 1 mSec timebase 
	TIMSK2= (1<<OCIE2A);	//turn on timer 2 cmp match ISR 
	OCR2A = 38;  		//set the compare re to 39 time ticks
	//set prescalar to divide by 64 
	TCCR2B= (1<<CS22); 
	// turn on clear-on-match
	TCCR2A= (1<<WGM21) ;
	

	DDRD = (1<<PIND2);
	DDRA = 0xff;
	//activate internal pullup on pins 6 and 4, PORTC; pushbutton pins
	PORTC = (1<<PINC4)|(1<<PINC6)|(1<<PINC2)|(1<<PINC0);
	//serial handshaking init
	DDRD |= (1<<rts);
	PORTD |= (1<<rts);		//make it high
	//PORTD |= (1<<cts); 		//cts active low input pullup

	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"MCU Online...\n\r");
		
	UCSR0B |= (1<<RXCIE0) ;

	//Enable interrupts
	sei();
}
