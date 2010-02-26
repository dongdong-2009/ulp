/*
 * 	miaofng@2009 initial version
 */
#ifndef __VECTOR_H_
#define __VECTOR_H_

typedef union {
	struct { /*a/b/c axis*/
		short a;
		short b;
	};
	struct { /*alpha/beta axis*/
		short alpha;
		short beta;
	};
	struct { /*d/q axis*/
		short d;
		short q;
	};
} vector_t;

/*
*note:
* 1. use poshorter instead of struct to avoid parameter copy, then increase the speed
* 2, each component of a vector shouldn't exceed 2^15, or overflow may occur
*/
void park(const vector_t *pvi, vector_t *pvo, short theta);
void ipark(const vector_t *pvi, vector_t *pvo, short theta);
void clarke(const vector_t *pvi, vector_t *pvo);
void iclarke(const vector_t *pvi, vector_t *pvo);
#endif /*__VECTOR_H_*/
