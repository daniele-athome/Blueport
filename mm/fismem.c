/* fismem.o -- allocazione fisica unaria */
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

/** Livello 1: allocazione fisica **
Questo livello si occupa di allocare singole pagine fisiche in una bitmap.
Questo livello non dipende da nessun altro livello.
Questo livello è necessario per il livello 2 (paginazione) e 3 (allocazione virtuale).
**/

#include <asm/bitops.h>
#include <asm/system.h>
#include <mm/fismem.h>
#include <mm/paging.h>
#include <mm/mem.h>
#include <multiboot.h>
#include <kernel.h>
#include <string.h>

extern unsigned long mem_end;
extern unsigned long mmap_count;
extern memory_map mmap[MAX_MMAP];
unsigned long fis_pages = 0;		/* numero di pagine fisiche realmente presenti nella macchina */
unsigned long fis_freepages = 0;	/* numero di pagine fisiche libere */
dword fis_bitmap[PAGE_VECTOR_SIZE];	/* bitmap di tutte le pagine fisiche possibili con 32 bit */

void * fis_manual_alloc(void * address,size_t count);
void * fis_manual_free(void * address,size_t count);

int init_fismem(void)
{
	unsigned long i = 0;
	unsigned long krnl_size;
	unsigned long tmp;
/* setta il numero di pagine della memoria */
	fis_pages = mem_end / PAGE_SIZE;	/* mem_end sarà sempre divisibile per 4096 */
	if(mem_end % PAGE_SIZE) fis_pages++;	/* ma non si sa mai... */
	fis_freepages = fis_pages;
/* svuota tutta la memoria (virtualmente, si intende...) */
	memset((void *)fis_bitmap,(char )0,sizeof(dword)*PAGE_VECTOR_SIZE);
/* alloca fisicamente il primo mega e il kernel (bios e kernel NON DEVONO ESSERE TOCCATI) */
	#ifdef DEBUG
	kprint("mm: g_start = 0x%x, g_end = 0x%x\n",(unsigned long)g_start,(unsigned long)g_end);
	#endif
/* alloca il primo mega */
	for(i=0;i<((1024*1024)/PAGE_SIZE);i++) set_bit(i,(void *)fis_bitmap);
	fis_freepages -= (1024*1024)/PAGE_SIZE;
	krnl_size = (g_end - g_start) / PAGE_SIZE + 1;
/* alloca il kernel */
	for(i=((unsigned long)load_adr/PAGE_SIZE);i<=((unsigned long)load_adr/PAGE_SIZE)+(krnl_size);i++)
	 set_bit(i,(void *)fis_bitmap);
	fis_freepages -= krnl_size;
	#ifdef DEBUG
	kprint("mm: allocati %d KB di memoria del BIOS e del kernel.\n",1024+((krnl_size*PAGE_SIZE)/1024));
	#endif
/* alloca la memory map */
	for(i=0;i<mmap_count;i++)
	{
		if(mmap[i].type != MMAP_USABLE)
		{
			tmp = mmap[i].length_low / PAGE_SIZE;
			if(mmap[i].length_low % PAGE_SIZE) tmp++;
			fis_manual_alloc((void *)mmap[i].base_addr_low,tmp);
		}
	}
	#ifdef DEBUG
	kprint("mm: memoria fisica allocata come definito dalla memory map del bootloader.\n");
	#endif
	return 0;
}

/* alloca <count> pagine fisiche -CONTIGUE- */
void * fis_alloc(size_t count)
{
	unsigned int page,i;
	long flags;
	save_flags(flags); cli();
	if(count >= fis_pages)
	{/* troppa roba, mi dispiace ;) */
		restore_flags(flags);
		return (void *)0;
	}
	page = fis_find_cont_pages(count);
	if(page < count)
	{/* count pagine non trovate */
		restore_flags(flags);
		return (void *)0;
	}
	for(i=0;i<count;i++)
	{
		page = find_next_zero_bit((void *)fis_bitmap,fis_pages,page);
		/* non controllo la pagina tanto l'ho già fatto prima */
		set_bit(page,(void *)fis_bitmap);
	}
	fis_freepages -= count;
	restore_flags(flags);
	#ifdef DEBUG2
	kprint("### FISMEM: 0x%x\n",page * PAGE_SIZE);
	#endif
	return (void *)(page * PAGE_SIZE);
}

/* libera <count> pagine -CONTIGUE-
 se l'indirizzo non è page aligned, restituisce l'indirizzo e non libera le pagine altrimenti libere le pagine e
 restituisce 0 */
void * fis_free(void * address,size_t count)
{
	unsigned int page,i;
	if(count >= fis_pages) return address;
	if((unsigned long)address % PAGE_SIZE) return address;
	page = (unsigned long)address / PAGE_SIZE;
	for(i=0;i<count;i++) if(test_bit(page,(void *)fis_bitmap)) clear_bit(page+i,(void *)fis_bitmap);
	fis_freepages += count;
	#ifdef DEBUG2
	kprint("fis_free: fis_freepages = %d\n",fis_freepages);
	#endif
	return (void *)0;
}

/* cerca nella memoria fisica <count> pagine libere -NON CONTIGUE-
 restituisce il numero di pagine trovate, -1 in caso di errore */
int fis_find_pages(size_t count)
{
	unsigned int old = 0,i;
	for(i=0;i<count;i++)
	{
		old = find_next_zero_bit((void *)fis_bitmap,fis_pages,old+1);
		if((old <= 0) || (old >= fis_pages)) return i+1;
	}
	return i;
}

/* cerca nella memoria fisica <pages> pagine libere -CONTIGUE-
 restituisce il numero di pagine contigue trovate, -1 in caso di errore
 nota: non è stata usata la find_clear_bits() poiché richiedeva una piccola modifica */
int fis_find_cont_pages(size_t count)
{
        unsigned long next = 0;
        unsigned long pages = 0;
        unsigned char bit = 0;
        unsigned long res = 0;
        next = find_first_zero_bit((void *)fis_bitmap,fis_pages);
        while(pages < count)
        {
                bit = test_bit(next,(void *)fis_bitmap);
                if(bit == 0)
                {
                        if(pages == 0) res = next;
                        pages++;
                        next++;
                }
                if(bit == 1)
                {
                        pages = 0;
                        next = find_next_zero_bit((void *)fis_bitmap,fis_pages,next);
                }
        }
        return res;
}

/* FUNZIONI "SEGRETE" */

/* mappa manualmente <count> pagine contigue all'indirizzo <address>
 restituisce <address>, 0 in caso di errori o di fine memoria */
void * fis_manual_alloc(void * address,size_t count)
{
	unsigned int page = (unsigned long)address / PAGE_SIZE,i;
	if((page+count) >= fis_pages) return (void *)0;
	if(count > fis_freepages) return (void *)0;
	for(i=page;i<page+count;i++) set_bit(i,(void *)fis_bitmap);
	fis_freepages -= count;
	return address;
}

/* libera manualmente <count> pagine contigue all'indirizzo <address>
 restituisce 0, <address> in caso di errori */
void * fis_manual_free(void * address,size_t count)
{
	unsigned int page = (unsigned long)address / PAGE_SIZE,i;
	if((page+count) > fis_pages) return address;
	for(i=page;i<page+count;i++) clear_bit(i,(void *)fis_bitmap);
	fis_freepages += count;
	return (void *)0;
}
