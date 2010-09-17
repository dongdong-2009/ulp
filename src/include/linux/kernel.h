#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */

#include <stdarg.h>
#include <linux/stddef.h>

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	char *__mptr = (ptr);	\
	(type *)( __mptr - offsetof(type,member) );})

#endif
