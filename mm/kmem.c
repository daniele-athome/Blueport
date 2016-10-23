/* kmem.c -- Kernel Memory functions */
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

/** Livello 6: allocazione di memoria -- livello kernel **
Questo livello alloca memoria bufferizzata e non -- per il kernel.
Questo livello dipende dal livello 5 (gestione memory area).
Questo livello è necessario per il kernel.
**/

#include <mm/kmem.h>
#include <mm/mem_area.h>
#include <asm/system.h>
#include <stddef.h>

memory_area kmem_area;

void * kmalloc(size_t size)
{
	size_t pages;
	pages = size / PAGE_SIZE;
	if(size % PAGE_SIZE) pages++;
	return mem_alloc(&kmem_area,pages,PAGE_SYSTEM);
}

void * kfree(void * address)
{
	return mem_free(&kmem_area,address);
}
