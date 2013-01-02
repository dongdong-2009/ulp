;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Sets ADC filter.
;Responsibility:
;	Group:		IAC Software.
;	Person:		Eckart Hartmann.
;Modifications:
;	12/12/2007	Design Start.

#include "../ADuC7060.inc"

; AdcFlt==========Sets filter details.
; C Function prototype: int AdcFlt(uint uSF, uint uAF, uint uFltCfg);
;uSF:{0-127}
;uAF:{0-63}
;uFltCfg:{FLT_NOTCH2|FLT_RAVG2|FLT_CHOP}
; Description of Function: Sets filter details.
; User interface:
;   Set uSF to desired Sinc3 Decimation factor:
;   Set uAF to desired averaging factor:
;   Set uFltCfg to bitwise or of the following:
;       0x80 or FLT_NOTCH2 for second notch.
;       0x4000 or FLT_RAVG2 for an average by 2.
;       0x8000 or FLT_CHOP to enable choping.
;   Call AdcFlt() to set values.
;       Returns significant bits of uSF.
; Robustness:
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcFlt
			CODE32

AdcFlt:
		mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		and		r0,r0,#0x7f			;Remove any extra bits.
		and		r1,r1,#0x3f
		bic		r2,r2,#0x7f
		bic		r2,r2,#0x3f00
		orr		r1,r0,r1,LSL#8		;Combine the bits.
		orr		r1,r1,r2
		str		r1,[r3,#Z_ADCFLT]	;Write to ADCFLT.
		bx		lr				

			END
