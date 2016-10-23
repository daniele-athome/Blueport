/* dev.c -- Devices management functions */
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

#include <fs/dev.h>
#include <fs/vfs.h>
#include <ktypes.h>
#include <mm/kmem.h>
#include <string.h>
#include <kernel.h>
#include <panic.h>
#include <disk/ramdisk.h>
#include <fs/errno.h>
#include <fs/path.h>
#include <asm/system.h>

#define DEVFS_BLOCKSIZE		1024	// dimensione del settore
#define DEVFS_BLOCKNUM		1024	// numero di settori

/* funzioni per l'interfaccia con il vfs */
int dev_hopen(const char * filename,unsigned char access,unsigned char lock,const char * device);
int dev_hclose(const char * filename,const char * device);
int dev_hread(const char * filename,unsigned long start,unsigned long length,void * buffer,const char * device);
int dev_hwrite(const char * filename,unsigned long start,unsigned long length,void * data,const char * device);
int dev_hcreate(const char * filename,const char * device);
int dev_hremove(const char * filename,const char * device);
int dev_hlink(const char * filename,const char * linkname,const char * device);
int dev_hmkdir(const char * dirname,const char * device);
int dev_hrmdir(const char * dirname,const char * device);
int dev_hlookup(const char * filename,const char * device);
void * dev_hlist(const char * pathname,const char * device);
file_t * dev_hfileinfo(const char * filename,const char * device);
int dev_hspecial(const char * filename,void * data,const char * device);
int dev_hidentify(const char * device);
int dev_hmount(const char * device);
int dev_humount(const char * device);

void dev_read_block(unsigned int block,devfs_entry * buffer);
void dev_write_block(unsigned int block,devfs_entry * entry);
int format_devfs(unsigned int block_size,unsigned int block_num);
void make_devfs_entry(devfs_entry * entry,char * filename,DEVENTRY_TYPE type,unsigned long data,
 unsigned long next_entry,unsigned long prev_entry,unsigned long parent_entry);
int dev_get_entry_block(const char * filename);
void dev_list(const char * dirname);

ramdisk_t devices;		/* ramdisk per il devfs */

/* crea e formatta la ramdisk per il devfs e restituisce l'indirizzo del filesystem */
filesystem * init_devfs(void)
{
	filesystem devfs;
	if(create_ramdisk(&devices,DEVFS_BLOCKSIZE,DEVFS_BLOCKNUM,(void *)0))
	 panic("dev: impossibile creare la ramdisk per il %s.\n",DEVFS_NAME);
	memset(devices.area,(char)0,DEVFS_BLOCKSIZE*DEVFS_BLOCKNUM);
	#ifdef DEBUG
	kprint("dev: creata ramdisk per il %s, dimensione: %d KB.\n",
		DEVFS_NAME,(DEVFS_BLOCKSIZE*DEVFS_BLOCKNUM)/1024);
	#endif
	if(format_devfs(DEVFS_BLOCKSIZE,DEVFS_BLOCKNUM))
	 panic("dev: errore durante la formattazione della ramdisk per il %s.\n",DEVFS_NAME);
	#ifdef DEBUG
	kprint("dev: ramdisk formattata in %s.\n",DEVFS_NAME);
	#endif
	strcpy(devfs.fsname,DEVFS_NAME);
	strcpy(devfs.fsdesc,DEVFS_DESC);
	devfs.nodev = 1;
	devfs.hopen = &dev_hopen;
	devfs.hclose = &dev_hclose;
	devfs.hread = &dev_hread;
	devfs.hwrite = &dev_hwrite;
	devfs.hcreate = &dev_hcreate;
	devfs.hremove = &dev_hremove;
	devfs.herase = &dev_hremove;
	devfs.hlink = &dev_hlink;
	devfs.hmkdir = &dev_hmkdir;
	devfs.hrmdir = &dev_hrmdir;
	devfs.hlookup = &dev_hlookup;
	devfs.hlist = &dev_hlist;
	devfs.hidentify = &dev_hidentify;
	devfs.hfileinfo = &dev_hfileinfo;
	devfs.hmount = &dev_hmount;
	devfs.humount = &dev_humount;
	devfs.hspecial = &dev_hspecial;
	#ifdef DEBUG
	kprint("dev: registrazione del filesystem %s...\n",devfs.fsname);
	#endif
	return register_fs(&devfs);
}

