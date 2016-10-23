/* mem_area.h -- Memory Area management definitions */
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

#ifndef __MEM_AREA_H
#define __MEM_AREA_H

#include <mm/virmem.h>
#include <mm/paging.h>
#include <mm/mem_desc.h>

/* Memory Area -- tutte le informazioni sulla memoria di un task o del kernel sono memorizzate qui dentro */
typedef struct
{
	vm_bitmap page_bitmap;		// bitmap della memoria virtuale
	pagedir_t * pagedir;		// indirizzo FISICO della pagedir di questa memory area
	pagedir_t * paging_data;	// indirizzo VIRTUALE della pagedir di questa memory area
	alloc_page * first_alloc;	// indirizzo virtuale prima pagina di allocazioni
} memory_area;

memory_area * init_memory_area(void);
int free_memory_area(memory_area * mem_area);
void * mem_alloc(memory_area * mem_area,size_t pages,unsigned int flags);
void * mem_free(memory_area * mem_area,void * address);
memory_area * get_current_memory_area(void);
void switch_memory_area(memory_area * mem_area);

#endif
