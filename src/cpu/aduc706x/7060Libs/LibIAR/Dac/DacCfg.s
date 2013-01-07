;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Sets the output range of a DAC.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; DacCfg==========Sets the output range of a DAC.
; C Function prototype: int DacCfg(uint uChan, uint uRange, uint uCfg);
;uChan:{0}         //DAC channel.
;uRange:{DAC_RINT, DAC_REXT0, DAC_REXT1, DAC_RVDD}
;uCfg:{DAC_I16|DAC_I|DAC_T1|BUF_OPA|DAC_NOBUF|BUF_LP|DAC_ON}
; Description of Function: Sets the DAC output range.
; User interface: Set uChan to 0.
;   Set uRange to one of the following symbolic constants:
;       DAC_RINT    for range 0 to internal referenc.
;       DAC_REXT0   for range 0 to main external reference.
;       DAC_REXT1   for range 0 to aux external reference.
;       DAC_RVDD    for range 0 to VDD.
;   Set uCfg to a combination of the following symbolic constants:
;       DAC_ON      Select if no other selection.
;       DAC_I16     Select for slower interpolation mode.
;       DAC_I       Select to enable interpolation mode.
;       DAC_CLR		Select is ignored.
;       DAC_T1      Select to use T1 instead of HCLK to update.
;       BUF_OPA     Select to use buffer as op-amp.
;       DAC_NOBUF   Select to bypass output buffer .
;       BUF_LP      Select to put buffer in low power mode.
;       DAC_PD      Select to power down DAC.
;       Call DacCfg().
;           Selected range and configuration is set.
;       Returns uRange.
; Robustness:  uChan is ignored since only 0 is valid.
; Side effects: Corrupts r1, r2 and r3.
;
			SECTION .text:CODE:NOROOT(2)
			PUBLIC DacCfg
			CODE32

DacCfg:	mov		r3,#0x00ff0000		;r3 = MMRBASE.
		orr		r3,r3,#0xff000000
		and		r1,r1,#3			;Isolate range bits
		and		r2,r2,#0x3fc		; and config bits
		orr		r0,r1,r2			; and combine,
		orr		r0,r0,#0x10			; clear clear,
		str		r0,[r3,#Z_DACCON]	; and put in DACCON.
		ldr		r0,[r3,#Z_DACCON]	;return DACCON;
		bx		lr

			END

