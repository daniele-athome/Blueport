/* mem.c -- memory managment functions */
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

#include <asm/bitops.h>
#include <asm/system.h>
#include <mm/mem.h>
#include <mm/paging.h>
#include <mm/fismem.h>
#include <mm/config.h>
#include <mm/virmem.h>
#include <mm/mem_desc.h>
#include <mm/mem_area.h>
#include <mm/kmem.h>
#include <mm/page_fault.h>
#include <io.h>
#include <math.h>
#include <string.h>
#include <ktypes.h>
#include <kernel.h>
#include <panic.h>
#include <stddef.h>
#include <version.h>
#include <progress.h>

unsigned long mem_end,memmap_size;
extern multiboot_info kernel_info;
extern memory_area kmem_area;
memory_map mmap[MAX_MMAP];
unsigned long mmap_count;

/* conta la memoria "probandola"
 potrebbe essere dannoso, usare con cautela
 setta la variabile mem_end con la dimensione della memoria fisica in byte */
void count_memory(void)
{
	register unsigned long *mem;
	unsigned long mem_count, a;
	unsigned short memkb;
	unsigned char irq1, irq2;
	unsigned long cr0;

/*
 * save IRQ's 
 */
	irq1 = inportb(0x21);
	irq2 = inportb(0xA1);

/*
 * kill all irq's 
 */
	outportb(0x21, 0xFF);
	outportb(0xA1, 0xFF);

	mem_count = 0;
	memkb = 0;

// store a copy of CR0
	__asm__ __volatile("movl %%cr0, %0":"=r"(cr0):);

// invalidate the cache
// write-back and invalidate the cache
	__asm__ __volatile__("wbinvd");

// plug cr0 with just PE/CD/NW
// cache disable(486+), no-writeback(486+), 32bit mode(386+)
	__asm__ __volatile__("movl %0, %%cr0"::
			     "g"(cr0 | 0x00000001 | 0x40000000 | 0x20000000));

	do {
		memkb++;
		mem_count += 1024 * 1024;
		mem = (unsigned long *) mem_count;

		a = *mem;

		*mem = 0x55AA55AA;

// the empty asm calls tell gcc not to rely on whats in its registers
// as saved variables (this gets us around GCC optimizations)
	      asm("":::"memory");
		if (*mem != 0x55AA55AA)
			mem_count = 0;
		else {
			*mem = 0xAA55AA55;
		      asm("":::"memory");
			if (*mem != 0xAA55AA55)
				mem_count = 0;
		}

	      asm("":::"memory");
		*mem = a;

	}
	while (memkb < 4096 && mem_count != 0);

	__asm__ __volatile__("movl %0, %%cr0"::"r"(cr0));

	mem_end = memkb << 20;
	//mem = (unsigned long *) 0x413;
	//bse_end = ((*mem) & 0xFFFF) << 6;

	outportb(0x21, irq1);
	outportb(0xA1, irq2);
}

/* memorizza il conteggio della memoria in mem_end leggendo dai dati del bootloader */
void multiboot_memory(void)
{
	mem_end = (kernel_info.mem_upper + 1024) * 1024;	// il multiboot ce lo manda in kilobyte
}

/* memorizza il conteggio della memoria in mem_end leggendo dalla BIOS Data Area */
void bda_memory(void)
{
	mem_end = *((unsigned long *)(0x413)) * 1024;
}

/* memorizza il conteggio della memoria in mem_end leggendo dai dati nel CMOS */
void cmos_memory(void)
{
	/* TODO */
	mem_end = 0;
}

