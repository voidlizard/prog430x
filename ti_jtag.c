
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

#ifdef SPI_MODE					// Shift via SPI pins, entry: TCK=1, TMS=0
    if(Format == F_WORD || Format == F_BYTE)    // ...but only for 8 or 16bit shifts!!
    {
	JTAGSEL |= (TDI | TDO | TCK);		// Function select to SPI module
	// Process 8 MSBs if 16 bits
	if (Format == F_WORD)
	{
		UCTL |= CHAR;			// SPI: set 8bit mode
		UTXBUF = (byte)(Data >> 8);	// Write TX Buffer: 8 MSBs
		while ((UTCTL & TXEPT) == 0);
		MSB = (URXBUF << 8);		// Get RX Buffer: 8 MSBs
	}
	// Process higher 7 LSBs
	UCTL &= ~CHAR;				// SPI: set 7bit mode
	UTXBUF = (byte)(Data);			// Write TX Buffer: 7(+1) LSBs
	while ((UTCTL & TXEPT) == 0);
	TDOword = MSB + (URXBUF << 1);		// Combine 15 MSBs
	JTAGSEL &= ~(TDI | TDO | TCK);		// Function select back to ports
	// process LSB discretely due to TMS   	
	((Data & 0x01) == 0) ? ClrTDI() : SetTDI();
	SetTMS();				// Last bit requires TMS=1
	ClrTCK();
	SetTCK();
	if (ScanTDO() != 0)
		TDOword++;	 	   	
    }
    else
    {
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
    }

#else						// Shift via port pins, no coding necessary

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

#endif

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
