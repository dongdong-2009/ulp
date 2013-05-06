;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

; ===============================================												
; ADuC7060 HEADER FILE REV 1.1												
; ===============================================												

; Set ALLOW_DEPRECATED_NAMES true to enable deprecated register names
#define ALLOW_DEPRECATED_NAMES         0x0

Z_BASE                         DEFINE       0xFFFF0000
F_BASE                         DEFINE       0xFFFFF000
MMR_BASE                       DEFINE       0xFFFF0000
MMR_BASE_LOW                   DEFINE       0xFFFF0000
MMR_BASE_HIGH                  DEFINE       0xFFFFF000

; INTERRUPT CONTROLLER

INTBASE                        DEFINE       0xFFFF0000
IRQSTA                         DEFINE       0xFFFF0000
IRQSIG                         DEFINE       0xFFFF0004
IRQEN                          DEFINE       0xFFFF0008
IRQCLR                         DEFINE       0xFFFF000C
SWICFG                         DEFINE       0xFFFF0010
IRQBASE                        DEFINE       0xFFFF0014
IRQVEC                         DEFINE       0xFFFF001C
IRQP0                          DEFINE       0xFFFF0020
IRQP1                          DEFINE       0xFFFF0024
IRQP2                          DEFINE       0xFFFF0028
IRQP3                          DEFINE       0xFFFF002C
IRQCONN                        DEFINE       0xFFFF0030
IRQCONE                        DEFINE       0xFFFF0034
IRQCLRE                        DEFINE       0xFFFF0038
IRQSTAN                        DEFINE       0xFFFF003C
FIQSTA                         DEFINE       0xFFFF0100
FIQSIG                         DEFINE       0xFFFF0104
FIQEN                          DEFINE       0xFFFF0108
FIQCLR                         DEFINE       0xFFFF010C
FIQVEC                         DEFINE       0xFFFF011C
FIQSTAN                        DEFINE       0xFFFF013C
Z_IRQSTA                       DEFINE       IRQSTA    -Z_BASE
Z_IRQSIG                       DEFINE       IRQSIG    -Z_BASE
Z_IRQEN                        DEFINE       IRQEN     -Z_BASE
Z_IRQCLR                       DEFINE       IRQCLR    -Z_BASE
Z_SWICFG                       DEFINE       SWICFG    -Z_BASE
Z_IRQBASE                      DEFINE       IRQBASE   -Z_BASE
Z_IRQVEC                       DEFINE       IRQVEC    -Z_BASE
Z_IRQP0                        DEFINE       IRQP0     -Z_BASE
Z_IRQP1                        DEFINE       IRQP1     -Z_BASE
Z_IRQP2                        DEFINE       IRQP2     -Z_BASE
Z_IRQP3                        DEFINE       IRQP3     -Z_BASE
Z_IRQCONN                      DEFINE       IRQCONN   -Z_BASE
Z_IRQCONE                      DEFINE       IRQCONE   -Z_BASE
Z_IRQCLRE                      DEFINE       IRQCLRE   -Z_BASE
Z_IRQSTAN                      DEFINE       IRQSTAN   -Z_BASE
Z_FIQSTA                       DEFINE       FIQSTA    -Z_BASE
Z_FIQSIG                       DEFINE       FIQSIG    -Z_BASE
Z_FIQEN                        DEFINE       FIQEN     -Z_BASE
Z_FIQCLR                       DEFINE       FIQCLR    -Z_BASE
Z_FIQVEC                       DEFINE       FIQVEC    -Z_BASE
Z_FIQSTAN                      DEFINE       FIQSTAN   -Z_BASE

T_IRQSTA                       DEFINE       IRQSTA    -INTBASE
T_IRQSIG                       DEFINE       IRQSIG    -INTBASE
T_IRQEN                        DEFINE       IRQEN     -INTBASE
T_IRQCLR                       DEFINE       IRQCLR    -INTBASE
T_SWICFG                       DEFINE       SWICFG    -INTBASE
T_IRQBASE                      DEFINE       IRQBASE   -INTBASE
T_IRQVEC                       DEFINE       IRQVEC    -INTBASE
T_IRQP0                        DEFINE       IRQP0     -INTBASE
T_IRQP1                        DEFINE       IRQP1     -INTBASE
T_IRQP2                        DEFINE       IRQP2     -INTBASE
T_IRQP3                        DEFINE       IRQP3     -INTBASE
T_IRQCONN                      DEFINE       IRQCONN   -INTBASE
T_IRQCONE                      DEFINE       IRQCONE   -INTBASE
T_IRQCLRE                      DEFINE       IRQCLRE   -INTBASE
T_IRQSTAN                      DEFINE       IRQSTAN   -INTBASE
T_FIQSTA                       DEFINE       FIQSTA    -INTBASE
T_FIQSIG                       DEFINE       FIQSIG    -INTBASE
T_FIQEN                        DEFINE       FIQEN     -INTBASE
T_FIQCLR                       DEFINE       FIQCLR    -INTBASE
T_FIQVEC                       DEFINE       FIQVEC    -INTBASE
T_FIQSTAN                      DEFINE       FIQSTAN   -INTBASE

