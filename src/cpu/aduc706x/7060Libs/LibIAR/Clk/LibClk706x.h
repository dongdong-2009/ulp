/*All ADuC706x library code provided by ADI, including this file, is provided
	as is without warranty of any kind, either expressed or implied.
 	You assume any and all risk from the use of this code.  It is the
 	responsibility of the person integrating this code into an application
 	to ensure that the resulting application performs as required and is safe.

;Status:		Alpha tested.
;Tools:			Developed using IAR V5.1.
;Hardware:		ADuC7060.
;Description:	Header file for clock library.
;Responsibility:
;    Group:		IAC Software
;    Person:	Eckart Hartmann
;Modifications:
;	2008/02/13	Design Start.
*/
extern uint ClkSta(uint uClrPllInt);
extern uint ClkPow(uint uCD, uint uPupSys, uint uPupPer);
extern uint ClkSpd(uint uCD);

#define	PUP_NONE	0x00
#define	PUP_CORE	0x01
#define	PUP_PER		0x02
#define	PUP_PLL		0x04
#define	PUP_XTAL	0x08
#define	PUP_ALL		0x0f
#define	PUP_ASIS	0x10

#define	PER_NONE	0x00
#define	SPI2C_DIV2	0x01
#define	SPI2C_DIV4	0x02
#define	SPI2C_ON	0x04
#define	UART_DIV2	0x08
#define	UART_DIV4	0x10
#define	UART_ON		0x20
#define	PWM_DIV2	0x40
#define	PWM_DIV4	0x80
#define	PWM_ON		0x100
#define	PER_ALL		0x124
