/* sys.h -- Application Program Interface/System Calls definitions */
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

#ifndef __SYS_H
#define __SYS_H

#define NR_SYSCALLS	2
#define INT_SYSCALLS	0x90	// PER CARITÀ!!! NON TOCCHIAMO L'INT 0x80 DI LINUX ;)

#ifndef ASM

#include <kernel.h>
#include <ktypes.h>

#define	SYSCALL		asmlinkage

int init_syscalls(void);
void system_call(void);	// the system calls handler

/* The System Calls */
SYSCALL void sys_test(u32 eax,u32 ebx,u32 ecx,u32 edx,u32 esi,u32 edi);
SYSCALL void sys_print(u32 eax,u32 ebx,u32 ecx,u32 edx,u32 esi,u32 edi);

#endif

#endif
