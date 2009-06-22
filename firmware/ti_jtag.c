#include "fet_hw.h"
#include "jtag.h"

#include "FlashErase.c"
#include "FlashWrite.c"

unsigned long AllShifts(word Format, unsigned long Data)
{
    word tclk = StoreTCLK();  // Store TCLK state;
    unsigned long TDOword = 0x00000000;
    unsigned long MSB = 0x00000000;
    word i;

    switch(Format)
    {
    case F_BYTE: MSB = 0x00000080;
      break;
    case F_WORD: MSB = 0x00008000;
      break;
    case F_ADDR: MSB = 0x00080000;
      break;
    case F_LONG: MSB = 0x80000000;
      break;
    default: // this is an unsupported format, function will just return 0
      return TDOword;
    }

    for (i = Format; i > 0; i--)
    {
      ((Data & MSB) == 0) ? ClrTDI() : SetTDI();
      Data <<= 1;
      if (i == 1)                       // Last bit requires TMS=1
        SetTMS();
      ClrTCK();
      SetTCK();
      TDOword <<= 1;                    // TDO could be any port pin
      if (ScanTDO() != 0)
        TDOword++;    
    }

    // common exit
    RestoreTCLK(tclk);                  // restore TCLK state

    // JTAG FSM = Exit-DR
    ClrTCK();
    SetTCK();
    // JTAG FSM = Update-DR
    ClrTMS();
    ClrTCK();
    SetTCK();
    // JTAG FSM = Run-Test/Idle
    return(TDOword);
}

word DR_Shift16(word data)
{
    // JTAG FSM state = Run-Test/Idle
    SetTMS();
    ClrTCK();
    SetTCK();

    // JTAG FSM state = Select DR-Scan
    ClrTMS();
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Capture-DR
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Shift-DR, Shift in TDI (16-bit)
    return(AllShifts(F_WORD, data));
    // JTAG FSM state = Run-Test/Idle
}

unsigned long DR_Shift20(unsigned long address)
{
    // JTAG FSM state = Run-Test/Idle
    SetTMS();
    ClrTCK();
    SetTCK();

    // JTAG FSM state = Select DR-Scan
    ClrTMS();
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Capture-DR
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Shift-DR, Shift in TDI (16-bit)
    return(AllShifts(F_ADDR, address));
    // JTAG FSM state = Run-Test/Idle
}


word IR_Shift(byte instruction)
{
    // JTAG FSM state = Run-Test/Idle
    SetTMS();
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Select DR-Scan
    ClrTCK();
    SetTCK();

    // JTAG FSM state = Select IR-Scan
    ClrTMS();
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Capture-IR
    ClrTCK();
    SetTCK();
    // JTAG FSM state = Shift-IR, Shift in TDI (8-bit)
    return(AllShifts(F_BYTE, instruction));
    // JTAG FSM state = Run-Test/Idle
}

void ResetTAP(void)
{
    word i;

    // process TDI first to settle fuse current
    SetTDI();
    SetTMS();
    SetTCK();

    // Now fuse is checked, Reset JTAG FSM
    for (i = 6; i > 0; i--)
    {
        ClrTCK();
        SetTCK();
    }
    // JTAG FSM is now in Test-Logic-Reset
    ClrTCK();
    ClrTMS();
    SetTCK();
    SetTMS();
    // JTAG FSM is now in Run-Test/IDLE
}

word StartJtag(void)
{
    word JtagId = 0x00;

    // drive JTAG/TEST signals
    {
      DrvSignals();
      MsDelay(10);             // delay 10ms
    }

    ResetTAP();  // reset TAP state machine -> Run-Test/Idle
    JtagId = (byte)IR_Shift(IR_BYPASS);

    return JtagId;
}

void StopJtag(void)
{
    // release JTAG/TEST signals
    {
      RlsSignals();
      MsDelay(10);             // delay 10ms
    }
}

