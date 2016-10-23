/* multiboot.c -- multiboot information management */
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

#include <multiboot.h>
#include <errno.h>
#include <string.h>
#include <kernel.h>
#include <panic.h>
#include <mm/fismem.h>
#include <mm/mem.h>
#include <mm/paging.h>
#include <mm/kmem.h>

extern multiboot_info kernel_info;
extern memory_map mmap[MAX_MMAP];
void * fis_manual_alloc(void * address,size_t count);
void * fis_manual_free(void * address,size_t count);
void * vir_manual_alloc(void * address,size_t count);
void * vir_manual_free(void * address,size_t count);

#define MB_STACK	(sizeof(multiboot_info)/sizeof(unsigned long))

int multiboot_check(unsigned long magic,multiboot_info * info)
{
	void * mbstack;
	unsigned long * tmp;
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) panic("boot: il numero magico Multiboot non corrisponde.\n");
	kprint("boot: magic = 0x%x   OK!\n",magic);	// funzionerà?
	if(!info) panic("boot: informazioni Multiboot non presenti.\n");
	memset((void *)&kernel_info,(char )0,sizeof(multiboot_info));
	mbstack = (void *)info+sizeof(info->flags);
	if(info->flags & MULTIBOOT_FLAGS_MEMINFO)
	{
		tmp = mbstack;
		/* filla variabili per la memoria */
		kernel_info.mem_lower = *tmp;
		mbstack += sizeof(info->mem_lower);
		tmp = mbstack;
		kernel_info.mem_upper = *tmp;
		mbstack += sizeof(info->mem_upper);
		kprint("boot: mem_lower = %dKB, mem_upper = %dKB\n",kernel_info.mem_lower,kernel_info.mem_upper);
	}
	if(info->flags & MULTIBOOT_FLAGS_BOOTDRIVE)
	{
		tmp = mbstack;
		memcpy((void *)&kernel_info.boot_device,(void *)tmp,sizeof(drive_device));
		kprint("boot: drive di avvio: %xh\n",kernel_info.boot_device.drive);
		mbstack += sizeof(drive_device);
	}
	if(info->flags & MULTIBOOT_FLAGS_CMDLINE)
	{
		tmp = mbstack;
		kernel_info.cmdline = (unsigned char *)*(tmp);
		kprint("boot: linea di comando: %s\n",kernel_info.cmdline);
		mbstack += sizeof(info->cmdline);
	}
	if(info->flags & MULTIBOOT_FLAGS_MODULES)
	{
		tmp = mbstack;
		kernel_info.mods_count = *tmp;
		mbstack += sizeof(info->mods_count);
		tmp = mbstack;
		kernel_info.mods_addr = *tmp;
		mbstack += sizeof(info->mods_addr);
		kprint("boot: %d modul%c da caricare, informazioni all'indirizzo 0x%x\n",
		 kernel_info.mods_count,(kernel_info.mods_count == 1)?'o':'i',kernel_info.mods_addr);
	}
	else
	{	// purtroppo ci sta lo stesso...
		mbstack += sizeof(info->mods_count) + sizeof(info->mods_addr);
	}
	if((info->flags & MULTIBOOT_FLAGS_SYMS1) || (info->flags & MULTIBOOT_FLAGS_SYMS2))
	{
		mbstack += sizeof(info->u);
	}
	if(info->flags & MULTIBOOT_FLAGS_MMAP)
	{
		/* ci sarà un pò da lavorarci con questa... */
		tmp = mbstack;
		kernel_info.mmap_length = *tmp;
		mbstack += sizeof(info->mmap_length);
		tmp = mbstack;
		kernel_info.mmap_addr = (memory_map *)*tmp;
		kprint("boot: memory map all'indirizzo 0x%x\n",kernel_info.mmap_addr);
		memset((void *)mmap,(char )0,sizeof(memory_map)*MAX_MMAP);
		memcpy((void *)mmap,(void *)kernel_info.mmap_addr,kernel_info.mmap_length);
		#ifdef DEBUG
		unsigned int i;
		for(i=0;i<info->mmap_length/sizeof(memory_map);i++)
		{
			kprint("mmap[%d].size = 0x%x\n",i,mmap[i].size);
			kprint("mmap[%d].base_addr_low = 0x%x\n",i,mmap[i].base_addr_low);
			kprint("mmap[%d].base_addr_high = 0x%x\n",i,mmap[i].base_addr_high);
			kprint("mmap[%d].length_low = 0x%x\n",i,mmap[i].length_low);
			kprint("mmap[%d].length_high = 0x%x\n",i,mmap[i].length_high);
			kprint("mmap[%d].type = 0x%x\n",i,mmap[i].type);
		}
		#endif
	}
	#if 0
	if(info->flags & MULTIBOOT_FLAGS_DRIVES)
	{
		tmp = mbstack;
		kprint("mbstack = 0x%x, *mbstack = 0x%x\n",mbstack,*tmp);
		tmp = *tmp;
		kprint("tmp = 0x%x\n",*tmp);
		mbstack += sizeof(info->drives_length) + sizeof(drives_t *);
	}
	if(info->flags & MULTIBOOT_FLAGS_CONFIGTABLE)
	{
		tmp = mbstack;
		kprint("mbstack = 0x%x, *mbstack = 0x%x\n",mbstack,*tmp);
		kprint("tmp = \"%s\"\n",*tmp);
		mbstack += sizeof(info->config_table);
	}
	if(info->flags & MULTIBOOT_FLAGS_BOOTLDRNAME)
	{
		tmp = mbstack;
		kprint("mbstack = 0x%x, *mbstack = 0x%x\n",mbstack,*tmp);
		while(1);
		kernel_info.bootloader_name = (unsigned char *)*tmp;
		kprint("boot: reduce dal caricamento di %s\n",kernel_info.bootloader_name);
		mbstack += sizeof(info->bootloader_name);
	}
	#endif
	/* TODO: alcuni flag saltati */
	return 0;
}

