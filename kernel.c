/* kernel.c -- funzioni di avvio principali del kernel */
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

#include <idt.h>
#include <irq.h>
#include <panic.h>
#include <stdarg.h>
#include <kernel.h>
#include <vsprintf.h>
#include <version.h>
#include <mm/mem.h>
#include <mm/kmem.h>
#include <mm/mem_area.h>
#include <math.h>
#include <dev/keyboard.h>
#include <asm/system.h>
#include <fs/vfs.h>
#include <sys.h>
#include <string.h>
#include <fs/path.h>
#include <multiboot.h>
#include <asm/system.h>
#include <video.h>

#define	LOG_SIZE	4096	// log di kprint -- 4KB

multiboot_info kernel_info;	// multiboot ricopiate per sicurezza
char kprint_log[LOG_SIZE];	// log di kprint

asmlinkage int start_kernel(unsigned long magic,multiboot_info * info)
{
	int mb_check;
	#ifdef DEBUG
	unsigned long esp;
	#endif
/* pulisce il log di kprint */
	memset((void *)kprint_log,(char )0,sizeof(char)*LOG_SIZE);
/* inizializza il coprocessore matematico */
	fpu_init();
/* inizializza una console raw iniziale (prima riga :) */
	init_video(80,25,0);
	#ifdef DEBUG
	asm("mov %%esp,%0\n":"=m"(esp)::"memory");
	kprint("esp = 0x%x\n",esp);
	#endif
	if((mb_check = multiboot_check(magic,info)) < 0) panic("controllo Multiboot non passato (err = %d)\n",mb_check);
	#ifdef DEBUG
	kprint("gdt_48.limit = 0x%x, gdt_48.address = 0x%x\n",gdt_48.limit,gdt_48.address);
	kprint("gdt = 0x%x\n",&gdt);
	kprint("idt_48.limit = 0x%x, idt_48.address = 0x%x\n",idt_48.limit,idt_48.address);
	kprint("idt = 0x%x\n",&idt);
	#endif
	/* Memory Manager -- fase 1 */
	if(init_mm() < 0) panic("inizializzazione del gestore della memoria fallita\n");
	if((mb_check = (unsigned long)prepare_modules((multiboot_module *)kernel_info.mods_addr,
	 kernel_info.mods_count)) < 0) panic("moduli Multiboot non caricati\n");
	#ifdef DEBUG
	kprint("boot: %d modul%c caricat%c.\n",mb_check,(mb_check == 1)?'o':'i',(mb_check == 1)?'o':'i');
	#endif
	kprint("Salto a kernel_main() in 0x%x\n",&kernel_main);
	kernel_main();
	return 0;
}

int kernel_main(void)
{
	kprint("%s versione %d.%d.%d\n",KRNLNAME,KRNLVER_MAJ,KRNLVER_MIN,KRNLVER_REV);
	/* Componenti di basso livello */
	init_irq();
	init_idt();
	init_syscalls();
	/* Memory Manager -- fase 2 */
	if(init_mm() < 0) panic("inizializzazione del gestore della memoria fallita\n");
	if(copy_modules((multiboot_module *)kernel_info.mods_addr,kernel_info.mods_count) < 0)
	 panic("moduli Multiboot non caricati\n");
	sti();
	/* Virtual File System Layer */
	if(init_vfs() < 0) panic("inizializzazione del gestore dei filesystem fallita\n");
	kprint("Kernel pronto.\n");
	#if 0
	extern memory_area kmem_area;
	memory_area * new;
	kprint("*** creating memory area...\n");
	new = init_memory_area();
	kprint("*** memory area created at 0x%x\n",new);
	if(new)
	{
		kprint("*** testing memory area...\n");
		switch_memory_area(new);
		kprint("*** memory area ok\n");
		kprint("*** back:");
		switch_memory_area(&kmem_area);
		kprint(" kmem_area.\n");
		kprint("*** freeing memory area...\n");
		kprint("free_memory_area() returned %d\n",free_memory_area(new));
	}
	#endif
	while(1);
	return 0;
}

/* kernel print -- stampa videopage corrente, ignora la struttura delle console
 siccome questa funzione scrive direttamente in memoria video, per usarla bisogna essere in supervisor mode (CPL#0) */
int kprint(char * format,...)
{
	char buf[2048];	// mah siiiiiii!!!!!!!!! 2KB!!!!
	int len = 0;
	unsigned int i = 0;
	va_list args;
	va_start(args,format);
	len = vsprintf(buf,format,args);
	va_end(args);
	for(i=0;i<len;i++) printc(buf[i]);
	/* accoda la stringa al log */
	klogcat(buf);
	return len;
}

/* stampa il log di kprint (usare questa, se si stampa il log con kprint fa un casino!!!) */
void print_klog(void)
{
	unsigned int i;
	for(i=0;i<strlen(kprint_log);i++) printc(kprint_log[i]);
}

/* accoda la stringa <str> al log di kprint
 se è stata raggiunta la dimensione massima del log, lo shifta per farci entrare la stringa */
void klogcat(char * str)
{
	/* scala il buffer se necessario */
	if((strlen(kprint_log)+strlen(str)) > LOG_SIZE)
	 memcpy((void *)kprint_log,(const void *)(kprint_log+strlen(str)),LOG_SIZE-strlen(str));
	/* accoda la stringa al log */
	strcat(kprint_log,str);
}
