/*********************************************
Assumes SD card on SPI interface on PORTB
Serial connected to Computer, hyperterm running
at 9600Baud, 1 stop bit, no flow
LED on PIND2.
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

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

#define SPIDI	6	// Port B bit 6 (pin7): data in (data from MMC)
#define SPIDO	5	// Port B bit 5 (pin6): data out (data to MMC)
#define SPICLK	7	// Port B bit 7 (pin8): clock
#define SPICS	4	// Port B bit 4 (pin5: chip select for MMC

char sector[512];
//prototypes

void init(void);
void scan_files (char*);
void SPIinit(void);


//**********************************************************
//timer 0 overflow ISR
//**********************************************************
ISR (TIMER0_COMPA_vect) 
{
  disk_timerproc();
}

//**********************************************************************
//  Main
//**********************************************************************

int main(void) {
	init();
	
	FATFS fso;
	DIR root;
										//create file system object
	/* Create a work area for the drive */
	if (f_mount(0, &fso) == FR_INVALID_DRIVE) {
		fprintf(stdout,"Invalid drive number\n\r");
	}

	switch (f_opendir(&root,"DCIM")) {
		case(FR_OK):
		    fprintf(stdout,"The function succeeded and the directory object is created. It is used for subsequent calls to read the directory entries.");
			break;
		case(FR_NO_PATH):
		    fprintf(stdout,"Could not find the path.");
			break;
		case(FR_INVALID_NAME):
		    fprintf(stdout,"The path name is invalid.");
			break;
		case(FR_INVALID_DRIVE):
		    fprintf(stdout,"The drive number is invalid.");
			break;
		case(FR_NOT_READY):
		    fprintf(stdout,"The disk drive cannot work due to no medium in the drive or any other reason.");
			break;
		case(FR_RW_ERROR):
		    fprintf(stdout,"The function failed due to a disk error or an internal error.");
			break;
		case(FR_NOT_ENABLED):
		    fprintf(stdout,"The logical drive has no work area.");
			break;
		case(FR_NO_FILESYSTEM):
		    fprintf(stdout,"There is no valid FAT partition on the disk.");
			break;
		default: fprintf(stdout, "Rewrite this shit");
	}
	
	/* Display the disk contents (over serial)*/
	scan_files("");
	
	fprintf(stdout,"\n\r File scan done!\n\rblinking LED now\n\r");

	while (1) {
		// PIN2 PORTD clear -> LED off
		PORTD &= ~(1<<PIND2);
		_delay_ms(500);	
		// PIN2 PORTD set -> LED on
		PORTD |= (1<<PIND2); 
		_delay_ms(500);	
	}
	return 0;
}


void SPIinit(void) {
	DDRB &= ~(1 << SPIDI);	// set port B SPI data input to input
	DDRB |= (1 << SPICLK);	// set port B SPI clock to output
	DDRB |= (1 << SPIDO);	// set port B SPI data out to output 
	DDRB |= (1 << SPICS);	// set port B SPI chip select to output
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
	// SPSR = (1<< SPI2X);
	PORTB &= ~(1 << SPICS);	// set chip select to low (MMC is selected)
}

void scan_files (char* path)
{
    FILINFO finfo;
    DIR dirs;
    int i;

<<<<<<< .mine
	fprintf(stdout,"512 bytes sent\n\r");
	//serialterminate();
	fprintf(stdout,"blinking LED now\n\r");
	//serialterminate();

	// enable  PD5 as output
	DDRD |= (1<<PIND5);
	while (1) {
		// PIN5 PORTD clear -> LED off
		PORTD &= ~(1<<PIND5);
		_delay_ms(500);
		// PIN5 PORTD set -> LED on
		PORTD |= (1<<PIND5); 
		_delay_ms(500);	
=======
    if (f_opendir(&dirs, path) == FR_OK) {
        i = strlen(path);
        while ((f_readdir(&dirs, &finfo) == FR_OK) && finfo.fname[0]) {
            if (finfo.fattrib & AM_DIR) {
                sprintf(&path[i], "/%s", &finfo.fname[0]);
                scan_files(path);
                path[i] = 0;
            } else {
                fprintf(stdout,"%s/%s\n", path, &finfo.fname[0]);
            }
        }
    } else {
		fprintf(stdout,"\n\rscan_files failed\n\r");
>>>>>>> .r16
	}
}

void init(void) {
<<<<<<< .mine
	
=======
	
	//set up timer 0 for 10 mSec timebase 
	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR 
	OCR0A = 195;  		//set the compare re to 250 time ticks
	//set prescalar to divide by 1024 
	TCCR0B= (1<<CS02)|(1<<CS00); //0b00001011;	
	// turn on clear-on-match
	TCCR0A= (1<<WGM01) ;

	
>>>>>>> .r16
	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"UART running\n\r");
	
<<<<<<< .mine
	DDRD |= (1<<PIND5);
	SPIinit();

	fprintf(stdout,"MCU online\n\r");
	//serialterminate();
=======
	DDRD |= (1<<PIND2);
	SPIinit();
>>>>>>> .r16

<<<<<<< .mine
	MMC_Init();

	//fprintf(stdout,"SD card online\n\r");
	//serialterminate();

=======
>>>>>>> .r16
	sei(); // enable interrupts

}
