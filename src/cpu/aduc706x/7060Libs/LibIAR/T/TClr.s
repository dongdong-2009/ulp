;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Untested.
;Tools:			Developed using IAR V5.1
;Hardware:		ADuC7060
;Description:	Clears the selected timer interrupt.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	02/01/2008  Design Start.

#include "../ADuC7060.inc"

; TClr==========Clears the timer interrupt.
; C Function prototype: int TClr(uint uChan);
;uChan:{0,1,2,3}
; Description of Function: Clears the selected timer interrupt.
; User interface:
;   Set uChan to desired Timer, 0 to 3
;   Call TClr();
;   Returns uChan in r0.
; Side effects: Corrupts r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC TClr
			CODE32

TClr:	mov		r3,#0xff0000		;r3 = Z_BASE
		orr		r3,r3,#0xff000000
		orr		r3,r3,r0,lsl#5		; + uChan*0x20;
		cmp		r0,#3				;if(uChan<=3)
		strls	r0,[r3,#Z_T0CLRI]	; clear interupt;
TCRet:	bx		lr

			END

