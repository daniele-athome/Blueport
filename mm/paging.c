/* paging.c -- Hardware Paging MM Subsystem */
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

/** Livello 2: allocazione virtuale -- basso livello (paginazione) **
Questo livello mappa gli indirizzi fisici in indirizzi virtuali.
Questo livello dipende dal livello 1 (allocazione fisica).
Questo livelle è necessario per il livello 3 (allocazione virtuale -- alto livello).
**/

#include <mm/paging.h>
#include <mm/fismem.h>
#include <mm/virmem.h>
#include <mm/mem_area.h>
#include <mm/page_fault.h>
#include <string.h>
#include <kernel.h>
#include <panic.h>
#include <asm/bitops.h>
#include <asm/system.h>

//#define VIRT_TO_PHYS(a_) ((void*)((unsigned long *)(a_)-0xc0000000UL+0x100000))		// merda...

// ok, si presuppone che il primo mega non sia virtuale :)
#define KERNEL_PAGEDIR_TMP	0x10000
#define KERNEL_PAGETBL1_TMP	KERNEL_PAGEDIR_TMP+PAGE_SIZE
#define KERNEL_PAGETBL2_TMP	KERNEL_PAGETBL1_TMP+PAGE_SIZE

#if 0
#define block()	__asm__ __volatile__("movw $0x9F45,(0xB8000)\n" \
		"jmp .\n")
#else
#define block() __asm__ __volatile__("jmp .\n")
#endif

extern memory_area kmem_area;
void * vir_manual_alloc(void * address,size_t count);
void * fis_manual_alloc(void * address,size_t count);

void enable_paging(void)
{
	dword control;
	asm volatile ("mov %%cr0, %0":"=r" (control));
	control |= 0x80000000;
	asm volatile ("mov %0, %%cr0"::"r" (control));
/* ricarica i registri di segmento e fa un bel salto */
	asm volatile
	("movw %0,%%ax     \n"
	"movw %%ax,%%ds      \n"
	"movw %%ax,%%es      \n"
	"movw %%ax,%%fs      \n"
	"movw %%ax,%%gs      \n"
	"movw %%ax,%%ss      \n"
	"ljmp %1,$0f    \n"
	"nop\n" "nop\n"
	"0:\n"::"g"(KERNEL_DS_SEL),"g"(KERNEL_CS_SEL):"%eax");
}

void disable_paging(void)
{
	dword control;
	asm volatile ("mov %%cr0, %0":"=r" (control));
	control &= ~0x80000000;
	asm volatile ("mov %0, %%cr0"::"r" (control));
}

/* *** FUNZIONCIONA *** libera i dati di paginazione per un task (non è predisposta per liberare quelli del kernel)
 ATTENZIONE: questa funzione non libera le pagine mappate, ma solo le pagine occupate dalle varie pagetable
 pagedir: indirizzo VIRTUALE della pagedir */
int free_paging_data(pagedir_t * pd_virt)
{
	unsigned int i;
	if(!((unsigned long)get_physical_address(pd_virt) & 0xFFFFF000)) return -1;	// pagedir non corretta
/* assicuriamoci che non stiamo lavorando con la pagedir che dobbiamo eliminare (non si sa mai :) */
	if((get_CR3() & 0xFFFFF000) == (unsigned long)get_physical_address(pd_virt)) return -1;	// dati di paging in uso
/* mappiamo la pagedir in qualche zona per poterla leggere */
/* ok, guardiamoci le varie pde alla ricerca di pagetable allocate */
	for(i=256;i<768;i++)	// dal primo mega fino all'inizio del kernel
	 if((is_present(pd_virt->pde[i])) && (get_address(pd_virt->pde[i])))	// «Carica. Libera!»
		 fis_free((void *)get_address(pd_virt->pde[i]),1);
	vir_free(kmem_area.page_bitmap,pd_virt,1);	// libera la pagedir stessa
	return 0;
}

