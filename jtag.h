#ifndef __jtag_h
#define __jtag_h

#define F_BYTE                     8
#define F_WORD                     16
#define F_ADDR                     20
#define F_LONG                     32

// Constants for runoff status
#define STATUS_ERROR     0      // false
#define STATUS_OK        1      // true
#define STATUS_FUSEBLOWN 2      // GetDevice returns if the security fuse is blown

#define STATUS_ACTIVE    2
#define STATUS_IDLE      3

#define TCLK             TDI

#define ClrTMS()         ((JTAGOUT) &= (~TMS))
#define SetTMS()         ((JTAGOUT) |= (TMS))
#define ClrTDI()         ((JTAGOUT) &= (~TDI))
#define SetTDI()         ((JTAGOUT) |= (TDI))
#define ClrTCK()         ((JTAGOUT) &= (~TCK))
#define SetTCK()         ((JTAGOUT) |= (TCK))
#define ClrTCLK()        ((JTAGOUT) &= (~TCLK))
#define SetTCLK()        ((JTAGOUT) |= (TCLK))
#define StoreTCLK()      ((JTAGOUT  &   TCLK))
#define RestoreTCLK(x)   (x == 0 ? (JTAGOUT &= ~TCLK) : (JTAGOUT |= TCLK))
#define ScanTDO()        ((JTAGIN   &   TDO))   // assumes TDO to be bit0

#define SetRST()          RSTOUT  |=  RST_BIT
#define ClrRST()          RSTOUT  &= ~RST_BIT
#define ReleaseRST()     (RSTDIR  &= ~RST_BIT)
#define SetTST()         (TSTOUT  |=  TST_BIT)
#define ClrTST()         (TSTOUT  &= ~TST_BIT)

#define DrvSignals()     {JTAGOUT |= (TDI | TMS | TCK | TCLK); \
                          JTAGDIR |= (TDI | TMS | TCK | TCLK); \
                         }

#define RlsSignals()     {JTAGDIR  = 0; \
                          JTAGOUT  = 0; \
                         }

void init_target(void);
void release_target(void);

#endif

