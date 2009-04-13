/*********************************************
This program writes a sector to an SD card,
reads this sector, and spews this data over 
srial.

Assumes SD card on SPI interface on PORTB
Serial connected to Computer, hyperterm running
at 9600Baud, 1 stop bit, no flow
LED on PIND5.
*********************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
//#include <avr/iom16.h>
#include "uart.h"
#include "tff.h"
#include "integer.h"
#include "diskio.h"

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

//time for task
#define t1 250

//prototypes
void init(void);

//**********************************************************
//timer 0 overflow ISR
ISR (TIMER0_COMPA_vect) 
{     
  //Decrement the time if not already zero
  if (Timer1>0) Timer1--;
  if (Timer2>0) Timer2--;
}

//**********************************************************


int main(void) {
	init();
	
	//fillram();
	//writeramtommc();
	//sendmmc();

	//fprintf(stdout,"512 bytes sent\n\r");
	//serialterminate();
	//fprintf(stdout,"blinking LED now\n\r");
	//serialterminate();

	// enable  PD5 as output
	FATFS fatfs;
	DIR dir;
	FILINFO finfo;
	char filename[8+1+3+1] = "";

	while(disk_initialize(0)){
		fprintf(stdout,"Initilising...\n\r");
	}
	while (f_mount(0, &fatfs)) {
		fprintf(stdout,"Mounting disk...\n\r");
	}
	f_opendir(&dir, filename);
	fprintf(stdout,"\n\rListing files in root directory\n\r");
	while (f_readdir(&dir, &finfo)==FR_OK && finfo.fname!=NULL){
		fprintf(stdout,"%s\n\r",finfo.fname);
	}

	DDRD |= (1<<PIND2);
	while (1) {
		// PIN5 PORTD clear -> LED off
		PORTD &= ~(1<<PIND2);
		_delay_ms(500);
		// PIN5 PORTD set -> LED on
		PORTD |= (1<<PIND2); 
		_delay_ms(500);	
	}
	return 0;
}

void init(void) {
	
	//init a timer
	//set up timer 0 for 10 mSec timebase, clk 20MHz
	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR 
	OCR0A = 32;  		//set the compare re to 250 time ticks
	//set prescalar to divide by 1024
	TCCR0B= (1<<CS02)|(1<<CS01)|(0<<CS00); //0b00001011;	
	// turn on clear-on-match
	TCCR0A= (1<<WGM01) ;

	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"UART running\n\r");
	
	DDRD |= (1<<PIND2);
	//SPIinit();

	fprintf(stdout,"MCU online\n\r");
	//serialterminate();

	//MMC_Init();

	//fprintf(stdout,"SD card online\n\r");
	//serialterminate();

	sei(); // enable interrupts

}
