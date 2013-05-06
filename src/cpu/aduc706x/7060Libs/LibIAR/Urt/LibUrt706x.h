/*All ADuC706x library code provided by ADI, including this file, is provided
	as is without warranty of any kind, either expressed or implied.
 	You assume any and all risk from the use of this code.  It is the
 	responsibility of the person integrating this code into an application
 	to ensure that the resulting application performs as required and is safe.
*/
//Status:		Alpha tested
//Tools:		Developed using IAR V5.1
//Hardware:		ADuC7060
//Description:	Function prototypes and #defines for UART library.
//Responsibility:
//	Group:		IAC Software
//	Person:		Eckart Hartmann
//Modifications:
//	02/01/2008  Design Start.

extern int UrtWr(int iTx);
extern int getchar(void);
extern int putchar(int iTx);
extern int UrtCfg(uint uChan, uint uBaud, uint uFormat);
extern int UrtRd(void);
extern int UrtSta(void);

#define URT_6       0
#define URT_12      1
#define URT_24      2
#define URT_48      3
#define URT_96      4
#define URT_192     5
#define URT_384     6
#define URT_576     7
#define URT_1152    8
#define URT_2304    9
#define URT_4608    10

#define URT_DTR     1
#define URT_DSR     2
#define URT_LB      8

#define URT_68      1
#define URT_78      2
#define URT_2S      4
#define URT_PE      8
#define URT_PODD    16
#define URT_PS      32
#define URT_BR      64
