#ifndef  __crc_h_
#define __crc_h_

#include <stdint.h>

#define CRC32_POLY	      0x04C11DB7
#define CRC32_INIT_REM    0xFFFFFFFF
#define CRC32_FINAL_XOR   0xFFFFFFFF

#define CRC32R_POLY	      0xEDB88320
#define CRC32R_INIT_REM   0xFFFFFFFF
#define CRC32R_FINAL_XOR  0xFFFFFFFF

#define CRC16_POLY			0x8005
#define CRC16_INIT_REM      0x0
#define CRC16_FINAL_XOR     0x0

#define CRC16R_POLY         0xA001
#define CRC16R_INIT_REM     0x0
#define CRC16R_FINAL_XOR    0x0


#define CRC_TABLE_SIZE	    256

uint32_t crc32MakeBitwise2(uint32_t crc, uint32_t poly, uint8_t *pmsg, uint16_t msg_size);

uint16_t crc16_bitwise(uint16_t crc, uint16_t poly, unsigned char *pmsg, uint16_t msg_size);
unsigned short crc16_table(unsigned short crc, unsigned char *pbuffer, unsigned int length);


#endif