/* formatta la ramdisk per il DevFS */
int format_devfs(unsigned int block_size,unsigned int block_num)
{
	devfs_entry temp;
	/* crea la entry di root */
	memset((void *)&temp,(char)0,DEVFS_BLOCKSIZE);
	make_devfs_entry(&temp,"/",ENTRY_DIRECTORY,0,0,0,0);
	dev_write_block(0,&temp);
	return DEV_SUCCESS;
}

/* crea una entry (file, directory o link) nel DevFS */
int dev_create(const char * filename,DEVENTRY_TYPE type)
{
	devfs_entry parent;
	devfs_entry place;
	devfs_entry next;
	unsigned int c_parent = 0;
	long int fblock = 0;
	unsigned int c_next = 0;
	unsigned int i = 0;
	unsigned int p = 0;
	char part[256];
	char part2[256];
	char filename2[256];
	if(dev_get_entry_block(filename) > -1) return DEV_EXIST;	/* entry già esistente */
	path_get_part(filename,i,part);	// estrai subito la parte della root
	while(strcmp(part,""))
	{
		path_get_part(filename,i+1,part2);
		if(!strcmp(part2,"")) break;
		path_get_part(filename,++i,part);
	}
	path_get_left(filename,--i,filename2);
	/* ottieni il blocco della cartella che sarà il genitore di quella che si sta per creare */
	fblock = dev_get_entry_block(filename2);
	if(fblock < 0) return DEV_NOTFOUND;
	/* ok, trovato, leggilo */
	dev_read_block(fblock,&parent);
	c_parent = fblock;
	/* ora devo cercare un blocco libero in cui piazzare la mia nuova entry */
	for(p=0;p<DEVFS_BLOCKNUM;p++)
	{
		dev_read_block(p,&place);
		if(!place.type) goto found_block;
	}
	return DEV_FULL;	/* ...azzo! */
found_block:
	/* se arrivi qui, è stato trovato un blocco libero */
	fblock = p;
	memset((void *)&place,(char)0,sizeof(devfs_entry));
	path_get_part(filename,i+1,part2);
	place.next_entry = 0;	// ultimo elemento, nessun successivo
	place.parent_entry = c_parent;
	place.type = type;
	strcpy((char *)place.device.filename,part2);
/* aggiorna il parent o la next_entry se necessario */
	if(!parent.data)	/* cartella vuota, quello da creare è quindi il primo elemento */
	{
		parent.data = fblock;
		place.prev_entry = 0;
		dev_write_block(c_parent,&parent);
	}
	else
	{
		/* aggiorna il prev_entry */
		c_next = parent.data;
		do
		{
			dev_read_block(c_next,&next);
			if(next.next_entry) c_next = next.next_entry;
		} while(next.next_entry);
		/* ok, abbiamo l'ultima entry della cartella: ora possiamo scrivere */
		place.prev_entry = c_next;
		/* aggiorna il next della prev_entry */
		dev_read_block(c_next,&next);
		next.next_entry = fblock;
		dev_write_block(c_next,&next);
	}
	/* FINITO!!!! Finalmente posso scrivere la entry sulla ramdisk */
	dev_write_block(fblock,&place);
	return DEV_SUCCESS;
}

/* rimuove un file/link/cartella
 nel caso della cartella, il suo contenuto andrà perso
 restituisce il blocco dove è stata scritta la entry */
int dev_remove(const char * filename)
{
	int fblock = 0;
	int c_next = 0;
	int c_prev = 0;
	int c_parent = 0;
	devfs_entry fentry;
	devfs_entry next;
	devfs_entry prev;
	devfs_entry parent;
	fblock = dev_get_entry_block(filename);
	if(fblock < 0) return DEV_NOTFOUND;
	dev_read_block(fblock,&fentry);
	c_next = fentry.next_entry;
	c_prev = fentry.prev_entry;
	c_parent = fentry.parent_entry;
	if(c_parent > 0) dev_read_block(c_parent,&parent);
	if(c_next > 0) dev_read_block(c_next,&next);
	if(c_prev > 0) dev_read_block(c_prev,&prev);
	if((c_next > 0) && (c_prev > 0))
	/* la entry si trova nel mezzo di due entry */
	{
		next.prev_entry = fentry.prev_entry;
		prev.next_entry = fentry.next_entry;
		memset((void *)&fentry,(char)0,sizeof(devfs_entry));
	}
	if((c_next > 0) && (!c_prev))
	/* la entry è la prima nella cartella */
	{
		parent.data = fentry.next_entry;
		next.prev_entry = 0;
	}
	if((!c_next) && (c_prev > 0))
	/* la entry è l'ultima nella cartella */
	{
		prev.next_entry = 0;
	}
	if((!c_next) && (!c_prev))
	/* la entry è sola nella cartella */
	{
		parent.data = 0;
	}
	dev_write_block(fblock,&fentry);
	if(c_next > 0) dev_write_block(c_next,&next);
	if(c_prev > 0) dev_write_block(c_prev,&prev);
	if(c_parent > 0) dev_write_block(c_parent,&parent);
	return fblock;
}