IRQSTA_OFFSET                  DEFINE       0x0000
IRQSIG_OFFSET                  DEFINE       0x0004
IRQEN_OFFSET                   DEFINE       0x0008
IRQCLR_OFFSET                  DEFINE       0x000C
SWICFG_OFFSET                  DEFINE       0x0010
IRQBASE_OFFSET                 DEFINE       0x0014
IRQVEC_OFFSET                  DEFINE       0x001C
IRQP0_OFFSET                   DEFINE       0x0020
IRQP1_OFFSET                   DEFINE       0x0024
IRQP2_OFFSET                   DEFINE       0x0028
IRQP3_OFFSET                   DEFINE       0x002C
IRQCONN_OFFSET                 DEFINE       0x0030
IRQCONE_OFFSET                 DEFINE       0x0034
IRQCLRE_OFFSET                 DEFINE       0x0038
IRQSTAN_OFFSET                 DEFINE       0x003C
FIQSTA_OFFSET                  DEFINE       0x0100
FIQSIG_OFFSET                  DEFINE       0x0104
FIQEN_OFFSET                   DEFINE       0x0108
FIQCLR_OFFSET                  DEFINE       0x010C
FIQVEC_OFFSET                  DEFINE       0x011C
FIQSTAN_OFFSET                 DEFINE       0x013C

; REMAP AND SYSTEM CONTROL

REMAPBASE                      DEFINE       0xFFFF0200
REMAP                          DEFINE       0xFFFF0220
RSTSTA                         DEFINE       0xFFFF0230
RSTCLR                         DEFINE       0xFFFF0234
Z_REMAP                        DEFINE       REMAP     -Z_BASE
Z_RSTSTA                       DEFINE       RSTSTA    -Z_BASE
Z_RSTCLR                       DEFINE       RSTCLR    -Z_BASE

T_REMAP                        DEFINE       REMAP     -REMAPBASE
T_RSTSTA                       DEFINE       RSTSTA    -REMAPBASE
T_RSTCLR                       DEFINE       RSTCLR    -REMAPBASE

REMAP_OFFSET                   DEFINE       0x0220
RSTSTA_OFFSET                  DEFINE       0x0230
RSTCLR_OFFSET                  DEFINE       0x0234

; TIMER 0

T0BASE                         DEFINE       0xFFFF0320
T0LD                           DEFINE       0xFFFF0320
T0VAL                          DEFINE       0xFFFF0324
T0CON                          DEFINE       0xFFFF0328
T0CLRI                         DEFINE       0xFFFF032C
T0CAP                          DEFINE       0xFFFF0330
Z_T0LD                         DEFINE       T0LD      -Z_BASE
Z_T0VAL                        DEFINE       T0VAL     -Z_BASE
Z_T0CON                        DEFINE       T0CON     -Z_BASE
Z_T0CLRI                       DEFINE       T0CLRI    -Z_BASE
Z_T0CAP                        DEFINE       T0CAP     -Z_BASE

T_T0LD                         DEFINE       T0LD      -T0BASE
T_T0VAL                        DEFINE       T0VAL     -T0BASE
T_T0CON                        DEFINE       T0CON     -T0BASE
T_T0CLRI                       DEFINE       T0CLRI    -T0BASE
T_T0CAP                        DEFINE       T0CAP     -T0BASE

T0LD_OFFSET                    DEFINE       0x0320
T0VAL_OFFSET                   DEFINE       0x0324
T0CON_OFFSET                   DEFINE       0x0328
T0CLRI_OFFSET                  DEFINE       0x032C
T0CAP_OFFSET                   DEFINE       0x0330

; WAKE UP TIMER

T1BASE                         DEFINE       0xFFFF0340
T1LD                           DEFINE       0xFFFF0340
T1VAL                          DEFINE       0xFFFF0344
T1CON                          DEFINE       0xFFFF0348
T1CLRI                         DEFINE       0xFFFF034C
Z_T1LD                         DEFINE       T1LD      -Z_BASE
Z_T1VAL                        DEFINE       T1VAL     -Z_BASE
Z_T1CON                        DEFINE       T1CON     -Z_BASE
Z_T1CLRI                       DEFINE       T1CLRI    -Z_BASE

T_T1LD                         DEFINE       T1LD      -T1BASE
T_T1VAL                        DEFINE       T1VAL     -T1BASE
T_T1CON                        DEFINE       T1CON     -T1BASE
T_T1CLRI                       DEFINE       T1CLRI    -T1BASE

T1LD_OFFSET                    DEFINE       0x0340
T1VAL_OFFSET                   DEFINE       0x0344
T1CON_OFFSET                   DEFINE       0x0348
T1CLRI_OFFSET                  DEFINE       0x034C

; WATCHDOG TIMER

T2BASE                         DEFINE       0xFFFF0360
T2LD                           DEFINE       0xFFFF0360
T2VAL                          DEFINE       0xFFFF0364
T2CON                          DEFINE       0xFFFF0368
T2CLRI                         DEFINE       0xFFFF036C
T2RCFG                         DEFINE       0xFFFF0370
Z_T2LD                         DEFINE       T2LD      -Z_BASE
Z_T2VAL                        DEFINE       T2VAL     -Z_BASE
Z_T2CON                        DEFINE       T2CON     -Z_BASE
Z_T2CLRI                       DEFINE       T2CLRI    -Z_BASE
Z_T2RCFG                       DEFINE       T2RCFG    -Z_BASE

T_T2LD                         DEFINE       T2LD      -T2BASE
T_T2VAL                        DEFINE       T2VAL     -T2BASE
T_T2CON                        DEFINE       T2CON     -T2BASE
T_T2CLRI                       DEFINE       T2CLRI    -T2BASE
T_T2RCFG                       DEFINE       T2RCFG    -T2BASE

T2LD_OFFSET                    DEFINE       0x0360
T2VAL_OFFSET                   DEFINE       0x0364
T2CON_OFFSET                   DEFINE       0x0368
T2CLRI_OFFSET                  DEFINE       0x036C
T2RCFG_OFFSET                  DEFINE       0x0370

