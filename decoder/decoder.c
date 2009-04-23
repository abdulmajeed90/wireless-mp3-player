#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
//#include <avr/iom16.h>
#include "uart.h"
#include "decode.h"

#define begin	{
#define end		}

//pin setters
#define set(port,pin) (port |= (1<<pin))
#define clr(port,pin) (port &= ~(1<<pin))


// Define STA013 Pins
#define I2C_SDA_direction   PINA0
#define I2C_SDA_in      	PINA0
#define I2C_SDA_out     	PINA0
#define I2C_SCL             PINA1
#define DATA            	PINA2
#define CLOCK           	PINA3
#define DATA_REQ            PINA4
#define RESET           	PINA5

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);	

//constants
#define t 5		//sets I2C clock half period



//STA013 variables
uint8_t address, data, errorFlag,I2C_byte,mp3_byte;

//prototypes
void init(void);				// Initialize the MCU
void sta013_I2C_start(void);		// I2C Start Condition
void sta013_I2C_stop(void);			// I2C Stop Condition                
void sta013_I2C_read(void);			// Read a byte via the I2C interface
void sta013_I2C_write(void);		// Write a byte via the I2C interface
void sta013_read(void);				// Read from the given address on the STA013
void sta013_write(void);			// Write to the given address on the STA013   
unsigned int config_sta013(void);	// Configure the STA013 with the data file
void sta013_start(void);			// Instruct the STA013 to start running
void sta013_play(void);             // Instruct the STA013 to start playing
void sta013_stop(void);				// Instruct the STA013 to stop playing



int main(void) {
	init();

	unsigned int i = 0;
	unsigned int size = sizeof(mp3_data);
	sta013_start();
	sta013_play();
    
	while(1){
	    while(~PINA & (1<<DATA_REQ) && i<size){
			for (int8_t j = 7; j >=0; j--) {
	    		set(PORTA,CLOCK);
	        	_delay_us(1);
	        	((mp3_data[i] >> j) & 0x01) ? set(PORTA,DATA): clr(PORTA,DATA);
	        	_delay_us(1);
	    		clr(PORTA,CLOCK);
	       		_delay_us(1);
	  	  	}
	      	i++;
			//if (i>>2 & 0x1) fprintf(stdout, "%d\n\r",i);
		}
	if (i>=size) i=0;
 /*     
  		EEAR = i;
  		EECR |= (1<<EERE); 	//initiate a read of eeprom
      	mp3_byte = EEDR;
  		fprintf(stdout,"location: %d\tValue:%d\n\r",i, mp3_byte);  //print out eeprom value
		for (int8_t j = 7; j >=0; j--) {
    		clr(PORTA,CLOCK);
        	_delay_us(5);
        	((mp3_byte >> j) & 0x01) ? set(PORTA,DATA): clr(PORTA,DATA);
        	_delay_us(5);
    		set(PORTA,CLOCK);
       		_delay_us(5);
  	  	}
      	i++;
    }
      // while(PINA & 1<<DATA_REQ){fprintf(stdout,"PINA = %x\n\r",PINA);} //wait for STA to drive DATA_REQ pin low
  		_delay_ms(50);
  		set(PORTD,PIND2);
  		_delay_ms(50);
  		clr(PORTD,PIND2);*/
    //sta013_stop();
  }
}



void init(void) {
	
	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"UART running\n\r");
	
	DDRD |= (1<<PIND2);
	set(PORTD,PIND2);

	//set output pins as outputs
	DDRA = (1<<I2C_SCL)|(1<<DATA)|(1<<CLOCK)|(1<<RESET);
	clr(PORTA,RESET);
	//activate internal pullup of data request input
	//set(PORTA,DATA_REQ);

	fprintf(stdout,"MCU online\n\r");
	set(PORTA,RESET);
	//set up the STA013
	while(config_sta013()){}
	//config_sta013();
	fprintf(stdout,"STA013 initialized\n\r\n\r");

	//start up the STA013
	sta013_start();
	fprintf(stdout,"started successfully!!!\n\r\n\r");

  	
	sei(); // enable interrupts

}

void sta013_I2C_start(void) begin

   // High to low transition of I2C_SDA while I2C_SCL is high
   set(DDRA,I2C_SDA_direction);	//set to output
   clr(PORTD,PIND2);
   _delay_us(t);
   set(PORTA,I2C_SDA_out);
   _delay_us(t);
   set(PORTA,I2C_SCL);
   _delay_us(t);
   clr(PORTA,I2C_SDA_out);
   _delay_us(t);
   clr(PORTA,I2C_SCL);
   _delay_us(t);
end

