#ifndef __jtag_h
#define __jtag_h

#define F_BYTE                     8
#define F_WORD                     16
#define F_ADDR                     20
#define F_LONG                     32

// Instructions for the JTAG control signal register
#define IR_CNTRL_SIG_16BIT         0xC8   // 0x13 original values
#define IR_CNTRL_SIG_CAPTURE       0x28   // 0x14
#define IR_CNTRL_SIG_RELEASE       0xA8   // 0x15
// Instructions for the JTAG Fuse
#define IR_PREPARE_BLOW            0x44   // 0x22
#define IR_EX_BLOW                 0x24   // 0x24
// Instructions for the JTAG data register
#define IR_DATA_16BIT              0x82   // 0x41
#define IR_DATA_QUICK              0xC2   // 0x43
// Instructions for the JTAG PSA mode
#define IR_DATA_PSA                0x22   // 0x44
#define IR_SHIFT_OUT_PSA           0x62   // 0x46
// Instructions for the JTAG address register
#define IR_ADDR_16BIT              0xC1   // 0x83
#define IR_ADDR_CAPTURE            0x21   // 0x84
#define IR_DATA_TO_ADDR            0xA1   // 0x85
// Bypass instruction
#define IR_BYPASS                  0xFF   // 0xFF

// JTAG identification value for all existing Flash-based MSP430 devices
#define JTAG_ID                    0x89
#define JTAG_ID91                  0x91

// additional instructions for JTAG_ID91 architectures
#define IR_COREIP_ID               0xE8   // 0x17
#define IR_DEVICE_ID               0xE1   // 0x87
// Instructions for the JTAG mailbox
#define IR_JMB_EXCHANGE            0x86   // 0x61
#define IR_TEST_REG                0x54   // 0x2A

// Constants for JTAG mailbox data exchange
#define OUT1RDY 0x0008
#define IN0RDY  0x0001
#define JMB32B  0x0010
#define OUTREQ  0x0004
#define INREQ   0x0001


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

void init_target();
void release_target(void);

#include "ti_jtag.h"


#endif