/* costruisce i dati di paginazione per un task
 prepara la roba per la paginazione, mappa la roba del kernel e il primo mega
 mette in pd_virt l'indirizzo VIRTUALE della pagedir creata 
 restituisce l'indirizzo FISICO della pagedir creata, altrimenti 0 in caso di errore */
pagedir_t * init_paging_data(pagedir_t ** pd_virt)
{
	unsigned int i;
	pagedir_t * pd, *source_pd = (pagedir_t *)PD_VIRT;
/* questa funzione verrà usata solo per creare i dati di paginazione per un task, quindi tanto vale usare la mem_alloc */
	pd = mem_alloc(&kmem_area,sizeof(pagedir_t),PAGE_SYSTEM);
	#ifdef DEBUG2
	kprint("Page Directory at 0x%x (0x%x)\n",pd,get_physical_address(pd));
	#endif
	if(!pd || !get_physical_address(pd)) return (pagedir_t *)0;
	memset((void *)pd,(char)0,sizeof(pagedir_t));
/* mappatura fisica primo mega in privilegio kernel (tranne la pagina 0) */
	pd->pde[0] = source_pd->pde[0];	// ah, proprio così...?
/* mappatura di tutta la roba mappata dal kernel fin'ora (cioè da 0xc0000000 in poi, compresa la ricorsione quindi) */
	for(i=768;i<TABLE_ENTRIES;i++) pd->pde[i] = source_pd->pde[i];
	*pd_virt = pd;
	return (pagedir_t *)get_physical_address(pd);
}

/* da una pulita ai dati di paginazione correnti (tanto era tutta roba user :)
 per ora leva la robba mappata dopo il primo mega (non presente) */
int clean_paging(void)
{
	unsigned int i;
	pagetable_t * pt = (pagetable_t *)PT_VIRT;
	for(i=256;i<TABLE_ENTRIES;i++) pt->pte[i] = 0;
	return 0;
}

/* inizializza i dati di paginazione per il kernel al boot
 questa funzione mappa il primo mega e lo spazio del kernel */
void init_paging(void)
{
	pagedir_t * pd = (pagedir_t *)(KERNEL_PAGEDIR_TMP);
	pagetable_t * pt;
	unsigned int i,krnl_size,pt_entry;
/* pulisce la pagedir */
	memset((void *)pd,(char)0,sizeof(pagedir_t));
/* setta la ricorsione */
	pd->pde[TABLE_ENTRIES-1] = make_pagedir(pd,PAGE_LASTPDE);
/* smappa il primo mega */
	pt = (pagetable_t *)(KERNEL_PAGETBL1_TMP);
	memset((void *)pt,(char)0,sizeof(pagetable_t));
	pd->pde[0] = make_pagedir(pt,PAGE_TABLE);
/* mappa fisicamente il primo mega e la pagedir del kernel come PAGE_SYSTEM */
	for(i=1;i<(1048576/PAGE_SIZE);i++) pt->pte[i] = make_pagetable(i*PAGE_SIZE,PAGE_RESERVED);
	pt_entry = ((unsigned long)pd / PAGE_SIZE) % TABLE_ENTRIES;
	pt->pte[pt_entry] = make_pagetable(pd,PAGE_SYSTEM);
/* mappa fisicamente il kernel (il kernel deve finire prima del 4° mega) */
	krnl_size = (g_end - g_start) / PAGE_SIZE;
	if((g_end - g_start) % PAGE_SIZE) krnl_size++;
	if((((unsigned long)load_adr / PAGE_SIZE)+krnl_size) >= 0x400000) block();	// peccato :P
	for(i=(unsigned long)load_adr/PAGE_SIZE;i<=((unsigned long)load_adr/PAGE_SIZE)+krnl_size;i++)
	 pt->pte[i%TABLE_ENTRIES] = make_pagetable(i*PAGE_SIZE,PAGE_SYSTEM);
	/* asm("movl	$0x11000,%%ebx\n"
	    "movl	1024(%%ebx),%%eax\n"
	    "jmp	.\n"
	    :::"%ebx","%eax"); */
/* mappa virtualmente il kernel */
	/* controlla che il kernel non passi per più di una pagetable */
	if( ( ((unsigned long)g_start / PAGE_SIZE) / TABLE_ENTRIES ) != 
	 ( ( ((unsigned long)g_start / PAGE_SIZE) + krnl_size) / TABLE_ENTRIES)) block();
	pt = (pagetable_t *)(KERNEL_PAGETBL2_TMP);
	memset((void *)pt,(char)0,sizeof(pagetable_t));
	pd->pde[((unsigned long)g_start / PAGE_SIZE) / TABLE_ENTRIES] = make_pagedir(pt,PAGE_TABLE);
	/* parti da g_start/PAGE_SIZE per arrivare fino a g_end/PAGE_SIZE (più o meno ;) */
	for(i=(unsigned long)g_start / PAGE_SIZE;i<=((unsigned long)g_end / PAGE_SIZE + 1);i++)
	 pt->pte[i%TABLE_ENTRIES] = make_pagetable((i-(unsigned long)g_start/PAGE_SIZE)*PAGE_SIZE+load_adr,PAGE_SYSTEM);
/* aggiorna CR3 */
	set_CR3((dword)pd & 0xFFFFF000);
}

