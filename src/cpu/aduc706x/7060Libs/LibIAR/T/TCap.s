;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1
;Hardware:		ADuC7060
;Description:	Configures and reads capture details (only for T0 and T3).
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	02/01/2008	Design Start.

#include "../ADuC7060.inc"

; TCap==========Configures and reads capture details.
; C Function prototype: int TCap(uint uChan, int iCapCh);
;uChan:{0,3}
;iCapCh:{IREAD,I0FF,IT0,IT1,IT2,IT3,IADC,IUART,ISPI,XIRQ0,XIRQ1,I2CM,I2CS,IPWM,XIRQ2,XIRQ3}
; Description of Function:
;	Configures and writes capture details (for T0 and T3 only).
; User interface:
;   Set uChan to desired Timer, 0 or 3
;   Set iCapCh to the desired capture trigger source.
;		-1 = IREAD Only read capture register.
;		0  = IOFF -> capture off.	13 = XIRQ0 => External interupt 0
;		3  = IT0 => T0				14 = XIRQ1 => External interupt 1
;		4  = IT1 => T1				15 = I2CM => I2C master
;		5  = IT2 => T2				16 = I2CS => I2C slave
;		6  = IT3 => T3				17 = IPWM => PWM
;		10 = IADC => ADC ready		18 = XIRQ2 => External interupt 2
;		11 = IUART => UART			19 = XIRQ3 => External interupt 3
;		12 = ISPI => SPI
;		Note: Except for values -1 and 0 the source number mirrors the IRQ/FIQ
;			MMR bit definitions and is 2 more than bits 12 to 16 of TnCON.
;	Call TCap().	Sets the capture details if parameters within range.
;	Returns TnCAP in r0.
; Robustness:
;	If uChan != 0 or 3 then TCap() does nothing and returns iT unchanged.
;	Interupt functionality may be added separately.
; Side effects: Corrupts r0 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC TCap
			CODE32

TCap:	mov		r3,#0xff0000		; r3 = Z_BASE
		orr		r3,r3,#0xff000000
		orr		r3,r3,r0,lsl#5		;   + r0*0x20;
		cmp		r0,#0				;if(uChan==0
		cmpne	r0,#3				;  || uChan==3)
		bne		TCRet
		cmp		r1,#20				;  && (uint)iCapCh<20)
		bhs		TCRet				; {
		cmp		r1,#2				; if((uint)iCapCh>2)
		subhi	r1,r1,#2			;  iCapCh -= 2;
TCNxt:	ldr		r2,[r3,#Z_T0CON]	; else { r2 = TnCON;
		and		r2,r2,#0xfffc0fff	;  Remove cap bits.
		orr		r2,r2,r1,lsl#0x0c	;  r2 |= iCapCh<<12;
		orrhi	r2,r2,#0x20000		;  if((uint)iCapCh>2) Set event select bit.

		str		r2,[r3,#Z_T0CON]	;  TnCON = r2; }
TCRet:	ldr		r0,[r3,#Z_T0CAP]	; return TnCAP;
		bx		lr					; }

			END

