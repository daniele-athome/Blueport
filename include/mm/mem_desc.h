/* mem_desc.h -- Memory Allocation Descriptors definitions */
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

#ifndef __MEM_DESC_H
#define __MEM_DESC_H

#include <mm/paging.h>
#include <mm/virmem.h>

/* sizeof corrente: 8 byte */
typedef struct
{
	void * address;	// indirizzo di base (page aligned)
	size_t size;	// dimensione in pagine
} __attribute__ ((packed)) alloc_desc;

#define	ALLOC_NUM	PAGE_SIZE/sizeof(alloc_desc)-1

/* sizeof corrente: 4096 byte (bozza...) */
typedef struct
{
	alloc_desc allocs[ALLOC_NUM];
	dword free;
	void * next;
} __attribute__ ((packed)) alloc_page;

int init_desc(vm_bitmap bitmap,int count,alloc_page ** addr);
int free_desc(vm_bitmap bitmap,alloc_page * addr);
int new_allocation(vm_bitmap bitmap,alloc_page * first_page,void * address,size_t size);
int free_allocation(alloc_page * first_page,void * address/*,size_t size*/);

#endif
