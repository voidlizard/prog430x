#include "crc.h"

static const unsigned short crc16_tbl[CRC_TABLE_SIZE] = {
#include "./crc16Table.txt"
};

uint32_t crc32MakeBitwise2(uint32_t crc, uint32_t poly, uint8_t *pmsg, uint16_t msg_size)
{
    uint16_t i, j;
    uint32_t msg;
            
    for(i = 0 ; i < msg_size ; i++)
    {
        msg = *pmsg++;
        msg <<= 24;
        
        for(j = 0 ; j < 8 ; j++)
        {
            if((msg ^ crc) >> 31) crc = (crc << 1) ^ poly;
			else crc <<= 1;
			msg <<= 1;
        }
    }
    
    return(crc ^ CRC32_FINAL_XOR);
}

unsigned short crc16_table(unsigned short crc, unsigned char *pbuffer, unsigned int length)
{
    while(length--) 
        crc = crc16_tbl[((crc >> 8) ^ *pbuffer++)] ^ (crc << 8);	// normal

    return(crc ^ CRC16_FINAL_XOR);
}

// this is a C-optimized implementation
uint16_t crc16_bitwise(uint16_t crc, uint16_t poly, unsigned char *pmsg, uint16_t msg_size)
{
    uint16_t i, j;
    uint16_t msg;
    
    for(i = 0 ; i < msg_size ; i ++)
    {
        msg = (*pmsg++ << 8);
        
		for(j = 0 ; j < 8 ; j++)
        {
            if((msg ^ crc) >> 15) crc = (crc << 1) ^ poly;
			else crc <<= 1;	
			msg <<= 1;
        }
    }
   
    return(crc ^ CRC16_FINAL_XOR);
}

