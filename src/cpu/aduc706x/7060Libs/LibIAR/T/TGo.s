;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1
;Hardware:		ADuC7060
;Description:	Starts/Stops a timer.
;Responsibility:
;	Group:		IAC Software
;	Person:		Eckart Hartmann
;Modifications:
;	02/01/2008  Design Start.

#include "../ADuC7060.inc"

; TGo==========Starts/stops timer with specified reload value.
; C Function prototype: int TGo(uint uChan, uint uTLd, uint uTGo);
;uChan:{0,1,2,3}
;uTLd:{0-0xffffffff} -> 32bit reload value for T1, 16 bit for all others
;uTGo:{T_STOP,T_RUN,T_RD}
; Description of Function: Starts/stops timer with specified reload value.
;	if uTGo = T_STOP or 0 writes uTLd to the reload value of Tnand stops Tn or
;	if uTGo = T_RUN or 1 writes uTLd to the reload value of Tn and starts Tn.
; User interface:
;	Set uChan to desired Timer Tn, 0 to 3.
;	Put the desired reload value in uTLd.
;	Set uTGo to one of the following values:
;       0 or T_STOP -> Stop T.
;       1 or T_RUN  -> write TLD and Run T.
;       >1 or T_RD  -> only read TnVAL.
;   Call TGo().	Depending on uTGo stop or runs Tn with uTLd as start
;       value or only reads TnVAL.
;   Returns current TnVAL in r0.
; Robustness: If uChan > 3 then TGo() only reads TnVAL.
;   Interupt functionality may be added separately.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC TGo
			CODE32

TGo:	mov		r3,#0xff0000		;r3 = Z_BASE
		orr		r3,r3,#0xff000000
		orr		r3,r3,r0,lsl#5		; + uChan*0x20;
		cmp		r0,#3				;if(uChan>3)
		bhi		TGRet				; return uChan;

		cmp		r2,#1				;if(uTGo>1)
		bhi		TGRet				; only read TnVAL.
		ldr		r2,[r3,#Z_T0CON]	;else { r2 = TCON;
		andmi	r2,r2,#0x7f			; if(uTGo<1) clear enable bit.
		orreq	r2,r2,#0x80			; if(uTGo==1) set enable bit. }
		str		r2,[r3,#Z_T0CON]	;T0CON = r2;
		streq	r1,[r3,#Z_T0LD]		;if(uTGo==1) TnLD = uTLd;

TGRet:	ldr		r0,[r3,#Z_T0VAL]	;return TnVAL;
		bx		lr

			END

