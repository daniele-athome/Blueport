/* vfs.c -- Virtual File System public interface */
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

#include <fs/vfs.h>
#include <fs/dev.h>
#include <fs/initfs.h>
#include <fs/errno.h>
#include <fs/path.h>
#include <multiboot.h>
#include <string.h>
#include <asm/system.h>
#include <mm/kmem.h>
#include <kernel.h>
#include <panic.h>

#define INITRD_BLOCK_SIZE	512
#define INITRD_DEVICE		"initrd"	// relativo a DEVFS_DIR

filesystem fs_collection[MAX_FS]; /* collezione dei filesystem registrati (globale) */
fsmount mounts[MAX_MOUNTS];	  /* archivio dei montaggi (globale) */
file_t * files = 0;		  /* archivio dei file aperti corrente (task-specific -- cambiato allo scheduler) */

file_t * kernel_files[MAX_FILES]; /* archivio dei file aperti dal kernel -- temporaneo (verrà usato idle_task->files) */
ramdisk_t initrd;		  /* ramdisk della initrd */

int find_first_free_fscollection(void);
int find_first_free_mount(void);
int get_mount_point(const char * filename,unsigned int mount_count,char * rest);
int dev_create(const char * filename,DEVENTRY_TYPE type);
int dev_setparams(const char * filename,device_t * devparams);
device_t * dev_getparams(const char * filename);

int initrd_handler(const char * filename,devreq_t * request);

int initfs_hread(const char * filename,unsigned long start,unsigned long length,void * buffer,const char * device);

/* init_vfs() -- INIZIALIZZA IL GESTORE DEI FILESYSTEM
 1. prepara il filesystem dei device e lo monta
 2. prepara il filesystem iniziale con l'eventuale ramdisk
 3. monta la ramdisk iniziale */
int init_vfs()
{
	filesystem * initfs;	// filesystem iniziale
	filesystem * devfs;	// filesystem dei device
	multiboot_module mod_buf;
	device_t params;
	unsigned int i,count;
	unsigned long mod_size;	// lunghezza modulo in blocchi da INITRD_BLOCKS_SIZE
/* pulisci la roba del vfs */
	kprint("VFS: preparazione gestore filesystem...\n");
	memset((void *)fs_collection,(char)0,sizeof(filesystem)*MAX_FS);
	memset((void *)mounts,(char)0,sizeof(fsmount)*MAX_MOUNTS);
	memset((void *)kernel_files,(char)0,sizeof(file_t)*MAX_FILES);
/* inizializza il sotto-sistema dei device */
	if(!(devfs = init_devfs())) panic("avvio senza ramdisk iniziale non supportato (errore devfs)\n");
/* monta il devfs su /dev */
	if(mount(DEVFS_DIR,"devfs",devfs,FS_READWRITE,DEV_LOCK_RW) < 0)
	 panic("impossibile montare il filesystem dei device.\n");
/* registra il filesystem iniziale */
	if(!(initfs = register_initfs())) panic("avvio senza ramdisk iniziale non supportato (errore initfs)\n");
/* prepara la initial ramdisk presa dai moduli multiboot */
	#ifdef DEBUG
	kprint("VFS: controllo moduli...\n");
	#endif
	if(!(count = multiboot_module_count())) panic("avvio senza ramdisk iniziale non supportato (nessun modulo)\n");
	memset((void *)&mod_buf,(char)0,sizeof(multiboot_module));
	memset((void *)&initrd,(char)0,sizeof(ramdisk_t));
	/* prepara la ramdisk e il device, poi andremo a sostituire ogni volta */
	#ifdef DEBUG
	kprint("VFS: creazione ramdisk (initrd.area = 0x%x)\n",initrd.area);
	#endif
	if(create_ramdisk(&initrd,INITRD_BLOCK_SIZE,1,&initrd) < 0)
	 panic("avvio senza ramdisk iniziale non supportato (errore della ramdisk)\n");
	/* crea il device per la ramdisk */
/**#**/
	if(dev_create("/" INITRD_DEVICE,ENTRY_DEVICE) < 0)
	 panic("avvio senza ramdisk iniziale non supportato (errore device)\n");
	strcpy((char *)params.desc,"Initial RAMDisk");
	params.dev_type = DEV_OTHER;
	params.block_size = INITRD_BLOCK_SIZE;
	params.other_data = 0;
	params.handler = &initrd_handler;
	if(dev_setparams("/" INITRD_DEVICE,&params) < 0)
	 panic("avvio senza ramdisk iniziale non supportato (errore handler)\n");
/**#**/
	kprint("device %s creato.\n",DEVFS_DIR "/" INITRD_DEVICE);
	for(i=0;i<count;i++)
	{
		kprint("caricamento modulo %d...\n",i);
		if(!multiboot_get_module(i,&mod_buf))	// abbiamo il modulo
		{
			/* sostituisci dati della ramdisk */
			mod_size = (mod_buf.mod_end - mod_buf.mod_start) / INITRD_BLOCK_SIZE;
			if((mod_buf.mod_end - mod_buf.mod_start) % INITRD_BLOCK_SIZE) mod_size++;
			initrd.block_num = params.block_count = mod_size;	// lunghezza in blocchi
			kprint("params.block_count = %d, mod_size = %d\n",params.block_count,mod_size);
			if(dev_setparams("/" INITRD_DEVICE,&params) < 0)	// aggiorna i parametri del device
			 panic("avvio senza ramdisk iniziale non supportato (errore handler)\n");
			initrd.area = (void *)mod_buf.mod_start;
			/* identifica il device della initrd con il filesystem iniziale */
			kprint("identificazione filesystem (initfs = 0x%x)\n",initfs);
			if((initfs->hidentify)("/" INITRD_DEVICE) == 1) goto initrd_found;	// trovata!!!
		}
	}
	panic("avvio senza ramdisk iniziale non supportato (ramdisk iniziale non trovata)\n");
initrd_found:	// initrd trovata, procedi
/* monta la initrd sulla root */
	kprint("monto %s su /\n",DEVFS_DIR "/" INITRD_DEVICE);
	if(mount("/",DEVFS_DIR "/" INITRD_DEVICE,initfs,FS_READONLY,DEV_LOCK_RW) < 0)
	 panic("impossibile montare la ramdisk iniziale.\n");
	#if DEBUG
	char * test;
	int nread;
	test = (char *)kmalloc(10+1);
	kprint("test = 0x%x\n",test);
	if(test) nread = initfs_hread("/menu.lst",2767,100,test,"/initrd");
	kprint("test = 0x%x, nread = %d\n",(unsigned long)test,nread);
	if(nread > 0) kprint("bufferino: \"%s\"\n",test);
	kprint("bella zi!!!!!\n");
	while(1);
	#endif
	return 0;
}

