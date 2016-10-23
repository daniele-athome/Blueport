/* dev.h -- Device special files management driver definitions */
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

#ifndef __DEV_H
#define __DEV_H

#include <ktypes.h>
#include <stddef.h>
#include <disk/ramdisk.h>
#include <fs/vfs.h>

#define MAX_DEVICES		100

#define DEV_LOCK_FREE	0
#define DEV_LOCK_READ	1
#define DEV_LOCK_RW	2

#define DEV_SUCCESS	0
#define DEV_ERROR	-1	/* errore */
#define DEV_FULL	-2	/* spazio per i device esaurito */
#define DEV_EXIST	-3	/* il device esiste già */
#define DEV_NOTFOUND	-4	/* device non trovato */
#define DEV_NOTCONFIG	-5	/* handler non installato */
#define DEV_READONLY	-6	/* device in sola lettura */
#define DEV_WRITEONLY	-7	/* device in sola scrittura */
#define DEV_INVARG	-8	/* argomento non valido */

#define DEVFS_NAME	"DevFS"
#define DEVFS_DESC	"Device File System"
#define DEVFS_DIR	"/dev"

typedef enum
{
	ENTRY_EMPTY = 0,
#define ENTRY_EMPTY	ENTRY_EMPTY
	ENTRY_DEVICE = 1,
#define ENTRY_DEVICE	ENTRY_DEVICE
	ENTRY_DIRECTORY = 2,
#define ENTRY_DIRECTORY	ENTRY_DIRECTORY
	ENTRY_LINK = 3
#define ENTRY_LINK	ENTRY_LINK
} DEVENTRY_TYPE;

typedef enum
{
	DEV_EMPTY = 0,	/* TODO: per usi futuri */
#define DEV_EMPTY DEV_EMPTY
	DEV_FD = 1,	/* floppy drive */
#define DEV_FD	DEV_FD
	DEV_HD = 2,	/* hard disk drive */
#define DEV_HD	DEV_HD
	DEV_CD = 3,	/* cd-rom drive */
#define DEV_CD	DEV_CD
	DEV_MOUSE = 4,	/* mouse */
#define DEV_MOUSE DEV_MOUSE
	DEV_CONSOLE = 5,/* tty console */
#define DEV_CONSOLE DEV_CONSOLE
	DEV_OTHER = 6,	/* other device */
#define DEV_OTHER DEV_OTHER
	MASK_CTRL = 0x20,	/* mask: device attached to a controller */
#define MASK_CTRL MASK_CTRL
	MASK_PCI = 0x30,	/* mask: device attached to a pci slot */
#define MASK_PCI MASK_PCI
	MASK_PORT = 0x40,	/* mask: device attached to a port (serial,parallel,usb,ecc.) */
#define MASK_ISA MASK_ISA
	MASK_ISA = 0x50		/* mask: device attached to an isa slot */
} DEVTYPE;

typedef enum
{
	REQ_READ = 0,
#define REQ_READ REQ_READ
	REQ_WRITE = 1
#define REQ_WRITE REQ_WRITE
} REQTYPE;

typedef struct
{
	REQTYPE req_type;	/* tipo di richiesta: lettura o scrittura */
	u32 start_block;	/* blocco di partenza */
	u32 block_count;	/* numero di blocchi */
	u32 block_size;		/* lunghezza di un blocco */
	void * buffer;		/* buffer dati entrata/uscita */
	void * other;		/* può essere qualsiasi cosa, per altre applicazioni */
} devreq_t;

typedef struct
{
	unsigned char filename[256];	/* nome del device */
	unsigned char desc[256];	/* descrizione della periferica */
	DEVTYPE dev_type;		/* tipo di periferica */
	u32 block_size;			/* lunghezza predefinita di un blocco */
	u32 block_count;		/* numero di blocchi (in caso di block device, 0 altrimenti) */
	void * other_data;		/* altri dati */
	int (*handler)(const char * filename,devreq_t * request); /* handler per il device */
	/* l'handler restituisce il numero di blocchi/byte letti/scritti, altrimenti 0 o un codice di errore DEV */
} __attribute__ ((packed)) device_t;

/* fixato in 1024 byte */
typedef struct
{
	device_t device;	/* dati per il device */
	unsigned long next_entry;
	unsigned long prev_entry;
	unsigned long parent_entry;	/* entry genitore */
	unsigned long data;	/* directory: prima sotto-entry, device: niente, link: niente */
	DEVENTRY_TYPE type:2;
/* fix 1024 byte data */
	unsigned char fix1:6;
	unsigned char source[256];	/* tanto c'è spazio! :) */
	unsigned char fix2[222];
} __attribute__ ((packed)) devfs_entry;

filesystem * init_devfs(void);

int dev_create(const char * filename,DEVENTRY_TYPE type);
int dev_remove(const char * filename);
int dev_link(const char * filename,const char * linkname);

int dev_ioctl_device(const char * filename,devreq_t * request);
int dev_setparams(const char * filename,device_t * devparams);
device_t * dev_getparams(const char * filename);

#endif
