/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/* Note that Tiny-FatFs supports only single drive and always            */
/* accesses drive number 0.                                              */

#define ATA		0
#define MMC		1
#define USB		2

/*************************************************************************/
/*prototypes*/
char SPI(char d);
char Command(char, uint16_t, uint16_t, char );
int MMC_disk_initialize(void);
int MMC_disk_write(char*, long, char);
int MMC_disk_read(char*, long, char);


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_initialize();
		// translate the reslut code here

		return stat;

	case MMC :
		result = MMC_disk_initialize();
		// translate the reslut code here

		return stat;

	case USB :
		result = USB_disk_initialize();
		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_status();
		// translate the reslut code here

		return stat;

	case MMC :
		result = MMC_disk_status();
		// translate the reslut code here

		return stat;

	case USB :
		result = USB_disk_status();
		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_read(buff, sector, count);
		// translate the reslut code here

		return res;

	case MMC :
		result = MMC_disk_read(buff, sector, count);
		// translate the reslut code here

		return res;

	case USB :
		result = USB_disk_read(buff, sector, count);
		// translate the reslut code here

		return res;
	}
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_write(buff, sector, count);
		// translate the reslut code here

		return res;

	case MMC :
		result = MMC_disk_write(buff, sector, count);
		// translate the reslut code here

		return res;

	case USB :
		result = USB_disk_write(buff, sector, count);
		// translate the reslut code here

		return res;
	}
	return RES_PARERR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		// pre-process here

		result = ATA_disk_ioctl(ctrl, buff);
		// post-process here

		return res;

	case MMC :
		// pre-process here

		result = MMC_disk_ioctl(ctrl, buff);
		// post-process here

		return res;

	case USB :
		// pre-process here

		result = USB_disk_ioctl(ctrl, buff);
		// post-process here

		return res;
	}
	return RES_PARERR;
}




char SPI(char d) {  // send character over SPI
	char received = 0;
	SPDR = d;
	while(!(SPSR & (1<<SPIF)));
	received = SPDR;
	return (received);
}


char Command(char befF, uint16_t AdrH, uint16_t AdrL, char befH )
{	// sends a command to the MMC
	SPI(0xFF);
	SPI(befF);
	SPI((uint8_t)(AdrH >> 8));
	SPI((uint8_t)AdrH);
	SPI((uint8_t)(AdrL >> 8));
	SPI((uint8_t)AdrL);
	SPI(befH);
	SPI(0xFF);
	return SPI(0xFF);	// return the last received character
}

int MMC_disk_initialize(void){
	char i;
	PORTB |= (1 << SPICS); // disable MMC
	// start MMC in SPI mode
	for(i=0; i < 10; i++) SPI(0xFF); // send 10*8=80 clock pulses
	PORTB &= ~(1 << SPICS); // enable MMC

	if (Command(0x40,0,0,0x95) != 1) goto mmcerror; // reset MMC

st: // if there is no MMC, prg. loops here
	if (Command(0x41,0,0,0xFF) !=0) goto st;
	return 1;
mmcerror:
	fprintf(stdout,"MMC init error");
	return 0;
}

int MMC_disk_write(char *buff, long sector, char count){
	int i;
	uint8_t c;
	// 512 byte-write-mode
	if (Command(0x58,((sector>>16)&0xff),(sector & 0xff),0xFF) !=0) {
		fprintf(stdout,"MMC: write error 1 ");
		return 1;	
	}
	SPI(0xFF);
	SPI(0xFF);
	SPI(0xFE);
	// write ram sectors to MMC
	for (i=0;i<count;i++) {
		SPI(buff[i]);
	}
	// at the end, send 2 dummy bytes
	SPI(0xFF);
	SPI(0xFF);

	c = SPI(0xFF);
	c &= 0x1F; 	// 0x1F = 0b.0001.1111;
	if (c != 0x05) { // 0x05 = 0b.0000.0101
		fprintf(stdout,"MMC: write error 2 ");
		return 1;
	}
	// wait until MMC is not busy anymore
	while(SPI(0xFF) != (char)0xFF);
	return 0;
}

int MMC_disk_read(char *buff, long sector, char count){
	int i;
	// 512 byte-read-mode 
	if (Command(0x51,((sector>>16)&0xff),(sector & 0xff),0xFF) != 0) {
		fprintf(stdout,"MMC: read error 1 ");
		return 1;
	}
	// wait for 0xFE - start of any transmission
	// ATT: typecast (char)0xFE is a must!
	while(SPI(0xFF) != (char)0xFE);

	for(i=0; i < 512; i++) {
		buff[i] = SPI(0xFF);
	}
	// at the end, send 2 dummy bytes
	SPI(0xFF); // actually this returns the CRC/checksum byte
	SPI(0xFF);
	return 0;
}