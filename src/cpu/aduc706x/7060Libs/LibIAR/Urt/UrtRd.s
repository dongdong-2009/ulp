;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Reads the UART data register.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; UrtRd==========Reads the UART data register.
; C Function prototype: int UrtRd(void);
; Description of Function: Reads the received UART data.
; User interface:
;	Call UrtRd.
;	Returns COMRX:
; Robustness:
;	Reads whatever character arrived last.
;	Use UrtBsy to ensure new character is available.
; Side effects: Corrupts r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC UrtRd
			CODE32

Z_B = 0xffff0000

UrtRd:
		mov		r3,#0xff0000			;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		ldr		r0,[r3,#COMRX-Z_B]		;Get COMRX.
		bx		lr

			END
