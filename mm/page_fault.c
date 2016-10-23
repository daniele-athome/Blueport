/* page_fault.c -- Page fault management */
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

#include <mm/page_fault.h>
#include <asm/system.h>
#include <panic.h>
#include <mm/paging.h>

asmlinkage int page_fault(unsigned long error_code);

int init_pagefault(void)
{
	set_trap_gate(14, &page_fault);
	return 0;
}

/* typedef-compatible page fault handler */
asmlinkage int page_fault(unsigned long error_code)
{
	unsigned long fault_adr;
	unsigned int pd_entry;
	pagedir_t * kpd,* pd;
/*
	POSSIBILI CASI DI PAGE FAULT:
	1. pagina utente non presente (cr2 < 0xc0000000): termina processo || demand paging
	2. pagina kernel non presente (cr2 >= 0xc0000000): controlla allocazione e se è presente mappala, sennò kill :)
	3. lettura/scrittura non permessa: termina processo
*/
	fault_adr = get_page_fault_address();
	if(!(error_code & PF_NOTPRESENT))	// pagina non presente
	{
		if((fault_adr >= (unsigned long)g_start) && (fault_adr < PT_VIRT))	// kernel address space
		{
			/* controlla se è da allocare sennò killa processo */
			pd_entry = (fault_adr / PAGE_SIZE) / TABLE_ENTRIES;
			kpd = get_kernel_pagedir();
			if(!is_present(kpd->pde[pd_entry]))
			{
				/* TODO: pagina non da mappare -- killa il processo */
				panic("page fault at kernel address space (0x%x, 0x%x)\n",fault_adr,error_code);
			}
			else
			{	/* aggiorna pagedir */
				pd = (pagedir_t *)PD_VIRT;
				pd->pde[pd_entry] = kpd->pde[pd_entry];
			}
		}
		else if(fault_adr >= PT_VIRT)	// accesso ai dati di paginazione non permesso -- killa il processo
		{
			/* TODO: killa il processo */
			panic("page fault at paging data (0x%x, 0x%x)\n",fault_adr,error_code);
		}
		else	// user address space
		{
			/* TODO: demand paging */
			panic("page fault at user address space (0x%x, 0x%x)\n",fault_adr,error_code);
		}
	}
	else	// errore di protezione di pagina -- killa processo (per ora)
	{
		panic("page protection violation at 0x%x (0x%x)\n",fault_adr,error_code);
	}
	return 0;
}
