/* bitops.h -- Operazioni sui bit */
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

#ifndef __BITOPS_H
#define __BITOPS_H

#include <stddef.h>

// set_bit - setta a 1 il bit nr in addr
static __inline__ void set_bit(int nr, volatile void * addr){
	__asm__ __volatile__( "btsl %1,%0"
		:"=m" (*(volatile long *) addr)
		:"Ir" (nr));
}

// clear_bit - setta a 0 l'nr bit in addr
static __inline__ void clear_bit(int nr, volatile void * addr){
	__asm__ __volatile__(
		"btrl %1,%0"
		:"=m" (*(volatile long *) addr)
		:"Ir" (nr));
}

// change_bit - inverte il bit nr in addr
static __inline__ void change_bit(int nr, volatile void * addr){
	__asm__ __volatile__(
		"btcl %1,%0"
		:"=m" (*(volatile long *) addr)
		:"Ir" (nr));
}

//contralla se un bit è settato oppure no. restiruisce il suo valore
static __inline__ int test_bit(size_t nr, const void * addr)
{
	return ((1UL << (nr & 31)) & (((const unsigned int *) addr)[nr >> 5])) != 0;
}

// test_and_set_bit - setta un bit e ritorna il suo vecchio valore  
static __inline__ int test_and_set_bit(int nr, volatile void * addr){
	int oldbit;

	__asm__ __volatile__( 
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*(volatile long *) addr)
		:"Ir" (nr) : "memory");
	return oldbit;
}

//cancella il bit e ritorna quello vecchio
static __inline__ int test_and_clear_bit(int nr, volatile void * addr){
	int oldbit;

	__asm__ __volatile__(
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*(volatile long *) addr)
		:"Ir" (nr) : "memory");
	return oldbit;
}

//inverte un bit e ritorna il suo vecchio valore
static __inline__ int test_and_change_bit(int nr, volatile void * addr){
	int oldbit;

	__asm__ __volatile__( 
		"btcl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*(volatile long *) addr)
		:"Ir" (nr) : "memory");
	return oldbit;
}

// find_first_zero_bit - trova il primo zero in una zora di memoria. Ritorna la posizione dall'inizio
static __inline__ int find_first_zero_bit(void * addr, unsigned size){
	int d0, d1, d2;
	int res;

	if (!size)
		return 0;
	__asm__ __volatile__(
		"movl $-1,%%eax\n\t"
		"xorl %%edx,%%edx\n\t"
		"repe; scasl\n\t"
		"je 1f\n\t"
		"xorl -4(%%edi),%%eax\n\t"
		"subl $4,%%edi\n\t"
		"bsfl %%eax,%%edx\n"
		"1:\tsubl %%ebx,%%edi\n\t"
		"shll $3,%%edi\n\t"
		"addl %%edi,%%edx"
		:"=d" (res), "=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"1" ((size + 31) >> 5), "2" (addr), "b" (addr));
	return res;
}

//  find_first_bit - trova il primo bit a 1 in una zona di memoria e restituisce la sua posizione dall'inizio 
static __inline__ int find_first_bit(void * addr, unsigned size){
	int d0, d1;
	int res;

	__asm__ __volatile__(
		"xorl %%eax,%%eax\n\t"
		"repe; scasl\n\t"
		"jz 1f\n\t"
		"leal -4(%%edi),%%edi\n\t"
		"bsfl (%%edi),%%eax\n"
		"1:\tsubl %%ebx,%%edi\n\t"
		"shll $3,%%edi\n\t"
		"addl %%edi,%%eax"
		:"=a" (res), "=&c" (d0), "=&D" (d1)
		:"1" ((size + 31) >> 5), "2" (addr), "b" (addr));
	return res;
}

// find_next_zero_bit - trova un bit a zero nella zona di memoria
//restituisce la posizione dall'inizio da inizia a cercare da offset in poi
//offset è il numero di bit dall'inizio NON I BYTE
static __inline__ int find_next_zero_bit (void * addr, int size, int offset){
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;

	if (bit) {
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (~(*p >> bit)));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}

	res = find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}

// find_next_bit - trova un bit a uno nella zona di memoria
//restituisce la posizione dall'inizio da inizia a cercare da offset in poi
//offset è il numero di bit dall'inizio NON I BYTE
static __inline__ int find_next_bit (void * addr, int size, int offset){
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;

	if (bit) {
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (*p >> bit));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	
	res = find_first_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}

/* vecchia funzione: find_count_bit; file: kmalloc.c, funzione: trovare più pagine continue libere */
/* restituisce il numero di partenza del bit nella bitmap, dopo aver controllato che ci siano <count>
 bit non settati di seguito, -1 se non sono stati trovati */
static __inline__ int find_clear_bits(void * bitmap,unsigned int size,unsigned int count)
{
	unsigned long next = 0;
	unsigned long pages = 0;
	unsigned char bit = 0;
	unsigned long res = 0;
	next = find_first_zero_bit(bitmap,size);
	while(pages < count)
	{
		bit = test_bit(next,bitmap);
		if(bit == 0)
		{
			if(pages == 0) res = next;
			pages++;
			next++;
		}
		if(bit == 1)
		{
			pages = 0;
			next = find_next_zero_bit(bitmap,size,next);
		}
	}
	if(pages == count) return res;
	return -1;
}

/* stessa cosa di prima, ma cerca i bit settati invece di quelli a zero */
static __inline__ int find_set_bits(void * bitmap,unsigned int size,unsigned int count)
{
	unsigned long next = 0;
	unsigned long pages = 0;
	unsigned char bit = 0;
	unsigned long res = 0;
	next = find_first_bit(bitmap,size);
	while(pages < count)
	{
		bit = test_bit(next,bitmap);
		if(bit == 1)
		{
			if(pages == 0) res = next;
			pages++;
			next++;
		}
		if(bit == 0)
		{
			pages = 0;
			next = find_next_bit(bitmap,size,next);
		}
	}
	if(pages == count) return res;
	return -1;
}

#endif
