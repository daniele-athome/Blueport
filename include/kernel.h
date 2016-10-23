/* kernel.h -- Kernel main definitions */
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

#ifndef __KERNEL_H
#define __KERNEL_H

/* Selector offsets */
#define KERNEL_CS	0x8
#define KERNEL_DS	0x10
#define USER_CS		0x18
#define USER_DS		0x20
#define KERNEL_TSS	0x28

/* Segment Selectors */
#define KERNEL_CS_SEL	(KERNEL_CS & 0xFFF8)
#define KERNEL_DS_SEL	(KERNEL_DS & 0xFFF8)
#define USER_CS_SEL	((USER_CS & 0xFFF8)|3)
#define USER_DS_SEL	((USER_DS & 0xFFF8)|3)
#define KERNEL_TSS_SEL	(KERNEL_TSS & 0xFFF8)

/* la lunghezza dello stack (16KB). */
#define STACK_SIZE                      0x4000

/* formato dei simboli C. HAVE_ASM_USCORE è definito da configure */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

#ifndef ASM
#include <multiboot.h>

/* Linker Script variables */
extern const char g_start[];	// virtual kernel start address
extern const char g_end[];	// virtual kernel end address
extern const char load_adr[];	// kernel load address
extern const char virt_adr[];	// kernel virtual address

#define asmlinkage	__attribute__((regparm(0)))

asmlinkage int start_kernel(unsigned long magic,multiboot_info * info);
int kernel_main(void);

/* kprint stampa sulla console corrente */
asmlinkage int kprint(char *format,...);
void print_klog(void);
void klogcat(char * str);

#endif

#endif