/* monta <device> su <mount_point>
 rw: se è 0, il file system viene montato in sola lettura, se è 1, in lettura e scrittura
 lock_device: se è 0, il device montato non viene bloccato, se è 1, verrà protetto da scrittura,
  se è 2 verrà protetto da lettura e da scrittura
 fs: filesystem da montare */
int mount(const char * mount_point,const char * device,filesystem * fs,unsigned char rw,unsigned char lock_device)
{
	device_t * dparams;
	char * devicefs;
	unsigned int i;
	int mnt = 0;
	if(!fs || !mount_point || !device) return VFS_FAILED;
	mnt = find_first_free_mount();
	if(mnt < 0)	// nessun mount disponibile
	 return VFS_MAXMOUNTS;
	/* si comincia!!! */
/* passo 1: controlla se è montato qualche filesystem */
	for(i=0;i<MAX_MOUNTS;i++) if(mounts[i].fs) break;
	if(i >= MAX_MOUNTS)	// nessun filesystem montato: montaggio a piacere, solo filesystem nodev
	{
		/* qui possiamo fare tutto quello che ci pare, a patto che il filesystem da montare sia nodev */
		if(!fs->nodev) return VFS_NOTFOUND;	// filesystem normali non montabili in questa situazione
		/* identifica comunque il filesystem per sicurezza */
		#ifdef DEBUG2
		int idres;
		kprint("mount: identificazione (fs->hidentify = 0x%x)...\n",fs->hidentify);
		kprint("calling identifier...\n");
		idres = (fs->hidentify)((char *)device);
		kprint("idres = %d\n",idres);
		if(idres != 1) return VFS_FSERROR;
		#else
		if((fs->hidentify)((char *)device) != 1) return VFS_FSERROR;
		#endif
		#if 0
		/* controlla validità percorso <mount_point> */
		if(path_check_path(mount_point,"") < 0) return VFS_FAILED;
		#endif
		/* setta la entry nei montaggi */
		strcpy(mounts[0].device,device);
		strcpy(mounts[0].mount_point,mount_point);
		mounts[0].fs = fs;
		/* PRIMO FILESYSTEM MONTATO */
		kprint("mount: primo montaggio: %s su %s\n",device,mount_point);
		return VFS_SUCCESS;
	}
/* passo 2: verifica esistenza device */
	/* ignora controllo se filesystem nodevice */
	if(fs->nodev) goto nodev;
	/* separa la parte del DEVFS_DIR dal percorso del device */
	kprint("device = \"%s\"\n",device);
	if(strncmp(device,DEVFS_DIR "/",strlen(DEVFS_DIR))) return VFS_NOTFOUND; // device non trovato in DEVFS_DIR
	devicefs = (char *)(device+strlen(DEVFS_DIR));
	kprint("devicefs = \"%s\"\n",devicefs);
	if(!(dparams = dev_getparams(devicefs))) return VFS_NOTFOUND;	// device non trovato nel devfs
	/* identifica filesystem */
nodev:	// nodevice filesystem
	if(fs->nodev)
	{
		if((fs->hidentify)(device)) return VFS_FSERROR;
	}	// metto le graffe sennò gcc fa storie ;)
	else
	{
		if((fs->hidentify)(devicefs) != 1) return VFS_FSERROR;
	}
/* passo 3: verifica esistenza mount_point */
	if(strcmp(mount_point,"/"))	// root: caso particolare
	{
		/* TODO: altre non-root */
		return VFS_NOTFOUND;
	}
/* passo 4: chiama l'handler del montaggio */
	if((fs->hmount)(devicefs) < 0) return VFS_FSERROR;
/* passo 5: setta la entry nei montaggi */
	strcpy(mounts[mnt].device,device);
	strcpy(mounts[mnt].mount_point,mount_point);
	mounts[mnt].fs = fs;
	kprint("mount: montato %s su %s\n",device,mount_point);
	return VFS_SUCCESS;
}

