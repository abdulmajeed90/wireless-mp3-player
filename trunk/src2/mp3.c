#include <avr/io.h>
#include"MMC_SD/MMC_SD.h" //head files
#include"FAT/FAT.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include "UART/uart.h"
#include <stddef.h>

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

#define PATH (unsigned char *)("\\")

//file read variables
struct direntry MusicInfo;			//the mp3 file item whichi will be played

uint8 retry = 0;
uint8 sub = 0;

//prototypes
void init(void);
void listFiles(uint8 *);
void play(uint8 *);
char* str_tok(char*, char*);

char *last;

int main() {

	init();
	uint8 r1;
	
//	while (1) {
		MMC_SD_Init();	//SPI initialize
		fprintf(stdout,"SPI initialized\n\r");
	
		while(r1 = MMC_SD_Reset())//初始化SD卡					//sd card initialize
		{
			retry++;
			fprintf(stdout,"%d ",r1);
			if (retry>200)retry = 0;
			if(retry>200)
			{
			//	LED2_ON();
				while(1)
				{
					PORTD &= ~(1<<PIND2);
					_delay_ms(150);
					PORTD |= (1<<PIND2);
					_delay_ms(250);
				}//while
			}//if
		}//while
		fprintf(stdout,"SD card online\n\r");
		if(FAT_Init())//initialize file system  FAT16 and FAT32 are supported
		{
			while(1)
			{
				PORTD &= ~(1<<PIND2);
				_delay_ms(250);
				PORTD |= (1<<PIND2);
				_delay_ms(500);
			}
		}
		fprintf(stdout,"FAT file system online\n\r");

    listFiles(PATH);
	play(PATH);
//	}
}

void play(uint8 *path){
//starts playing the first file in the root folder
	
	uint16_t songs = 0;					//total songs in the root directery on the SD card
	uint16_t totalsongs;
	uint8_t type;							//file type
	uint8_t buffer[512];
	uint32 p;
	uint32 totalsect, sectors;
	uint16 leftbytes;
	uint8_t SectorsPerClust = 4;

	Search(PATH,&MusicInfo,&songs,&type);  //find total number of files
	totalsongs = songs;

	songs = 1;

	Search(PATH,&MusicInfo,&songs,&type);  //obtain first file
	fprintf(stdout,"Name: %s", MusicInfo.deName);

	p = MusicInfo.deStartCluster+(((uint32)MusicInfo.deHighClust)<<16);//读文件首簇	//the first cluster of the file
		
	totalsect = MusicInfo.deFileSize/512; 	//calculate the total sectors
	leftbytes = MusicInfo.deFileSize%512; 	//calculate the left bytes	
	sectors=0;
	do
	for (int k = 0; k<SectorsPerClust; k++){
		FAT_LoadPartCluster(p,k,buffer);//读一个扇区	//read a sector
		for(int i = 0; i<512; i++) {
			//fprintf(stdout,"%x",buffer[i]);
			loop_until_bit_is_set(UCSR0A, UDRE0);
  			UDR0 = buffer[i];
			_delay_us(200);
		}
		p=FAT_NextCluster(p);
		sectors++;
	}while (sectors<totalsect);
	
	songs = 2;

	Search(PATH,&MusicInfo,&songs,&type);  //obtain first file
	fprintf(stdout,"Name: %s", MusicInfo.deName);

	p = MusicInfo.deStartCluster+(((uint32)MusicInfo.deHighClust)<<16);//读文件首簇	//the first cluster of the file
		
	totalsect = MusicInfo.deFileSize/512; 	//calculate the total sectors
	leftbytes = MusicInfo.deFileSize%512; 	//calculate the left bytes	
	sectors=0;
	do{
	for (int k = 0; k<SectorsPerClust; k++){
		FAT_LoadPartCluster(p,k,buffer);//读一个扇区	//read a sector
		for(int i = 0; i<512; i++) {
			//fprintf(stdout,"%x",buffer[i]);
			loop_until_bit_is_set(UCSR0A, UDRE0);
  			UDR0 = buffer[i];
			_delay_us(200);
		}
		sectors++;
	}
	p=FAT_NextCluster(p);
	}while (sectors<totalsect);
}

void listFiles(uint8 *path){
  
  char dirname[10];
  char filename[10];
  char ext[5];
  char pathstr[100];
  uint16 totalsongs = 0;					//total songs in the root directery on the SD card
  uint8 type;							//file type
  uint16 count = 0;


  strcat(pathstr, path);

  Search(path,&MusicInfo,&totalsongs,&type);//count the number of files in PATH
  count = totalsongs;//save that number
  for(uint16 s = 1; s <= count; s++){ //read each file, output name
    totalsongs = s;
    Search(path,&MusicInfo,&totalsongs,&type);
    for (uint8 i = 0; i<8;i++){//remove trailing spaces in file name
        if (MusicInfo.deName[i]==' ') {
          filename[i] = '\0';
          break; 
        }
        filename[i] = MusicInfo.deName[i];
      }
    for (uint8 i = 0; i<3;i++){//remove trailing spaces in extension 
       ext[i] = MusicInfo.deExtension[i];
    }
    ext[3] = '\0';
    if (type != 5) 
      fprintf(stdout, "\tFile name: %s.%s \n\r", filename,ext);
    else if (type == 5)        //subdirectoy
    {
      dirname[0] = '\\'; //directory name seperator
      for (uint8 i = 1; i<9;i++){//remove trailing spaces
        if (MusicInfo.deName[i-1]==' ') {
          dirname[i] = '\0';
          break; 
        }
        dirname[i] = MusicInfo.deName[i-1];
      }
      fprintf(stdout, "Directory name: %s \n\r", dirname);
      strcat(pathstr, dirname);
      listFiles(dirname);
    }
  }
  fprintf(stdout,"_______________________________________\n\r"); //a line between directory entries


//	fprintf(stdout,"\n\rCard size = %\n\r",MMC_SD_ReadCapacity());
}


void init(void) {
	
	//init serial
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"UART running\n\r");
	
	DDRD |= (1<<PIND2);

	fprintf(stdout,"MCU online\n\r");
	//serialterminate();

	//fprintf(stdout,"SD card online\n\r");
	//serialterminate();

	sei(); // enable interrupts

}