/* discardabile -- toglie la mappatura fisica del kernel all'avvio dopo init_paging */
void remove_kernelident(void)
{
	pagetable_t * pt;
	unsigned int krnl_size,i;
	krnl_size = (g_end - g_start) / PAGE_SIZE;
	if((g_end - g_start) % PAGE_SIZE) krnl_size++;
/* ora posso togliere la mappatura fisica del kernel */
	pt = (pagetable_t *)(PT_VIRT + (sizeof(pagetable_t) * (((unsigned long)load_adr/PAGE_SIZE)/TABLE_ENTRIES)));
	for(i=(unsigned long)load_adr/PAGE_SIZE;i<=((unsigned long)load_adr/PAGE_SIZE)+krnl_size;i++)
	 pt->pte[i] = 0;
}

/* mappa fisicamente <count> pagine all'indirizzo <physical>
 restituisce l'indirizzo virtuale, altrimenti 0 in caso di errori */
void * identity_mapping(void * physical,size_t count,unsigned int flags)
{
	unsigned int i;
	/* comincia a mappare: in caso di errore, smappa tutte quelle che avevi mappato prima */
	for(i=0;i<count;i++)
	{
		#ifdef DEBUG2
		kprint("allocation = 0x%x\n",
		 paging_alloc((void *)PAGE_ALIGN(physical+PAGE_SIZE*i),(void *)PAGE_ALIGN(physical+PAGE_SIZE*i),flags));
		#else
		if(!get_address((unsigned long)paging_alloc(physical+PAGE_SIZE*i,physical+PAGE_SIZE*i,flags)))
		{
			paging_free(physical,i);
			return (void *)0;
		}
		#endif
	}
	return physical;
}

/* mappa <count> pagine non necessariamente contigue su <virtual> come base */
void * paging_multi(void * virtual,void * fis_addrs[],size_t count,unsigned int flags)
{
	unsigned int i;
	/* comincia a mappare: in caso di errore, smappa tutte quelle che avevi mappato prima */
	for(i=0;i<count;i++)
	{
		if(!get_address((unsigned long)paging_alloc(fis_addrs[i],virtual+PAGE_SIZE*i,flags)))
		{
			paging_free(virtual,i);
			return (void *)0;
		}
	}
	return virtual;
}

/* mappa una pagina all'indirizzo fisico <physical> in una pagina all'indirizzo virtuale <virtual>
 entrambi DEVONO essere page aligned
 questa funzione prende come riferimento la pagedir corrente
 restituisce l'indirizzo virtuale mappato, altrimenti forma un codice di errore paging */