word ExecutePOR_430Xv2(void)
{
  word i = 0;

  // provide one clock
  ClrTCLK();
  SetTCLK();

  // prepare access to the JTAG CNTRL SIG register  
  IR_Shift(IR_CNTRL_SIG_16BIT);
  // release CPUSUSP signal and apply POR signal
  DR_Shift16(0x0C01);
  // release POR signal again
  DR_Shift16(0x0401);
  
  // provide 5 clock cycles
  for (i = 0; i < 5; i++)
  {
    ClrTCLK();
    SetTCLK();
  }
  // now set CPUSUSP signal again
  DR_Shift16(0x0501);
  // and provide one more clock
  ClrTCLK();
  SetTCLK();
  // the CPU is now in 'Full-Emulation-State'
  
  // disable Watchdog Timer on target device now by setting the HOLD signal
  // in the WDT_CNTRL register
  WriteMem_430Xv2(F_WORD, 0x015C, 0x5A80);

  // Check if device is again in Full-Emulation-State and return status
  IR_Shift(IR_CNTRL_SIG_CAPTURE);
  if(DR_Shift16(0) & 0x0301)
  {
    return(STATUS_OK);
  }
  
  return(STATUS_ERROR);
}

word SyncJtag_AssertPor (void)
{
  word i = 0;

  IR_Shift(IR_CNTRL_SIG_16BIT);
  DR_Shift16(0x1501);                  // Set device into JTAG mode + read
  if (IR_Shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID91)
  {
    return(STATUS_ERROR);
  }
  // wait for sync
  while(!(DR_Shift16(0) & 0x0200) && i < 50)
  {
    i++;
  };
  // continues if sync was sucessfull
  if(i >= 50)
  {
    return(STATUS_ERROR);
  }

  // execute a Power-On-Reset
  if(ExecutePOR_430Xv2() != STATUS_OK)
  {
    return(STATUS_ERROR);
  }
  
  return(STATUS_OK);
}

word ReadMem_430Xv2(word Format, unsigned long Addr)
{
  word TDOword = 0;
  
  // Check Init State at the beginning
  IR_Shift(IR_CNTRL_SIG_CAPTURE);
  if(DR_Shift16(0) & 0x0301)
  {
    // Read Memory
    ClrTCLK();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    if  (Format == F_WORD)
    {
      DR_Shift16(0x0501);             // Set word read
    }
    else
    {
      DR_Shift16(0x0511);             // Set byte read
    }
    IR_Shift(IR_ADDR_16BIT);
    DR_Shift20(Addr);                   // Set address
    IR_Shift(IR_DATA_TO_ADDR);
    SetTCLK();
    ClrTCLK();
    TDOword = DR_Shift16(0x0000);       // Shift out 16 bits
    
    SetTCLK();
    // one or more cycle, so CPU is driving correct MAB
    ClrTCLK();
    SetTCLK();
    // Processor is now again in Init State
  }
  
  return TDOword;
}

void WriteMem_430Xv2(word Format, unsigned long Addr, word Data)
{
  // Check Init State at the beginning
  IR_Shift(IR_CNTRL_SIG_CAPTURE);
  if(DR_Shift16(0) & 0x0301)
  {
    ClrTCLK();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    if  (Format == F_WORD)
    {
      DR_Shift16(0x0500);
    }
    else
    {
      DR_Shift16(0x0510);
    }
    IR_Shift(IR_ADDR_16BIT);
    DR_Shift20(Addr);
    
    SetTCLK();
    // New style: Only apply data during clock high phase
    IR_Shift(IR_DATA_TO_ADDR);
    DR_Shift16(Data);           // Shift in 16 bits
    ClrTCLK();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0501);
    SetTCLK();
    // one or more cycle, so CPU is driving correct MAB
    ClrTCLK();
    SetTCLK();
    // Processor is now again in Init State
  }
}

