;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1
;Hardware:		ADuC7060
;Description:	Sets mode of timer.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	02/01/2008	Design Start.

#include "../ADuC7060.inc"

; TMod==========Sets mode of timer.
; C Function prototype: int TMod(uint uChan, uint uUp, uint uFormat);
;uChan:{0,1,2,3}
;uUp:{T_DOWN,T_UP}
;uFormat:{T_BIN,T_23H,T_255H}
; Description of Function: Sets mode of timer.
; User interface:
;   Set uChan to desired Timer, 0 to 3
;   Set uUp to 0 or T_DOWN to count down else count is up.
;   Set uFormat to the desired counter mode:
;       0 = T_BIN  -> Binary mode.
;       2 = T_23H  -> 23 hour real time mode.
;       3 = T_255H -> 255 hour real time mode.
;   Call TMod().
;       Timer Tn is stopped and the specified modes set.
;   Returns previous TnCON value in r0.
; Robustness:
;	Certain features are not available for certain timers. Refer to the
;	data sheet for more information.
;	For T2 watchdog mode use TWd().
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC TMod
			CODE32

TMod:	cmp		r0,#3				;if(parameters in range)
;		cmpls	r1,#1
		cmpls	r2,#3
		bhi		TMRet				; {
		mov		r3,#0xff0000		; r3 = Z_BASE
		orr		r3,r3,#0xff000000
		orr		r3,r3,r0,lsl#5		; + uChan*0x20;

		cmp		r1,#0				;Check uUp.
		ldr		r1,[r3,#Z_T0CON]	;r1 = TnCON;
		and		r1,r1,#0xfffffecf	;Remove uUp and iFormat bits.
		orrne	r1,r1,#0x100		;if(uUp) Set Up bit.
		cmp		r0,#2				;if(uChan<2)
		orrls	r1,r1,r2,lsl#4		; Set Format bits.

T3Mod:	ldr		r0,[r3,#Z_T0CON]	;return TnCON.
		str		r1,[r3,#Z_T0CON]	;TnCON = r1;
TMRet:	bx		lr

			END