; TIMER 3

T3BASE                         DEFINE       0xFFFF0380
T3LD                           DEFINE       0xFFFF0380
T3VAL                          DEFINE       0xFFFF0384
T3CON                          DEFINE       0xFFFF0388
T3CLRI                         DEFINE       0xFFFF038C
T3CAP                          DEFINE       0xFFFF0390
Z_T3LD                         DEFINE       T3LD      -Z_BASE
Z_T3VAL                        DEFINE       T3VAL     -Z_BASE
Z_T3CON                        DEFINE       T3CON     -Z_BASE
Z_T3CLRI                       DEFINE       T3CLRI    -Z_BASE
Z_T3CAP                        DEFINE       T3CAP     -Z_BASE

T_T3LD                         DEFINE       T3LD      -T3BASE
T_T3VAL                        DEFINE       T3VAL     -T3BASE
T_T3CON                        DEFINE       T3CON     -T3BASE
T_T3CLRI                       DEFINE       T3CLRI    -T3BASE
T_T3CAP                        DEFINE       T3CAP     -T3BASE

T3LD_OFFSET                    DEFINE       0x0380
T3VAL_OFFSET                   DEFINE       0x0384
T3CON_OFFSET                   DEFINE       0x0388
T3CLRI_OFFSET                  DEFINE       0x038C
T3CAP_OFFSET                   DEFINE       0x0390

; PLL AND OSCILLATOR CONTROL

PLLBASE                        DEFINE       0xFFFF0400
PLLSTA                         DEFINE       0xFFFF0400
POWKEY1                        DEFINE       0xFFFF0404
POWCON0                        DEFINE       0xFFFF0408
POWKEY2                        DEFINE       0xFFFF040C
PLLKEY1                        DEFINE       0xFFFF0410
PLLCON                         DEFINE       0xFFFF0414
PLLKEY2                        DEFINE       0xFFFF0418
POWKEY3                        DEFINE       0xFFFF0434
POWCON1                        DEFINE       0xFFFF0438
POWKEY4                        DEFINE       0xFFFF043C
GP0KEY1                        DEFINE       0xFFFF0464
GP0CON1                        DEFINE       0xFFFF0468
GP0KEY2                        DEFINE       0xFFFF046C
Z_PLLSTA                       DEFINE       PLLSTA    -Z_BASE
Z_POWKEY1                      DEFINE       POWKEY1   -Z_BASE
Z_POWCON0                      DEFINE       POWCON0   -Z_BASE
Z_POWKEY2                      DEFINE       POWKEY2   -Z_BASE
Z_PLLKEY1                      DEFINE       PLLKEY1   -Z_BASE
Z_PLLCON                       DEFINE       PLLCON    -Z_BASE
Z_PLLKEY2                      DEFINE       PLLKEY2   -Z_BASE
Z_POWKEY3                      DEFINE       POWKEY3   -Z_BASE
Z_POWCON1                      DEFINE       POWCON1   -Z_BASE
Z_POWKEY4                      DEFINE       POWKEY4   -Z_BASE
Z_GP0KEY1                      DEFINE       GP0KEY1   -Z_BASE
Z_GP0CON1                      DEFINE       GP0CON1   -Z_BASE
Z_GP0KEY2                      DEFINE       GP0KEY2   -Z_BASE

T_PLLSTA                       DEFINE       PLLSTA    -PLLBASE
T_POWKEY1                      DEFINE       POWKEY1   -PLLBASE
T_POWCON0                      DEFINE       POWCON0   -PLLBASE
T_POWKEY2                      DEFINE       POWKEY2   -PLLBASE
T_PLLKEY1                      DEFINE       PLLKEY1   -PLLBASE
T_PLLCON                       DEFINE       PLLCON    -PLLBASE
T_PLLKEY2                      DEFINE       PLLKEY2   -PLLBASE
T_POWKEY3                      DEFINE       POWKEY3   -PLLBASE
T_POWCON1                      DEFINE       POWCON1   -PLLBASE
T_POWKEY4                      DEFINE       POWKEY4   -PLLBASE
T_GP0KEY1                      DEFINE       GP0KEY1   -PLLBASE
T_GP0CON1                      DEFINE       GP0CON1   -PLLBASE
T_GP0KEY2                      DEFINE       GP0KEY2   -PLLBASE

PLLSTA_OFFSET                  DEFINE       0x0400
POWKEY1_OFFSET                 DEFINE       0x0404
POWCON0_OFFSET                 DEFINE       0x0408
POWKEY2_OFFSET                 DEFINE       0x040C
PLLKEY1_OFFSET                 DEFINE       0x0410
PLLCON_OFFSET                  DEFINE       0x0414
PLLKEY2_OFFSET                 DEFINE       0x0418
POWKEY3_OFFSET                 DEFINE       0x0434
POWCON1_OFFSET                 DEFINE       0x0438
POWKEY4_OFFSET                 DEFINE       0x043C
GP0KEY1_OFFSET                 DEFINE       0x0464
GP0CON1_OFFSET                 DEFINE       0x0468
GP0KEY2_OFFSET                 DEFINE       0x046C



; ADC INTERFACE REGISTERS

