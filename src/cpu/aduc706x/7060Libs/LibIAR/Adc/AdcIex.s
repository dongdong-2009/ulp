;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Sets ADC diagnostic currents.
;Responsibility:
;	Group:		IAC Software.
;	Person:		Eckart Hartmann.
;Modifications:
;	12/12/2007	Design Start.

#include "../ADuC7060.inc"

; AdcIex==========Sets filter details.
; C Function prototype: int AdcIex(int iIuA, uint uOn, uint uPin);
;iIuA:{0-1010}
;uOn:{IEX_OFF|IEX_0ON|IEX_1ON}
;uPin:{IEX_STD|IEX_0ALT|IEX_1ALT}
; Description of Function: Sets filter details.
; User interface:
;   Set iI to desired current in uA:
;		Multiples of 200uA are allowed plus an additional 10uA up to 1010uA.
;		Current is rounded down to nearest allowed value.
;   Set uOn to the bitwise or of:
;		0 or IEX_OFF for neither current on.
;		0x40 or IEX_0ON for current 0 on.
;		0x80 or IEX_0ON for current 1 on.
;   Set uPin to the bitwise or of:
;		0 or IEX_STD for both currents on default pins.
;		0x10 or IEX_0ALT for current 0 on alternate pin.
;		0x20 or IEX_1ALT for current 1 on alternate pin.
;   Call AdcIex() to set values.
;       Returns difference between current set and requested current.
; Robustness: Both currents may be output on one pin.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcIex
			CODE32

AdcIex:	and		r1,r1,#0xc0			;Remove any extra bits.
		and		r2,r2,#0x30
		orr		r1,r1,r2			;Combine uOn and uPin.
;		subne	r0,r0,#10			;Subtract 10 from current.
		mov		r2,#5				;for(r2=5; r2>0; r2--)
AILp:	subs	r0,r0,#200			; { if((r0-=200)<0)
		blt		AIWr				;  break;
		add		r1,r1,#2			; r1 += 2;
		subs	r2,r2,#1
		bgt		AILp				; }
AIWr:	cmp		r0,#-190
		addge	r1,r1,#1			;if(r1>=-190) add 10uA.
		mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		str		r1,[r3,#Z_IEXCON]	;Write to IEXCON.
		bx		lr				

			END
