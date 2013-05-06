/*All ADuC706x library code provided by ADI, including this file, is provided
	as is without warranty of any kind, either expressed or implied.
 	You assume any and all risk from the use of this code.  It is the
 	responsibility of the person integrating this code into an application
 	to ensure that the resulting application performs as required and is safe.
*/
//Status:		Alpha tested
//Tools:		Developed using IAR V5.1
//Hardware:		ADuC7060
//Description:	Function prototypes and #defines for ADC library.
//Responsibility:
//	Group:		IAC Software
//	Person:		Eckart Hartmann
//Modifications:
//	02/01/2008  Design Start.

#define	uint	unsigned

extern int AdcAdj(uint uAdcAdj);
extern int AdcRd(int *piAdcMain, int *piAdcAux);
extern int AdcGo(uint uChan, uint uStart, uint uPower, uint uOther);
extern int AdcFlt(uint uSF, uint uAF, uint uFltCfg);
extern int AdcIex(int iIuA, uint uOn, uint uPin);
extern int AdcCfg(uint uChan, uint uAdcIn, uint uAdcCfg);
extern int AdcRng(uint uChan, uint uPga, uint uAdcRef);

#define	ADC_OFF		0
#define	ADC_CONT	1
#define	ADC_SINGLE	2
#define	ADC_IDLE	3

#define	ADC_NORMAL	0
#define	ADC_LP	 	1
#define	ADC_LP2		2

#define	ADC_IREF	0
#define	ADC_XREF 	1
#define	ADC_AREF	2
#define	ADC_VREF	3
#define	ADC_TREF	4

#define	ADCLPMREFSEL 	0x20
#define	ADC_CLKSEL	0x80

#define	ADC001		0
#define	ADC00		1
#define	ADC01		2
#define	ADC0REF		3
#define	ADC0CALR	4
#define	ADC023		5
#define	ADC02		6
#define	ADC03		7
#define	ADC000		8
#define	ADC011		9

#define	ADC123		0
#define	ADC145		1
#define	ADC167		2
#define	ADC189		3
#define	ADC12		4
#define	ADC13		5
#define	ADC14		6
#define	ADC16		7
#define	ADC17		8
#define	ADC18		9
#define	ADC19		0xa
#define	ADC1ITMP	0xb
#define	ADC1REF		0xc
#define	ADC1DAC		0xd
#define	ADC1CALR	0xe
#define	ADC133		0xf

#define	ADC_CODE
#define	ADC_CM
#define	ADC_ISRC0
#define	ADC_ISRC1

#define	IEX_OFF		0
#define	IEX_0ON 	0x40
#define	IEX_1ON		0x80

#define	IEX_STD		0
#define	IEX_0ALT 	0x10
#define	IEX_1ALT	0x20

#define	FLT_NOTCH2	0x80
#define	FLT_RAVG2 	0x4000
#define	FLT_CHOP	0x8000
