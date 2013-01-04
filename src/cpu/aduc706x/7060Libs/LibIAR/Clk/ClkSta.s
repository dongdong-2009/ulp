;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Reads the clock status.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	2008/02/13	Design Start.

#include "../ADuC7060.inc"

; ClkSta==========Reads the Clock status.
; C Function prototype: uint ClkSta(uint uClrPllInt);
;uClrPllInt:{0,1}
; Description of Function: Reads the Clock status PLLSTA.
; User interface:
;	Put 1 in uClrPllInt if PLLI is to be cleared.
;	Call ClkSta.
;	Returns PLLSTA (Status before PLLI is cleared).
;		PLLSTA.0 = PLLI = PLL interupt bit.
;		PLLSTA.1 = LOCK = Indicates PLL is locked.
;		PLLSTA.2 = PLLI = Indicates oscillator is OK.
;		PLLSTA.3 = PLLI = Indicates VCO is OK.
; Robustness:  No known issues.
; Side effects: Corrupts r3.


			SECTION .text:CODE:NOROOT(2)
			PUBLIC ClkSta
			CODE32

ClkSta:	cmp		r0,#0				;if(uClrPllInt)
		mov		r3,#0xff000000
		orr		r3,r3,#0xff0000
		ldr		r0,[r3,#Z_PLLSTA]
		streq	r0,[r3,#Z_PLLSTA]	;	clear PLLI.
		bx		lr					;return PLLSTA.

			END

