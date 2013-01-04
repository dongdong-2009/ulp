;All ADuC706x library code provided by ADI, including this file, is provided
; as is without warranty of any kind, either expressed or implied.
; You assume any and all risk from the use of this code.  It is the
; responsibility of the person integrating this code into an application
; to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Sets chip power levels.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	2008/02/13	Design Start.

#include "../ADuC7060.inc"

; ClkPow==========Sets chip power levels.
; C Function prototype: uint ClkPow(uint uCD, uint uPupSys, uint uPupPer);
;uCD:{0-7}
;uPupSys:{PUP_NONE|PUP_CORE|PUP_PER|PUP_PLL|PUP_XTAL|PUP_ALL|PUP_ASIS}
;uPupPer:{PER_NONE|SPI2C_DIV2|SPI2C_DIV4|SPI2C_ON|UART_DIV2|UART_DIV4|UART_ON|PWM_DIV2|PWM_DIV4|PWM_ON|PER_ALL}
; Description of Function: Sets a powerdown modes for the part.
; User interface:
;	Set uCD to the required CD divider value.
;	Set uPupSys to the required power control mode as the bitwise or of
;		0x00 or PUP_NONE = All parts powered up.
;		0x01 or PUP_CORE = Core powered up,
;		0x01 or PUP_PER = Peripherals powered up,
;		0x04 or PUP_PLL = PLL powered up.
;		0x08 or PUP_XTAL = Crystal PLL powered up.
;		0x0f or PUP_ALL = All powered up.
;		>0x0f or PUP_ASIS = Leave power down status unchanged.
;	Set uPupPer to the required power control mode as the bitwise or of
;		0x00 or PER_NONE = All peripherals powered off.
;		0x01 or SPI2C_DIV2 = SPI2C clock divide by 2.
;		0x02 or SPI2C_DIV4 = SPI2C clock divide by 4.
;		0x04 or SPI2C_ON = SPI2C powered on.
;		0x08 or UART_DIV2 = UART clock divide by 2.
;		0x10 or UART_DIV4 = UART clock divide by 4.
;		0x20 or UART_ON = UART powered on.
;		0x40 or PWM_DIV2 = PWM clock divide by 2.
;		0x80 or PWM_DIV4 = PWM clock divide by 4.
;		0x100 or PWM_ON = PWM powered on.
;		0x124 or PER_ALL = All peripherals powered on at full speed.
;	Call ClkPow().	Sets chip power levels.
;		Returns value written to POWCON0 register.
; Robustness:  See part documentation for details of reactivation
;               after powerdown.
; Side effects: Corrupts r1 to r3.

			SECTION .text:CODE:NOROOT(2)
			PUBLIC ClkPow
			CODE32

ClkPow:	and		r0,r0,#0x7			;Isolate bits of uCD
		orr		r0,r0,r1,LSL#0x3	; and combine with uPupSys.
		mov		r3,#0xff000000		; r3 = Z_BASE;
		orr		r3,r3,#0xff0000
		mov		r1,#0x76			;Write POWCON1 with keys.
		str		r1,[r3,#Z_POWKEY3]
		str		r2,[r3,#Z_POWCON1]
		mov		r1,#0xb1
		str		r1,[r3,#Z_POWKEY4]
		ldr		r2,[r3,#Z_POWCON0]	;Get POWCON0
		bic		r2,r2,#0xf			; and strip old CD bits.
		cmp		r0,#0x80			;if(PUP:CD<0x80)
		biclo	r2,r2,#0x70			; { Strip old PUP bits
		orrlo	r2,r2,r0			; add new PUP and CD bits. }
		andhs	r0,r0,#0x7			;else { add only
		orrhs	r2,r2,r0			; CD bits. }
		mov		r1,#1				;Write POWCON0 with keys.
		str		r1,[r3,#Z_POWKEY1]
		str		r2,[r3,#Z_POWCON0]
		mov		r1,#0xf4
		str		r1,[r3,#Z_POWKEY2]
RClkP:	ldr		r0,[r3,#Z_POWCON0]	;return POWCON0;
		bx		lr

			END


