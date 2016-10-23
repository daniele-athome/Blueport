/* virmem.c -- Virtual Memory allocation */
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

/** Livello 3: allocazione virtuale -- alto livello (bitmap + mappatura) **
Questo livello si occupa di allocare pagine virtuali allocandole con il basso livello e nella relativa bitmap.
Questo livello dipende dal livello 2 (allocazione virtuale -- basso livello).
Questo livello è necessario per il livello 4 (gestione descrittori di allocazione) e 5 (gestione memory area).
**/

#include <asm/system.h>
#include <mm/virmem.h>
#include <mm/fismem.h>
#include <mm/paging.h>
#include <kernel.h>
#include <asm/bitops.h>
#include <string.h>
#include <mm/mem.h>
#include <mm/mem_area.h>

extern memory_area kmem_area;

/* mappa nella bitmap <bitmap> lo spazio del primo mega e del kernel
 se kernel è diverso da 0, mappa lo spazio da 0 a 3 GB, altrimenti mappa il primo mega e il terzo gigabyte */
int init_vm(vm_bitmap bitmap,int kernel)
{
	unsigned int i;
/* pulisci la bitmap innanzitutto */
	memset((void *)bitmap,(char )0,sizeof(vm_bitmap));
	if(kernel)
	{
	/* mappa tutto lo spazio dei primi 3 GB + il kernel stesso */
		for(i=0;i<=((unsigned long)g_end/PAGE_SIZE)+1;i++) set_bit(i,(void *)bitmap);
	}
	else
	{
	/* mappa il primo mega */
		for(i=0;i<(0x100000/PAGE_SIZE);i++) set_bit(i,(void *)bitmap);
	/* mappa il kernel fino alla fine della memoria */
		for(i=((unsigned long)g_start/PAGE_SIZE);i<MAX_PAGES;i++) set_bit(i,(void *)bitmap);
	}
/* non mappiamo la memory map tanto quelli da occupare per fare danni sono indirizzi fisici ;) */
/* FIXME: E INVECE NO: gli indirizzi per l'hardware sono virtuali!!! */
	return 0;
}

/* alloca <pages> pagine virtuali
 bitmap: bitmap della memoria virtuale del kernel/task
 privilege: 0 - supervisor, 1 - user
 rw: 0 - read only, 1 - read/write */
void * vir_alloc(vm_bitmap bitmap,size_t pages,unsigned int flags)
{
	extern unsigned long fis_freepages;
	int avail,fisav,p,i;
	void * addr;
/* per prima cosa, vediamo se ci sono abbastanza pagine fisiche e virtuali */
	avail = find_clear_bits((void *)bitmap,MAX_PAGES,pages);
	if(((avail+pages) >= MAX_PAGES) || (avail <= 0)) return (void *)0;	// azz... finita la memoria virtuale :(
/* ok, a posto, abbiamo la memoria virtuale, ora vediamo come stiamo messi con la memoria fisica */
	fisav = fis_find_pages(pages);
	#ifdef DEBUG2
	kprint("fisav = %d, fis_freepages = %d, pages = %d\n",fisav,fis_freepages,pages);
	#endif
	if((fisav > fis_freepages) || (fisav <= 0) || (pages > fisav))
	 return (void *)0;	// azz... finita la memoria fisica :(
/* buono, abbiamo pure la memoria fisica, procediamo con l'allocazione e la mappatura */
	p = avail;
	#ifdef DEBUG2
	kprint("Arrivo con le pagine!!! (p*PAGE_SIZE = 0x%x)\n",p*PAGE_SIZE);
	#endif
	for(i=0;i<pages;i++)
	{
		addr = fis_alloc(1);	// nessun controllo indirizzo
		#ifdef DEBUG2
		kprint("### %da pagina, addr = 0x%x\n",i+1,addr);
		kprint("\tfisav = %d\n",fisav);
		kprint("PAPAPAPAPAPAPA: allocazione di 0x%x su 0x%x\n",addr,(avail+i)*PAGE_SIZE);
		#endif
		if(!get_address((unsigned long)paging_alloc(addr,(void *)((avail+i)*PAGE_SIZE),flags)))
		{/* TODO: liberare le pagine fisiche e virtuali occupate precedentemente */
			fis_free(addr,1);	// bello ;)
			return (void *)0;
		}
		set_bit(p,(void *)bitmap);
	/* in questa fase non controllo la contiguità delle pagine, poiché l'ho già controllata prima */
		p = find_next_zero_bit((void *)bitmap,MAX_PAGES,p);
		#ifdef DEBUG2
		kprint("SUCCESSIVA: trovata pagina virtuale 0x%x\n",p);
		#endif
	}
	#ifdef DEBUG2
	kprint("vir_alloc: finito, troverai i dati a 0x%x\n",avail*PAGE_SIZE);
	#endif
/* aaah, abbiamo finito: ripristina i flag e restituisci l'indirizzo virtuale */
	return (void *)(avail * PAGE_SIZE);
}

