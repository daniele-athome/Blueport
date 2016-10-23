/* idt.c -- interrupt/traps management and IDT */
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

#include <idt.h>
#include <kernel.h>
#include <asm/system.h>
#include <panic.h>
#include <sys.h>

#define IDT_ELEMENTS (sizeof(idt)/sizeof(struct desc_struct))

/* struttura da caricare nell'IDTR */
struct {
	unsigned short limit __attribute__ ((packed));
	struct desc_struct *idt_base __attribute__ ((packed));
} loadidt = {
(IDT_ELEMENTS * sizeof(struct desc_struct) - 1), idt};

extern void _unhand_int(void);

/* IRQ functions */
extern void _hwint0(void);
extern void _hwint1(void);
extern void _hwint2(void);
extern void _hwint3(void);
extern void _hwint4(void);
extern void _hwint5(void);
extern void _hwint6(void);
extern void _hwint7(void);
extern void _hwint8(void);
extern void _hwint9(void);
extern void _hwint10(void);
extern void _hwint11(void);
extern void _hwint12(void);
extern void _hwint13(void);
extern void _hwint14(void);
extern void _hwint15(void);

void unhand_int(void)
{
	panic("Interrupt senza handler!\n");
}

int init_idt(void) {
	int i;
	kprint("idt: inizializzazione interrupt...\n");

	set_trap_gate(0, &_int0);
	set_trap_gate(1, &_int1);
	set_trap_gate(2, &_int2);
	set_system_gate(3, &_int3);
	set_system_gate(4, &_int4);
	set_system_gate(5, &_int5);
	set_trap_gate(6, &_int6);
	set_trap_gate(7, &_int7);
	set_trap_gate(8, &_int8);
	set_trap_gate(9, &_int9);
	set_trap_gate(10, &_int10);
	set_trap_gate(11, &_int11);
	set_trap_gate(12, &_int12);
	set_trap_gate(13, &_int13);
	set_trap_gate(14, &_int14);	// hihihihi...
	set_trap_gate(15, &_int15);
	set_trap_gate(16, &_int16);
	set_trap_gate(17, &_int17);

	for(i=18; i<0x68; i++) set_trap_gate(i, &_unhand_int);

	set_intr_gate(0x68, &_hwint0);
	set_intr_gate(0x69, &_hwint1);
	set_intr_gate(0x6A, &_hwint2);
	set_intr_gate(0x6B, &_hwint3);
	set_intr_gate(0x6C, &_hwint4);
	set_intr_gate(0x6D, &_hwint5);
	set_intr_gate(0x6E, &_hwint6);
	set_intr_gate(0x6F, &_hwint7);
	set_intr_gate(0x70, &_hwint8);
	set_intr_gate(0x71, &_hwint9);
	set_intr_gate(0x72, &_hwint10);
	set_intr_gate(0x73, &_hwint11);
	set_intr_gate(0x74, &_hwint12);
	set_intr_gate(0x75, &_hwint13);
	set_intr_gate(0x76, &_hwint14);
	set_intr_gate(0x77, &_hwint15);

	for(i=0x78; i<IDT_ELEMENTS; i++) set_trap_gate(i, &_unhand_int);

	asm("lidt (%0)                 \n"	/* Load the IDT                */
	"pushfl                    \n"	/* Clear the NT flag           */
	"andl $0xffffbfff,(%%esp)  \n"
	"popfl                     \n"::"r"((char *) &loadidt));

	return 0;
}

void _int0()
{
	panic("codice 0: divisione per zero\n");
}

void _int1()
{
	panic("codice 1: eccezione di debug\n");
}

void _int2()
{
	panic("codice 2: interrupt non mascherabile\n");
}

void _int3()
{
	panic("codice 3: break point\n");
}

void _int4()
{
	panic("codice 4: overflow\n");
}

void _int5()
{
	panic("codice 5: indice non compreso nell'intervallo\n");
}

void _int6()
{
	panic("codice 6: codice operativo non valido\n");
}

void _int7()
{
	panic("codice 7: coprocessore non avviabile\n");
}

void _int8()
{
	panic("codice 8: double fault\n");
}

void _int9()
{
	panic("codice 9: overrun del coprocessore\n");
}

void _int10()
{
	panic("codice 10: stato del processo non valido\n");
}

void _int11()
{
	panic("codice 11: segmento inesistente\n");
}

void _int12()
{
	panic("codice 12: eccezione dello stack\n");
}

void _int13()
{
	panic("codice 13: protezione generale\n");
}

/* page fault handler semplice con panic */
void _int14()
{
	unsigned int addr;
	__asm__ volatile ("movl  %%cr2, %0":"=r" (addr));
	panic("codice 14: un accesso all'indirizzo 0x%x ha causato un page fault\n",addr);
}

void _int15()
{
	panic("codice 15: errore sconosciuto\n");
}

void _int16()
{
	panic("codice 16: errore del coprocessore\n");
}

void _int17()
{
	panic("codice 17: memoria non allineata\n");
}
