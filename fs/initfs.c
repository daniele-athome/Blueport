/* initfs.c -- INITFS management */
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

/* FILESYSTEM IMPLEMENTATO IN SOLA LETTURA */

#include <fs/initfs.h>
#include <fs/vfs.h>
#include <fs/errno.h>
#include <fs/dev.h>
#include <fs/path.h>
#include <kernel.h>
#include <mm/kmem.h>
#include <mm/paging.h>
#include <string.h>
#include <stdlib.h>
#include <panic.h>

/* funzioni di interfacciamento con il vfs */
int initfs_hopen(const char * filename,unsigned char access,unsigned char lock,const char * device);
int initfs_hclose(const char * filename,const char * device);
int initfs_hread(const char * filename,unsigned long start,unsigned long length,void * buffer,const char * device);
int initfs_hlookup(const char * filename,const char * device);
void * initfs_hlist(const char * pathname,const char * device);
file_t * initfs_hfileinfo(const char * filename,const char * device);
int initfs_hspecial(const char * filename,void * data,const char * device);
int initfs_hidentify(const char * device);
int initfs_hmount(const char * device);
int initfs_humount(const char * device);

/* funzioni private initfs */
static int make_checksum(char * header);
static long read_file(char * filename,size_t start,size_t size,void * buffer,char * device);
static long long get_file_size(char * filename,char * device);
static long get_file_block(char * filename,char * device);
static int convert_checksum(char * chksum);

filesystem * register_initfs(void)
{
	filesystem initfs;
	memset((void *)&initfs,(char)0,sizeof(filesystem));
	strcpy(initfs.fsname,INITFS_NAME);
	strcpy(initfs.fsdesc,INITFS_DESC);
	initfs.nodev = 0;
	initfs.hopen = &initfs_hopen;
	initfs.hclose = &initfs_hclose;
	initfs.hread = &initfs_hread;
	initfs.hwrite = 0;
	initfs.hlookup = &initfs_hlookup;
	initfs.hlist = &initfs_hlist;
	initfs.hspecial = &initfs_hspecial;
	initfs.hidentify = &initfs_hidentify;
	initfs.hfileinfo = &initfs_hfileinfo;
	initfs.hmount = &initfs_hmount;
	initfs.humount = &initfs_humount;
	return register_fs(&initfs);
}

/* legge da <filename> <size> byte a partire da <start> dal device <device>
 restituisce il numero di byte letti, altrimenti 0 in caso di end-of-file oppure un codice di errore VFS */
static long read_file(char * filename,size_t start,size_t size,void * buffer,char * device)
{
	char * tmpbuf;
	long long filesize;
	long fileblock;
	size_t toread = 0;
	unsigned int blocks;
	device_t * dparams;
	devreq_t request;
	if(!filename || !device || !buffer) return VFS_FAILED;
/* passo 1: ottieni parametri del device */
	if(!(dparams = dev_getparams(device))) return VFS_DEVERROR;
/* passo 2: verifica quanti byte dobbiamo leggere */
	toread = size;
	if((filesize = get_file_size(filename,device)) < 0)
	{
		kfree(dparams);
		#ifdef DEBUG2
		kprint("errore filesize\n");
		#endif
		return filesize;	// errorino :P
	}
	#ifdef DEBUG2
	kprint("filesize = %d\n",filesize);
	kprint("start+size = %d\n",start+size);
	#endif
	if(start >= filesize)	// troppo eof :|
	{
		kfree(dparams);
		return 0;
	}
	if(start+size >= filesize) toread = filesize - start + 1;	// quanto so forte!!!! ;)
/* passo 3: comincia la lettura */
	request.req_type = REQ_READ;
	if((fileblock = get_file_block(filename,device)) < 0)
	{
		kfree(dparams);
		#ifdef DEBUG2
		kprint("errore fileblock\n");
		#endif
		return fileblock;
	}
	unsigned int end_block,start_block;
	request.start_block = fileblock + (start/dparams->block_size) + 1;	// 4
	/* numero di blocchi da leggere */
	end_block = (start + toread) / dparams->block_size;
	if((start + toread) % dparams->block_size) end_block++;
	start_block = request.start_block;	// 4
	blocks = end_block - start_block + 1;	// 4
	#ifdef DEBUG2
	kprint("lettura di %d byte\n",toread);
	kprint("start_block = %d, end_block = %d, block_count = %d\n",start_block,end_block,blocks);
	#endif
	request.block_size = dparams->block_size;
	request.block_count = blocks;
	request.other = 0;
	if(!(request.buffer = kmalloc(blocks * dparams->block_size)))
	{
		kfree(dparams);
		return VFS_NOMEM;
	}
	#ifdef DEBUG2
	kprint("buffer size = %d bytes (%d blocks)\n",blocks*dparams->block_size,blocks);
	#endif
	memset(request.buffer,(char)0,(blocks * dparams->block_size));
	#ifdef DEBUG2
	kprint("lettura...\n");
	#endif
	if(dev_ioctl_device(device,&request) < 0)
	{
		kfree(dparams);
		kfree(request.buffer);
		return VFS_DEVERROR;
	}
	#ifdef DEBUG2
	kprint("bufferone: \"%s\"\n",(char *)request.buffer);
	#endif
	kprint("pulito, toread = %d\n",toread);
	/* copia solo il necessario */
	tmpbuf = request.buffer;
	#ifdef DEBUG2
	kprint("tmpbuf = 0x%x\n",(unsigned long)tmpbuf);
	#endif
	memset(buffer,(char)0,size);
	tmpbuf += (start % dparams->block_size);
	#ifdef DEBUG2
	kprint("tmpbuf = 0x%x\n",(unsigned long)tmpbuf);
	#endif
	memcpy(buffer,(void *)tmpbuf,toread);
/* passo 4: libera tutto ed esci */
	kfree(request.buffer);
	kfree(dparams);
	return toread;
}

