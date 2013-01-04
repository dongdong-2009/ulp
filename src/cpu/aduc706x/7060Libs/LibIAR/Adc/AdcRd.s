;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Reads the ADC data.
;Responsibility:
;	Group:		IAC Software
; 	Person:		Eckart Hartmann
;Modifications:
;	13/11/2007	Design Start.

#include "../ADuC7060.inc"

; AdcRd==========Reads the ADC data.
; C Function prototype: int AdcRd(int *piAdcMain, int *piAdcAux);
;*piAdcMain:{}
;*piAdcAux:{}
; Description of Function: Reads the ADC data.
; User interface:
;	Set up pointer piAdcMain and piAdcAux.
;   Call AdcRd().
;	Places main and aux ADC value in piAdcMain and piAdcAux respectively.
;	The data format is corrected to be a signed int
;	with 0x0fffffff representing VRef), 0x00000000 representing 0V,
;	0xffffffff representing -1 LSB, and 0xf0000000 representing -VRef.
;	Returns details as follows:
;	 Least significant nibble gives details of main ADC:
;	  Bit 0 set -> data available. Else no new data and ignore remaining bits.
;	  Bit 1 set -> gain adjust advised to improve accuracy.
;	  Bit 2 and 1 set -> decrease gain. Bit 2 clear and 1 set -> increase gain.
;	  Bit 3 set -> overflow. Bit 3 and 2 set -> signal beyond chip capability.
;	 Second least significant nibble gives details of aux ADC:
;	  Bit 0 set -> data available. Else no new data and ignore remaining bits.
;	  Bit 3 and 2 set -> signal beyond chip capability.
;	 For additional information see AdcAdj() source file.
; Robustness:
;		For aux ADC use gain of 1 only.
; Side effects:
;     Corrupts r1 to r3 and r12.
;     Clears ADCSTA.0 and ADCSTA.1

			SECTION .text:CODE:NOROOT(2)
			PUBLIC AdcRd
			CODE32
AdcRd:
		mov		r3,#0xff0000		;r3 = Z_BASE;
		orr		r3,r3,#0xff000000
        ldr     r12,[r3,#Z_ADCSTA]	;Get ADCSTA.
		ands	r2,r12,#2			;if(ADC1 not ready)
		beq		ARTry0				; try ADC0.
		ldr		r2,[r3,#Z_ADC1DAT]	;else { Get ADC1DAT
		mov		r2,r2,LSL#5			; Adjust to 28 bits
		str		r2,[r1]				;  and save. }
ARTry0:	ands	r1,r12,#1			;if(ADC0 not ready)
		beq		ARQErr				; return. }
		ldr		r2,[r3,#Z_ADC0DAT]	;Get ADC0DAT.
		mov		r2,r2,LSL#5			;Adjust to 28 bits.
		ldr		r3,[r3,#Z_ADC0CON]	;Adjust for PGA (r3)
		and		r3,r3,#0x7
		mov		r2,r2,ASR r3
		str		r2,[r0]				; and save.
		mov		r2,r2,LSL r3		;Restore *piAdcMain.
		cmp		r2,#0				;r2 = |*piAdcMain<0|
		mvnlt	r2,r2
ARQErr:	mov		r0,#0				;Initialise return.
		ands	r1,r12,#2			;if(ADC1 not ready)
		beq		ARErr0				; done ADC1.
		orrne	r0,r0,#0x10			;else add ADC1 ready flag.
		ands	r1,r12,#0x2000		;if(ADC1 overflow)
		orrne	r0,r0,#0xf0			; flag overrange.
ARErr0:	ands	r1,r12,#1			;if(ADC0 not ready)
		beq		ARRet				; done.
		orrne	r0,r0,#1			;else add ADC0 ready flag.
		cmp		r2,#0x04000000		;if(ADC0<0x4000000)
		orrlt	r0,r0,#6			; add increase gain flag
;		blt		ARRet				; and done.
		cmp		r2,#0x08000000		;else if(ADC0<0x8000000)
		blt		ARRet				; done.
		cmp		r3,#0				;else if(PGA!=0)
		orrne	r0,r0,#2			; add decrease gain flags.
		ands	r1,r12,#0x1000		;if(ADC0 not overflow)
		beq		ARRet				; done.
		orr		r0,r0,#0x8			;else add overflow flag.
		cmp		r3,#0				; if(PGA==0)
		orreq	r0,r0,#4			;  flag overrange and
		andeq	r0,r0,#0xfd			;  remove change gain flag.
ARRet:	bx		lr

			END
