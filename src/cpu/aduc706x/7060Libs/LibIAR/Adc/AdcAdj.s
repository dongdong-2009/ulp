;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Adjusts ADC0 as proposed by the return value of AdcRd();.
;Responsibility:
;	Group:		IAC Software.
;	Person:		Eckart Hartmann.
;Modifications:
;	11/12/2007	Design Start.

#include "../ADuC7060.inc"

; AdcAdj==========Reads the ADC status.
; C Function prototype: int AdcAdj(uint uAdcAdj);
;uAdcAdj:{AdcRd,}
; Description of Function: Adjusts gain of ADC0.
; User interface:
;	Obtain return value from AdcRd() and put in uAdcAdj;
;	Call AdcAdj(uAdcAdj).
;	Adjusts ADC0 for optimal gain if iAdcAdj indicates valid conversion.
;		If last raw reading was below 25% for that gain setting the gain
;		will be doubled for the next conversion. If last raw reading was
;		above 50% the gain will be halved for the next conversion unless
;		the gain is already at minimum. If last raw reading was between 25%
;		and 50% gain will not be changed.
;		For additional information see AdcRd() source file.
;	Returns gain setting.
; Robustness:  No known issues.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcAdj
			CODE32
AdcAdj:
		mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
		ldr		r2,[r3,#Z_ADC0CON]	; if(gain>0)
		ands	r1,r0,#1			;if(!(uAdcAdj&1))	//Valid conversion?
;		beq		AARet				; end.
		andsne	r1,r0,#2			;else if(!(uAdcAdj&2)) //Require adjustment?
		beq		AARet				; end.
		ands	r1,r0,#4			;else if(uAdcAdj&4)	//In/Decrease?
		bne		AAQUp				; {
		ands	r1,r2,#0xf			; if(gain>0)
		subne	r2,r2,#1			;  ADC0CON--;
;		strne	r2,[r3,#Z_ADC0CON]
		b		AARetC				; }
AAQUp:	ands	r1,r2,#0xf			; if(gain<6)
		cmp		r1,#0x6				;else if(gain<6)
		addlt	r2,r2,#1			; ADC0CON++;
AARetC:	strne	r2,[r3,#Z_ADC0CON]
AARet:	and		r0,r2,#0xf			;return gain setting.
		bx		lr


			END
