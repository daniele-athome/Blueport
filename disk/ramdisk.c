/* ramdisk.c -- RAMDisk internal driver functions */
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

#include <disk/ramdisk.h>
#include <ktypes.h>
#include <string.h>
#include <errno.h>
#include <kernel.h>
#include <mm/kmem.h>

/*
  gestione RAMDisk:
  lettura: ramdisk_read([ramdisk struct],[blocco di partenza, parte da 0],
		[numero di blocchi, parte da 1],[buffer dati]);
  scrittura: ramdisk_write([ramdisk struct],[blocco di partenza, parte da 0],
		[numero di blocchi, parte da 1],[dati da scrivere]);
*/

/* crea una ramdisk
 se area == 0 allora la ramdisk verrà allocata dinamicamente nella memoria del kernel */
int create_ramdisk(ramdisk_t * rd,u32 block_size,u32 block_num,void * area)
{
	rd->area = area;
	if(!area)
	/* per le applicazioni si consiglia di non usare questo metodo, ma di allocare prima */
	 if(!(rd->area = kmalloc(block_size * block_num))) return -1;
	rd->block_size = block_size;
	rd->block_num = block_num;
	return 0;
}

/* legge blocchi da una ramdisk
 restituisce il numero di blocchi letti, altrimenti 0 */
int ramdisk_read(ramdisk_t * rd,u32 block_start,u32 block_num, void * buffer)
{
	void * data_start;
	u32 data_len;
	#ifdef DEBUG2
	kprint("ramdisk: rd = 0x%x\n",rd);
	kprint("ramdisk: buffer = 0x%x\n",buffer);
	kprint("ramdisk: starting... (rd->block_size = %d)\n",rd->block_size);
	#endif
	if(block_start > rd->block_num) return 0;
	#ifdef DEBUG2
	kprint("ramdisk: verifying 1... (rd->block_size = %d)\n",rd->block_size);
	#endif
	data_start = rd->area + (block_start * rd->block_size);
	#ifdef DEBUG2
	kprint("ramdisk: verifying 2... (rd->block_size = %d)\n",rd->block_size);
	#endif
	data_len = block_num * rd->block_size;
	#ifdef DEBUG2
	kprint("ramdisk: copying... (rd->block_size = %d)\n",rd->block_size);
	#endif
	memcpy(buffer,data_start,data_len);
	#ifdef DEBUG2
	kprint("ramdisk: after copy: rd->block_size = %d\n",rd->block_size);
	kprint("returning... (rd->block_size = %d)\n",rd->block_size);
	#endif
	return data_len / rd->block_size;
}

/* scrive blocchi su una ramdisk
 restituisce il numero di blocchi scritti, altrimenti 0 */
int ramdisk_write(ramdisk_t * rd,u32 block_start,u32 block_num, void * data)
{
	void * data_start;
	if(block_start > rd->block_num) return 0;
	data_start = rd->area + (block_start * rd->block_size);
	u32 data_len = block_num * rd->block_size;
	memcpy(data_start,data,data_len);
	return data_len / rd->block_size;
}
