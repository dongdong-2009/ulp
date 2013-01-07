/*All ADuC706x library code provided by ADI, including this file, is provided
	as is without warranty of any kind, either expressed or implied.
 	You assume any and all risk from the use of this code.  It is the
 	responsibility of the person integrating this code into an application
 	to ensure that the resulting application performs as required and is safe.
*/
//Status:		Untested.
//Tools:		Developed using IAR V5.1
//Hardware:		ADuC7060
//Description:	Function prototypes and #defines for Timer library.
//Responsibility:
//	Group:		IAC Software
//	Person:		Eckart Hartmann
//Modifications:
//	02/01/2008  Design Start.

#define	uint	unsigned

extern int TCfg(uint uChan, uint uScale, uint uClk, uint uPer);
extern int TMod(uint uChan, uint uUp, uint uFormat);
extern int TGo(uint uChan, uint uTLd, uint uTGo);
extern int TClr(uint uChan);
extern int TCap(uint uChan, int iCapCh);
extern int TWd(uint uChan, uint uWd, uint uSc, uint uWdIrq);

#define T_DOWN		0
#define T_UP		1

#define T_D1		0
#define T_D16		4
#define T_D256		8
#define T_D32K		0xf

#define T_32K		0
#define T_HCLK		1
#define T_10M		2
#define T_PIN		3

#define T_BIN		0
#define T_23H		2
#define T_255H		3

#define T_STOP		0
#define T_RUN		1
#define T_RD		2

#define IREAD		-1
#define IOFF		0
#define IT0			3
#define IT1			4
#define IT2			5
#define IT3			6
#define IADC		10
#define IUART		11
#define ISPI		12
#define XIRQ0		13
#define XIRQ1		14
#define I2CM		15
#define I2CS		16
#define IPWM		17
#define XIRQ2		18
#define XIRQ3		19