ADCBASE                        DEFINE       0xFFFF0500
ADCSTA                         DEFINE       0xFFFF0500
ADCMSKI                        DEFINE       0xFFFF0504
ADCMDE                         DEFINE       0xFFFF0508
ADC0CON                        DEFINE       0xFFFF050C
ADC1CON                        DEFINE       0xFFFF0510
ADCFLT                         DEFINE       0xFFFF0514
ADCCFG                         DEFINE       0xFFFF0518
ADC0DAT                        DEFINE       0xFFFF051C
ADC1DAT                        DEFINE       0xFFFF0520
ADC0OF                         DEFINE       0xFFFF0524
ADC1OF                         DEFINE       0xFFFF0528
ADC0GN                         DEFINE       0xFFFF052C
ADC1GN                         DEFINE       0xFFFF0530
ADCORCR                        DEFINE       0xFFFF0534
ADCORCV                        DEFINE       0xFFFF0538
ADCOTH                         DEFINE       0xFFFF053C
ADCOTHC                        DEFINE       0xFFFF0540
ADCOTHV                        DEFINE       0xFFFF0544
ADCOACC                        DEFINE       0xFFFF0548
ADCOATH                        DEFINE       0xFFFF054C
IEXCON                         DEFINE       0xFFFF0570
Z_ADCSTA                       DEFINE       ADCSTA    -Z_BASE
Z_ADCMSKI                      DEFINE       ADCMSKI   -Z_BASE
Z_ADCMDE                       DEFINE       ADCMDE    -Z_BASE
Z_ADC0CON                      DEFINE       ADC0CON   -Z_BASE
Z_ADC1CON                      DEFINE       ADC1CON   -Z_BASE
Z_ADCFLT                       DEFINE       ADCFLT    -Z_BASE
Z_ADCCFG                       DEFINE       ADCCFG    -Z_BASE
Z_ADC0DAT                      DEFINE       ADC0DAT   -Z_BASE
Z_ADC1DAT                      DEFINE       ADC1DAT   -Z_BASE
Z_ADC0OF                       DEFINE       ADC0OF    -Z_BASE
Z_ADC1OF                       DEFINE       ADC1OF    -Z_BASE
Z_ADC0GN                       DEFINE       ADC0GN    -Z_BASE
Z_ADC1GN                       DEFINE       ADC1GN    -Z_BASE
Z_ADCORCR                      DEFINE       ADCORCR   -Z_BASE
Z_ADCORCV                      DEFINE       ADCORCV   -Z_BASE
Z_ADCOTH                       DEFINE       ADCOTH    -Z_BASE
Z_ADCOTHC                      DEFINE       ADCOTHC   -Z_BASE
Z_ADCOTHV                      DEFINE       ADCOTHV   -Z_BASE
Z_ADCOACC                      DEFINE       ADCOACC   -Z_BASE
Z_ADCOATH                      DEFINE       ADCOATH   -Z_BASE
Z_IEXCON                       DEFINE       IEXCON    -Z_BASE

T_ADCSTA                       DEFINE       ADCSTA    -ADCBASE
T_ADCMSKI                      DEFINE       ADCMSKI   -ADCBASE
T_ADCMDE                       DEFINE       ADCMDE    -ADCBASE
T_ADC0CON                      DEFINE       ADC0CON   -ADCBASE
T_ADC1CON                      DEFINE       ADC1CON   -ADCBASE
T_ADCFLT                       DEFINE       ADCFLT    -ADCBASE
T_ADCCFG                       DEFINE       ADCCFG    -ADCBASE
T_ADC0DAT                      DEFINE       ADC0DAT   -ADCBASE
T_ADC1DAT                      DEFINE       ADC1DAT   -ADCBASE
T_ADC0OF                       DEFINE       ADC0OF    -ADCBASE
T_ADC1OF                       DEFINE       ADC1OF    -ADCBASE
T_ADC0GN                       DEFINE       ADC0GN    -ADCBASE
T_ADC1GN                       DEFINE       ADC1GN    -ADCBASE
T_ADCORCR                      DEFINE       ADCORCR   -ADCBASE
T_ADCORCV                      DEFINE       ADCORCV   -ADCBASE
T_ADCOTH                       DEFINE       ADCOTH    -ADCBASE
T_ADCOTHC                      DEFINE       ADCOTHC   -ADCBASE
T_ADCOTHV                      DEFINE       ADCOTHV   -ADCBASE
T_ADCOACC                      DEFINE       ADCOACC   -ADCBASE
T_ADCOATH                      DEFINE       ADCOATH   -ADCBASE
T_IEXCON                       DEFINE       IEXCON    -ADCBASE

ADCSTA_OFFSET                  DEFINE       0x0500
ADCMSKI_OFFSET                 DEFINE       0x0504
ADCMDE_OFFSET                  DEFINE       0x0508
ADC0CON_OFFSET                 DEFINE       0x050C
ADC1CON_OFFSET                 DEFINE       0x0510
ADCFLT_OFFSET                  DEFINE       0x0514
ADCCFG_OFFSET                  DEFINE       0x0518
ADC0DAT_OFFSET                 DEFINE       0x051C
ADC1DAT_OFFSET                 DEFINE       0x0520
ADC0OF_OFFSET                  DEFINE       0x0524
ADC1OF_OFFSET                  DEFINE       0x0528
ADC0GN_OFFSET                  DEFINE       0x052C
ADC1GN_OFFSET                  DEFINE       0x0530
ADCORCR_OFFSET                 DEFINE       0x0534
ADCORCV_OFFSET                 DEFINE       0x0538
ADCOTH_OFFSET                  DEFINE       0x053C
ADCOTHC_OFFSET                 DEFINE       0x0540
ADCOTHV_OFFSET                 DEFINE       0x0544
ADCOACC_OFFSET                 DEFINE       0x0548
ADCOATH_OFFSET                 DEFINE       0x054C
IEXCON_OFFSET                  DEFINE       0x0570

