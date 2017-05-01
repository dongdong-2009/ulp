/*
 * misc helper routines for ulp platform
 *
 * miaofng@2013-7-22 initial version
 *
 */

#include "common/bitops.h"

int htoi(char s[])
{
	int i;
	int c, n;

	n = 0;
	for (i = 0; (c = s[i]) != '\0'; ++i) {
		n *= 16;
		if (i == 0 && c == '0') {
			/* Drop the 0x of 0X from the start of the string. */
			c = s[++i];
			if (c != 'x' && c != 'X')
				--i;
		} else if (c >= '0' && c <= '9')
			/* c is a numerical digit. */
			n += c - '0';
		else if (c >= 'a' && c <= 'f')
			/* c is a letter in the range 'a'-'f' */
			n += 10 + (c - 'a');
		else if (c >= 'A' && c <= 'F')
			/* c is a letter in the range 'A'-'F' */
			n += 10 + (c - 'A');
		else
			/* invalid input */
			return n;
	}
	return n;
}

//return how many 1 in para x
int bitcount(int x)
{
	const int nbits[] = {
		0, 1, 1, 2, /*0-3*/
		1, 2, 2, 3, /*4-7*/
		1, 2, 2, 3, /*8-11*/
		2, 3, 3, 4, /*12-15*/
	};

	int n = 0, subx;
	while(x > 0) {
		subx = x & 0xff;
		x >>= 4;
		n += nbits[subx];
	}
	return n;
}
