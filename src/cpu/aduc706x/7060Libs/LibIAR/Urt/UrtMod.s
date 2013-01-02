;Legal: This code is provided as is without warranty.  It is
;   the responsibility of the person integrating this code 
;   into an application to ensure that the combination 
;   performs as required and is safe.
;Status:     In development
;Incomplete: All
;Tools:      Developed using IAR V4.40A
;Hardware:   ADuC7060
;Description:
;    Sets UART modes.
;Responsibility:
;    Group:      IAC Software
;    Person:     Eckart Hartmann
;Modifications:
;       13/11/2007  Design Start.

#include "../ADuC7060.inc"

; UrtMod==========UART Modem.
; C Function prototype: int UrtMod(int iMcr);
;iMcr:{URT_DTR|URT_RTS|URT_LB}
; Description of Function: Write iMcr to UART Modem Control Register 
;   and return Modem Status Register.
; User interface:
;       Set iMcr to the modem control required (COMCON1):
;           COMCON0.0 -> DTR Data terminal ready.
;           COMCON0.1 -> RTS Request to send.
;           COMCON0.2 and COMCON0.3 -> Unused.
;           COMCON0.4 -> 0 = LB Loop back.
;           COMCON0.5 to COMCON.31 -> Unused. 
;       Call UrtMod to write iMcr to COMCON1.
;       Returns value of COMSTA1:
;           COMSTA1.0 = DCTS -> CTS changed.
;           COMSTA1.1 = DDSR -> DSR changed.
;           COMSTA1.2 = TERI -> RI Trailing edge.
;           COMSTA1.3 = DDCD -> DCD changed.
;           COMSTA1.4 = CTS -> Clear to send.
;           COMSTA1.5 = DSR -> Data set ready.
;           COMSTA1.6 = RI -> Ring indicator.
;           COMSTA1.7 = DCD -> Data carrier detect.
; Robustness:	This function does not change the Port Multiplexers.
; Side effects: Corrupts r3.

		SECTION .text:CODE:NOROOT(2)
                PUBLIC UrtMod
                CODE32

UrtMod:
        mov     r3,#0xff0000            ;r3 = Z_BASE;
        orr     r3,r3,#0xff000000
        str     r0,[r3,#Z_COMCON1]
        ldr     r0,[r3,#Z_COMSTA1]
        bx      lr                      ;return COMSTA1. 


                 END