; DAC INTERFACE REGISTERS

DACBASE                        DEFINE       0xFFFF0600
DACCON                         DEFINE       0xFFFF0600
DACDAT                         DEFINE       0xFFFF0604
Z_DACCON                       DEFINE       DACCON    -Z_BASE
Z_DACDAT                       DEFINE       DACDAT    -Z_BASE

T_DACCON                       DEFINE       DACCON    -DACBASE
T_DACDAT                       DEFINE       DACDAT    -DACBASE

DACCON_OFFSET                  DEFINE       0x0600
DACDAT_OFFSET                  DEFINE       0x0604

; 450 COMPATIABLE UART CORE REGISTERS

UARTBASE                       DEFINE       0xFFFF0700
COMTX                          DEFINE       0xFFFF0700
COMRX                          DEFINE       0xFFFF0700
COMDIV0                        DEFINE       0xFFFF0700
COMIEN0                        DEFINE       0xFFFF0704
COMDIV1                        DEFINE       0xFFFF0704
COMIID0                        DEFINE       0xFFFF0708
COMCON0                        DEFINE       0xFFFF070C
COMCON1                        DEFINE       0xFFFF0710
COMSTA0                        DEFINE       0xFFFF0714
COMSTA1                        DEFINE       0xFFFF0718
COMSCR                         DEFINE       0xFFFF071C
COMDIV2                        DEFINE       0xFFFF072C
Z_COMTX                        DEFINE       COMTX     -Z_BASE
Z_COMRX                        DEFINE       COMRX     -Z_BASE
Z_COMDIV0                      DEFINE       COMDIV0   -Z_BASE
Z_COMIEN0                      DEFINE       COMIEN0   -Z_BASE
Z_COMDIV1                      DEFINE       COMDIV1   -Z_BASE
Z_COMIID0                      DEFINE       COMIID0   -Z_BASE
Z_COMCON0                      DEFINE       COMCON0   -Z_BASE
Z_COMCON1                      DEFINE       COMCON1   -Z_BASE
Z_COMSTA0                      DEFINE       COMSTA0   -Z_BASE
Z_COMSTA1                      DEFINE       COMSTA1   -Z_BASE
Z_COMSCR                       DEFINE       COMSCR    -Z_BASE
Z_COMDIV2                      DEFINE       COMDIV2   -Z_BASE

T_COMTX                        DEFINE       COMTX     -UARTBASE
T_COMRX                        DEFINE       COMRX     -UARTBASE
T_COMDIV0                      DEFINE       COMDIV0   -UARTBASE
T_COMIEN0                      DEFINE       COMIEN0   -UARTBASE
T_COMDIV1                      DEFINE       COMDIV1   -UARTBASE
T_COMIID0                      DEFINE       COMIID0   -UARTBASE
T_COMCON0                      DEFINE       COMCON0   -UARTBASE
T_COMCON1                      DEFINE       COMCON1   -UARTBASE
T_COMSTA0                      DEFINE       COMSTA0   -UARTBASE
T_COMSTA1                      DEFINE       COMSTA1   -UARTBASE
T_COMSCR                       DEFINE       COMSCR    -UARTBASE
T_COMDIV2                      DEFINE       COMDIV2   -UARTBASE

COMTX_OFFSET                   DEFINE       0x0700
COMRX_OFFSET                   DEFINE       0x0700
COMDIV0_OFFSET                 DEFINE       0x0700
COMIEN0_OFFSET                 DEFINE       0x0704
COMDIV1_OFFSET                 DEFINE       0x0704
COMIID0_OFFSET                 DEFINE       0x0708
COMCON0_OFFSET                 DEFINE       0x070C
COMCON1_OFFSET                 DEFINE       0x0710
COMSTA0_OFFSET                 DEFINE       0x0714
COMSTA1_OFFSET                 DEFINE       0x0718
COMSCR_OFFSET                  DEFINE       0x071C
COMDIV2_OFFSET                 DEFINE       0x072C

; I2C BUS PERIPHERAL DEVICE

I2CBASE                        DEFINE       0xFFFF0900
I2CMCON                        DEFINE       0xFFFF0900
I2CMSTA                        DEFINE       0xFFFF0904
I2CMRX                         DEFINE       0xFFFF0908
I2CMTX                         DEFINE       0xFFFF090C
I2CMCNT0                       DEFINE       0xFFFF0910
I2CMCNT1                       DEFINE       0xFFFF0914
I2CADR0                        DEFINE       0xFFFF0918
I2CADR1                        DEFINE       0xFFFF091C
I2CREP                         DEFINE       0xFFFF0920
I2CDIV                         DEFINE       0xFFFF0924
I2CSCON                        DEFINE       0xFFFF0928
I2CSSTA                        DEFINE       0xFFFF092C
I2CSRX                         DEFINE       0xFFFF0930
I2CSTX                         DEFINE       0xFFFF0934
I2CALT                         DEFINE       0xFFFF0938
I2CID0                         DEFINE       0xFFFF093C
I2CID1                         DEFINE       0xFFFF0940
I2CID2                         DEFINE       0xFFFF0944
I2CID3                         DEFINE       0xFFFF0948
I2CFSTA                        DEFINE       0xFFFF094C
I2CRCON                        DEFINE       0xFFFF0950
Z_I2CMCON                      DEFINE       I2CMCON   -Z_BASE
Z_I2CMSTA                      DEFINE       I2CMSTA   -Z_BASE
Z_I2CMRX                       DEFINE       I2CMRX    -Z_BASE
Z_I2CMTX                       DEFINE       I2CMTX    -Z_BASE
Z_I2CMCNT0                     DEFINE       I2CMCNT0  -Z_BASE
Z_I2CMCNT1                     DEFINE       I2CMCNT1  -Z_BASE
Z_I2CADR0                      DEFINE       I2CADR0   -Z_BASE
Z_I2CADR1                      DEFINE       I2CADR1   -Z_BASE
Z_I2CREP                       DEFINE       I2CREP    -Z_BASE
Z_I2CDIV                       DEFINE       I2CDIV    -Z_BASE
Z_I2CSCON                      DEFINE       I2CSCON   -Z_BASE
Z_I2CSSTA                      DEFINE       I2CSSTA   -Z_BASE
Z_I2CSRX                       DEFINE       I2CSRX    -Z_BASE
Z_I2CSTX                       DEFINE       I2CSTX    -Z_BASE
Z_I2CALT                       DEFINE       I2CALT    -Z_BASE
Z_I2CID0                       DEFINE       I2CID0    -Z_BASE
Z_I2CID1                       DEFINE       I2CID1    -Z_BASE
Z_I2CID2                       DEFINE       I2CID2    -Z_BASE
Z_I2CID3                       DEFINE       I2CID3    -Z_BASE
Z_I2CFSTA                      DEFINE       I2CFSTA   -Z_BASE
Z_I2CRCON                      DEFINE       I2CRCON   -Z_BASE

