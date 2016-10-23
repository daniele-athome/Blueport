/* mem_desc.h -- global memory allocation descriptors management */

#ifndef __MEM_DESC_H
#define __MEM_DESC_H

#include <stddef.h>
#include <mm/paging.h>

#define KERNEL_AREA	0
#define USER_AREA	1

/* in teoria questo servirebbe solo per vedere se un descrittore è libero, ma già che ci sono metto anche
 altri stati a buffo */
typedef enum
{
	VUOTO = 0,		/* descrittore vuoto */
	ALLOCATION = 1,		/* descrittore usato per un'allocazione */
} ALLOC_TYPE;

/* descrittore del blocco di allocazione */
typedef struct
{
	void * address;		/* indirizzo di base dell'allocazione */
	size_t size:25;		/* grandezza in pagine dell'allocazione */
	ALLOC_TYPE type:7;	/* giusto per arrotondare... */
} alloc_desc;

#define PAGE_VECTOR_SIZE	MAX_PAGES / (sizeof(dword)*8)
/* riserva: 1MB+4MB = 5 MB */
#define DESC_PER_PAGE		(PAGE_SIZE-sizeof(void*)-sizeof(size_t))/(sizeof(alloc_desc))

/* struttura di una pagina delle allocazioni */
typedef struct
{
	alloc_desc desc[DESC_PER_PAGE];	/* allocazioni contenute in una pagina */
	size_t free_desc;	/* numero di descrittori liberi (più che altro serve per arrotondare...) */
	void * next;		/* prossima pagina con le allocazioni */
} alloc_page;

/* area di memoria (fisica o virtuale che sia, dimensione massima data dal limite dell'architettura) */
typedef struct
{
	dword page_bitmap[PAGE_VECTOR_SIZE];	/* bitmap delle pagine */
	unsigned char privilege;	/* 0 = kernel, 1 = user */
	pagedir_t * pagedir;		/* page directory per questa memoria virtuale */
	alloc_page * first_page;	/* prima pagina contenente i descrittori delle allocazioni */
} memory_area;

void *malloc(memory_area * mem_area,size_t size);
void *mfree(memory_area * mem_area,void *address);
int init_mem_area(memory_area * mem_area,unsigned char privilege,unsigned char is_kernel);
void *create_allocation_page(memory_area * mem_area);
void *virtual_alloc(memory_area * mem_area,size_t pages);
void *virtual_free(memory_area * mem_area,void * virtual);

/* ex: paging.h -- rimosse per problemi di dichiarazioni */

int init_pagedata(memory_area * mem_area);
void * paging_alloc(memory_area * mem_area,void * virtual,void * address,unsigned int rw);
void * paging_free(memory_area * mem_area, void *virtual);

#endif
