;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Writes to flash.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	2008/01/22	Design Start.

#include "../ADuC7060.inc"

; FeeWr==========Writes to flash.
; C Function prototype: int FeeWr(char* pcStart, char* pcData, uint uCnt);
;pcStart:{0x00080000-0x0008f7ff}
;pcData:{}
;uCnt:{1-0xf800}
; Description of Function: Writes uCnt bytes of pcData to
;	pcStart in flash.
; User interface:
;	Set pcStart to the address where data is to be written.
;	Set pcData to the pointer to the data to write.
;	Set uCnt to the number of bytes to write.
;	Call FeeWr().
;		If pcStart and pcStart+uCnt within
;		0x00080000-0x0008f7ff pcData is writen to pcStart.
;   Returns address of first byte not written or
;		zero if error.
; Robustness:
;	Writing can only change bits to low. To change bits to
;   	high an entire page must be erased (see FeeClr()).
;	Can not write to protected pages.
; Side effects: Corrupts r1 to r3 and r12.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC FeeWr
			CODE32

FeeWr:	cmp		r0,#0x80000			;if(pcStart<0x80000)
		blo		FWRet				; return pcStart;
		add		r2,r0,r2			;pcEnd = pcStart+uCnt-1;
		sub		r2,r2,#1
        cmp		r0,r2				;if(pcStart>pcEnd)
		bhi		FWRet				; return pcStart.
		mov		r3,#0x80000			;if(pcEnd>=0x8f800)
		add		r3,r3,#0xf800
		cmp		r2,r3
		bhs		FWRet				; return pcStart;
		tst		r0,#1				;if(pcStart odd)
		beq		FWLp				; {
		mov		r12,#0xffff00ff		; iData = r12 = 0xffff00ff;
		ldrb	r3,[r1]				; iData += *pcData<<8;
		add		r12,r12,r3,LSL#8
		sub		r0,r0,#0x1			; Adjust to halfword boundary.
		b		FWWr
FWLp:	cmp		r0,r2				;while(pcStart<=pcEnd)
		bhi		FWRet				; {
		mov		r12,#0xffffff00		; iData = r12 = 0xffffff00;
		ldrb	r3,[r1]				; iData += *pcData++;
		add		r12,r12,r3
		add		r1,r1,#1
        cmp		r0,r2				; if(pcStart>pcEnd)
		bhs		FWWr				;  Write last byte.
		ldrb	r3,[r1]				; else {
		and		r12,r12,#0xffff00ff	;  iData &= 0xffff00ff;
		add		r12,r12,r3,LSL#8	;  iData += *pcData;	}
FWWr:	add		r1,r1,#1			; pcData++;
		mov		r3,#0xff000000		; r3 = MMRBASE.
		orr		r3,r3,#0xff0000
		str		r0,[r3,#Z_FEEADR]	; FEEADR = iAddr;
		str		r12,[r3,#Z_FEEDAT]	; FEEDAT = iData;
		mov		r12,#0x108			; FEEMOD = 0x108.
        str		r12,[r3,#Z_FEEMOD]
		mov		r12,#2				; FEECON = 0x2.
        str		r12,[r3,#Z_FEECON]
FWBsy:	ldr		r12,[r3,#Z_FEESTA]	; while(busy);
		tst		r12,#4
		bne		FWBsy
		tst		r12,#2				; if(flash error)
		movne	r0,#0				;  return 0;
		bne		FWRet
		add		r0,r0,#0x2			; pcStart += 2;
		b       FWLp				; }
FWRet:	bx		lr

			END
