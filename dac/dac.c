#include <avr\io.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

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
volatile char r_index;  //current string index
volatile char r_buffer[16];	//input string 
volatile char r_ready;  //flag for receive done
volatile char r_char;  //current character  

uint8_t count = 0;
int16_t time = 0;
uint16_t dac_value = 0;

//*****************************************************************************

ISR (USART0_RX_vect) {

/*	r_char = UDR0 ;    //get a char
	UDR0 = r_char;    //then print it
	//build the input string
	if (r_char != '\r') 	// Is the input a <enter>?
	begin 
		if (r_char == '\b')	// Is the input a backspace? 
		begin
			putchar(' '); 	// erase the character on the screen
			putchar('\b');	// backup
			--r_index ;		// wipe a character from the string
		end
		else   
 			r_buffer[r_index++] = r_char ; // add a character to the string
	end
	else 					// Human pressed <enter>
	begin
		putchar('\n');  			//use putchar to avoid overwrite
		r_buffer[r_index] = 0x00;   //zero terminate
		r_ready = 1;				//signal cmd processor
		UCSR0B ^= (1<<RXCIE0) ;   	//stop rec ISR -- clear rxc
	end
*/
	r_char = UDR0;
	r_ready = 1;
	UCSR0B &= ~(1<<RXCIE0) ;
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
		
		if(r_ready == 1) {
			dac_value = (0x4000 | (r_char));
			SPI_MasterTransmit(dac_value);
			r_ready = 0;
			UCSR0B |= (1<<RXCIE0) ;
		}

		if (time == 0) {
			PORTD = ~PORTD;
			time = t1;
		}
	}
}

void SPI_MasterInit(void){
	// Set SS, MOSI and SCK output, all others input
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<SS);

	// Enable SPI, Master, set clock rate fck/4
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<CPOL)|(0<<CPHA);

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

  //set up timer 0 for 1 mSec timebase 
  TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR 
  OCR0A = 249;  		//set the compare re to 250 time ticks
  //set prescalar to divide by 64 
  TCCR0B= 3; //0b00001011;	
  // turn on clear-on-match
  TCCR0A= (1<<WGM01) ;


	DDRD = (1<<PIND2);
	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"MCU Online...\n\r");
		
	UCSR0B |= (1<<RXCIE0) ;

	//Enable interrupts
	sei();
}