T_I2CMCON                      DEFINE       I2CMCON   -I2CBASE
T_I2CMSTA                      DEFINE       I2CMSTA   -I2CBASE
T_I2CMRX                       DEFINE       I2CMRX    -I2CBASE
T_I2CMTX                       DEFINE       I2CMTX    -I2CBASE
T_I2CMCNT0                     DEFINE       I2CMCNT0  -I2CBASE
T_I2CMCNT1                     DEFINE       I2CMCNT1  -I2CBASE
T_I2CADR0                      DEFINE       I2CADR0   -I2CBASE
T_I2CADR1                      DEFINE       I2CADR1   -I2CBASE
T_I2CREP                       DEFINE       I2CREP    -I2CBASE
T_I2CDIV                       DEFINE       I2CDIV    -I2CBASE
T_I2CSCON                      DEFINE       I2CSCON   -I2CBASE
T_I2CSSTA                      DEFINE       I2CSSTA   -I2CBASE
T_I2CSRX                       DEFINE       I2CSRX    -I2CBASE
T_I2CSTX                       DEFINE       I2CSTX    -I2CBASE
T_I2CALT                       DEFINE       I2CALT    -I2CBASE
T_I2CID0                       DEFINE       I2CID0    -I2CBASE
T_I2CID1                       DEFINE       I2CID1    -I2CBASE
T_I2CID2                       DEFINE       I2CID2    -I2CBASE
T_I2CID3                       DEFINE       I2CID3    -I2CBASE
T_I2CFSTA                      DEFINE       I2CFSTA   -I2CBASE
T_I2CRCON                      DEFINE       I2CRCON   -I2CBASE

I2CMCON_OFFSET                 DEFINE       0x0900
I2CMSTA_OFFSET                 DEFINE       0x0904
I2CMRX_OFFSET                  DEFINE       0x0908
I2CMTX_OFFSET                  DEFINE       0x090C
I2CMCNT0_OFFSET                DEFINE       0x0910
I2CMCNT1_OFFSET                DEFINE       0x0914
I2CADR0_OFFSET                 DEFINE       0x0918
I2CADR1_OFFSET                 DEFINE       0x091C
I2CREP_OFFSET                  DEFINE       0x0920
I2CDIV_OFFSET                  DEFINE       0x0924
I2CSCON_OFFSET                 DEFINE       0x0928
I2CSSTA_OFFSET                 DEFINE       0x092C
I2CSRX_OFFSET                  DEFINE       0x0930
I2CSTX_OFFSET                  DEFINE       0x0934
I2CALT_OFFSET                  DEFINE       0x0938
I2CID0_OFFSET                  DEFINE       0x093C
I2CID1_OFFSET                  DEFINE       0x0940
I2CID2_OFFSET                  DEFINE       0x0944
I2CID3_OFFSET                  DEFINE       0x0948
I2CFSTA_OFFSET                 DEFINE       0x094C
I2CRCON_OFFSET                 DEFINE       0x0950

; SERIAL PORT INTERFACE PERIPHERAL

SPIBASE                        DEFINE       0xFFFF0A00
SPISTA                         DEFINE       0xFFFF0A00
SPIRX                          DEFINE       0xFFFF0A04
SPITX                          DEFINE       0xFFFF0A08
SPIDIV                         DEFINE       0xFFFF0A0C
SPICON                         DEFINE       0xFFFF0A10
Z_SPISTA                       DEFINE       SPISTA    -Z_BASE
Z_SPIRX                        DEFINE       SPIRX     -Z_BASE
Z_SPITX                        DEFINE       SPITX     -Z_BASE
Z_SPIDIV                       DEFINE       SPIDIV    -Z_BASE
Z_SPICON                       DEFINE       SPICON    -Z_BASE

T_SPISTA                       DEFINE       SPISTA    -SPIBASE
T_SPIRX                        DEFINE       SPIRX     -SPIBASE
T_SPITX                        DEFINE       SPITX     -SPIBASE
T_SPIDIV                       DEFINE       SPIDIV    -SPIBASE
T_SPICON                       DEFINE       SPICON    -SPIBASE

