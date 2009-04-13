#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
//#include <avr/iom16.h>
#include "uart.h"

#define begin	{
#define end		}


// Define STA013 Pins
#define I2C_SDA_direction   DDRA.0
#define I2C_SDA_in      	PINA.0
#define I2C_SDA_out     	PORTA.0
#define I2C_SCL             PORTA.1
#define DATA            	PORTA.2
#define CLOCK           	PORTA.3
#define DATA_REQ            PINA.4
#define RESET           	PORTA.5

//STA013 chip variables
uint8_t address, data, errorFlag,max_PLL_index,I2C_byte;

//prototypes
unsigned int config_sta013(void);
void sta013_I2C_start(void);
void sta013_I2C_write(void);
void sta013_I2C_stop(void);
void sta013_I2C_read(void);
void sta013_write(void);
void sta013_start(void);
void sta013_read(void);

int main(void) {

}

void sta013_I2C_start(void) begin

   // High to low transition of I2C_SDA while I2C_SCL is high
   I2C_SDA_direction = 1;
   _delay_us(5);
   I2C_SDA_out = 1;
   _delay_us(5);
   I2C_SCL = 1;
   _delay_us(5);
   I2C_SDA_out = 0;
   _delay_us(5);
   I2C_SCL = 0;
   _delay_us(5);
end

void sta013_I2C_write(void) begin

   // Clock each bit onto the SDA bus (starting with the MSB)
   I2C_SDA_direction = 1;

   for(j = 7; j >= 0; j--) begin
      _delay_us(5);   
      I2C_SCL = 0; //I2C_SCL is the clock (what is the min/max clock speed?)
      _delay_us(5);
      I2C_SDA_out = (I2C_byte >> j) & 0x01; // Write each bit while I2C_SCL is low
      _delay_us(5);
      I2C_SCL = 1;
      _delay_us(5);
   end

   // Get the ack bit
   I2C_SCL = 0;
   _delay_us(5);   
   I2C_SDA_direction = 0;
   _delay_us(5);
   I2C_SCL = 1;
   _delay_us(5);
   errorFlag = I2C_SDA_in; //should be a 0
   _delay_us(5);
   I2C_SCL = 0;  
end      

void sta013_I2C_read(void) begin

   data = 0x00;

   // Clock each bit off of the SDA bus
   I2C_SDA_direction = 0;
   _delay_us(5);
   I2C_SCL = 0;
   _delay_us(5);  

   for(j = 7; j >= 0; j--) begin   
      I2C_SCL = 1;
      _delay_us(5);
      data = data | (I2C_SDA_in << j);     // Read the bit while I2C_SCL is high
      _delay_us(5);
      I2C_SCL = 0;
      _delay_us(5);
   end
end

void sta013_I2C_stop(void) begin

   // Low to high transition of I2C_SDA while I2C_SCL is high
   I2C_SDA_direction = 1;
   _delay_us(5);
   I2C_SDA_out = 0;
   _delay_us(5);
   I2C_SCL = 1;
   _delay_us(5);
   I2C_SDA_out = 1;
   _delay_us(5);
   I2C_SCL = 0;
   _delay_us(5);
end

 

void sta013_read(void) begin

  printf("inside sta013_read\r\n");

   // Start Condition
   sta013_I2C_start();

   //i2c_start();
   fprintf("called I2C_start\r\n");

  

   // Send the device address with the R/W bit (i.e., the 8th bit) cleared
   fprintf("about to try sta013_I2C_write\r\n");
   I2C_byte = 0x86
   sta013_I2C_write();

    if (errorFlag != 0) begin
       fprintf("Read Error: Error writing device address (W) to STA013.\r\n");
       return;
    end

    printf("finished calling sta013_I2C_write, errorFlag = %d\r\n", errorFlag);

   // Send the read address
   fprintf("about to try sta013_I2C_write again\r\n");
   I2C_byte = address;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf("Read Error: Error writing read address to STA013.\r\n");
      return;
   end

   fprintf("finished calling sta013_I2C_write again, errorFlag = %d\r\n", errorFlag);

  

   // Start Condition
   sta013_I2C_start();
   fprintf("called I2C_start again\r\n"); 

   // Send the device address with the R/W bit (i.e., the 8th bit) set
   fprintf("about to try sta013_I2C_write 3rd time\r\n");
   I2C_byte = 0x87;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      fprintf("Read Error: Error writing device address (R) to STA013.\r\n");
      return;
   end

   printf("finished calling sta013_I2C_write 3rd time, errorFlag = %d\r\n", errorFlag);

  

   // Read the data from the given address
   fprintf("about to try sta013_I2C_read\r\n");
   sta013_I2C_read();
   fprintf("finished calling sta013_I2C_read, data = %x\r\n", data);

   // Stop Condition
   sta013_I2C_stop();
