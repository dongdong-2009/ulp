;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Waits for UART available then outputs character.
;Responsibility:
;	Group:		IAC Software
;	Person:	Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; putchar==========UART put character.
; C Function prototype: int putchar(int iTx);
;iTx:{}
; Description of Function: Writes iTx to the UART for transmission.
; User interface:
;   Put value to send in iTx.
;   Call putchar. Will wait untill buffer not full then output iTx.
;   Returns iTx in r0 always.
; Robustness:
;   UART must be configured before writing data.
;   Waits as long as UART is full.
; Side effects: Corrupts r1 and r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC putchar
			CODE32

Z_B = 0xffff0000

putchar:
		mov		r3,#0xff0000			;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
PcLoop1:
		ldr		r1,[r3,#COMSTA0-Z_B]	;while(COMSTA.6);
		ands	r1,r1,#0x040
		beq		PcLoop1
		str		r0,[r3,#COMTX-Z_B]		;COMTX = iTx;
PcLoop2:
		ldr		r1,[r3,#COMSTA0-Z_B]	;while(COMSTA.6);
		ands	r1,r1,#0x040
		beq		PcLoop2
		cmp		r0,#0x0A				;Compare r0 with '\n'
		moveq	r1,#0x0D				; Move 0D into r1
		streq	r1,[r3,#COMTX-Z_B]		; COMTX = 0x0D;
		bx		lr						;return r0.

			END