void sta013_I2C_write(void) begin
	 //fprintf(stdout,"inside sta013_I2C_write. Writing %x\n\r",I2C_byte);
   // Clock each bit onto the SDA bus (starting with the MSB)
   set(DDRA,I2C_SDA_direction);	//set to output
   clr(PORTD,PIND2);

   for(int8_t j = 7; j >= 0; j--) begin
      _delay_us(t);   
      clr(PORTA,I2C_SCL); //I2C_SCL is the clock (what is the min/max clock speed?)
      _delay_us(t);
      ((I2C_byte >> j) & 0x01)?set(PORTA,I2C_SDA_out):clr(PORTA,I2C_SDA_out); // Write each bit while I2C_SCL is low
      _delay_us(t);
      set(PORTA,I2C_SCL);
      _delay_us(t);
   end

   // Get the ack bit
   clr(PORTA,I2C_SCL);
   _delay_us(t);
   clr(PORTA,I2C_SDA_out);
   clr(DDRA,I2C_SDA_direction);	//set to input
   set(PORTD,PIND2);
   _delay_us(t);
   set(PORTA,I2C_SCL);
   _delay_us(t);
   errorFlag = ((PINA & (1<<I2C_SDA_in))>>I2C_SDA_in); //should be a 0
   _delay_us(t);
   clr(PORTA,I2C_SCL);  
end      

void sta013_I2C_read(void) begin

   data = 0x00;

   // Clock each bit off of the SDA bus
   clr(DDRA,I2C_SDA_direction);	//set to intput
   set(PORTD,PIND2);
   _delay_us(t);
   clr(PORTA,I2C_SCL);
   _delay_us(t);  

   for(int8_t j = 7; j >= 0; j--) begin   
      set(PORTA,I2C_SCL);
      _delay_us(t);
      data = data | (((PINA & (1<<I2C_SDA_in)) >> I2C_SDA_in)<<j);     // Read the bit while I2C_SCL is high
      _delay_us(t);
      clr(PORTA,I2C_SCL);
      _delay_us(t);
   end
end

void sta013_I2C_stop(void) begin

   // Low to high transition of I2C_SDA while I2C_SCL is high
   set(DDRA,I2C_SDA_direction);	//set to output
   clr(PORTD,PIND2);
   _delay_us(t);
   clr(PINA,I2C_SDA_out);
   _delay_us(t);
   set(PORTA,I2C_SCL);
   _delay_us(t);
   set(PORTA,I2C_SDA_out);
   _delay_us(t);
   clr(PORTA,I2C_SCL);
   _delay_us(t);
end

 

void sta013_read(void) begin

   fprintf(stdout,"Inside sta013_read\r\n");

   // Start Condition
   sta013_I2C_start();

   //i2c_start();
   fprintf(stdout, "called I2C_start\r\n");

  

   // Send the device address with the R/W bit (i.e., the 8th bit) cleared
   fprintf(stdout,"about to try sta013_I2C_write\r\n");
   I2C_byte = 0x86;
   sta013_I2C_write();

    if (errorFlag != 0) begin
       fprintf(stdout,"Read Error: Error writing device address (W) to STA013.\r\n");
       return;
    end

    fprintf(stdout,"finished calling sta013_I2C_write, errorFlag = %d\r\n", errorFlag);

   // Send the read address
   fprintf(stdout,"about to try sta013_I2C_write again\r\n");
   I2C_byte = address;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf(stdout,"Read Error: Error writing read address to STA013.\r\n");
      return;
   end

   fprintf(stdout,"finished calling sta013_I2C_write again, errorFlag = %d\r\n", errorFlag);

  

   // Start Condition
   sta013_I2C_start();
   fprintf(stdout,"called I2C_start again\r\n"); 

   // Send the device address with the R/W bit (i.e., the 8th bit) set
   fprintf(stdout,"about to try sta013_I2C_write 3rd time\r\n");
   I2C_byte = 0x87;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf(stdout,"Read Error: Error writing device address (R) to STA013.\r\n");
      return;
   end

   fprintf(stdout,"finished calling sta013_I2C_write 3rd time, errorFlag = %d\r\n", errorFlag);

  

   // Read the data from the given address
   fprintf(stdout,"about to try sta013_I2C_read\r\n");
   sta013_I2C_read();
   fprintf(stdout,"finished calling sta013_I2C_read, data = %x\r\n", data);

   // Stop Condition
   sta013_I2C_stop();
end

