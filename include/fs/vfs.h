/* vfs.h -- Virtual File System definitons */
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

#ifndef __VFS_H
#define __VFS_H

#include <kernel.h>

#define FILENAME_LEN	255+1		// 255 caratteri + lo zero
#define MAX_FS		5		// 5 filesystem per ora
#define MAX_MOUNTS	10		// 10 montaggi per ora
#define MAX_FILES	20		// 20 file aperti per ora

#define FS_READONLY	0
#define FS_READWRITE	1

typedef enum
{
	D_FILE = 0,
#define D_FILE D_FILE
	D_DIRECTORY = 1,
#define D_DIRECTORY D_DIRECTORY
	D_LINK = 2,
#define D_LINK D_LINK
	D_DEVICE = 3
#define D_DEVICE D_DEVICE
} FILETYPE;

typedef struct
{
	char filename[FILENAME_LEN];	/* nome del file */
	unsigned long long size;	/* lunghezza in byte (wow, a 64 bit!!!!) */
	FILETYPE type;			/* tipo di file */
/* altre info: date e ore */
} file_t;

/* Filesystem Information */
typedef struct
{
	char fsname[255+1];		/* nome del filesystem */
	char fsdesc[255+1];		/* descrizione del filesystem */
	unsigned char nodev;		/* file sistema senza device */
	int (*hopen)(const char * filename,unsigned char access,unsigned char lock,const char * device);
	int (*hclose)(const char * filename,const char * device);
	int (*hread)(const char * filename,unsigned long start,unsigned long length,void * buffer,const char * device);
	int (*hwrite)(const char * filename,unsigned long start,unsigned long length,void * data,const char * device);
	int (*hcreate)(const char * filename,const char * device);
	int (*hremove)(const char * filename,const char * device);
	int (*herase)(const char * filename,const char * device);
	int (*hlink)(const char * filename,const char * linkname,const char * device);
	int (*hmkdir)(const char * dirname,const char * device);
	int (*hrmdir)(const char * dirname,const char * device);
	int (*hlookup)(const char * filename,const char * device);
/* la list dovrebbe restituire l'indirizzo di un buffer di memoria allocato dalla funzione hlist stessa
 contenente un'array di elementi formata dai nomi dei file dentro la cartella, ci penseremo dopo ad
 ottenere le informazioni sui singoli file */
	void *(*hlist)(const char * pathname,const char * device);
	file_t *(*hfileinfo)(const char * filename,const char * device);
	int (*hspecial)(const char * filename,void * data,const char * device); // funzione riservata per ogni fs
	int (*hidentify)(const char * device);	// richiesta di identificazione del filesystem
	int (*hmount)(const char * device);	// funzione chiamata prima del montaggio
	int (*humount)(const char * device);	// funzione chiamata prima dello smontaggio
} filesystem;

typedef struct
{
	char device[FILENAME_LEN];	/* device montato */
	char mount_point[FILENAME_LEN];	/* directory di montaggio */
	//unsigned long block_size;	/* lunghezza del blocco logico */
	//int fsid;			/* id del filesystem nella collezione dei fs registrati */
	filesystem * fs;		/* puntatore della struct del filesystem */
} fsmount;

/* varie */
int init_vfs();
int check_filesystem(char * device,filesystem * fs);
int get_fsname(filesystem * fs,char * fsname_buf);
int get_fsdesc(filesystem * fs,char * fsdesc_buf);

/* mounting */
int mount(const char * mount_point,const char * device,filesystem * fs,unsigned char rw,unsigned char lock_device);
int umount(const char * mount_point);

/* directory */
int mkdir(const char * dirname);
int rmdir(const char * dirname);

/* special files */
int link(const char * filename,const char * linkname);
int install_ioctl(const char * filename,void * ioctl_handler);
int uninstall_ioctl(const char * filename);

/* file operations */
int create(const char * filename,FILETYPE type);
int remove(const char * filename);
int erase(const char * filename);
int open(const char * filename,unsigned char access,unsigned char lock,file_t * file);
int close(unsigned int file_nr);
int read(unsigned int file_nr,unsigned long start,unsigned long length,unsigned char back,void * buffer);
int write(unsigned int file_nr,unsigned long start,unsigned long length,unsigned char back,void * data);

/* file search */
/* TODO */

/* filesystem registering */
filesystem * register_fs(filesystem * fs);
int unregister_fs(filesystem * fs);

#endif
