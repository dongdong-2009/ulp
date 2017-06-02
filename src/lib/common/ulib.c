/*
 * misc helper routines for ulp platform
 *
 * miaofng@2013-7-22 initial version
 *
 */

#include "ulp/sys.h"
#include "common/bitops.h"
#include <string.h>
#include <ctype.h>

int htoi(const char *s)
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

void hexdump(const char *prefix, const void *data, int nbytes)
{
	unsigned i, j, v;
	const char *p = (const char *) data;
	char *hex = (char *) sys_malloc(64);
	char *txt = (char *) sys_malloc(64);
	char *all = (char *) sys_malloc(128);

	for(i = 0; i < nbytes; i += 8) {
		hex[0] = '\0';
		txt[0] = '\0';
		for(j = 0; j < 8; j ++) {
			if(i + j < nbytes) {
				v = (unsigned) p[i + j];
				v &= 0xff;
				sprintf(hex, "%s%02x ", hex, v);

				v = isprint(v) ? v : '.';
				sprintf(txt, "%s%c", txt, v);
			}
		}

		prefix = (prefix == NULL) ? "" : prefix;
		if((i >= 8) || (nbytes > i + j)) { //multi line add addr counter
			sprintf(all, "%s %04X: %s", prefix, i, hex);
		}
		else { //single line
			sprintf(all, "%s: %s", prefix, hex);
		}
		printf("%-40s%s\n", all, txt);
		//printf("%s%-28s%s\n", prefix, hex, txt);
	}
	sys_free(hex);
	sys_free(txt);
	sys_free(all);
}