void ReadMemQuick_430Xv2(unsigned long StartAddr, unsigned long Length, word *DataArray)
{
  unsigned long i;
  
  SetPC_430Xv2(StartAddr);
  SetTCLK();
  IR_Shift(IR_CNTRL_SIG_16BIT);
  DR_Shift16(0x0501);
  IR_Shift(IR_ADDR_CAPTURE);
  
  IR_Shift(IR_DATA_QUICK);
  
  for (i = 0; i < Length; i++)
  {
    SetTCLK();
    ClrTCLK();
    *DataArray++   = DR_Shift16(0);  // Read data from memory.
  }
  IR_Shift(IR_CNTRL_SIG_CAPTURE);
}

void WriteMemQuick_430Xv2(unsigned long StartAddr, unsigned long Length, word *DataArray)
{
  unsigned long i;
  
  for (i = 0; i < Length; i++)
  {
    WriteMem_430Xv2(F_WORD, StartAddr, DataArray[i]);
    StartAddr += 2;
  }
}

void SetPC_430Xv2(unsigned long Addr)
{
  unsigned short Mova;
  unsigned short Pc_l;
  
  Mova  = 0x0080;
  Mova += (unsigned short)((Addr>>8) & 0x00000F00);
  Pc_l  = (unsigned short)((Addr & 0xFFFF));
  
  // Check Full-Emulation-State at the beginning
  IR_Shift(IR_CNTRL_SIG_CAPTURE);
  if(DR_Shift16(0) & 0x0301)
  {
    // MOVA #imm20, PC
    ClrTCLK();
    // take over bus control during clock LOW phase
    IR_Shift(IR_DATA_16BIT);
    SetTCLK();
    DR_Shift16(Mova);
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x1400);
    IR_Shift(IR_DATA_16BIT);
    ClrTCLK();
    SetTCLK();
    DR_Shift16(Pc_l);
    ClrTCLK();
    SetTCLK();
    DR_Shift16(0x4303);    
    ClrTCLK();
    IR_Shift(IR_ADDR_CAPTURE);    
    DR_Shift20(0x00000);
  }
}


void WriteFLASH_430Xv2(unsigned long StartAddr, unsigned long Length, word *DataArray)
{
  word loadAddr  = 0x1C00;                       // RAM start address of MSP430F5438
  word startAddr = loadAddr + FlashWrite_o[0];   // start address of the program in traget RAM

  FlashWrite_o[2] = (unsigned short)(StartAddr);     // set write start address
  FlashWrite_o[3] = (unsigned short)(StartAddr>>16);
  FlashWrite_o[4] = (unsigned short)(Length);        // set number of words to write
  FlashWrite_o[5] = (unsigned short)(Length>>16);
//  FlashErase_o[6] = 0xA508;         // -> FCTL3: unlock INFO Segment A
  FlashErase_o[6] = 0xA548;           // -> FCTL3: keep INFO Segment A locked

  WriteMemQuick_430Xv2(loadAddr, FlashWrite_o_length/2, (word*)FlashWrite_o);
  ReleaseDevice_430Xv2(startAddr);

  {
    unsigned long Jmb = 0;
    unsigned long Timeout = 0;
    
    do
    {
      Jmb = i_ReadJmbOut();
      Timeout++;
    }
    while(Jmb != 0xABADBABE && Timeout < 3000);
    
    if(Timeout < 3000)
    {
      unsigned long i;
      
      for(i = 0; i < Length; i++)
      {
        i_WriteJmbIn(DataArray[i]);
      }
    }
  }
  {
    unsigned long Jmb = 0;
    unsigned long Timeout = 0;
    
    do
    {
      Jmb = i_ReadJmbOut();
      Timeout++;
    }
    while(Jmb != 0xCAFEBABE && Timeout < 3000);
  }

  SyncJtag_AssertPor();

  // clear RAM here - init with JMP $
  {
    word i;

    for (i = 0; i < FlashWrite_o_length/2; i++)
    {
      WriteMem_430Xv2(F_WORD, loadAddr, 0x3fff);
      loadAddr += 2;
    }
  }
}


