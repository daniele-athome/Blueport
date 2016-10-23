/* paging.h -- memory driver paging definitions */
/******************************************************************************
* Blueport Operating System                                                   *
*                                                                             *
* This program is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU General Public License                 *
* as published by the Free Software Foundation; either version 2              *
* of the License, or (at your option) any later version.                      *
*                                                                             *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU General Public License for more details.                                *
*                                                                             *
* You should have received a copy of the GNU General Public License           *
* along with this program; if not, write to the Free Software                 *
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
*******************************************************************************/

#ifndef __PAGING_H
#define __PAGING_H

#include <stddef.h>

/* codici di errore usati da paging_alloc, paging_free, get_physical */
#define ERR_PAGENOTPRES		0x000	// 000000000000 -- pagina non trovata/non presente
#define ERR_PAGEALREADY		0x001	// 000000000001 -- pagina gi� mappata
#define ERR_PGTABLENOTPRES	0x002	// 000000000010 -- pagetable per la pagina richiesta non presente
#define ERR_NOTPAGEALIGNED	0x004	// 000000000100 -- indirizzo non page-aligned
#define ERR_NOPHYSMEM		0x008	// 000000001000 -- memoria fisica esaurita

#define SIZEOF_FLAGS	0xFFF	// limite delle maschere dei flag

/* costanti dei bit delle pde/pte */
#define PAGE_PRESENT	0x001	// 000000000001
#define PAGE_RW		0x002	// 000000000010
#define PAGE_USER	0x004	// 000000000100

/* Flag pre-confezionti :) */
#define PAGE_NULL	0					// pagina inesistente
#define PAGE_LASTPDE	PAGE_PRESENT | PAGE_RW			// flag per la pde di ricorsione
#define PAGE_TABLE	PAGE_PRESENT | PAGE_RW | PAGE_USER	// flag per una page directory entry
#define PAGE_SHARED	PAGE_PRESENT | PAGE_RW | PAGE_USER	// flag per una pagina utente (kernel/user space)
#define PAGE_SYSTEM	PAGE_PRESENT | PAGE_RW			// flag per una pagina di sistema (user space)
#define PAGE_RESERVED	PAGE_PRESENT				// flag per una pagina riservata (solo kernel mode)
//#define PAGE_COPY	PAGE_PRESENT | PAGE_USER		// flag per una pagina copy-on-write -- TODO

/*
Pagedir kernel:
	PDE#nn: PAGE_TABLE
	PTE#nn: user: --
		1mb: PAGE_RESERVED
		kernel: PAGE_SHARED	(tanto la usiamo solo noi...)

Pagedir user:
	PDE#nn:	PAGE_TABLE
	PTE#nn:	user: PAGE_SHARED
		1mb: PAGE_RESERVED
		kernel: PAGE_SYSTEM	(qui dobbiamo proteggere...)
*/

/* costanti varie */
#define PAGE_SIZE       4096
#define TABLE_ENTRIES	1024

#define MAX_PAGES		1048576		// 4GB / 4KB
#define PAGE_VECTOR_SIZE	MAX_PAGES / (sizeof(dword)*8)

/* indirizzi virtuali dei dati di paginazione */
#define PD_VIRT		0xFFFFF000	// ((TABLE_ENTRIES*TABLE_ENTRIES-1)*PAGE_SIZE)
#define PT_VIRT		0xFFC00000	// ((TABLE_ENTRIES*(TABLE_ENTRIES-1))*PAGE_SIZE)

/* Address alignment */
#define ADDR_ALIGN(x,a)	((unsigned long)(x) & (0xFFFFFFFF-((a)-1)))
#define PAGE_ALIGN(x)	ADDR_ALIGN(x,PAGE_SIZE)