/* restituisce il numero del blocco contenente la entry <filename>, -1 se non è stata trovata. */
int dev_get_entry_block(const char * filename)
{
	unsigned int out = 0;
	unsigned int fblock = 0;
	unsigned int i = 0;
	devfs_entry fentry;
	char part[256];
	path_get_part(filename,0,part);
	dev_read_block(fblock,&fentry);	// leggi subito la entry di root
	while(strcmp(part,""))
	{
		do
		{
			if(!strcmp((char *)fentry.device.filename,part))
			{
				/* entry "part" trovata */
				out = fblock;
				goto found_cluster;
			}
			fblock = fentry.next_entry;
			dev_read_block(fentry.next_entry,&fentry);
		} while(fblock > 0);
		/* se arrivo qui, significa che il blocco della parte NON è stato trovato */
		return -1;
	found_cluster:
		fblock = fentry.data;
		dev_read_block(fblock,&fentry);
		/* blocco della parte trovato, passa al successivo */
		path_get_part(filename,++i,part);
	}
	return out;
}

void dev_read_block(unsigned int block,devfs_entry * buffer)
{
	ramdisk_read(&devices,block,1,(void *)buffer);
}

void dev_write_block(unsigned int block,devfs_entry * entry)
{
	ramdisk_write(&devices,block,1,(void *)entry);
}

void make_devfs_entry(devfs_entry * entry,char * filename,DEVENTRY_TYPE type,unsigned long data,
 unsigned long next_entry,unsigned long prev_entry,unsigned long parent_entry)
{
	strcpy((char *)entry->device.filename,filename);
	entry->type = type;
	entry->data = data;
	entry->next_entry = next_entry;
	entry->prev_entry = prev_entry;
	entry->parent_entry = parent_entry;
	
}

/* FUNZIONE DI DEBUG, da convertire per uso globale */
/* lista gli elementi all'interno di <dirname> */
void dev_list(const char * dirname)
{
	int fblock = 0;
	devfs_entry fentry;
	fblock = dev_get_entry_block(dirname);
	if(fblock < 0) return;
	dev_read_block(fblock,&fentry);
	kprint("Contenuto di %s:\n",fentry.device.filename);
	/* preleva la prima sotto-cartella della cartella */
	fblock = fentry.data;
	if(!fblock) return;
	/* stampa soltanto le entry con parent = parent */
	do
	{
		dev_read_block(fblock,&fentry);
		kprint("\t%s\n",fentry.device.filename);
		fblock = fentry.next_entry;
	} while(fentry.next_entry);
}

/* crea un link <linkname> a <filename> */
int dev_link(const char * filename,const char * linkname)
{
	devfs_entry dest;
	int c_src = 0;
	int c_dest = 0;
	c_src = dev_get_entry_block(filename);
	if(c_src < 0) return DEV_NOTFOUND;
	c_dest = dev_create(linkname,ENTRY_LINK);
	if(c_dest < 0) return c_dest;
	dev_read_block(c_dest,&dest);
	strcpy((char *)dest.source,filename);
	dev_write_block(c_dest,&dest);
	return DEV_SUCCESS;
}

/* invia una richiesta di i/o al device <filename> (percorso relativo al devfs)
 restituisce il valore di ritorno dell'handler */
int dev_ioctl_device(const char * filename,devreq_t * request)
{
	int fblock;
	devfs_entry fentry;
	if(!filename || !request) return DEV_ERROR;	// controllo dati passati
	if((fblock = dev_get_entry_block(filename)) < 0) return DEV_NOTFOUND;
	dev_read_block(fblock,&fentry);	// legge la entry
	if(!fentry.device.handler) return DEV_NOTCONFIG;
	return (fentry.device.handler)(filename,request);	// richiama l'handler
}

/* imposta i parametri per il device <filename>
 naturalmente il campo devparams->filename è ignorato */