unsigned long i_ReadJmbOut(void)
{
  unsigned short sJMBINCTL;
  unsigned long  lJMBOUT = 0;
  unsigned short sJMBOUT0, sJMBOUT1;
  
  sJMBINCTL = 0;
  
  IR_Shift(IR_JMB_EXCHANGE);
  lJMBOUT = DR_Shift16(sJMBINCTL);
  
  if(lJMBOUT & OUT1RDY)
  {
    sJMBINCTL |= JMB32B + OUTREQ;
    lJMBOUT  = DR_Shift16(sJMBINCTL);
    sJMBOUT0 = (unsigned short)DR_Shift16(0);
    sJMBOUT1 = (unsigned short)DR_Shift16(0);
    
    lJMBOUT = ((unsigned long)sJMBOUT1<<16) + sJMBOUT0;
  }
  
  return lJMBOUT;
}

void i_WriteJmbIn(word lJMBIN)
{
  unsigned short sJMBINCTL;
  unsigned short sJMBOUTCTL;
  unsigned long Timeout = 0;
    
  sJMBINCTL = INREQ;
  
  IR_Shift(IR_JMB_EXCHANGE);
  do
  {
    sJMBOUTCTL = (unsigned short)DR_Shift16(sJMBINCTL);
    Timeout++;
  }
  while(!(sJMBOUTCTL & IN0RDY) && Timeout < 3000);
  
  if(Timeout < 3000)
  {
    DR_Shift16(lJMBIN);
  }
}

void ReleaseDevice_430Xv2(unsigned long Addr)
{
  switch(Addr)
  {
  case V_BOR:
    // perform a BOR via JTAG - we loose control of the device then...
    IR_Shift(IR_TEST_REG);
    DR_Shift16(0x0200);
    MsDelay(5);     // wait some time before doing any other action
    // JTAG control is lost now - GetDevice() needs to be called again to gain control.
    break;
  case V_RESET:
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0C01);                 // Perform a reset
    DR_Shift16(0x0401);
    IR_Shift(IR_CNTRL_SIG_RELEASE);
    break;
  default:
    SetPC_430Xv2(Addr);                 // Set target CPU's PC
    // prepare release & release
    SetTCLK();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0401);
    IR_Shift(IR_ADDR_CAPTURE);
    IR_Shift(IR_CNTRL_SIG_RELEASE);
  }
}

void EraseFLASH_430Xv2(word EraseMode, unsigned long EraseAddr)
{
  word loadAddr  = 0x1C00;                       // RAM start address of MSP430F5438
  word startAddr = loadAddr + FlashErase_o[0];   // start address of the program in target RAM

  FlashErase_o[2] = (unsigned short)(EraseAddr);     // set dummy write address
  FlashErase_o[3] = (unsigned short)(EraseAddr>>16);
  FlashErase_o[4] = EraseMode;                       // set erase mode
//  FlashErase_o[5] = 0xA508;         // -> FCTL3: unlock INFO Segment A
  FlashErase_o[5] = 0xA548;           // -> FCTL3: keep INFO Segment A locked

  WriteMemQuick_430Xv2(loadAddr, FlashErase_o_length/2, (word*)FlashErase_o);
  ReleaseDevice_430Xv2(startAddr);

  {
    unsigned long Jmb = 0;
    unsigned long Timeout = 0;
    
    do
    {
      Jmb = i_ReadJmbOut();
      Timeout++;
    }
    while(Jmb != 0xCAFEBABE && Timeout < 3000);
  }

  SyncJtag_AssertPor();

  // clear RAM here - init with JMP $
  {
    word i;

    for (i = 0; i < FlashErase_o_length/2; i++)
    {
      WriteMem_430Xv2(F_WORD, loadAddr, 0x3fff);
      loadAddr += 2;
    }
  }

}

