/* mem_area.c -- Memory Area management */
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

/** Livello 5: gestione memory area **
Questo livello gestisce le allocazioni di alto livello pre-utente e prepara le memory area per l'uso.
Questo livello dipende dal livello 6 (allocazione di memoria).
Questo livello è necessario per il livello 4 (gestione descrittori di allocazione).
**/

#include <mm/mem_area.h>
#include <mm/mem_desc.h>
#include <mm/virmem.h>
#include <mm/kmem.h>
#include <string.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <kernel.h>

memory_area * cmem_area;	// memory area corrente
extern memory_area kmem_area;

memory_area * init_memory_area(void)
{
	memory_area * mem_area,	// indirizzo memory area creata nella memoria corrente
		 * oldmem_area;	// indirizzo memory area corrente
	unsigned int pages;
	/* dimensione della memory area in pagine */
	pages = sizeof(memory_area) / PAGE_SIZE;
	if(sizeof(memory_area) % PAGE_SIZE) pages++;
/* alloca la nuova memory area */
	#ifdef DEBUG2
	kprint("1. Allocating memory area...\n");
	#endif
	if(!(mem_area = mem_alloc(&kmem_area,pages,PAGE_SYSTEM))) goto exit_failure;
/* prepara i dati di paginazione */
	#ifdef DEBUG2
	kprint("2. Setting paging data...\n");
	#endif
	if(!(mem_area->pagedir = init_paging_data(&mem_area->paging_data))) goto exit_failure1;
/* prepara la bitmap della memoria virtuale */
	#ifdef DEBUG2
	kprint("3. Setting virtual page bitmap...\n");
	#endif
	if(init_vm(mem_area->page_bitmap,0) < 0) goto exit_failure2;
/* CAMBIO DELLA GUARDIA */
	#ifdef DEBUG2
	kprint("4. Switching to the new memory area...\n");
	#endif
	oldmem_area = cmem_area;
	switch_memory_area(mem_area);	// dovremmo aver mappato tutto... speriamo bene...
/** SIAMO NELLA NUOVA MEMORY AREA **/
/* alloca i primi descrittori di allocazione */
	#ifdef DEBUG2
	kprint("5. Setting allocation descriptors...\n");
	#endif
	if(init_desc(mem_area->page_bitmap,1,&mem_area->first_alloc) <= 0) goto exit_failure3;	// porca zozza lurida!!
/* ritorna alla vecchia memory area */
	#ifdef DEBUG2
	kprint("6. Switching back to old memory area...\n");
	#endif
	switch_memory_area(oldmem_area);	// si torna a casa...
/** SIAMO TORNATI NELLA VECCHIA MEMORY AREA **/
	return mem_area;
exit_failure3:
	switch_memory_area(oldmem_area);
exit_failure2:
	free_paging_data(mem_area->pagedir);
exit_failure1:
	mem_free(cmem_area,mem_area);
exit_failure:
	return (memory_area *)0;
}

/* cancella una memory area dalla faccia della terra (o dovremmo dire della memoria :) */
int free_memory_area(memory_area * mem_area)
{
	alloc_page * dpage;
	unsigned int i;
/* eseguiamo le operazioni al contrario rispetto a init_memory_area */
/* 1. DESCRITTORI DI ALLOCAZIONE */
	/* liberazione di tutte le allocazioni fatte dall'applicazione */
	dpage = mem_area->first_alloc;
	do
	{
		if(dpage->free >= ALLOC_NUM) goto tocont;	// descrittori liberi in questa pagina
	/* libera ogni allocazione nella pagina di allocazioni corrente */
		for(i=0;i<ALLOC_NUM;i++) if(dpage->allocs[i].address) mem_free(mem_area,dpage->allocs[i].address);
tocont:
		dpage = (alloc_page *)dpage->next;
	}while(dpage);
	/* liberazione della memoria occupata dai descrittori di allocazione */
	free_desc(mem_area->page_bitmap,mem_area->first_alloc);
/* 2. DATI DI PAGINAZIONE */
	free_paging_data(mem_area->paging_data);
/* 3. LIBERAZIONE MEMORY AREA */
	mem_free(&kmem_area,mem_area);
	return 0;
}

/* alloca nella memory area corrente <pages> pagine di memoria
 restituisce l'indirizzo allocato, altrimenti 0 in caso di errore */
void * mem_alloc(memory_area * mem_area,size_t pages,unsigned int flags)
{
	void * addr;
//if((get_CR3() & 0xFFFFF000) != (unsigned long)mem_area->pagedir) return (void *)0; // la memory area non è in uso
/* PASSO 1: allocazione virtuale */
	if(!(addr = vir_alloc(mem_area->page_bitmap,pages,flags))) goto exit_failure1;
/* PASSO 2: settaggio dei descrittori di allocazione */
	if(new_allocation(mem_area->page_bitmap,mem_area->first_alloc,addr,pages)) goto exit_failure2;
/* ALLOCATO! */
	return addr;
exit_failure2:
	vir_free(mem_area->page_bitmap,addr,pages);
exit_failure1:
	return (void *)0;
}

/* libera nella memory area corrente l'indirizzo <address>
 restituisce 0, altrimenti <address> in caso di errore */
void * mem_free(memory_area * mem_area,void * address)
{
	int pages;
//if((get_CR3() & 0xFFFFF000) != (unsigned long)mem_area->pagedir) return (void *)0; // la memory area non è in uso
	if((pages = free_allocation(mem_area->first_alloc,address)) < 0) return address;
	if(!pages) return (void *)0;
	return vir_free(mem_area->page_bitmap,address,pages);
}

/* esegue il cambio di memory area */
void switch_memory_area(memory_area * mem_area)
{
	if(!mem_area) return;	// ammazza aho...
	if(!mem_area->pagedir) return; 	// ma porca di quella puttana...
	cmem_area = mem_area;
	set_CR3(mem_area->pagedir);	// cambio della guardia vero e proprio
	big_jump(KERNEL_CS_SEL);	// jumpone
}

memory_area * get_current_memory_area(void)
{
	return cmem_area;
}