void * paging_alloc(void * physical,void * virtual,unsigned int flags)
{
	unsigned int pd_entry,pt_entry;
	pagetable_t * pt;
	pagedir_t * pd = (pagedir_t *)PD_VIRT;
	pagedir_t * kpd;
	#ifdef DEBUG2
	kprint("paging_alloc: controllo page alignment...\n");
	#endif
	if(((unsigned long)physical % PAGE_SIZE) || ((unsigned long)virtual % PAGE_SIZE))
	 return (void *)ERR_NOTPAGEALIGNED;
	pd_entry = ((unsigned long)virtual / PAGE_SIZE) / TABLE_ENTRIES;
	pt_entry = ((unsigned long)virtual / PAGE_SIZE) % TABLE_ENTRIES;
/* verifica esistenza pagetable */
	#ifdef DEBUG2
	kprint("INDIRIZZO VIRTUALE: 0x%x, INDIRIZZO FISICO: 0x%x\n",virtual,physical);
	kprint("paging_alloc: controllo presenza pagetable...\n");
	kprint("\tpd_entry = %d\n\tpd->pde[%d].table_address = 0x%x\n",
	 pd_entry,pd_entry,get_address(pd->pde[pd_entry]));
	#endif
	if(!is_present(pd->pde[pd_entry]))
	{
		/* la pagetable non esiste, creala */
		#ifdef DEBUG2
		kprint("paging_alloc: creazione pagetable...\n");
		#endif
		pt = fis_alloc(1);
		if(!pt) return (void *)ERR_NOPHYSMEM;	// niente memoria fisica :(
		/* in pt ora ho la pagina fisica che conterrà la nuova pagetable */
		/* intanto, setta la entry nella pagedir */
		#ifdef DEBUG2
		kprint("paging_alloc: allocazione pagetable...\n");
		kprint("\tpt = 0x%x\n",pt);
		#endif
		pd->pde[pd_entry] = make_pagedir(pt,PAGE_TABLE);
		/* aggiorna la pagedir del kernel se necessario */
		if(pd_entry >= (((unsigned long)g_start / PAGE_SIZE) / TABLE_ENTRIES))
		{// che tajooo!!!!
			kpd = get_kernel_pagedir();
			kpd->pde[pd_entry] = pd->pde[pd_entry];
		}
		/* mamma mia quanto so forte ;) */
		/* eh, mo la devo pulire... mo so cazzi :| */
		memset((void *)(PT_VIRT + (sizeof(pagetable_t) * pd_entry)),(char )0,sizeof(pagetable_t));
	}
	#ifdef DEBUG2
	kprint("paging_alloc: ricerca pagetable...\n");
	#endif
	/* indirizzo pagetable: PT_VIRT[pd_entry] */
	pt = (pagetable_t *)(PT_VIRT + (sizeof(pagetable_t) * pd_entry));
	#ifdef DEBUG2
	kprint("paging_alloc: pt = 0x%x\n",pt);
	kprint("\tpt->pte[%d].page_address = 0x%x\n",pt_entry,get_address(pt->pte[pt_entry]));
	#endif
	if(is_present(pt->pte[pt_entry])) return (void *)ERR_PAGEALREADY;	// l'indirizzo è già mappato
	#ifdef DEBUG2
	kprint("paging_alloc: allocazione...\n");
	#endif
	pt->pte[pt_entry] = make_pagetable(physical,flags);
	#ifdef DEBUG2
	kprint("paging_alloc: ritorno di 0x%x\n",virtual);
	kprint("\tpt->pte[%d].page_address = 0x%x\n",pt_entry,(pt->pte[pt_entry]));
	#endif
	return virtual;
}

