;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Sets clock speed.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	2008/02/13	Design Start.

#include "../ADuC7060.inc"

; ClkSpd==========Writes the CD bits.
; C Function prototype: uint ClkSpd(uint uCD);
;uCD:{0,1,2,3,4,5,6,7}          //CD bits.
; Description of Function: Writes uCD to the CD bits in POWCON.
; User interface:
;       Call ClkSpd with CD bits in uCD.
;       Returns value written to POWCON0.
; Robustness:  No known issues.
; Side effects: Corrupts r2 and r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC ClkSpd
			CODE32

ClkSpd:	and		r0,r0,#0x7			;Strip unused bits.
		mov		r3,#0xff000000		;r3 = Z_BASE;
		orr		r3,r3,#0xff0000
		ldr		r2,[r3,#Z_POWCON0]	;Get POWCON0
		bic		r2,r2,#0x07			; and remove old CD
		orr		r0,r0,r2			; and add to new bits.
		mov		r2,#1				;Write POWCON with keys.
		str		r2,[r3,#Z_POWKEY1]
		str		r0,[r3,#Z_POWCON0]
		mov		r2,#0xf4
		str		r2,[r3,#Z_POWKEY2]
		bx		lr					;return.

			END

