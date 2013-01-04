;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Erases pages of flash.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	2008/01/22	Design Start.

#include "../ADuC7060.inc"

; FeeClr==========Erases pages of flash.
; C Function prototype: int FeeClr(char* pcStart, char* pcEnd);
;pcStart:{0x00080000-0x0008f7ff}
;pcEnd:{0x00080000-0x0008f7ff}
; Description of Function: Erases flash from page containing
;	pcStart to the page containing pcEnd.
; User interface:
;   Set pcStart to an address in the first page to erase.
;   Set pcEnd to an address in the last page to erase.
;   Call FeeClr().
;		Erases selected pages if pcStart and pcEnd within
;		0x00080000-0x0008f7ff.  Exits if an erase is not
;		successful.
;   Returns address of first page not erased or zero if
;		erase error.
; Robustness:
;	Page erasure takes about 20ms per page in which time
;		any access to flash will stall the core.
;	If pcStart and pcEnd are in the same page that page
;		will be erased even if pcStart >= pcEnd.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC FeeClr
			CODE32

FeeClr:	mov		r3,#0xff000000		;r3 = MMRBASE.
		orr		r3,r3,#0xff0000
		and		r0,r0,r3,ASR#7		;Adjust r0 to page boundary.
		cmp		r0,#0x80000			;if(pcStart<0x80000)
		blo		FCRet				; return;
		mov		r2,#0x80000			;if(pcEnd>=0x8f800)
		add		r2,r2,#0xf800
		cmp		r1,r2
		bhs		FCRet				; return;
FCLp:	cmp		r0,r1				;while(pcStart<=pcEnd)
		bhi		FCRet				; {
		ldr		r2,[r3,#Z_FEESTA]	; Clear FEESTA.
		mov		r2,#0x108			; FEEMOD = 0x108.
        str		r2,[r3,#Z_FEEMOD]
		str		r0,[r3,#Z_FEEADR]	; FEEADR = r0;
        mov		r2,#5				; FEECON = erase command;
        str		r2,[r3,#Z_FEECON]
FCBsy:	ldr		r2,[r3,#Z_FEESTA]	; while(busy);
		tst		r2,#4
		bne		FCBsy
		tst		r2,#2				; if(flash error)
		movne	r0,#0				;  return 0;
		bne		FCRet
ErSkp:	add		r0,r0,#0x200		; pcStart += 0x200;
		b       FCLp				; }
FCRet:	bx		lr

			END
