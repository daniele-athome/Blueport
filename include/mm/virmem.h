/* virmem.h -- Virtual Memory allocation definitions */
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

#ifndef __VIRMEM_H
#define __VIRMEM_H

#include <stddef.h>
#include <mm/paging.h>

typedef dword vm_bitmap[PAGE_VECTOR_SIZE];

int init_vm(vm_bitmap bitmap,int kernel);

void * vir_alloc(vm_bitmap bitmap,size_t pages,unsigned int flags);
void * vir_free(vm_bitmap bitmap,void * address,size_t pages);

void * vir_get_free_pages(vm_bitmap bitmap,size_t count);

#endif