#if 0
typedef struct
{
	unsigned int present:1;		/* Page Present Bit */
	unsigned int readwrite:1;	/* Read/Write Bit */
	unsigned int privilege:1;	/* User/Supervisor Bit */
	unsigned int pagewrite:1;	/* Page Write Through Bit */
	unsigned int pagecache:1;	/* Page Cache Disable Bit */
	unsigned int accessed:1;	/* Accessed Bit */
	unsigned int dirty:1;		/* Dirty Bit */
	unsigned int reserved:1;	/* (Page Size Bit) */
	unsigned int global:1;		/* Global Page Bit */
	unsigned int unused:3;		/* Unused (Free Bits) */
	unsigned int page_address:20;
} __attribute__ ((packed)) pagetable_entry;

typedef struct
{
	unsigned int present:1;		/* Page Present Bit */
	unsigned int readwrite:1;	/* Read/Write Bit */
	unsigned int privilege:1;	/* User/Supervisor Bit */
	unsigned int pagewrite:1;	/* Page Write Through Bit */
	unsigned int pagecache:1;	/* Page Cache Disable Bit */
	unsigned int accessed:1;	/* Accessed Bit */
	unsigned int reserved:1;	/* (Dirty Bit) */
	unsigned int pagesize:1;	/* Page Size Bit */
	unsigned int global:1;		/* Global Page Bit (ignored) */
	unsigned int unused:3;		/* Unused (Free Bits) */
	unsigned int table_address:20;
} __attribute__ ((packed)) pagedir_entry;
#endif

typedef unsigned long pagetable_entry;
typedef unsigned long pagedir_entry;

typedef struct
{
	pagedir_entry pde[TABLE_ENTRIES];
} pagedir_t;

typedef struct
{
	pagetable_entry pte[TABLE_ENTRIES];
} pagetable_t;

/* Costruzione entry */
#define make_pagetable(addr,flags) (((unsigned long)(addr) & 0xFFFFF000) | ((flags) & 0x00000FFF))
#define make_pagedir(addr,flags) make_pagetable(addr,flags)

/* Modifica entry */
/* presenza */
#define set_present(entry) (entry | PAGE_PRESENT)
#define set_notpresent(entry) (entry & ~PAGE_PRESENT)

/* permessi */
#define set_writable(entry) (entry | PAGE_WRITE)
#define set_readonly(entry) (entry & ~PAGE_WRITE)

/* privilegi */
#define set_userpage(entry) (entry | PAGE_USER)
#define set_supervisor(entry) (entry & ~PAGE_USER)

/* Estrazione indirizzo/flags */
#define get_address(entry) (entry & 0xFFFFF000)
#define get_flags(entry) (entry & 0x00000FFF)

/* Verifiche varie */
#define is_present(entry) (entry & PAGE_PRESENT)
#define is_writable(entry) (entry & PAGE_WRITE)
#define is_userpage(entry) (entry & PAGE_USER)

#define flush_tlb_one(addr) __asm__ __volatile__("invlpg %0": :"m" (*(char *) addr))
#define set_CR3(value) __asm__ __volatile__("mov %0, %%cr3"::"r"(value));
#define get_CR3() ({ \
	unsigned int __dummy; \
	__asm__( \
		"movl %%cr3,%0\n\t" \
		:"=r" (__dummy)); \
	__dummy; \
})
#define get_CR2() ({ \
	unsigned int __dummy; \
	__asm__( \
		"movl %%cr2,%0\n\t" \
		:"=r" (__dummy)); \
	__dummy; \
})

#define invalidate() __asm__ __volatile__("movl %%cr3,%%eax\n\tmovl %%eax,%%cr3": : :"%eax")

void enable_paging(void);
void disable_paging(void);

void init_paging(void);
pagedir_t * init_paging_data(pagedir_t ** pd_virt);
int clean_paging(void);
int free_paging_data(pagedir_t * pd_virt);
void * identity_mapping(void * physical,size_t count,unsigned int flags);
void * paging_multi(void * virtual,void * fis_addrs[],size_t count,unsigned int flags);
void * paging_alloc(void * physical,void * virtual,unsigned int flags);
void * paging_free(void * virtual,size_t count);

void * get_physical(void * virtual);
pagedir_t * get_kernel_pagedir(void);
void print_pagedir(pagedir_t * pd,int index);
void print_pagetable(pagetable_t * pt,int index);

#define get_physical_address(x)	get_address((unsigned long)get_physical(x))

#endif