/* restituisce la dimensione del file <filename> */
static long long get_file_size(char * filename,char * device)
{
	char sumc[12+1];
	initfs_block * entry;
	devreq_t request;
	long block;
	if((block = get_file_block(filename,device)) < 0) return block;
	if(!(entry = (void *)kmalloc(sizeof(initfs_block)))) return VFS_NOMEM;
	request.start_block = block;
	request.block_count = 1;
	request.block_size = sizeof(initfs_block);
	request.buffer = (void *)entry;
	request.other = 0;
	if(dev_ioctl_device(device,&request) < 0) return VFS_DEVERROR;
	#ifdef DEBUG
	kprint("make_checksum() = 0x%x, convert_checksum = 0x%x\n",
	 make_checksum((char *)entry),convert_checksum(entry->chksum));
	#endif
	if(make_checksum((char *)entry) != convert_checksum(entry->chksum)) return VFS_FSERROR;
	memset((void *)sumc,(char)0,12+1);
	memcpy((void *)sumc,(void *)entry->size,12);
	return (unsigned long long)strtoul(sumc,(char**)0,8);
}

/* cerca il file <filename> e ne restituisce il blocco all'interno del device <device> */
static long get_file_block(char * filename,char * device)
{
	unsigned long long file_size = 0;
	char size_octal[12+1];
	char * conv = 0;
	initfs_block * entry;
	unsigned int i;
	long block = VFS_NOTFOUND;
	device_t * dparams;
	initfs_cache * cache;
	devreq_t request;
	if(!filename || !device) return VFS_FAILED;
/* passo 1: ottieni parametri del device */
	if(!(dparams = dev_getparams(device))) return VFS_DEVERROR;
	cache = dparams->other_data;
	if(cache)	// abbiamo la cache, controllace ;)
	{
		for(i=0;i<N_ENTRIES;i++)
		{
			if(!strcmp(cache[i]->name,filename))
			{
				#ifdef DEBUG2
				kprint("entry in cache\n");
				#endif
				block = cache[i]->block;
				break;
			}
		}
		/* entry non trovata nella cache, procedi manualmente */
		if(i >= N_ENTRIES) goto nocache;
	}
	else
	{	// ricerca manuale
nocache:
		#ifdef DEBUG2
		kprint("entry non in cache\n");
		#endif
		/* cerca da dopo le N_ENTRIES in poi */
		if(!(entry = kmalloc(sizeof(initfs_block)))) return VFS_NOMEM;
		for(i=N_ENTRIES-1;i>=0;i--) if(cache[i]->block) break;
		#ifdef DEBUG2
		kprint("i = %d, N_ENTRIES = %d\n",i,N_ENTRIES);
		#endif
		if(i >= N_ENTRIES) i = 0;	// cache sfonnata sicuramente o niente file nel tar (???)
		request.start_block = cache[i]->block;
		request.block_size = sizeof(initfs_block);
		request.block_count = 1;
		request.buffer = (void *)entry;
		request.other = 0;
		memset((void *)entry,(char)0,sizeof(initfs_block));
		if(dev_ioctl_device(device,&request) < 0)
		{
			kfree(entry);
			return VFS_DEVERROR;
		}
		do	// comincia a ciclare le varie entry
		{
			#ifdef DEBUG2
			kprint("entry->name = \"%s\"\n",entry->name);
			#endif
			if(!strcmp(entry->name,""))	// sospetta fine archivio
			{
				request.start_block++;
				#ifdef DEBUG2
				kprint("lettura...\n");
				#endif
				if(dev_ioctl_device(device,&request) < 0)
				{
					kfree(entry);
					return VFS_DEVERROR;
				}
				#ifdef DEBUG2
				kprint("sospetti: tutti!!! (0x%x)\n",entry);
				#endif
				if(!strcmp(entry->name,""))
				{
					#ifdef DEBUG2
					kprint("fine archivio!!!\n");
					#endif
					kfree(entry);
					return VFS_NOTFOUND;
				}
				return VFS_FSERROR;	// non ci può essere una entry vuota!!!
			}
		/* file già letto, ottieni dimensione e passa al file successivo */
			memset((void *)size_octal,(char)0,12+1);
			memcpy((void *)size_octal,entry->size,12);
			file_size = strtoul(size_octal,&conv,8);	// conversione da base 8
			#ifdef DEBUG2
			kprint("size_octal = \"%s\"\n",size_octal);
			kprint("file_size = %d\n",file_size);
			#endif
			if(*conv)	// cifra non affidabile
			{
				#ifdef DEBUG2
				kprint("cifra non affidabile (%d)\n",file_size);
				#endif
				kfree(entry);
				return VFS_FSERROR;
			}
			request.start_block += file_size / sizeof(initfs_block) + 1;
			if(file_size % sizeof(initfs_block)) request.start_block++;
			#ifdef DEBUG2
			kprint("start_block = %d\n",request.start_block);
			#endif
			if(dev_ioctl_device(device,&request) < 0)
			{
				kfree(entry);
				return VFS_DEVERROR;
			}
			block = request.start_block;	// porca m'ero scordato!!! ;)
		} while(strcmp(filename,entry->name));	// "cerca mentre non corrisponde"
	}
	/* restituisci blocco/codice di errore */
	return block;
}

