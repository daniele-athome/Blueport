/* page_fault.h -- Page fault management definitions */
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

#ifndef __PAGE_FAULT_H
#define __PAGE_FAULT_H

#include <kernel.h>
#include <mm/paging.h>

#define	PF_NOTPRESENT	0x1	// 001 -- (0)accesso ad una pagina non presente/(1)violazione della protezione di pagina
#define PF_READONLY	0x2	// 010 -- violazione in scrittura
#define PF_PRIVILEGE	0x4	// 100 -- violazione in user mode

#define get_page_fault_address()	get_CR2()

typedef asmlinkage int (*pagefault_handler)(unsigned long);
int init_pagefault(void);
asmlinkage int page_fault(unsigned long error_code);	// default page fault handler

#endif	/* __PAGE_FAULT_H */
