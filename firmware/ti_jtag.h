#ifndef __ti_jtag_h
#define __ti_jtag_h

#include "platform.h"
#include "fet_hw.h"

#define V_RESET    0xFFFE
#define V_BOR      0x1B08

#define ERASE_MASS 0xA50C // one bank in main mem arrays
#define ERASE_SGMT 0xA50A // SELECTED segment

#define MsDelay(x)  delay_ms((x))
#define usDelay(x)  delay_us((x))

unsigned long AllShifts(word Format, unsigned long Data);
word DR_Shift16(word data);
unsigned long DR_Shift20(unsigned long address);
word IR_Shift(byte instruction);
void ResetTAP(void);
word StartJtag(void);
void StopJtag(void);

word ExecutePOR_430Xv2(void);
word SyncJtag_AssertPor(void);

void ReleaseDevice_430Xv2(unsigned long Addr);

void SetPC_430Xv2(unsigned long Addr);

word ReadMem_430Xv2(word Format, unsigned long Addr);
void ReadMemQuick_430Xv2(unsigned long StartAddr, unsigned long Length, word *DataArray);

void WriteMem_430Xv2(word Format, unsigned long Addr, word Data);
void WriteMemQuick_430Xv2(unsigned long StartAddr, unsigned long Length, word *DataArray);

unsigned long i_ReadJmbOut(void);
void i_WriteJmbIn(word lJMBIN);

void WriteFLASH_430Xv2(unsigned long StartAddr, unsigned long Length, word *DataArray);
void WriteFLASH_430Xv2_wo_release(unsigned long StartAddr, unsigned long Length, word *DataArray);

void EraseFLASH_430Xv2(word EraseMode, unsigned long EraseAddr);
void EraseFLASH_430Xv2_wo_release(word EraseMode, unsigned long EraseAddr);

word IsLockKeyProgrammed(void);

#endif

