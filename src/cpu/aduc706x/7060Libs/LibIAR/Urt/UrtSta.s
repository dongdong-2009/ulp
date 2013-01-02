;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Reads the status of the UART.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;       13/11/2007  Design Start.

#include "../ADuC7060.inc"

; UrtSta==========UART status.
; C Function prototype: int UrtSta(void);
; Description of Function: Reads the status byte of the UART.
; User interface:
;	Call UrtSta.
;	Returns value of COMSTA0:
;		COMSTA0.0 = Data ready.
;		COMSTA0.1 = Overrun error.
;		COMSTA0.2 = Parity error.
;		COMSTA0.3 = Framing error.
;		COMSTA0.4 = Break indicator.
;		COMSTA0.5 = Transmit queue empty.
;		COMSTA0.6 = Transmit holding register empty.
; Robustness:	UART must be configured before checking status.
; Side effects: Corrupts r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC UrtSta
			CODE32

Z_B = 0xffff0000

UrtSta:
		mov		r3,#0xff0000			;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		ldr		r0,[r3,#COMSTA0-Z_B]
		bx		lr


                 END