int dev_setparams(const char * filename,device_t * devparams)
{
	devfs_entry fentry;
	int fblock = 0;
	if(!devparams) return DEV_ERROR;
	fblock = dev_get_entry_block(filename);
	if(fblock < 0) return DEV_NOTFOUND;
	dev_read_block(fblock,&fentry);
	strcpy((char *)fentry.device.desc,(char *)devparams->desc);
	fentry.device.dev_type = devparams->dev_type;
	fentry.device.block_size = devparams->block_size;
	fentry.device.block_count = devparams->block_count;
	fentry.device.other_data = devparams->other_data;
	fentry.device.handler = devparams->handler;
	dev_write_block(fblock,&fentry);
	return DEV_SUCCESS;
}

/* restituisce i parametri per il device <filename> */
device_t * dev_getparams(const char * filename)
{
	int fblock;
	devfs_entry fentry;
	device_t * devparams;
	if(!filename) return (device_t *)0;
	if(!(devparams = (device_t *)kmalloc(sizeof(device_t)))) return (device_t *)0;
	if((fblock = dev_get_entry_block(filename)) < 0) return (device_t *)0;
	dev_read_block(fblock,&fentry);	// legge la entry
	memcpy((void *)devparams,(void *)&fentry.device,sizeof(device_t));
	return devparams;
}

/* funzioni di handler per l'interfaccia con il vfs */

int dev_hopen(const char * filename,unsigned char access,unsigned char lock,const char * device)
{
	return VFS_SUCCESS;
}

int dev_hclose(const char * filename,const char * device)
{
	return VFS_SUCCESS;
}

int dev_hread(const char * filename,unsigned long start,unsigned long length,void * buffer,const char * device)
{
	return VFS_SUCCESS;
}

int dev_hwrite(const char * filename,unsigned long start,unsigned long length,void * data,const char * device)
{
	return VFS_SUCCESS;
}

int dev_hcreate(const char * filename,const char * device)
{
	if(dev_create(filename,ENTRY_DEVICE) < 0) return VFS_ERROR;
	return VFS_SUCCESS;
}

int dev_hremove(const char * filename,const char * device)
{
	if(dev_remove(filename) < 0) return VFS_ERROR;
	return VFS_SUCCESS;
}

int dev_hlink(const char * filename,const char * linkname,const char * device)
{
	if(dev_link(filename,linkname) < 0) return VFS_ERROR;
	return VFS_SUCCESS;
}

int dev_hmkdir(const char * dirname,const char * device)
{
	if(dev_create(dirname,ENTRY_DIRECTORY) < 0) return VFS_ERROR;
	return VFS_SUCCESS;
}

int dev_hrmdir(const char * dirname,const char * device)
{
	int fblock = 0;
	devfs_entry directory;
	fblock = dev_get_entry_block(dirname);
	if(fblock < 0) return VFS_ERROR;
	dev_read_block(fblock,&directory);
	if(directory.data > 0) return VFS_ERROR;	/* cartella non vuota, impossibile rimuoverla */
	if(dev_remove(dirname) < 0) return VFS_ERROR;
	return VFS_SUCCESS;
}

/* vfs: controlla l'esistenza di un elemento */
int dev_hlookup(const char * filename,const char * device)
{
	int blocked = 0;
	blocked = dev_get_entry_block(filename);
	if(blocked < 0) return 0;
	return 1;
}

/* vfs: restituisce l'indirizzo della lista concatenata contenente il contenuto della cartella ;) */
void * dev_hlist(const char * pathname,const char * device)
{
	/* TODO */
	return (void *)0;
}

file_t * dev_hfileinfo(const char * filename,const char * device)
{
	return (file_t *)0;
}

/* la speciale del devfs restituisce la entry device_t per il device <filename> */
int dev_hspecial(const char * filename,void * data,const char * device)
{
	devfs_entry fentry;
	int fblock = 0;
	fblock = dev_get_entry_block(filename);
	if(fblock < 0) return -1;
	dev_read_block(fblock,&fentry);
	memcpy(data,(void *)&fentry.device,sizeof(device_t));
	return VFS_SUCCESS;
}

/* identificazione di un devfs -- solo per sicurezza :) */
int dev_hidentify(const char * device)
{
	devfs_entry root_entry;
	/* possiamo anche ignorare il device, controlla l'integrità della root */
	dev_read_block(0,&root_entry);
	if(strcmp(device,"devfs")) return 0;	// mah...
	return (strcmp((char *)root_entry.device.filename,"/")) ? 0 : 1; // 1 = identificato
	/* bel controllo :P */
}

int dev_hmount(const char * device)
{
	if(strcmp(device,"devfs")) return VFS_DEVERROR;	// ;)
	return VFS_SUCCESS;
}

int dev_humount(const char * device)
{
	return VFS_BUSY;	// non puoi smontare il devfs!!!
}