/* libera <pages> pagine virtuali (se sono state allocate)
 se l'indirizzo non è page aligned, restituisce l'indirizzo e non libera le pagine, altrimenti libera le pagine e
 restituisce 0 */
void * vir_free(vm_bitmap bitmap,void * address,size_t pages)
{
	unsigned int page = 0,i;
	void * phys;
	if((unsigned long)address % PAGE_SIZE) return address;	// controllo page aligned
	page = (unsigned long)address / PAGE_SIZE;
	#ifdef DEBUG2
	kprint("pages = %d\n",pages);
	#endif
	for(i=0;i<pages;i++)
	{
		#ifdef DEBUG2
		kprint("vir_free: liberazione pagina %d...\n",i);
		#endif
		if(!test_bit(page+i,(void *)bitmap)) return address;	// controllo allocazione
		#ifdef DEBUG2
		kprint("get_physical (0x%x)...\n",address+(PAGE_SIZE+i));
		#endif
		if(!(phys = get_physical(address+(PAGE_SIZE*i)))) return address;
		#ifdef DEBUG2
		kprint("paging_free...\n");
		#endif
		if(paging_free(address+(PAGE_SIZE*i),1)) return address;	// smappatura
		clear_bit(page+i,(void *)bitmap);		// liberazione
		#ifdef DEBUG2
		kprint("\tFISICAAAA!!!!\n");
		#endif
		fis_free(phys,1);
	}
	return (void *)0;
}

/* cerca <count> pagine virtuali contigue nella bitmap <bitmap>
 restituisce l'indirizzo della prima pagina, 0 se non sono state trovate */
void * vir_get_free_pages(vm_bitmap bitmap,size_t count)
{
	int avail = find_clear_bits((void *)bitmap,MAX_PAGES,count);
	if(((avail+count) >= MAX_PAGES) || (avail <= 0)) return (void *)0;	// azz... finita la memoria virtuale :(
	return (void *)(avail*PAGE_SIZE);
}

/* FUNZIONI SEGRETE */

/* allocazione manuale nella bitmap del kernel */

/* allocazione virtuale manuale */
void * vir_manual_alloc(void * address,size_t count)
{
	unsigned int page = (unsigned long)address / PAGE_SIZE,i;
	if((page+count) >= MAX_PAGES) return (void *)0;
	for(i=page;i<page+count;i++) set_bit(i,(void *)kmem_area.page_bitmap);
	return address;
}

/* liberazione virtuale manuale */
void * vir_manual_free(void * address,size_t count)
{
	unsigned int page = (unsigned long)address / PAGE_SIZE,i;
	if((page+count) > MAX_PAGES) return address;
	for(i=page;i<page+count;i++) clear_bit(i,(void *)kmem_area.page_bitmap);
	return (void *)0;
}
