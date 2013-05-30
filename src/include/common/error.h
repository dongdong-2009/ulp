/*
 * 	miaofng@2013-3-30 initial version
 *
 *
 */
#ifndef __ULP_ERROR_H_
#define __ULP_ERROR_H_

void __error_printf(char *file, int line, const char *fmt, ...);

#define __error(...) do { \
	__error_printf(__FILE__, __LINE__, __VA_ARGS__); \
} while(0)

#endif /*__ULP_ERROR_H_*/
