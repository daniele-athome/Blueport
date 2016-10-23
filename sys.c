/* sys.c -- Application Program Interface/System Calls management */
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

#include <sys.h>
#include <kernel.h>
#include <asm/system.h>
#include <ktypes.h>

void *sys_call_table[] = { sys_test, sys_print };

int init_syscalls(void)
{
	// handle system call using interrupt INT_SYSCALLS
	set_system_gate(INT_SYSCALLS, &system_call);
	kprint("sys: interfaccia per le applicazioni pronta all'interrupt 0x%x.\n",INT_SYSCALLS);
	return 0;
}

SYSCALL void sys_test(u32 eax,u32 ebx,u32 ecx,u32 edx,u32 esi,u32 edi)
{
	kprint("sys: eax = 0x%x, ebx = 0x%x, ecx = 0x%x, edx = 0x%x, esi = 0x%x, edi = 0x%x\n",eax,ebx,ecx,edx,esi,edi);
}

/* stampa una stringa raw a schermo usando kprint (per ora)
 eax: numero syscall
 ebx: indirizzo stringa */
SYSCALL void sys_print(u32 eax,u32 ebx,u32 ecx,u32 edx,u32 esi,u32 edi)
{
	kprint((char *)ebx);
}
