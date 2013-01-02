;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Outputs a byte to the UART.
;Responsibility:
;	Group:		IAC Software.
;	Person:		Eckart Hartmann.
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; UrtWr==========UART data write.
; C Function prototype: int UrtWr(int iTx);
;iTx:{}
; Description of Function: Writes 8 bits of iTx to the UART
;	for transmission.
; User interface:
;	Put value to send in iTx.
;	Call UrtWr will output iTx if TX registers not full already.
;	Returns 1 if successful or 0 if TX registers full already:
; Robustness:
;	UART must be configured before writing data.
;	Does not wait if UART full.
; Side effects: Corrupts r1 and r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC UrtWr
			CODE32
			
Z_B = 0xffff0000

UrtWr:
		mov		r3,#0xff0000			;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		ldr		r1,[r3,#COMSTA0-Z_B]
		ands	r1,r1,#0x040			;if(COMSTA.6)
		strne	r0,[r3,#COMTX-Z_B]		; { COMTX = iTx; r0 = 1; }
		movne	r0,#1
		moveq	r0,#0					;else r0 = 0;
		bx      lr

			END