SPISTA_OFFSET                  DEFINE       0x0A00
SPIRX_OFFSET                   DEFINE       0x0A04
SPITX_OFFSET                   DEFINE       0x0A08
SPIDIV_OFFSET                  DEFINE       0x0A0C
SPICON_OFFSET                  DEFINE       0x0A10

; GPIO AND SERIAL PORT MUX

GPIOBASE                       DEFINE       0xFFFF0D00
GP0CON0                        DEFINE       0xFFFF0D00
GP1CON                         DEFINE       0xFFFF0D04
GP2CON                         DEFINE       0xFFFF0D08
GP0DAT                         DEFINE       0xFFFF0D20
GP0SET                         DEFINE       0xFFFF0D24
GP0CLR                         DEFINE       0xFFFF0D28
GP0PAR                         DEFINE       0xFFFF0D2C
GP1DAT                         DEFINE       0xFFFF0D30
GP1SET                         DEFINE       0xFFFF0D34
GP1CLR                         DEFINE       0xFFFF0D38
GP1PAR                         DEFINE       0xFFFF0D3C
GP2DAT                         DEFINE       0xFFFF0D40
GP2SET                         DEFINE       0xFFFF0D44
GP2CLR                         DEFINE       0xFFFF0D48
GP2PAR                         DEFINE       0xFFFF0D4C
Z_GP0CON0                      DEFINE       GP0CON0   -Z_BASE
Z_GP1CON                       DEFINE       GP1CON    -Z_BASE
Z_GP2CON                       DEFINE       GP2CON    -Z_BASE
Z_GP0DAT                       DEFINE       GP0DAT    -Z_BASE
Z_GP0SET                       DEFINE       GP0SET    -Z_BASE
Z_GP0CLR                       DEFINE       GP0CLR    -Z_BASE
Z_GP0PAR                       DEFINE       GP0PAR    -Z_BASE
Z_GP1DAT                       DEFINE       GP1DAT    -Z_BASE
Z_GP1SET                       DEFINE       GP1SET    -Z_BASE
Z_GP1CLR                       DEFINE       GP1CLR    -Z_BASE
Z_GP1PAR                       DEFINE       GP1PAR    -Z_BASE
Z_GP2DAT                       DEFINE       GP2DAT    -Z_BASE
Z_GP2SET                       DEFINE       GP2SET    -Z_BASE
Z_GP2CLR                       DEFINE       GP2CLR    -Z_BASE
Z_GP2PAR                       DEFINE       GP2PAR    -Z_BASE

T_GP0CON0                      DEFINE       GP0CON0   -GPIOBASE
T_GP1CON                       DEFINE       GP1CON    -GPIOBASE
T_GP2CON                       DEFINE       GP2CON    -GPIOBASE
T_GP0DAT                       DEFINE       GP0DAT    -GPIOBASE
T_GP0SET                       DEFINE       GP0SET    -GPIOBASE
T_GP0CLR                       DEFINE       GP0CLR    -GPIOBASE
T_GP0PAR                       DEFINE       GP0PAR    -GPIOBASE
T_GP1DAT                       DEFINE       GP1DAT    -GPIOBASE
T_GP1SET                       DEFINE       GP1SET    -GPIOBASE
T_GP1CLR                       DEFINE       GP1CLR    -GPIOBASE
T_GP1PAR                       DEFINE       GP1PAR    -GPIOBASE
T_GP2DAT                       DEFINE       GP2DAT    -GPIOBASE
T_GP2SET                       DEFINE       GP2SET    -GPIOBASE
T_GP2CLR                       DEFINE       GP2CLR    -GPIOBASE
T_GP2PAR                       DEFINE       GP2PAR    -GPIOBASE

GP0CON0_OFFSET                 DEFINE       0x0D00
GP1CON_OFFSET                  DEFINE       0x0D04
GP2CON_OFFSET                  DEFINE       0x0D08
GP0DAT_OFFSET                  DEFINE       0x0D20
GP0SET_OFFSET                  DEFINE       0x0D24
GP0CLR_OFFSET                  DEFINE       0x0D28
GP0PAR_OFFSET                  DEFINE       0x0D2C
GP1DAT_OFFSET                  DEFINE       0x0D30
GP1SET_OFFSET                  DEFINE       0x0D34
GP1CLR_OFFSET                  DEFINE       0x0D38
GP1PAR_OFFSET                  DEFINE       0x0D3C
GP2DAT_OFFSET                  DEFINE       0x0D40
GP2SET_OFFSET                  DEFINE       0x0D44
GP2CLR_OFFSET                  DEFINE       0x0D48
GP2PAR_OFFSET                  DEFINE       0x0D4C

; FLASH CONTROL INTERFACE

FLASHBASE                      DEFINE       0xFFFF0E00
FEESTA                         DEFINE       0xFFFF0E00
FEEMOD                         DEFINE       0xFFFF0E04
FEECON                         DEFINE       0xFFFF0E08
FEEDAT                         DEFINE       0xFFFF0E0C
FEEADR                         DEFINE       0xFFFF0E10
FEESIGN                        DEFINE       0xFFFF0E18
FEEPRO                         DEFINE       0xFFFF0E1C
FEEHIDE                        DEFINE       0xFFFF0E20
Z_FEESTA                       DEFINE       FEESTA    -Z_BASE
Z_FEEMOD                       DEFINE       FEEMOD    -Z_BASE
Z_FEECON                       DEFINE       FEECON    -Z_BASE
Z_FEEDAT                       DEFINE       FEEDAT    -Z_BASE
Z_FEEADR                       DEFINE       FEEADR    -Z_BASE
Z_FEESIGN                      DEFINE       FEESIGN   -Z_BASE
Z_FEEPRO                       DEFINE       FEEPRO    -Z_BASE
Z_FEEHIDE                      DEFINE       FEEHIDE   -Z_BASE

