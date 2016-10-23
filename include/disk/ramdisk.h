/* ramdisk.h -- RAMDisk internal driver definitions */
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

#ifndef __RAMDISK_H
#define __RAMDISK_H

#include <ktypes.h>

typedef struct {
    u32 block_size;	/* dimensione di un blocco */
    u32 block_num;	/* numero di blocchi */
    void * area;	/* area di memoria per la ramdisk */
} ramdisk_t;

int create_ramdisk(ramdisk_t * rd,u32 block_size,u32 block_num,void * area);
int ramdisk_read(ramdisk_t * rd,u32 block_start,u32 block_num, void * buffer);
int ramdisk_write(ramdisk_t * rd,u32 block_start,u32 block_num, void * data);

#endif
