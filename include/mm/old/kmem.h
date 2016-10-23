/* kmem.h -- Kernel Virtual Memory area management definitions */

#ifndef __KMEM_H
#define __KMEM_H

#include <stddef.h>

int init_kmem(void);
void * kmalloc(size_t size);
void * kmfree(void *address);

#endif
