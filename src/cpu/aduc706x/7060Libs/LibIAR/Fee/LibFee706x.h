/*All ADuC706x library code provided by ADI, including this file, is provided
	as is without warranty of any kind, either expressed or implied.
 	You assume any and all risk from the use of this code.  It is the
 	responsibility of the person integrating this code into an application
 	to ensure that the resulting application performs as required and is safe.
*/
//Status:		Alpha tested
//Tools:		Developed using IAR V5.1
//Hardware:		ADuC7060
//Description:	Function prototypes and #defines for flash library.
//Responsibility:
//	Group:		IAC Software
//	Person:		Eckart Hartmann
//Modifications:
//	02/01/2008  Design Start.

extern int FeeClr(char* pcStart, char* pcEnd);
extern int FeeWr(char* pcStart, char* pcData, uint uCnt);
