;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Outputs a value to the DAC.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; DacOut==========Writes a value to the DAC.
; C Function prototype: int DacOut(uint uChan, int iValue);
;uChan:{0}
;iValue:{0-0x0fff0000}
; Description of Function: Sets the DAC output.
; User interface: Set iChan to 0.
;   Put the required output value in iValue.
;   Call DacOut().
;       iValue is written to DAC0DAT.
;       0x0fff0000 => iRange set by DacRng().
;   Returns value in DAC0DAT.
; Robustness:  uChan is ignored since only 0 is valid.
; Side effects: Sets r3 to 0xffff0000.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC DacOut
			CODE32
			
DacOut:
		mov		r3,#0xff0000			;r3	= Z_BASE;
		orr		r3,r3,#0xff000000
		str		r1,[r3,#Z_DACDAT]		;Write to DAC0DAT.
		ldr		r0,[r3,#Z_DACDAT]		;return DAC0DAT.
		bx		lr
;
			END
