/* initfs.h -- INITFS definitions */
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

#ifndef __INITFS_H
#define __INITFS_H

#include <fs/vfs.h>
#include <kernel.h>

#define INITFS_NAME	"initfs"
#define INITFS_DESC	"Initial File System"

/* TAR Flags */
#define TMAGIC   "ustar  "	/* ustar and a null */
#define TMAGLEN  6+2
//#define TVERSION "00"		/* 00 and no null */
//#define TVERSLEN 2

/* Values used in typeflag field.  */
#define REGTYPE	 '0'		/* regular file */
#define AREGTYPE '\0'		/* regular file */
#define LNKTYPE  '1'		/* link */
#define SYMTYPE  '2'		/* reserved */
#define CHRTYPE  '3'		/* character special */
#define BLKTYPE  '4'		/* block special */
#define DIRTYPE  '5'		/* directory */
#define FIFOTYPE '6'		/* FIFO special */
#define CONTTYPE '7'		/* reserved */

#define MAX_TARNAME_LEN		99

/* blocco di una entry */
typedef struct	/* TAR POSIX Header */
{				/* byte offset */
	char name[MAX_TARNAME_LEN+1];	/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[MAX_TARNAME_LEN+1];	/* 157 */
	char magic[6+2];	/* 257 */
	//char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[154+1];	/* 345 */
	char padding[12];	/* 500 */
				/* 512 -- end of block */
}__attribute__ ((packed)) initfs_block;

#define	N_ENTRIES	39

typedef struct
{
	char name[MAX_TARNAME_LEN+1];	// percorso e nome del file
	unsigned long block;		// blocco di partenza del file
} initfs_cache[N_ENTRIES];	// in modo da entrare in meno di 4096 byte :/

filesystem * register_initfs(void);

#endif