/* inizializza i gestori di memoria
 1. conta la memoria
 2. controlla la memoria necessaria all'esecuzione del sistema
 3. inizializza i gestori di memoria
*/
int init_mm(void)
{
	unsigned long mem_needed;
	unsigned long tmp = 0;

	if((unsigned long)load_adr != 0x100000)
	{	/* FIXME: bruttissima sta cosa :| */
		panic("mm: il kernel non è stato caricato all'indirizzo 0x100000. Non posso continuare.\n");
		return -1;
	}

 if(!mem_end) {
	#ifdef DEBUG
	kprint("mm: calcolo della memoria disponibile...\n");
	#endif
	#ifdef MM_PROBE
	switch(MM_PROBE)
	{
		case MM_COUNT:
			count_memory();
			break;
		case MM_MULTIBOOT:
			multiboot_memory();
			break;
		case MM_CMOS:
			cmos_memory();
			break;
		case MM_BDA:
			bda_memory();
			break;
		default:
			multiboot_memory();
	}
	#else
	multiboot_memory();
	#endif
	kprint("mm: memoria fisica totale: %d MB (%d KB)\n",mem_end/(1024*1024),mem_end/1024);
/* calcola memoria usabile (sommiamo solo i primi 32 bit per ora) */
	mmap_count = kernel_info.mmap_length/sizeof(memory_map);
	for(tmp=0;tmp<mmap_count;tmp++)
	{
		/* eliminazione 32-bit più significativi */
		if(mmap[tmp].base_addr_high > 0)
		{
			mmap[tmp].base_addr_low = 0xFFFFFFFF;
			mmap[tmp].base_addr_high = 0;
		}
		if(mmap[tmp].length_high > 0)
		{
			mmap[tmp].length_low = 0xFFFFFFFF;
			mmap[tmp].length_high = 0;
		}
		if(mmap[tmp].type == MMAP_USABLE) memmap_size += mmap[tmp].length_low;
	}
	kprint("mm: memoria fisica usabile: %d MB (%d KB)\n",memmap_size/(1024*1024),memmap_size/1024);

/* memoria necessaria: DA CALCOLARE */
	mem_needed = 1024 * 1024 * 4;	/* a buffo: 4 MB :D */
	if(memmap_size < mem_needed)
	{
		tmp = mem_needed/(1024*1024);
		if(mem_needed % (1024*1024)) tmp++;
		kprint("mm: necessari %d MB di memoria per continuare l'esecuzione di %s.\n",tmp,OSNAME);
		return -1;
	}
	/* pulisco la memory area del kernel */
	memset((void *)&kmem_area,(char)0,sizeof(memory_area));
/* MEMORIA FISICA */
	kprint("mm: inizializzazione memoria fisica...\n");
	if(init_fismem() < 0)
	{
		kprint("mm: memoria fisica insufficente.\n");
		return -1;
	}
/* MEMORIA VIRTUALE */
	kprint("mm: inizializzazione memoria virtuale del kernel...\n");
	if(init_vm(kmem_area.page_bitmap,1) < 0)
	{
		kprint("mm: memoria virtuale del kernel non impostata.\n");
		return -1;
	}
	return 0;
 } else {
/* PAGINAZIONE */
 	if(kmem_area.pagedir) return 0;	// mm già iniziliazzate
	kmem_area.pagedir = kmem_area.paging_data = get_kernel_pagedir();
	/* imposta l'handler per il page fault */
	init_pagefault();
	switch_memory_area(&kmem_area);	// fico...
/* DESCRITTORI DI ALLOCAZIONE */
	kprint("mm: inizializzazione dei descrittori di allocazione...\n");
	if(init_desc(kmem_area.page_bitmap,1,&kmem_area.first_alloc) <= 0)
	{
		kprint("mm: descrittori di allocazione non validi.\n");
		return -1;
	}
/* BENCHMARK */
	#ifdef DEBUG2
	unsigned int i;
	void * t = 0;
	kprint("mm: test della memoria... ");
	star_progress_stop(0);
	star_progress_start(0);

	for(i=0;i<10;i++)
	{
	t = kmalloc(0x400000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0x500000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0x600000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0x700000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0x800000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0x900000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0xA00000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0xB00000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0xC00000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0xD00000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0xE00000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0xF00000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	t = kmalloc(0x1000000);
	star_progress_step(1);
	if(t) kfree(t);
	else goto end_memtest;
	star_progress_step(1);

	}
end_memtest:
	star_progress_stop(1);
	if(i>=10) kprint("mm: test della memoria completato con successo.\n");
	else kprint("mm: test della memoria terminato al tentativo %d di 10.\n",i+1);
	#endif
	kprint("mm: memoria pronta per l'uso.\n");
	return 0;
 } /* ! if(!mem_end) ! */
}
