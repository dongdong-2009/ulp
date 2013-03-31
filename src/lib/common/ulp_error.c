/*
 * 	miaofng@2013-3-31 initial version
 */

#include "common/error.h"
#include "common/ansi.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void __error_printf(char *file, int line, const char *fmt, ...)
{
	va_list ap;
	char *p = strrchr(file, 92); // "\"
	p = (p == NULL) ? strrchr(file, '/') : p;
	file = (p == NULL) ? file : p + 1;

	printf(ANSI_FONT_RED);
	printf("ERROR: ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(" .. line %d of %s\n", line, file);
	printf(ANSI_FONT_DEF);
}
