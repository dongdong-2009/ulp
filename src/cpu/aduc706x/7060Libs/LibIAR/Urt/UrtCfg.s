;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Configures UART.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; UrtCfg==========Configures UART.
; C Function prototype: int UrtCfg(uint uChan, uint uBaud, uint uFormat);
;uChan:{0}
;uBaud:{URT_6,URT_12,URT_24,URT_48,URT_96,URT_192,URT_384,URT_576,URT_1152,URT_2304}
;uFormat:{URT_68|URT_78|URT_STP2|URT_PE|URT_PODD|URT_PS|URT_BR}
; Description of Function: Reads the status byte of the UART.
; User interface:
;  Set uChan to 0. This value is ignored since there is only one channel.
;  Set uBaud to the baudrate required:
;	Either set uBaud to one of 0 to 10 for one of the
;   	baudrates listed above (URT_nx100). In this case
;   	the clock must be 10240000Hz+-1%.
;   	Alternatively set uBaud to Clk/Baudrate
;   	(e.g. 10240000/9600 = 2133).
;  Set uFormat to the data format required (COMCON0):
;   COMCON0.0 = URT_68\_/ Data length 5, 6, 7 or 8 bits.
;   COMCON0.1 = URT_78/ \
;   COMCON0.2 = URT_STP2 -> 2 stop bits.
;   COMCON0.3 = URT_PE -> Parity used.
;   COMCON0.4 = URT_PODD -> Odd parity.
;   COMCON0.5 = URT_PS -> Sticky parity.
;   COMCON0.6 = URT_BR -> Break (Set SOUT to 0).
;   COMCON0.7 to COMCON.31 -> Ignored.
;  Call UrtCfg.
; Returns value of COMSTA0: See UrtSta function for bit details.
; Robustness:
;  User must check that rounding errors plus clock errors
;  are acceptable.
;  The baudrate is set using the current CD bits. The baudrate
;  will change if the CD bits are changed subsequently.
;  The higher baudrates are only possible with high core clocks.
; Side effects:
;  Corrupts r1, r2 and r3.
;  Reconfigures the UART.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC	UrtCfg
			CODE32

Z_B = 0xffff0000

UrtCfg:
		mov		r3,#0xff0000			;r3	= Z_BASE;
		orr		r3,r3,#0xff000000
		ldr		r0,[r3,#GP1CON-Z_B]		;Switch UART
		and		r0,r0,#0xffffff00
		orr		r0,r0,#0x00000011		;
		str		r0,[r3,#GP1CON-Z_B]		; onto IO pins.
UrtBr:	cmp		r1,#10					;if(uBaud>10)
		bls		UrtLst
		mov		r1,r1,LSR#5				; r1 = uBaud>>5;
		b		UrtDl
UrtLst:	cmp		r1,#6					;else { if(uBaud<6)
;		ldrls	r0,=0x92c				;  r0 = 2348; //45088768/(600*16*2)
;		ldrhi	r0,=0xc3b				; else r0 = 3131; } //45088768*128/(57600*16*2)
		ldrls	r0,=0x215				;  r0 = 533; //10240000/(600*16*2)
		ldrhi	r0,=0x2c7				; else r0 = 711; } //10240000*128/(57600*16*2)
		mov		r1,r0,LSR r1			;r1 = r0>>uBaud;
UrtDl:	mov		r0,#0					;COMDIV2 = 0;
		str		r0,[r3,#COMDIV2-Z_B]
		str		r0,[r3,#COMIEN0-Z_B]		;COMIEN0 = 0;
		ldr		r0,[r3,#POWCON1-Z_B]		;r1 >>= (POWCON1>>3)&3;
		mov		r0,r0,LSR#3
		and		r0,r0,#3
		mov		r1,r1,LSR r0
		mov		r0,#0x83				;COMDIV0-1 = r1.
		strb	r0,[r3,#COMCON0-Z_B]
		strb	r1,[r3,#COMDIV0-Z_B]
		mov		r1,r1,LSR#8
		strb	r1,[r3,#COMDIV1-Z_B]
		and		r1,r2,#0x7F				;COMCON0 = uFormat&0x7f;
		strb	r1,[r3,#COMCON0-Z_B]
		bx		lr
	
			END