void sta013_write(void) begin
	//fprintf(stdout,"inside sta013_write\n\r");

   // Start Condition
   sta013_I2C_start();

   // Send the device address with the R/W bit (i.e., the 8th bit) cleared
   I2C_byte = 0x86;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf(stdout,"Write Error: Error writing device address (W) to STA013.\r\n");
      return;
   end

   // Send the write address
   I2C_byte = address;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf(stdout,"Write Error: Error writing write address to STA013.\r\n");
      return;
   end

   // Send the data
   I2C_byte = data;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf(stdout,"Write Error: Error writing data to STA013.\r\n");
      return;
   end 

   // Stop Condition
   sta013_I2C_stop();     
end

void sta013_start(void) begin

   fprintf(stdout,"Inside sta013_start\r\n");

   // Write 1(0x01) to address 114(0x72)
   address = 0x72;
   data = 0x01;
   sta013_write();

   // Check for an error
   if (errorFlag != 0x00)
      fprintf(stdout,"Error: STA013 Start Failed, %d\r\n", errorFlag);
   else
      fprintf(stdout,"Starting mp3 player...\r\n");
end


unsigned int config_sta013(void) begin    

   // Read from address 0x01 on the STA013
   fprintf(stdout,"Starting config_sta013\r\n");

   address = 0x01;                                        // Load the address into the address reg, r4

   sta013_read();                                        // Read data from the STA013

   // Test if address 0x01 contains 0xAC
   fprintf(stdout,"data = %x, errorFlag = %d\r\n", data, errorFlag);

   if((data != 0xac) || (errorFlag != 0x00)) begin
      fprintf(stdout,"Error: STA013 Not Present, %d, \r\n", errorFlag);    // Print out the error flag
      return 1;                                           // Return 1 to indicate that the STA013 is not present
   end

   else

      fprintf(stdout,"STA013 Present\r\n");


   fprintf(stdout,"beginning to load STA013 config file\r\n");  

   // Send STA013_updatedata information to STA013 (i.e., STA013 config file)
   for (uint16_t i = 0 ; i < sizeof(STA013_UpdateData) ;) begin
      address = STA013_UpdateData[i];                // Load the address into the address reg, r4
      data    = STA013_UpdateData[i+1];              // Load the data into the data reg, r5
      sta013_write();                                // Write data to STA013

      fprintf(stdout,"Writing config entry %d\n\r",i);
      // Increment i
      i += 2;

      // Check for an error in the write
      if (errorFlag != 0x00) begin
         fprintf(stdout,"Error: STA013 Configuration Failed, %d\r\n", errorFlag);   // Print out the error flag
         return 2;                                   // Return 2 to indicate sta013 failed configuration
      end

      // Delay initialization for soft reboot
      if (address == 0x10) begin
          _delay_ms(1000);
      end    
   end

  

   fprintf(stdout,"finished loading STA013 config file\r\n");
   fprintf(stdout,"beginning to load STA013 board data\r\n");  

   // Send STA013 board specific data

   for (uint8_t i = 0; i < sizeof(config_PLL) ; ) begin
      address = config_PLL[i];                            // Load the address into the address reg, r4
      data    = config_PLL[i+1];                          // Load the data into the data reg, r5
      sta013_write();      
                                     // Write data to STA013
      fprintf(stdout,"Writing board data entry %d\n\r",i);
      // Increment i
      i =i+ 2;
      
      if (errorFlag != 0x00) begin
         fprintf(stdout,"Error: STA013 PLL Configuration Failed, %d\r\n", errorFlag);   // Print out the error flag
         return 2;                                   // Return 2 to indicate sta013 failed configuration
      end
   end
 
   fprintf(stdout,"finished loading STA013 board data\r\n"); 
   fprintf(stdout,"STA013 Configuration Complete\r\n"); 
   return 0;                                         // Return 0 to indicate pass configuration
end


void sta013_play(void) begin
   // Write 1(0x01) to address 19(0x13)
   address = 0x13;
   data = 0x01;
   sta013_write();
   
   // Check for an error
   if (errorFlag != 0x00) {
      fprintf(stdout,"Error: STA013 Play Failed, %d\r\n", errorFlag);
}   else            {
      fprintf(stdout,"Playing MP3...\r\n");   
}
   //address = 0x18;
   //data = 0x04;
   //sta013_write();

//	address = 0x14;
//	data = 0x00;
//	sta013_write();
end
 
void sta013_stop(void) begin
   // Write 0(0x01) to address 19(0x13)
   address = 0x13;
   data = 0x00;
   sta013_write();
   
   // Check for an error
   if (errorFlag != 0x00)
      fprintf(stdout,"Error: STA013 Stop Failed, %d\r\n", errorFlag);
   else
      fprintf(stdout,"Playback stopped.\r\n");   
end

