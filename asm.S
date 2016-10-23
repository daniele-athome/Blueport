/* asm.S -- base assembler functions */
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

#ifndef ASM
#define ASM

#include <asm/linkage.h>
#include <sys.h>
#include <kernel.h>

.extern unhand_int
.extern IRQ_TABLE

/*
 * Stack layout in ret_from_syscall:
 *
 *   0(%esp) - %ebx
 *   4(%esp) - %ecx
 *   8(%esp) - %edx
 *   C(%esp) - %esi
 *  10(%esp) - %edi
 *  14(%esp) - %ebp
 *  18(%esp) - %eax
 *  1C(%esp) - %ds
 *  20(%esp) - %es
 *  24(%esp) - %eip
 *  28(%esp) - %cs
 *  2C(%esp) - %eflags
 *  30(%esp) - %oldesp
 *  34(%esp) - %oldss
 */

/* registri pushati da noi */
EBX		= 0x00
ECX		= 0x04
EDX		= 0x08
ESI		= 0x0C
EDI		= 0x10
EBP		= 0x14
EAX		= 0x18
DS		= 0x1C
ES		= 0x20
/* registri pushati dal processore */
EIP		= 0x24
CS		= 0x28
EFLAGS		= 0x2C
OLDESP		= 0x30
OLDSS		= 0x34

/*
 * Save all registers
 */
#define SAVE_ALL	\
	cld;          \
	pushl	%es;  \
	pushl	%ds;  \
	pushl	%eax; \
	pushl	%ebp; \
	pushl	%edi; \
	pushl	%esi; \
	pushl	%edx; \
	pushl	%ecx; \
	pushl	%ebx; \

	pushl	%edx	/* salva edx */
	movl	$KERNEL_DS_SEL,%edx; \
	movl	%edx,%ds; \
	movl	%edx,%es; \
	movl	$USER_DS_SEL,%edx; \
	movl	%edx, %fs;
	popl	%edx	/* ripristina edx */

#define RESTORE_ALL \
    popl %ebx;  \
    popl %ecx;  \
    popl %edx;  \
    popl %esi;  \
    popl %edi;  \
    popl %ebp;  \
    popl %eax;  \
    popl %ds;   \
    popl %es;   \
    iret;

.extern sys_call_table
ENTRY(system_call)  /* int 0x90: EAX: call_num, parameters in EBX/ECX/EDX */
	SAVE_ALL
	STI     /* interrupts enabled during a system call */

	cmpl	$NR_SYSCALLS, %eax    /* exist? */
	jae	ret_from_syscall
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	*sys_call_table(,%eax,4)
	addl	$24,%esp
	movl	%eax, EAX(%esp)		/* get from the stack the return code */

/* funzione usata per ritornare da una system call o da un interrupt */
ENTRY(ret_from_syscall)
	CLI			/* NO INTERRUPT! */
	/* robba di schedulazione ecc. ecc. */
	RESTORE_ALL
	

ENTRY(_unhand_int)
	SAVE_ALL
	call	unhand_int  /* il codice C provvede... */
	jmp	ret_from_syscall


INT_CTL		= 0x20
INT_CTLMASK	= 0x21
INT2_CTL	= 0xA0
INT2_CTLMASK	= 0xA1
ENABLE		= 0x20

#define HWINT_MASTER(irq) \
    ENTRY(_hwint##irq)      \
    SAVE_ALL;           \
    inb $INT_CTLMASK, %al;  \
    orb $1<<irq, %al;       \
    outb    %al, $INT_CTLMASK;  \
    movb    $ENABLE, %al;       \
    outb    %al, $INT_CTL;      \
    STI;                \
    call    *(IRQ_TABLE+4*irq); \
    CLI;                \
    inb $INT_CTLMASK, %al;  \
    andb    $~(1<<irq), %al;    \
    outb    %al, $INT_CTLMASK;  \
    jmp ret_from_syscall;          
                                   /* Get the address of the ret_from_intr
									* routine from the stack.
									*/

#define HWINT_SLAVE(irq) \
    ENTRY(_hwint##irq)      \
    SAVE_ALL;           \
    inb $INT2_CTLMASK, %al; \
    orb $1<<(irq-8), %al;   \
    outb    %al, $INT2_CTLMASK; \
    movb    $ENABLE, %al;       \
    outb    %al, $INT_CTL;      \
    jmp .+2;            \
    outb    %al, $INT2_CTL;     \
    STI;                \
    call    *(IRQ_TABLE+4*irq); \
    CLI;                \
    inb $INT2_CTLMASK, %al; \
    andb    $~(1<<(irq-8)), %al;    \
    outb    %al, $INT2_CTLMASK; \
    jmp ret_from_syscall;

ENTRY(_hwint0)      /* timer handler: do not enable interrupt */
    SAVE_ALL
    call    *(IRQ_TABLE)    /* element 0 */
    movb    $ENABLE, %al
    outb    %al, $INT_CTL
    jmp ret_from_syscall;

HWINT_MASTER(1)
HWINT_MASTER(2)
HWINT_MASTER(3)
HWINT_MASTER(4)
HWINT_MASTER(5)
HWINT_MASTER(6)
HWINT_MASTER(7)

HWINT_SLAVE(8)
HWINT_SLAVE(9)
HWINT_SLAVE(10)
HWINT_SLAVE(11)
HWINT_SLAVE(12)
HWINT_SLAVE(13)
HWINT_SLAVE(14)
HWINT_SLAVE(15)

#endif
