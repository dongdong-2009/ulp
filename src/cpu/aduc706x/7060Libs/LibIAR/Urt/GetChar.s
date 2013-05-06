;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Waits for character from UART then returns it.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; getchar==========Reads the UART data register.
; C Function prototype: int getchar(void);
; Description of Function: Waits for character from UART then returns it.
; User interface:
;	Call getkey.
;	Returns COMRX when available.
; Robustness:  Waits until character available.
; Side effects: Corrupts r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC getchar
			CODE32

Z_B = 0xffff0000

getchar:
		mov		r3,#0xff0000			;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
GkLoop:	ldr		r0,[r3,#COMSTA0-Z_B]	;Loop till data ready.
		ands	r0,r0,#0x01
		beq		GkLoop
		ldrb	r0,[r3,#COMRX-Z_B]		;Get COMRX.
		bx		lr						;return.

			END