T_FEESTA                       DEFINE       FEESTA    -FLASHBASE
T_FEEMOD                       DEFINE       FEEMOD    -FLASHBASE
T_FEECON                       DEFINE       FEECON    -FLASHBASE
T_FEEDAT                       DEFINE       FEEDAT    -FLASHBASE
T_FEEADR                       DEFINE       FEEADR    -FLASHBASE
T_FEESIGN                      DEFINE       FEESIGN   -FLASHBASE
T_FEEPRO                       DEFINE       FEEPRO    -FLASHBASE
T_FEEHIDE                      DEFINE       FEEHIDE   -FLASHBASE

FEESTA_OFFSET                  DEFINE       0x0E00
FEEMOD_OFFSET                  DEFINE       0x0E04
FEECON_OFFSET                  DEFINE       0x0E08
FEEDAT_OFFSET                  DEFINE       0x0E0C
FEEADR_OFFSET                  DEFINE       0x0E10
FEESIGN_OFFSET                 DEFINE       0x0E18
FEEPRO_OFFSET                  DEFINE       0x0E1C
FEEHIDE_OFFSET                 DEFINE       0x0E20

; PWM

PWMBASE                        DEFINE       0xFFFF0F80
PWMCON                         DEFINE       0xFFFF0F80
PWM0COM0                       DEFINE       0xFFFF0F84
PWM0COM1                       DEFINE       0xFFFF0F88
PWM0COM2                       DEFINE       0xFFFF0F8C
PWM0LEN                        DEFINE       0xFFFF0F90
PWM1COM0                       DEFINE       0xFFFF0F94
PWM1COM1                       DEFINE       0xFFFF0F98
PWM1COM2                       DEFINE       0xFFFF0F9C
PWM1LEN                        DEFINE       0xFFFF0FA0
PWM2COM0                       DEFINE       0xFFFF0FA4
PWM2COM1                       DEFINE       0xFFFF0FA8
PWM2COM2                       DEFINE       0xFFFF0FAC
PWM2LEN                        DEFINE       0xFFFF0FB0
PWMCLRI                        DEFINE       0xFFFF0FB8
Z_PWMCON                       DEFINE       PWMCON    -Z_BASE
Z_PWM0COM0                     DEFINE       PWM0COM0  -Z_BASE
Z_PWM0COM1                     DEFINE       PWM0COM1  -Z_BASE
Z_PWM0COM2                     DEFINE       PWM0COM2  -Z_BASE
Z_PWM0LEN                      DEFINE       PWM0LEN   -Z_BASE
Z_PWM1COM0                     DEFINE       PWM1COM0  -Z_BASE
Z_PWM1COM1                     DEFINE       PWM1COM1  -Z_BASE
Z_PWM1COM2                     DEFINE       PWM1COM2  -Z_BASE
Z_PWM1LEN                      DEFINE       PWM1LEN   -Z_BASE
Z_PWM2COM0                     DEFINE       PWM2COM0  -Z_BASE
Z_PWM2COM1                     DEFINE       PWM2COM1  -Z_BASE
Z_PWM2COM2                     DEFINE       PWM2COM2  -Z_BASE
Z_PWM2LEN                      DEFINE       PWM2LEN   -Z_BASE
Z_PWMCLRI                      DEFINE       PWMCLRI   -Z_BASE

T_PWMCON                       DEFINE       PWMCON    -PWMBASE
T_PWM0COM0                     DEFINE       PWM0COM0  -PWMBASE
T_PWM0COM1                     DEFINE       PWM0COM1  -PWMBASE
T_PWM0COM2                     DEFINE       PWM0COM2  -PWMBASE
T_PWM0LEN                      DEFINE       PWM0LEN   -PWMBASE
T_PWM1COM0                     DEFINE       PWM1COM0  -PWMBASE
T_PWM1COM1                     DEFINE       PWM1COM1  -PWMBASE
T_PWM1COM2                     DEFINE       PWM1COM2  -PWMBASE
T_PWM1LEN                      DEFINE       PWM1LEN   -PWMBASE
T_PWM2COM0                     DEFINE       PWM2COM0  -PWMBASE
T_PWM2COM1                     DEFINE       PWM2COM1  -PWMBASE
T_PWM2COM2                     DEFINE       PWM2COM2  -PWMBASE
T_PWM2LEN                      DEFINE       PWM2LEN   -PWMBASE
T_PWMCLRI                      DEFINE       PWMCLRI   -PWMBASE

PWMCON_OFFSET                  DEFINE       0x0F80
PWM0COM0_OFFSET                DEFINE       0x0F84
PWM0COM1_OFFSET                DEFINE       0x0F88
PWM0COM2_OFFSET                DEFINE       0x0F8C
PWM0LEN_OFFSET                 DEFINE       0x0F90
PWM1COM0_OFFSET                DEFINE       0x0F94
PWM1COM1_OFFSET                DEFINE       0x0F98
PWM1COM2_OFFSET                DEFINE       0x0F9C
PWM1LEN_OFFSET                 DEFINE       0x0FA0
PWM2COM0_OFFSET                DEFINE       0x0FA4
PWM2COM1_OFFSET                DEFINE       0x0FA8
PWM2COM2_OFFSET                DEFINE       0x0FAC
PWM2LEN_OFFSET                 DEFINE       0x0FB0
PWMCLRI_OFFSET                 DEFINE       0x0FB8