static int convert_checksum(char * chksum)
{
	char sumc[8+1];
	memset((void *)sumc,(char)0,8+1);
	memcpy((void *)sumc,(void *)chksum,8);
	#ifdef DEBUG2
	kprint("sumc = \"%s\"\n",sumc);
	#endif
	return (int)strtol(sumc,(char**)0,8);
}

/* genera la checksum per <header> */
static int make_checksum(char * header)
{
	int sum = 0,i;
	initfs_block * entry = (initfs_block *)header;
	if(!header) return -1;	// hihihi :)
	for(i=0;i<sizeof(initfs_block);i++) sum += header[i];	// checksum iniziale
	for(i=0;i<8;i++) sum -= entry->chksum[i];	// rimuove dalla checksum il campo checksum stesso
	sum += ' ' * 8;	// ultimi ritocchi (mah...)
	#ifdef DEBUG2
	kprint("sum = 0x%x\n",sum);
	#endif
	return sum;
}

/* funzioni di interfacciamento con il vfs */

int initfs_hopen(const char * filename,unsigned char access,unsigned char lock,const char * device)
{
	/* che famo all'apertura? */
	return VFS_SUCCESS;
}

int initfs_hclose(const char * filename,const char * device)
{
	/* che famo alla chiusura? */
	return VFS_SUCCESS;
}

int initfs_hread(const char * filename,unsigned long start,unsigned long length,void * buffer,const char * device)
{
	/* filtra il filename senza lo slash iniziale */
	int res;
	char * postfile;
	if(!filename) return VFS_FAILED;
	postfile = kmalloc(strlen(filename));
	memcpy(postfile,(filename+1),strlen(filename)-1);
	#ifdef DEBUG2
	kprint("\"%s\", \"%s\"\n",filename,postfile);
	#endif
	res = read_file((char *)postfile,start,length,buffer,(char *)device);
	kfree(postfile);
	return res;
}

int initfs_hlookup(const char * filename,const char * device)
{
	int blocked = 0;
	//blocked = initrd_lookup(filename);
	if(blocked < 0) return 0;
	return 1;
}