void EraseFLASH_430Xv2_wo_release(word EraseMode, unsigned long EraseAddr)
{
  word loadAddr  = 0x1C00;                       // RAM start address of MSP430F5438
  word startAddr = loadAddr + FlashErase_o[0];   // start address of the program in target RAM

  FlashErase_o[2] = (unsigned short)(EraseAddr);     // set dummy write address
  FlashErase_o[3] = (unsigned short)(EraseAddr>>16);
  FlashErase_o[4] = EraseMode;                       // set erase mode
//  FlashErase_o[5] = 0xA508;         // -> FCTL3: unlock INFO Segment A
  FlashErase_o[5] = 0xA548;           // -> FCTL3: keep INFO Segment A locked

  WriteMemQuick_430Xv2(loadAddr, FlashErase_o_length/2, (word*)FlashErase_o);
  
  {
    word i;
    SetPC_430Xv2(startAddr);
    for (i=110; i--;)
    {
      ClrTCLK();
      SetTCLK();
    }
  }

  //max mass/segment erase time for F543x is 32uS
  //do not check mailbox, just wait..
  usDelay(35);

  //let Erase function finish
  {
    word i;
    for (i=110; i--;) //110 //to let finish the erase routine the restauration
    {
      ClrTCLK();
      SetTCLK();
    }
  }

  SyncJtag_AssertPor();

  // clear RAM here - init with JMP $
  {
    word i;

    for (i = 0; i < FlashErase_o_length/2; i++)
    {
      WriteMem_430Xv2(F_WORD, loadAddr, 0x3fff);
      loadAddr += 2;
    }
  }
}

void WriteFLASH_430Xv2_wo_release(unsigned long StartAddr, unsigned long Length, word *DataArray)
{
  word loadAddr  = 0x1C00;                       // RAM start address of MSP430F5438
  word startAddr = loadAddr + FlashWrite_o[0];   // start address of the program in traget RAM

  FlashWrite_o[2] = (unsigned short)(StartAddr);     // set write start address
  FlashWrite_o[3] = (unsigned short)(StartAddr>>16);
  FlashWrite_o[4] = (unsigned short)(Length);        // set number of words to write
  FlashWrite_o[5] = (unsigned short)(Length>>16);
//  FlashErase_o[6] = 0xA508;         // -> FCTL3: unlock INFO Segment A
  FlashErase_o[6] = 0xA548;           // -> FCTL3: keep INFO Segment A locked

  WriteMemQuick_430Xv2(loadAddr, FlashWrite_o_length/2, (word*)FlashWrite_o);
  
  //deliver CPU clocks from JTAG
  {
    word i;
    SetPC_430Xv2(startAddr);
    for (i=110; i--;) //110  //105 clocks till signal 'wait for data'
    {
      ClrTCLK();
      SetTCLK();
    }
  }

  {
    unsigned long i;
    IR_Shift(IR_JMB_EXCHANGE);    //set mailbox
    for(i = 0; i < Length; i++)
    {
      int j;
      
      DR_Shift16(INREQ);          //set request for mailbox
      DR_Shift16(DataArray[i]);   // put data into mailbox
      
      for (j=30; j--;)   // 30 TCLKs
      {
        ClrTCLK();
        SetTCLK();
      }
    }
  }

  //deliver CPU clocks from JTAG
  {
    int i;
    for (i=110; i--;) //110 //to let finish the write routine the restauration
    {
      ClrTCLK();
      SetTCLK();
    }
  }

  SyncJtag_AssertPor();

  // clear RAM here - init with JMP $
  {
    word i;

    for (i = 0; i < FlashWrite_o_length/2; i++)
    {
      WriteMem_430Xv2(F_WORD, loadAddr, 0x3fff);
      loadAddr += 2;
    }
  }
}


word IsLockKeyProgrammed(void)
{
    word i;
    word data = 0x0000;

    for (i = 3; i > 0; i--)     //  First trial could be wrong
    {
        IR_Shift(IR_CNTRL_SIG_CAPTURE);
        data = DR_Shift16(0xAAAA);

        if (data == 0x5555)
        {
            return(STATUS_OK);  // Fuse is blown
        }
    }
    return(STATUS_ERROR);       // fuse is not blown
}

