;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Configure ADC.
;Responsibility:
;	Group:		IAC Software.
;	Person:		Eckart Hartmann.
;Modifications:
;	14/12/2007	Design Start.

#include "../ADuC7060.inc"

; AdcCfg==========Configure ADC.
; C Function prototype: int AdcCfg(uint uChan, uint uAdcIn, uint uAdcCfg);
;uChan:{0,1}
;uAdcIn:{ADC001,ADC00,ADC01,ADC0REF,ADC0CALR,ADC023,ADC02,ADC03,ADC000,ADC011,ADC123,ADC145,ADC167,ADC189,ADC12,ADC13,ADC14,ADC16,ADC17,ADC18,ADC19,ADC1ITMP,ADC1REF,ADC1DAC,ADC1CALR,ADC133}
;uAdcCfg:{ADC_CODE|ADC_CM|ADC_ISRC0|ADC_ISRC1}
; Description of Function: Configure ADC.
; User interface:
;   Call AdcCfg() to start converting with these settings.
;       Returns ADCSTA.
; Robustness:
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcCfg
			CODE32

AdcCfg:	and		r1,r1,#3			;Strip any extra bits.
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
