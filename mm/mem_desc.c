/* mem_desc.c -- Memory Allocation Descriptors management */
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

/** Livello 4: gestione descrittori di allocazione **
Questo livello tiene conto dei blocchi di allocazione con i relativi descrittori.
Questo livello dipende dal livello 1 (allocazione fisica) e 3 (allocazione virtuale -- alto livello).
Questo livello è necessario per il livello 5 (gestione memory area).
**/

#include <mm/mem_desc.h>
#include <mm/fismem.h>
#include <mm/virmem.h>
#include <string.h>
#include <kernel.h>

alloc_page * find_last_desc_page(alloc_page * first_page);
alloc_page * find_first_free_desc_page(alloc_page * first_page);

/* prepara <count> pagine di descrittori di allocazione
 bitmap: bitmap dove allocarsi le pagine
 questa funzione usa la pagedir corrente per le allocazioni
 restituisce il numero di pagine allocate e mette in <addr> l'indirizzo della prima pagina */
int init_desc(vm_bitmap bitmap,int count,alloc_page ** addr)
{
	unsigned int i;
	alloc_page *vir,*old;
/* alloca la prima pagina */
	#ifdef DEBUG2
	kprint("init_desc: bitmap = 0x%x\n",&bitmap[0]);
	kprint("init_desc: allocazione prima pagina...\n");
	#endif
	if(!(vir = (alloc_page *)vir_alloc(bitmap,1,PAGE_SYSTEM))) return 0;
	#ifdef DEBUG2
	kprint("vir = 0x%x (0x%x)\n",vir,get_physical(vir));
	#endif
	memset((void *)vir,(char)0,sizeof(alloc_page));
	vir->free = ALLOC_NUM;
	*addr = vir;
	if(count == 1) return 1;
/* alloca le altre */
	#ifdef DEBUG2
	kprint("init_desc: allocazione altre pagine...\n");
	#endif
	for(i=1;i<count;i++)
	{
		#ifdef DEBUG2
		kprint("\tallocazione pagina %d...\n",i);
		#endif
		old = vir;
		if(!(vir = (void *)vir_alloc(bitmap,1,PAGE_SYSTEM))) return i;
		old->next = (void *)vir;
		/* abbiamo la pagina, prepariamola con i vari dati dentro */
		memset((void *)vir,(char)0,sizeof(alloc_page));
		vir->free = ALLOC_NUM;
	}
	#ifdef DEBUG2
	kprint("init_desc: ritorno di %d\n",i+1);
	#endif
	return i+1;
}

/* libera la memoria occupata dai descrittori di allocazione
 addr: indirizzo prima pagina di allocazioni */
int free_desc(vm_bitmap bitmap,alloc_page * addr)
{
	alloc_page * dpage, * next;
	dpage = addr;
	do
	{
		next = (alloc_page *)dpage->next;
		vir_free(bitmap,dpage,sizeof(alloc_page)/PAGE_SIZE);
		dpage = next;
	} while(dpage);
	return 0;	// speriamo funzioni...
}

/* setta il primo blocco di allocazione libero con i dati forniti
 restituisce 0 in caso di successo, altrimenti -1 */
int new_allocation(vm_bitmap bitmap,alloc_page * first_page,void * address,size_t size)
{
	alloc_page * page = first_page;
	unsigned int i;
	/* ok... calma e sangue freddo... dobbiamo sistemare i descrittori per allocare quell'indirizzo */
	/* cerca un blocco libero nelle pagine */
	#ifdef DEBUG2
	kprint("PRIMA: page = 0x%x, page->free = %d, page->next = 0x%x\n",page,page->free,page->next);
	#endif
	if(!page->free)	// peccato: prima pagina, niente :(
	 page = find_first_free_desc_page(first_page);
	#ifdef DEBUG2
	kprint("DOPO: page = 0x%x, page->free = %d, page->next = 0x%x\n",page,page->free,page->next);
	#endif
	if(!page->free)	// ammazza aho, niente descrittori... vabbe allochiamo una pagina nuova ;)
	{
		if((page->next = (void *)vir_alloc(bitmap,1,PAGE_SYSTEM)) <= 0) return -1;	// errore :(
		page = (alloc_page *)page->next;
		memset((void *)page,(char)0,sizeof(alloc_page));
		page->free = ALLOC_NUM;
	}
	#ifdef DEBUG2
	kprint("NUOVA: page = 0x%x, page->free = %d, page->next = 0x%x\n",page,page->free,page->next);
	#endif
	/* ora cerca il primo descrittore libero */
	for(i=0;i<ALLOC_NUM && page->allocs[i].size;i++);
	/* ok, mettici la robbbbbba dentro ;) */
	#ifdef DEBUG2
	kprint("new_allocation: descrittore trovato: i = %d\n",i);
	#endif
	page->allocs[i].address = address;
	page->allocs[i].size = size;
	page->free--;
	#ifdef DEBUG2
	kprint("FINE: page = 0x%x, page->free = %d, page->next = 0x%x\n",page,page->free,page->next);
	#endif
	return 0;
}

/* cerca l'allocazione con indirizzo <address> e dimensione <size> e lo libera
 restituisce il numero di pagine da liberare virtualmente in caso di successo, altrimenti -1 */
int free_allocation(alloc_page * first_page,void * address/*,size_t size*/)
{
	unsigned int i,p;
	/* allora, dobbiamo cercare address, quindi procediamo con ordine... */
	/* cicla tutte le pagine per trovare il descrittore da liberare */
	#ifdef DEBUG2
	kprint("free_allocation: controllo su 0x%x, indirizzo da cercare 0x%x\n",first_page,address);
	#endif
	do
	{
		#ifdef DEBUG2
		kprint("free_allocation: first_page->next = 0x%x\n",first_page->next);
		#endif
		if(first_page->free < ALLOC_NUM)	// ok, qualche descrittore ci sta
		{
			#ifdef DEBUG2
			kprint("free_allocation: trovata pagine con qualcosa (free = %d)...\n",first_page->free);
			#endif
			for(i=0;i<ALLOC_NUM;i++)	// cerchiamoci il nostro descrittore
			{
				if(first_page->allocs[i].address == address)/* && first_page->allocs[i].size == size) */
				{
					#ifdef DEBUG2
					kprint("free_allocation: descrittore trovato, i = %d\n",i);
					#endif
					p = first_page->allocs[i].size;
					first_page->allocs[i].address = 0;
					first_page->allocs[i].size = 0;
			 		//memset((void *)&first_page->allocs[i],(char)0,sizeof(alloc_desc));
					first_page->free++;
					return p;
				}
			}
		}
		if(first_page->next) first_page = (alloc_page *)first_page->next;
	} while(first_page->next);
	#ifdef DEBUG2
	kprint("free_allocation: descrittore non trovato.\n");
	#endif
	return -1;
}

/* cerca l'ultima pagina della catena di pagine di descrittori
 restituisce l'indirizzo della pagina trovata */
alloc_page * find_last_desc_page(alloc_page * first_page)
{
	while(first_page->next) first_page = (alloc_page *)first_page->next;
	return first_page;
}

/* cerca la prima pagina con descrittori liberi e ne restituisce l'indirizzo
 restituisce l'indirizzo dell'ultima pagina se non ci sono descrittori liberi */
alloc_page * find_first_free_desc_page(alloc_page * first_page)
{
	while(first_page->next)
	{
		if(first_page->free > 0) break;
		first_page = (alloc_page *)first_page->next;
	}
	return first_page;
}
