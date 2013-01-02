;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Untested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Configures timer.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	02/01/2008	Design Start.

#include "../ADuC7060.inc"

; TCfg==========Configures timer T.
; C Function prototype: int TCfg(uint uChan, uint uScale, uint uClk, uint uPer);
;uChan:{0,1,2,3}
;uScale:{T_D1,T_D16,T_D256,T_D32K}
;uClk:{T_32K,T_HCLK,T_10M,T_PIN}
;uPer:{0,1}
; Description of Function: Configures timer.
; User interface:
;   Set uChan to desired Timer, 0 to 3
;   Set uScale to a prescale value of
;		0x0 or T_D1 for divide by 1		0x8 = T_D256 for divide by 256
;		0x4 or T_D16 for divide by 16	0xf = T_D32K for divide by 32k (not T2).
;			Other values of uScale give unpredictable results.
;   Set uClk to the desired clock source.
;		0 = T_32k -> 32.768kHz			2 = T_10M => 10.24MHz
;		1 = T_HCLK => core clock		3 = T_PIN => for T0 P1.0 for T1 XTAL2.
;			For T2 clock is always 32.768kHz.	For T3 do not use uClk = 3.
;	Set uPer to 1 for periodic mode or 0 for free running mode.
;   Call TCfg().
;       Timer uChan is stopped and chosen configuration set.
;   Returns TnCON.
; Robustness:
;  Certain features are not available for certain timers. Refer to the
;  data sheet for more information.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC TCfg
			CODE32

TCfg:	cmp		r2,#3				;if(uClk>3
		cmpls	r1,#15				;  || uScale>15)
		bhi		TCRet				; return;	//only reading TnCON.
		cmp		r0,#2				;if(uChan==2)
		andeq	r1,r1,#0xfffffffc	; r1 = uScale with 2LSB cleared.
		cmp		r1,#1				;if(uChan==1
		bne		TCPer
		cmpeq	r2,#2				; && uClk.1)
		eoreq	r2,r2,#1			;  invert uClk.0;
TCPer:	cmp		r3,#1				;if(uPer)
		addeq	r1,r1,#0x40			; r1 = r1 + periodic bit.
		add		r1,r1,r2,LSL#9		;r1 += uClk<<9;
		cmp		r3,r3				;Force eq for later.

TCRet:	mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		orr		r3,r3,r0,lsl#5		; + uChan*0x20;

		ldr		r0,[r3,#Z_T0CON]	;r0 = TnCON & ~0x64f;
		andeq	r0,r0,#0xffffffb0	; //strip bits to modify.
		andeq	r0,r0,#0xfffff9ff
		orreq	r0,r0,r1			;Add bits to change.
		streq	r0,[r3,#Z_T0CON]	;Write to TnCON.
		bx		lr

			END

