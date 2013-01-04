;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Sets the ADC range.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	14/12/2007	Design Start.

#include "../ADuC7060.inc"

; AdcRng==========Set measurement range.
; C Function prototype: int AdcRng(uint uChan, uint uPga, uint uAdcRef);
;uChan:{0,1}
;uPga:{1-64}
;uAdcRef:{ADC_IREF,ADC_XREF,ADC_AREF,ADC_VREF,ADC_TREF}
; Description of Function: Set measurement range.
; User interface:
;   Call AdcRng().
; Robustness:
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcRng
			CODE32

AdcRng:	and		r1,r1,#3			;Strip any extra bits.
		and		r2,r2,#3
		and		r3,r3,#0xa0
		orr		r1,r1,r2,LSL #3		;Combine bits.
		orr		r1,r1,r3
		mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		str		r1,[r3,#Z_ADCMDE]	; and write to ADCMDE.
		ldr		r0,[r3,#Z_ADCSTA]	;return status.
		bx		lr				

			END