void * initfs_hlist(const char * pathname,const char * device)
{
	/* TODO */
	return (void *)0;
}

file_t * initfs_hfileinfo(const char * filename,const char * device)
{
	return (file_t *)0;
}

/* cosa fa la special dell'initfs? Boooooo!!! */
int initfs_hspecial(const char * filename,void * data,const char * device)
{
	/* TODO */
	return 0;
}

/* handler identificazione
 controlla solo l'integrità del primo blocco */
int initfs_hidentify(const char * device)
{
	int sum = 0;
	initfs_block * entry;
	devreq_t request;
	/* verifica l'integrità della prima entry */
	request.req_type = REQ_READ;
	request.start_block = 0;
	request.block_count = 1;
	request.block_size = sizeof(initfs_block);
	request.other = 0;
	if(!(request.buffer = entry = kmalloc(request.block_size))) return 0;
	#ifdef DEBUG
	kprint("lettura da ioctl a %s...\n",device);
	int iores;
	iores = dev_ioctl_device(device,&request);
	kprint("iores = %d\n",iores);
	if(iores <= 0) return 0;
	#else
	if(dev_ioctl_device(device,&request) <= 0) return 0;	// errore device
	#endif
	/* dentro <entry> avrò la entry dal device */
	/* verifica la signature e la versione */
	#ifdef DEBUG
	kprint("entry letta, verifica signature e versione in corso...\n");
	kprint("entry->magic = \"%s\"\n",entry->magic);
	#endif
	//if((memcmp(entry->magic,TMAGIC,TMAGLEN)) || (memcmp(entry->version,TVERSION,TVERSLEN))) goto failure_exit;
	if(memcmp(entry->magic,TMAGIC,TMAGLEN)) goto failure_exit;
	/* verifica il checksum */
	#ifdef DEBUG
	kprint("verifica signature e versione ok, controllo checksum...\n");
	#endif
	sum = make_checksum((char *)entry);
	if(sum != convert_checksum(entry->chksum)) goto failure_exit;
	kfree(entry);
	#ifdef DEBUG
	kprint("identificazione ok, uscita.\n");
	#endif
	return 1;
failure_exit:
	#ifdef DEBUG
	kprint("fallimento dell'identificazione, uscita.\n");
	#endif
	kfree(entry);
	return 0;
}

/* handler montaggio
 considerando che il filesystem è già stato identificato, questa funzione legge le prime N entry del filesystem
 e ne memorizza la posizione nel buffer in device->other_data */