/* smonta <mount_point> */
int umount(const char * mount_point)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* crea una directory */
int mkdir(const char * dirname)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* rimuove una directory */
int rmdir(const char * dirname)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* crea un file, se esiste non lo crea
 ATTENZIONE: questa funzione non apre il file! */
int create(const char * filename,FILETYPE type)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* cancellazione "logica" di un file */
int remove(const char * filename)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* cancellazione fisica di un file */
int erase(const char * filename)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* crea un collegamento fisico o logico */
int link(const char * filename,const char * linkname)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* apre un file, memorizza le informazioni sul file in <file> e ne restituisce il numero
 access: tipo di accesso. 0 sola lettura - 1 lettura/scrittura
 lock: se è 0, il file non è bloccato, se è 1 sarà bloccato in scrittura, se è 2 in scrittura e lettura */
int open(const char * filename,unsigned char access,unsigned char lock,file_t * file)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* chiude un file con numero <file_nr> */
int close(unsigned int file_nr)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* legge un file con numero <file_nr>
 start: byte di partenza (da 0)
 length: numero di byte da leggere
 back: indica se si deve procedere a ritroso nella lettura (0 = falso, !0 = vero)
 buffer: indirizzo del buffer dove mettere i dati
*/
int read(unsigned int file_nr,unsigned long start,unsigned long length,unsigned char back,void * buffer)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* scrive un file con numero <file_nr>
 start: byte di partenza (da 0)
 length: numero di byte da scrivere
 back: indica se si deve procedere a ritroso nella scrittura (0 = falso, >0 = vero)
 data: indirizzo del buffer che contiene i dati
*/
int write(unsigned int file_nr,unsigned long start,unsigned long length,unsigned char back,void * data)
{
	/* TODO */
	return VFS_SUCCESS;
}

int install_ioctl(const char * filename,void * ioctl_handler)
{
	/* TODO */
	return VFS_SUCCESS;
}

int uninstall_ioctl(const char * filename)
{
	/* TODO */
	return VFS_SUCCESS;
}

/* funzioni riservate ai moduli di sistema interni ed esterni */

/* registra un filesystem e ne restituisce l'indirizzo nell'array dei filesystem */
filesystem * register_fs(filesystem * fs)
{
	int fs_num = -1;
	fs_num = find_first_free_fscollection();
	if(fs_num < 0) return (filesystem *)0;
	memcpy((void *)&fs_collection[fs_num],(void *)fs,sizeof(filesystem));
	return &fs_collection[fs_num];
}

