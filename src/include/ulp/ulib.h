/*
 * 	miaofng@2013/7/22 initial version
 */

#ifndef __ULP_ULIB_H_
#define __ULP_ULIB_H_

/* htoi:  convert hexdicimal string s to integer, such as 0x1B, 1b .. */
int htoi(const char *s);
int bitcount(int n); //return how many 1 in para n
void hexdump(const char *prefix, const void *data, int nbytes);

#endif /*__ULP_ULIB_H_*/