/* setta non present e azzera il campo dell'indirizzo della pagina <virtual> per <count> pagine
 <virtual> DEVE essere page aligned
 come tutte le altre funzioni di questo file, prende come riferimento la pagedir corrente
 restituisce 0, altrimenti forma un indirizzo con <virtual> e con i flag di errore paging */
void * paging_free(void * virtual,size_t count)
{
	unsigned int pd_entry,pt_entry,i;
	pagetable_t * pt;
	pagedir_t * pd = (pagedir_t *)PD_VIRT;
	if((unsigned long)virtual % PAGE_SIZE) return (void *)ERR_NOTPAGEALIGNED;
	for(i=0;i<count;i++)
	{
		pd_entry = ((unsigned long)virtual / PAGE_SIZE) / TABLE_ENTRIES;
		pt_entry = ((unsigned long)virtual / PAGE_SIZE) % TABLE_ENTRIES;
		pt = (pagetable_t *)(PT_VIRT + (sizeof(pagetable_t) * pd_entry));
		#ifdef DEBUG2
		kprint("paging_free: controllo presenza pagetable...\n");
		#endif
		if(!is_present(pd->pde[pd_entry]))
		 return (void *)((unsigned long)virtual | ERR_PGTABLENOTPRES);	// page table non presente
		#ifdef DEBUG2
		kprint("paging_free: controllo presenza pagina...\n");
		#endif
		if(!is_present(pt->pte[pt_entry]))
		 return (void *)((unsigned long)virtual | ERR_PAGENOTPRES);	// la pagina non è stata allocata
		#ifdef DEBUG2
		kprint("paging_free: smappatura indirizzo 0x%x...\n",virtual);
		#endif
		pt->pte[pt_entry] = 0;
		virtual += PAGE_SIZE;	// pagina successiva
	}
	return (void *)0;
}

void * get_physical(void * virtual)
{
	unsigned int pd_entry,pt_entry;
	pagedir_t * pd = (pagedir_t *)PD_VIRT;
	pagetable_t * pt;
	if((unsigned long)virtual % PAGE_SIZE) return (void *)ERR_NOTPAGEALIGNED;
	pd_entry = ((unsigned long)virtual / PAGE_SIZE) / TABLE_ENTRIES;
	pt_entry = ((unsigned long)virtual / PAGE_SIZE) % TABLE_ENTRIES;
	if(!is_present(pd->pde[pd_entry])) return (void *)ERR_PGTABLENOTPRES;
	pt = (pagetable_t *)(PT_VIRT + (sizeof(pagetable_t) * pd_entry));
	if(!is_present(pt->pte[pt_entry])) return (void *)ERR_PAGENOTPRES;
	return (void *)(get_address(pt->pte[pt_entry]));
}

/* index == -1: tutta la page dir */
void print_pagedir(pagedir_t * pd,int index)
{
	int i = 0;
	if(index == -1)
	{
		/* stampa tutta la page dir */
		for(i=0;i<1024;i++)
			kprint("PAGEDIR[%d]: table_address = 0x%x\n",i,get_address(pd->pde[i]));
		return;
	}
	kprint("PAGEDIR[%d]: table_address = 0x%x\n",index,get_address(pd->pde[index]));
}

/* index == -1: tutta la page table */
void print_pagetable(pagetable_t * pt,int index)
{
	int i = 0;
	if(index == -1)
	{
		/* stampa tutta la page table */
		for(i=0;i<1024;i++)
			kprint("PAGETABLE[%d]: page_address = 0x%x\n",i,get_address(pt->pte[i]));
		return;
	}
	kprint("PAGETABLE[%d]: page_address = 0x%x, present = %d\n"
	 ,index,get_address(pt->pte[index]),is_present(pt->pte[index]));
}

/* restituisce la pagedir del kernel corrente
 FIXME: da rifare la pagedir in un posto più sicuro del primo mega */
pagedir_t * get_kernel_pagedir(void)
{
	return (pagedir_t *)(KERNEL_PAGEDIR_TMP);
}