/* cancella la registrazione del filesystem con descrittore <fs>
 TODO: controllare se il file system è in uso prima di cancellarlo */
int unregister_fs(filesystem * fs)
{
	if(fs->fsname[0] != '\0')
	{
		memset((void *)fs,(char)0,sizeof(filesystem));	// cancella tutto!
		return VFS_SUCCESS;
	}
	return VFS_NOTFOUND;
}

int find_first_free_fscollection(void)
{
	unsigned int i;
	for(i=0;i<MAX_FS;i++) if(fs_collection[i].fsname[0] == '\0') return i;
	return -1;
}

int find_first_free_mount(void)
{
	unsigned int i;
	for(i=0;i<MAX_MOUNTS;i++) if(mounts[i].mount_point[0] == '\0') return i;
	return -1;
}

/* risale <mount_count> mount point (senza verificarne l'esistenza) e ne restituisce l'indice in mounts[].
 mette in <rest> il percorso restante dal <mount_count>-esimo mount point */
int get_mount_point(const char * filename,unsigned int mount_count,char * rest)
{
	unsigned int i = 0;
	unsigned int m = 0;
	char left[256];
	char mount[256];
	char * initrest = 0;
	if(!rest) return -1;	// mo non me freghi
	for(i=0;i<mount_count;i++);
	{
		path_get_left(filename,i,left);
		for(m=0;m<MAX_MOUNTS;m++) if(!strcmp(mounts[m].mount_point,left))
		{
			strcpy(mount,mounts[m].mount_point);
			initrest = mounts[m].mount_point;
			initrest += strlen(left);
			strcpy(rest,initrest);
			return m;
		}
	}
	return -1;
}

/* interroga TUTTI i file system CON device per l'identificazione del filesystem <fs>
 restituisce 1 in caso corrisponda, in altro caso 0, oppure un codice di errore VFS */
int check_filesystem(char * device,filesystem * fs)
{
	if(!device) return VFS_FAILED;
	if(!fs->nodev) return 1;	// filesystem senza device -- corrisponde
	return (fs->hidentify)(device);	// chiamata handler identificazione
}

/* restituisce il nome del filesystem <fs> */
int get_fsname(filesystem * fs,char * fsname_buf)
{
	if(!fs || !fsname_buf) return VFS_FAILED;
	if(fs->fsname[0] == '\0') return VFS_NOTFOUND;
	strcpy(fsname_buf,fs->fsname);
	return VFS_SUCCESS;
}

/* restituisce la descrizione del filesystem <fs> */
int get_fsdesc(filesystem * fs,char * fsdesc_buf)
{
	if(!fs || !fsdesc_buf) return VFS_FAILED;
	if(fs->fsdesc[0] == '\0') return VFS_NOTFOUND;
	strcpy(fsdesc_buf,fs->fsdesc);
	return VFS_SUCCESS;
}

/* handler di lettura per la initrd
 legge settori raw dalla initrd */
int initrd_handler(const char * filename,devreq_t * request)
{
	device_t * dparams;
	/* controlli vari */
	if(strcmp(filename,"/" INITRD_DEVICE)) return DEV_NOTFOUND;	// il device non corrisponde
	if(request->req_type != REQ_READ) return DEV_READONLY;	// scrittura non permessa
	if(!(dparams = dev_getparams(filename))) return DEV_ERROR;	// errore parametri
	#ifdef DEBUG2
	kprint("dparams->block_size = %d\n",dparams->block_size);
	kprint("dparams->filename = %s\n",dparams->filename);
	kprint("initrd_handler: request->start_block = %d, request->block_count = %d\n",
	 request->start_block,request->block_count);
	kprint("initrd_handler: dparams->block_count = %d\n",dparams->block_count);
	#endif
	if(request->start_block+request->block_count > dparams->block_count) return DEV_INVARG; // end-of-file :|
	#ifdef DEBUG2
	kprint("request->block_size = %d\n",request->block_size);
	#endif
	if(request->block_size != dparams->block_size) return DEV_INVARG; // la dimensione blocco non corrisponde
	#ifdef DEBUG2
	kprint("request->buffer = 0x%x\n",request->buffer);
	#endif
	if(!request->buffer) return DEV_INVARG;	// buffer nullo = il fornitore della request è un cretino :)
	#ifdef DEBUG2
	kprint("initrd.area = 0x%x\n",initrd.area);
	#endif
	return ramdisk_read(&initrd,request->start_block,request->block_count,request->buffer);
}