end

void sta013_write(void) begin

   // Start Condition
   sta013_I2C_start();

   // Send the device address with the R/W bit (i.e., the 8th bit) cleared
   I2C_byte = 0x86;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      printf("Write Error: Error writing device address (W) to STA013.\r\n");
      return;
   end

   // Send the write address
   I2C_byte = address;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      printf("Write Error: Error writing write address to STA013.\r\n");
      return;
   end

   // Send the data
   I2C_byte = data;
   sta013_I2C_write();

   if (errorFlag != 0) begin
      printf("Write Error: Error writing data to STA013.\r\n");
      return;
   end 

   // Stop Condition
   sta013_I2C_stop();     
end

void sta013_start(void) begin

   printf("Inside sta013_start\r\n");

   // Write 1(0x01) to address 114(0x72)
   address = 0x72;
   data = 0x01;
   sta013_write();

   // Check for an error
   if (errorFlag != 0x00)
      printf("Error: STA013 Start Failed, %d\r\n", errorFlag);
   else
      printf("Starting mp3 player...\r\n");
end

unsigned int config_sta013(void) begin    

   // Read from address 0x01 on the STA013
   fprintf("Starting config_sta013\r\n");

   address = 0x01;                                        // Load the address into the address reg, r4

   sta013_read();                                        // Read data from the STA013

   // Test if address 0x01 contains 0xAC
   printf("data = %x, errorFlag = %d\r\n", data, errorFlag);

   if((data != 0xac) || (errorFlag != 0x00)) begin
      fprintf("Error: STA013 Not Present, %d, \r\n", errorFlag);    // Print out the error flag
      return 1;                                           // Return 1 to indicate that the STA013 is not present
   end

   else

      fprintf("STA013 Present\r\n");


   fprintf("beginning to load STA013 config file\r\n");  

   // Send STA013_updatedata information to STA013 (i.e., STA013 config file)
   for (i = 0 ; i < max_config_index ;) begin
      address = STA013_UpdateData[i];                // Load the address into the address reg, r4
      data    = STA013_UpdateData[i+1];              // Load the data into the data reg, r5
      sta013_write();                                // Write data to STA013

      // Increment i
      i += 2;

      // Check for an error in the write
      if (errorFlag != 0x00) begin
         printf("Error: STA013 Configuration Failed, %d\r\n", errorFlag);   // Print out the error flag
         return 2;                                   // Return 2 to indicate sta013 failed configuration
      end

      // Delay initialization for soft reboot
      if (address == 0x10) begin
          _delay_ms(1000);
      end    
   end

  

   printf("finished loading STA013 config file\r\n");
   printf("beginning to load STA013 board data\r\n");  

   // Send STA013 board specific data

   for (i = 0; i < max_PLL_index; ) begin
      address = config_PLL[i];                            // Load the address into the address reg, r4
      data    = config_PLL[i+1];                          // Load the data into the data reg, r5
      sta013_write();                                     // Write data to STA013

      // Increment i
      i += 2;

      if (errorFlag != 0x00) begin
         printf("Error: STA013 PLL Configuration Failed, %d\r\n", errorFlag);   // Print out the error flag
         return 2;                                   // Return 2 to indicate sta013 failed configuration
      end
   end
 
   printf("finished loading STA013 board data\r\n"); 
   printf("STA013 Configuration Complete\r\n"); 
   return 0;                                         // Return 0 to indicate pass configuration
end

