import os

with_dir  = lambda d,l: map(lambda x: os.path.join(d,x),l)  

CC = 'msp430-gcc'
MCU = '-mmcu=msp430x1611'

WARNINGS=" ".join([
	 '-Wall' 
	,'-Wshadow' 
	,'-Wpointer-arith' 
	,'-Wbad-function-cast' 
	,'-Wcast-align' 
	,'-Wsign-compare' 
	,'-Waggregate-return' 
#	,'-Wstrict-prototypes' 
#	,'-Wmissing-prototypes' 
	,'-Wmissing-declarations' 
	,'-Wunused'
])

CCFLAGS_DEBUG = " ".join([ '-Os'
                           ,"-g"
                           ,MCU
						   ,'-DGCC_MSP430',
                           '-DFET_HW_TEST'
						   ,WARNINGS
])

LDFLAGS = " ".join([
	 CCFLAGS_DEBUG
    ,'-Wl,--reduce-memory-overheads'
#   ,'-Wl,--verbose'
])

FW_FILES = [
	  'fet_hw.c'
	 ,'jtag.c'
     ,'main.c'
]

FILES = FW_FILES 

env = Environment( CC=CC,
                   CCFLAGS=CCFLAGS_DEBUG, LINKFLAGS=LDFLAGS,
				   CPPPATH=['.'],
				 )

env.PrependENVPath('PATH', '/opt/mspgcc/bin')

env.Program ( 'prog430x.elf', FILES)

