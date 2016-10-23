/* irq.c -- IRQ/PIC8259A management */
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

#include <irq.h>
#include <io.h>
#include <asm/system.h>
#include <kernel.h>

#define is_valid_irq(n) (n>=0 && n<16)

#define INT_CTL         0x20
#define INT_CTLMASK     0x21
#define INT2_CTL        0xA0
#define INT2_CTLMASK    0xA1
#define EOI    0x20
#define INT_VEC  0x68
#define INT2_VEC  0x70

static unsigned char cache_21 = 0xFB;   // IRQ 2 on Master is cascaded
static unsigned char cache_A1 = 0xFF;

/* IRQ handlers table */
volatile void *IRQ_TABLE[16];

void enable_irq(unsigned int irq_nr)
{
	unsigned char mask;
	if(!is_valid_irq(irq_nr)) return;
	mask = ~(1 << (irq_nr & 7));
	if (irq_nr <= 7)
	{
		cache_21 &= mask;
		outb(cache_21,INT_CTLMASK);
	}
	else
	{
		cache_A1 &= mask;
		outb(cache_A1,INT2_CTLMASK);
	}
}

void disable_irq(unsigned int irq_nr)
{
	unsigned char mask;
	if(!is_valid_irq(irq_nr)) return;
	mask = 1 << irq_nr;
	if (irq_nr <= 7)
	{
		cache_21 |= mask;
		outb(cache_21,INT_CTLMASK);
	}
	else
	{
		cache_A1 |= mask;
		outb(cache_A1,INT2_CTLMASK);
	}
}

void add_irq_handler(unsigned int irq_nr, void *handler)
{
	long flags;
	save_flags(flags); cli();
	if(is_valid_irq(irq_nr))
	{
		IRQ_TABLE[irq_nr] = handler;
		//enable_irq(irq_nr);	<-- attivare separatamente
	}
	restore_flags(flags);
}

void init_irq(void)
{

	kprint("irq: inizializzazione del gestore degli irq...");
	outportb(INT_CTL, 0x11);	/* Start 8259 initialization    */
	outportb(INT2_CTL, 0x11);

	outportb(INT_CTLMASK, INT_VEC);	/* Base interrupt vector        */
	outportb(INT2_CTLMASK, INT2_VEC);

	outportb(INT_CTLMASK, 1 << 2);	/* Bitmask for cascade on IRQ 2 */
	outportb(INT2_CTLMASK, 2);	/* Cascade on IRQ 2             */

	outportb(INT_CTLMASK, 0x01);	/* Finish 8259 initialization   */
	outportb(INT2_CTLMASK, 0x01);

	outportb(INT_CTLMASK, cache_21);	/* Mask all interrupts (tranne il 2 per il cascade) */
	outportb(INT2_CTLMASK, cache_A1);
	kprint("\n");
}