/* prepara i moduli per non farli incasinare con le mm
 pre_boot: indirizzo delle struct passate direttamente dal bootloader
 count: contiene il numero di moduli da processare
 restituisce il numero di moduli multiboot preparati, altrimenti -1 in caso di errore */
int prepare_modules(multiboot_module * pre_boot,unsigned long count)
{
	unsigned long i,pages;
	if(!count) return 0;
	// pagine occupate dalle struct di info sui moduli
	pages = (count * sizeof(multiboot_module)) / PAGE_SIZE;
	if((count * sizeof(multiboot_module)) % PAGE_SIZE) pages++;
	if(!fis_manual_alloc((void *)pre_boot,pages)) return -1;
	#ifdef DEBUG2
	kprint("pre_boot = 0x%x, count = 0x%x\n",pre_boot,count);
	#endif
	for(i=0;i<count;i++)
	{
		#ifdef DEBUG2
		kprint("pre_boot[%d].mod_start = 0x%x\n",i,pre_boot[i].mod_start);
		kprint("pre_boot[%d].mod_end = 0x%x\n",i,pre_boot[i].mod_end);
		kprint("pre_boot[%d].string = \"%s\"\n",i,pre_boot[i].string);
		kprint("pre_boot[%d].reserved = 0x%x\n",i,pre_boot[i].reserved);
		#endif
		pages = (pre_boot[i].mod_end - pre_boot[i].mod_start) / PAGE_SIZE;
		if((pre_boot[i].mod_end - pre_boot[i].mod_start) % PAGE_SIZE) pages++;
		/* allocazione fisica e virtuale manuale */
		if(!fis_manual_alloc((void *)pre_boot[i].mod_start,pages)) return -1;
	}
	#ifdef DEBUG
	kprint("boot: moduli preparati per lo spostamento in zona sicura.\n");
	#endif
	return count;
}

/* copia i moduli dopo le mm in un posto sicuro e allocato
 post_boot: indirizzo delle struct lavorate da prepare_modules
 restituisce 0, altrimenti -1 in caso di errore */
int copy_modules(multiboot_module * post_boot,unsigned long count)
{
	unsigned int i,pages;
	void * addr,* dest = dest;	// sennò il compilatore fa storie ;)
	if(!count) return 0;
	pages = (count * sizeof(multiboot_module)) / PAGE_SIZE;
	if((count * sizeof(multiboot_module)) % PAGE_SIZE) pages++;
	#ifdef DEBUG2
	kprint("pages = %d, identity mapping on 0x%x...\n",pages,post_boot);
	#endif
	/* si presuppone che la roba che stiamo per mappare sia stata già occupata fisicamente e virtualmente */
	identity_mapping((void *)post_boot,pages,PAGE_SHARED);
	for(i=0;i<count;i++)
	{
		pages = (post_boot[i].mod_end - post_boot[i].mod_start) / PAGE_SIZE;
		if((post_boot[i].mod_end - post_boot[i].mod_start) % PAGE_SIZE) pages++;
		#ifdef DEBUG2
		kprint("mapping %d pages on 0x%x...\n",pages,post_boot[i].mod_start);
		addr = identity_mapping((void *)post_boot[i].mod_start,pages,PAGE_SHARED);
		dest = kmalloc(pages*PAGE_SIZE);
		kprint("addr = 0x%x, dest = 0x%x\n",addr,dest);
		memcpy(dest,addr,pages*PAGE_SIZE);
		#else
		if((!(addr = identity_mapping((void *)post_boot[i].mod_start,pages,PAGE_SHARED))) ||
		(!(dest = kmalloc(pages*PAGE_SIZE)))) return -1;
		else memcpy(dest,addr,pages*PAGE_SIZE);
		#endif
		#ifdef DEBUG2
		kprint("modulo %d, addr = 0x%x, dest = 0x%x\n",i,addr,dest);
		#endif
		post_boot[i].mod_end -= post_boot[i].mod_start;
		post_boot[i].mod_start = (unsigned long)dest;
		post_boot[i].mod_end += post_boot[i].mod_start;
		if(addr) paging_free(addr,pages);
		fis_manual_free((void *)post_boot[i].mod_start,pages);
	}
	#ifdef DEBUG
	kprint("boot: moduli trasferiti in zona sicura.\n");
	#endif
	return 0;
}

/* restituisce il numero di moduli multiboot */
int multiboot_module_count(void)
{
	return kernel_info.mods_count;
}

/* copia le info sul modulo <index> in <mod_desc>
 restituisce 0, altrimenti -1 in caso di errore */
int multiboot_get_module(unsigned int index,multiboot_module * mod_desc)
{
	multiboot_module * mod_buf = (multiboot_module *)kernel_info.mods_addr;
	if(!mod_desc) return -1;
	if(index >= kernel_info.mods_count) return -1;
	memcpy((void *)mod_desc,&mod_buf[index],sizeof(multiboot_module));
	return 0;
}
