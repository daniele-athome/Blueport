/* system.h -- Assembler System Functions */
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

#ifndef __SYSTEM_H
#define __SYSTEM_H

#include <kernel.h>

/* Funzioni Assembler varie (flag, switch, registri, ecc.) */

#define save_flags(x) \
    __asm__ __volatile__("pushfl ; popl %0":"=r" (x): /* no input */ :"memory")

#define restore_flags(x) \
    __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"r" (x):"memory")

/* interrupt flag */
#define sti() __asm__ __volatile__ ("sti": : :"memory")
#define cli() __asm__ __volatile__ ("cli": : :"memory")

/* alias cretini :D */
#define nop() __asm__ __volatile__ ("nop")
#define ret() __asm__ ("ret")
#define iret() __asm__ __volatile__ ("iret": : :"memory")

/* gestione bit TS degli EFLAGS */
#define clts() __asm__ __volatile__ ("clts")
#define stts() \
__asm__ __volatile__ ( \
	"movl %%cr0,%%eax\n\t" \
	"orl $8,%%eax\n\t" \
	"movl %%eax,%%cr0" \
	: /* no outputs */ \
	: /* no inputs */ \
	:"ax")

#define big_jump(n) __asm__ __volatile__ ("ljmp %0,$0f\n" \
 "nop\n" \
 "nop\n" \
 "0:"::"g"(n))

/* eccolaaa!!! move_to_user_mode()
 questa bellissima nonché semplicissima funzione mi sta dando del filo da torcere
 questa funzione viene usata dal kernel solo una volta per passare al boot da kernel-mode a user-mode
 è proprio rozza sta funzione :D */
#define move_to_user_mode() \
__asm__ __volatile__ ("movl %%esp,%%eax\n\t" \
    "pushl %0\n\t" \
    "pushl %%eax\n\t" \
    "pushfl\n\t" \
    "pushl %1\n\t" \
    "pushl $1f\n\t" \
    "iret\n" \
    "1:\tmovl %0,%%eax\n\t" \
    "movw %%ax,%%ds\n\t" \
    "movw %%ax,%%es\n\t" \
    "movw %%ax,%%fs\n\t" \
    "movw %%ax,%%gs" \
    : /* no outputs */ :"i" (USER_DS_SEL), "i" (USER_CS_SEL):"%eax")

typedef struct desc_struct
{
	unsigned long a, b;
} __attribute__ ((packed)) desc_table[256];

desc_table idt;
extern desc_table gdt;

struct desc_register
{
	unsigned short limit;
	unsigned long address;
} __attribute__ ((packed)) gdt_48,idt_48;

#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	 : \
	 : "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "o" (*((char *) (gate_addr))), \
	 "o" (*(4+(char *) (gate_addr))), \
	 "d" ((char *) (addr)),"a" (KERNEL_CS_SEL << 16))

#define set_intr_gate(n,addr) \
    _set_gate(&idt[n],14,0,addr)    // 14 (1110) = interrupt disabilitati

#define set_system_gate(n,addr) \
    _set_gate(&idt[n],15,3,addr)    // 15 (1111) = interrupt abilitati

#define set_trap_gate(n,addr) \
    _set_gate(&idt[n],15,0,addr)    // 15 (1111) = interrupt abilitati

#define set_call_gate(a,addr) \
	_set_gate(a,12,3,addr)

#define read_cr0() ({ \
	unsigned int __dummy; \
	__asm__( \
		"movl %%cr0,%0\n\t" \
		:"=r" (__dummy)); \
	__dummy; \
})
#define write_cr0(x) \
	__asm__("movl %0,%%cr0": :"r" (x));

#endif
