/*All ADuC706x library code provided by ADI, including this file, is provided
	as is without warranty of any kind, either expressed or implied.
 	You assume any and all risk from the use of this code.  It is the
 	responsibility of the person integrating this code into an application
 	to ensure that the resulting application performs as required and is safe.
*/
//Status:		Alpha tested
//Tools:		Developed using IAR V5.1
//Hardware:		ADuC7060
//Description:	Function prototypes and #defines for DAC library.
//Responsibility:
//	Group:		IAC Software
//	Person:		Eckart Hartmann
//Modifications:
//	02/01/2008  Design Start.

#define	uint	unsigned

extern int DacOut(uint uChan, int iValue);
extern int DacCfg(uint uChan, uint uRange, uint uCfg);

#define	DAC_RVDD	3
#define	DAC_REXT1	2
#define	DAC_REXT0	1
#define	DAC_RINT	0

#define	DAC_ON		0
#define	DAC_I16		4
#define	DAC_I	 	8
#define	DAC_CLR		0x10
#define	DAC_T1		0x20
#define	BUF_OPA		0x40
#define	DAC_NOBUF	0x80
#define	BUF_LP		0x100
#define	DAC_PD		0x200