int initfs_hmount(const char * device)
{
	int sum = 0;
	char sumc[8+1];
	unsigned int i,cur_block = 0;
	unsigned long file_size;
	char size_octal[12+1];
	char * conv = 0;
	int iores;
	void * test;
	device_t * dparams;
	initfs_cache * cache;
	initfs_block * entry;
	devreq_t request;
/* ottieni i parametri del device */
	if(!(dparams = dev_getparams(device))) return VFS_DEVERROR;
/* alloca il buffer di cache */
	if(!(request.buffer = kmalloc(dparams->block_size))) return VFS_NOMEM;
	if(!(dparams->other_data = kmalloc(sizeof(initfs_cache))))
	{
		kfree(request.buffer);
		return VFS_NOMEM;
	}
	cache = dparams->other_data;
	entry = request.buffer;
	memset((void *)cache,(char)0,sizeof(initfs_cache));
	dev_setparams(device,dparams);
/* leggi le prime N entry e mettile in cache */
	request.req_type = REQ_READ;
	request.block_count = 1;
	request.block_size = dparams->block_size;
	request.other = 0;
	for(i=0;i<N_ENTRIES;i++)
	{
		#ifdef DEBUG2
		kprint("cachamento entry %d...\n",i);
		#endif
		request.start_block = cur_block;
		iores = dev_ioctl_device(device,&request);
		if(iores < 0)	// errore
		{
			kfree(test);
			kfree(cache);
			kfree(dparams);
			kfree(request.buffer);
			return VFS_DEVERROR;
		}
		/* verifica integrità header */
		sum = make_checksum((char *)entry);
		memset((void *)sumc,(char)0,8+1);
		memcpy((void *)sumc,(void *)entry->chksum,8);
		#ifdef DEBUG2
		kprint("sum = 0x%x, sumc = \"%s\", strtol() = 0x%x\n",sum,sumc,strtoul(sumc,(char**)0,8));
		#endif
		if(!strcmp(sumc,"")) goto suspect_eof;
		if(sum != strtol(sumc,(char**)0,8))
		{
			kfree(test);
			kfree(cache);
			kfree(dparams);
			kfree(request.buffer);
			return VFS_FSERROR;				
		}
		/* copia i dati nella cache */
		#ifdef DEBUG
		kprint("\tentry[%d] name = \"%s\", cur_block = %d\n",i,entry->name,cur_block);
		#endif
		strcpy(cache[i]->name,entry->name);
		cache[i]->block = cur_block;
		if(!iores) break;	// fine del file :)
		/* aggiungi la dimensione del file corrente per ottenere il cur_block del file successivo */
		memset((void *)size_octal,(char)0,12+1);
		memcpy((void *)size_octal,entry->size,12);
		#ifdef DEBUG2
		kprint("entry->size = \"%s\", size_octal = \"%s\"\n",entry->size,size_octal);
		#endif
		if(!strcmp(size_octal,""))	// sospetta fine archivio
		{
suspect_eof:
			/* prova a leggere anche il blocco successivo, se è zero l'archivio è terminato */
			if(!(test = kmalloc(dparams->block_size)))
			{
				kfree(test);
				kfree(cache);
				kfree(dparams);
				kfree(request.buffer);
				return VFS_NOMEM;
			}
			memset(test,(char)0,dparams->block_size);
			if(!memcmp(entry,test,dparams->block_size))
			{
				/* leggi blocco sucessivo */
				request.start_block = cur_block+1;
				iores = dev_ioctl_device(device,&request);
				if(iores < 0)	// errore
				{
					kfree(cache);
					kfree(dparams);
					kfree(request.buffer);
					kfree(test);
					return VFS_DEVERROR;
				}
				if(!memcmp(entry,test,dparams->block_size))	// fine archivio
				{
					#ifdef DEBUG
					kprint("fine archivio!!!\n");
					#endif
					break;	// fine con successo
				}
				else	// archivio danneggiato
				{
					kfree(cache);
					kfree(dparams);
					kfree(request.buffer);
					kfree(test);
					return VFS_FSERROR;				
				}
			}
			/* finisci qui in ogni caso di errore */
			kfree(test);
			kfree(cache);
			kfree(dparams);
			kfree(request.buffer);
			return VFS_FSERROR;
		}
		file_size = strtoul(size_octal,&conv,8);	// conversione da base 8
		if(*conv)	// errore, cifra sfonnataaaaa!!!
		{
			#ifdef DEBUG2
			kprint("file_size = %d, *conv = 0x%x\n",file_size,*conv);
			#endif
			kfree(cache);
			kfree(dparams);
			kfree(request.buffer);
			return VFS_FSERROR;
		}
		if(!file_size)
		{	// file nullo -- cur_block + 1
			cur_block++;
			#ifdef DEBUG2
			kprint("cur_block unario, corrente %d\n",cur_block);
			#endif
		}
		else	// file non nullo -- cur_block + [calcola -- bubbieeee!!! :D]
		{
			cur_block += (file_size / dparams->block_size) + 1;
			#ifdef DEBUG2
			kprint("file_size = %d, block_size = %d\n",file_size,dparams->block_size);
			#endif
			if(file_size % dparams->block_size) cur_block++;
			#ifdef DEBUG2
			kprint("cur_block avanzato, corrente %d\n",cur_block);
			#endif
		}
		#ifdef DEBUG2
		kprint("entry->size = \"%s\", size_octal = \"%s\"\n",entry->size,size_octal);
		#endif
	}
/* finito, pulisci la roba */
	kfree(dparams);
	kfree(request.buffer);
/* allora, dato che qui ci finiamo solo se il cachamento ha avuto successo, cancelliamo l'ultima entry che rappresenta
 la fine dell'archivio */
 	memset((void *)cache[i]->name,(char )0,MAX_TARNAME_LEN+1);
	cache[i]->block = 0;
	return VFS_SUCCESS;
}

/* per ora libera la cache e basta */
int initfs_humount(const char * device)
{
	device_t * dparams;
	#ifdef DEBUG
	kprint("initfs_humount: liberazione cache...\n");
	#endif
	if(!(dparams = dev_getparams(device))) return VFS_DEVERROR;
	dparams->other_data = kfree(dparams->other_data);
	dev_setparams(device,dparams);
	kfree(dparams);
	return VFS_SUCCESS;
}
