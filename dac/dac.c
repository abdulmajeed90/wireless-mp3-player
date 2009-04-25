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

#define t1	200

//Prototypes
void SPI_MasterTransmit(unsigned int);	//transmit a 16bit value ovr SPI
void SPI_MasterInit(void);							//initialise the SPI port
void init(void);												//microcon initialise

// RXC ISR variables								
volatile char r_buffer[1026];	//ring buffer 
volatile unsigned int rpos = 0;  //keeps track of write position
volatile unsigned int wpos = 0;  //keeps track of read position


uint8_t count = 0;
int16_t time = 0;
volatile uint16_t dac_value = 0;
uint16_t buff_size = 0;

//*****************************************************************************

ISR (USART0_RX_vect) {

	r_buffer[wpos] = UDR0;
	wpos = (wpos+1)&0x3ff;  //1024 counter
}

//**********************************************************
//timer 2 overflow ISR
ISR (TIMER2_COMPA_vect) {
      
	//Decrement the time if not already zero
	if(rpos != wpos) {
		dac_value = (0x4000 | (r_buffer[rpos]));
		SPI_MasterTransmit(dac_value);

		rpos = (rpos+1)&0x3ff;  //1024 counter
	
	}
}


ISR (TIMER0_COMPA_vect){
	//timer0 compare match
	//count++;
	
	if(time>0) time--;
}
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
			buff_size = wpos + 1024 - rpos;
		}
		if (buff_size < 256) {
			while (! (UCSR0A | (1<<UDRE0))){}
  			UDR0 = 0x01;						//command to request more data
		}
		if (time == 0) {
			PORTD = ~PORTD;
			time = t1;
		}
		//SPI_MasterTransmit(dac_value);
		//PORTC = ~PORTC;
	}
}

void SPI_MasterInit(void){
	// Set SS, MOSI and SCK output, all others input
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<SS);

	// Enable SPI, Master, set clock rate fck/4
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(0<<CPHA);

	//Set double data rate for fck/2 operation
	//SPSR = (1<<SPI2X);
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
*/
	
	//set up timer 0 for 1 mSec timebase 
	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR 
	OCR0A = 249;  		//set the compare re to 250 time ticks
	//set prescalar to divide by 64 
	TCCR0B= 3; //0b00001011;	
	// turn on clear-on-match
	TCCR0A= (1<<WGM01) ;
	
	//set up timer 0 for 1 mSec timebase 
	TIMSK2= (1<<OCIE2A);	//turn on timer 2 cmp match ISR 
	OCR2A = 38;  		//set the compare re to 39 time ticks
	//set prescalar to divide by 64 
	TCCR2B= (1<<CS22); 
	// turn on clear-on-match
	TCCR2A= (1<<WGM21) ;
	

	DDRD = (1<<PIND2);
	DDRC = (1<<PINC0);
	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"MCU Online...\n\r");
		
	UCSR0B |= (1<<RXCIE0) ;

	//Enable interrupts
	sei();
}
