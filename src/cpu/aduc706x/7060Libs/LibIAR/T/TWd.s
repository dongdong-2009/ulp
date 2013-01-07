;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Untested.
;Tools:			Developed using IAR V5.1
;Hardware:		ADuC7060
;Description:	Sets the watchdog mode switches of T2.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	02/01/2008	Design Start.

#include "../ADuC7060.inc"

; TWd==========Sets the watchdog mode switches of T2.
; C Function prototype: int TWd(uint uWdClk, uint uPdOff, uint uWdIrq);
;uWdClk:{WD_32K,WD_2K,WD_0K5}
;uPdOff:{0,1}
;uWdIrq:{0,1}
; Description of Function: Configures watchdog mode switches.
; User interface:
;	Configure timer 2 using TCfg() and TVal() before using TWd().  After the
;		watchdog is enabled it can not be changed except by a power cycle and
;		it must be regularly refreshed using TClr() to prevent a timeout.
;   Set uWdClk to one of the following:
;		0 or WD_32K for 32kHz clock.
;		4 or WD_2K for 2kHz clock.
;		8 or WD_0K5 for 500Hz clock.
;   Set uPdOff to non zero to disable watchdog when peripherals are powered
;		down by POWCON.4.
;   Set uWdIrq to non zero to produce an IRQ instead of a reset when
;		watchdog reaches 0.
;	Call TWd().	Watchdog is set as requested.
;	Returns previous T2CON value in r0.
; Robustness:
;	Only works with T2. Will have no effect if used on other timers.
; Side effects: Corrupts r1 and r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC TWd
			CODE32

TWd:	and		r0,r0,#0xc			;Clean uWdClk;
		cmp		r1,#0				;Check uPdOff.
		orr		r1,r0,#0xa0			;Add watchdog bits to uWdClk.
		orrne	r1,r1,#1			;if(uPdOff) Add T2PDOFF bit.
		cmp		r2,#0				;if(uWdIrq)
		orrne	r1,r1,#2			; add IRQ mode bit.

		mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		ldr		r0,[r3,#Z_T2CON]	;Read old T2CON.
		str		r1,[r3,#Z_T2CON]	;Write new T2CON.
		bx		lr

			END

