;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Start ADC conversion.
;Responsibility:
;	Group:		IAC Software.
;	Person:		Eckart Hartmann.
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; AdcGo==========Start ADC conversion.
; C Function prototype: int AdcGo(uint uChan, uint uStart, uint uPower, uint uOther);
;uChan:{3}
;uStart:{ADC_OFF,ADC_CONT,ADC_SINGLE,ADC_IDLE}
;uPower:{ADC_NORMAL,ADC_LP,ADC_LP2}
;uOther:{0|ADCLPMREFSEL|ADC_CLKSEL}
; Description of Function: Start ADC conversion.
; User interface:
;   Set uChan to 3:
;       This value is ignored since both channels are started together.
;       Individual channels are enabled by AdcCfg().
;   Set uStart to one of the following:
;       0 or ADC_OFF for power down.
;       1 or ADC_CONT for continuous conversion.
;       2 or ADC_SINGLE for single conversion.
;       3 or ADC_IDLE for idle mode.
;	Set uPower to one of the following:
;		0 or ADC_NORMAL for normal mode.
;		1 or ADC_LP for low power mode.
;		2 or ADC_LP2 for low power 2 mode.
;	Set uOther to the bitwise or of the following:
;		0x20 or ADCLPMREFSEL for normal power reference in low power mode.
;		0x80 or ADC_CLKSEL for high speed ADC clock.
;   Call AdcGo() to start converting with these settings.
;       Returns ADC status (ADCSTA).
; Robustness:  Up to 3 conversion results after the ADC had been powered
;		up are not accurate.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcGo
			CODE32

AdcGo:	and		r1,r1,#3			;Strip any extra bits.
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
