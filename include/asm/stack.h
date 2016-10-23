/* stack.h -- funzioni di manipolazione dello stack */

#ifndef __STACK_H
#define __STACK_H

/* gestione registro ESP */
#define get_esp() ({ \
	unsigned int __dummy; \
	__asm__( \
		"movl %%esp,%0\n\t" \
		:"=r" (__dummy)); \
	__dummy; \
})
#define set_esp(x)	__asm__("movl %0,%%esp": :"r" (x))

/* push e pop manuali (dword) */
#define mpush(esp,x)	__asm__("movl %0,(%1)":"r"(x):"r"(esp))
#define mpop(esp) ({ \
	unsigned int __dummy; \
	__asm__( \
		"movl (%0),%1":"=r" (__dummy):"r"(esp)); \
	__dummy; \
})

/* push e pop automatici (dword) */
#define apush(x)	__asm__("pushl	%0": : "r" (x))
#define apop() ({ \
	unsigned int __dummy; \
	__asm__( \
		"popl	%0\n\t" \
		:"=r" (__dummy)); \
	__dummy; \
})

#endif
