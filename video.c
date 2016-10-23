/* video.c -- video management definitions */
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

#include <asm/system.h>
#include <mm/paging.h>
#include <video.h>
#include <io.h>
#include <string.h>

#define VGA_COLOR_MEM		0xB8000
#define VGA_MONO_MEM		0xB0000

/* Porte VGA */
#define VGA_MISC_READ		0x3CC
#define VGA_COLOR_PORT		0x3D4
#define VGA_MONO_PORT		0x3B4

/* Comandi VGA */
#define VGA_SET_CURSOR_START	0xA
#define VGA_SET_CURSOR_END	0xB
#define VGA_SET_ADDRESS_HIGH	0xC
#define VGA_SET_ADDRESS_LOW	0xD
#define VGA_SET_CURSOR_HIGH	0xE
#define VGA_SET_CURSOR_LOW	0xF

static unsigned int max_w = 0;
static unsigned int max_h = 0;
unsigned char * videomem = 0;
unsigned short ioaddr = 0;

typedef struct
{
	short data;
} VIDEOCHAR;

videopage videopgs[MAX_VIDEOPAGES];
videopage * curpage;	// puntatore alla videopage corrente
unsigned int cp_index;	// indice della videopage corrente in videopgs[]

/* inizializza il sistema video testuale a pagine
 max_x, max_y: numero massimo di colonne e di righe
 lasty: numero della riga da cui partire con la stampa */
int init_video(unsigned int max_x,unsigned int max_y,unsigned int lasty)
{
	unsigned int i,pages;
/* imposta altezza e larghezza dello schermo */
	if((!max_x) || (!max_y)) return -1;	// eh beh....
	max_w = max_x;
	max_h = max_y;
/* rileva tipo di monitor e relativo indirizzo video */
	if(inportb(VGA_MISC_READ) & 0x01)
	{	/* monitor a colori */
		videomem = (unsigned char *)VGA_COLOR_MEM;
		ioaddr = VGA_COLOR_PORT;
	}
	else
	{	/* monitor b/n */
		videomem = (unsigned char *)VGA_MONO_MEM;
		ioaddr = VGA_MONO_PORT;
	}
/* mappa la memoria video testuale e crea le videopage */
	pages = ((max_w * max_h * 2)*(MAX_VIDEOPAGES)) / PAGE_SIZE;
	if(((max_w * max_h * 2)*(MAX_VIDEOPAGES)) % PAGE_SIZE) pages++;
	identity_mapping(videomem,pages,PAGE_RESERVED);
	for(i=1;i<=MAX_VIDEOPAGES;i++) make_videopage(i);
/* comincia dalla videopage 0 */
	curpage = &videopgs[0];
	cp_index = 0;
	curpage->y = lasty + (lasty ? 1 : 0);
	if(curpage->y >= max_y-1) scroll_up(curpage);
	clrscr(curpage);
	update_HW_cursor();
	return 0;
}

void set_x(videopage * vp,unsigned int x)
{
	vp->x = x;
	update_HW_cursor();
}

void set_y(videopage * vp,unsigned int y)
{
	vp->y = y;
	update_HW_cursor();
}

unsigned int get_x(videopage * vp)
{
	return vp->x;
}

unsigned int get_y(videopage * vp)
{
	return vp->y;
}

/* prepare una struct videopage
 n: numero della videopage a partire da 1 */
void make_videopage(unsigned int n)
{
	memset((void *)&videopgs[n-1],(char )0,sizeof(videopage));
	videopgs[n-1].x = 0;
	videopgs[n-1].y = 0;
	videopgs[n-1].addr = videomem + (max_w * max_h * 2)*(n-1);
	videopgs[n-1].text = BIOS_DEFAULT_TEXT;
	videopgs[n-1].back = BIOS_DEFAULT_BACK;
}

/* solo per la videopage corrente
 aggiorna il cursore visivo */
void update_HW_cursor(void)
{
	long flags;
	save_flags(flags); cli();
	outb(14,ioaddr);  // low byte
	outb((((int)(curpage->addr-videomem))/2 + curpage->y*max_w+curpage->x)>>8,ioaddr+1);
	outb(14+1,ioaddr);  // high byte
	outb((((int)(curpage->addr-videomem))/2 + curpage->y*max_w+curpage->x),ioaddr+1);
	restore_flags(flags);
}

/* cambia la videopage corrente
 page: pagina video a partire da 1 */
void change_videopage(unsigned int page)
{
	long flags;
	save_flags(flags); cli();
	curpage = &videopgs[page - 1];
	cp_index = page - 1;
	outb(VGA_SET_ADDRESS_HIGH,ioaddr);
	outb(0xFF & (((int )(curpage->addr-videomem))>>9),ioaddr+1);
	outb(VGA_SET_ADDRESS_LOW,ioaddr);
	outb(0xFF & (((int)(curpage->addr-videomem))>>1),ioaddr+1);
	update_HW_cursor();
	restore_flags(flags);
}

void clear_line(videopage * vp,int n)
{
    char *mem;
    char *line_end;

    if (n<0 && n>max_h-1) {
        return;
    }

    mem = (char*)vp->addr + (max_w*2)*n;
    line_end = mem + (max_w*2);

    for (; mem<line_end; mem+=2) {
        *mem = 0x20;        /* 0x20 = ' ' */
        *(mem+1) = vp->text + vp->back;
    }

    update_HW_cursor();
}

void scroll_up(videopage * vp)
{
	VIDEOCHAR *memvideo = (VIDEOCHAR*)vp->addr;

    // Move the rows upwards  */
    memmove(memvideo,&memvideo[max_w],sizeof(VIDEOCHAR)*(max_w)*(max_h-1));

    // clear last line
    clear_line(vp,max_h-1);
	vp->y = max_h - 1;

    update_HW_cursor();
}

void new_line(videopage * vp)
{
	vp->y++;
	if(vp->y >= max_h-1) scroll_up(vp);
	vp->x = 0;
	update_HW_cursor();
}

void clrscr(videopage * vp)
{
	unsigned int i;
	for (i = 0; i < (80 * 25); i++) {
		vp->addr[i * 2] = 0x20;
		vp->addr[i * 2 + 1] = vp->text + vp->back;
	}
	vp->x = 0;
	vp->y = 0;
	update_HW_cursor();
}

void putc(videopage * vp,const char ch)
{
	unsigned int i;
	long flags;
	save_flags(flags); cli();
	switch (ch)
	{
		case '\0': /* ma vaffanculo, va... */
			break;
		case '\r':
			vp->x = 0;
			break;
		case '\n':
			vp->y++;
			vp->x=0;
			break;
		case '\b':
			if(!vp->x) break;
			vp->x--;
		     *(char *)(vp->addr + 2 * vp->x + 160 * vp->y) = ' ';
	   *(char *)((vp->addr + 2 * vp->x + 160 * vp->y)+1) = vp->text+vp->back;
			break;
		case '\t':
			/* da fare: allinea il tab alla griglia */
			for(i=0;i<8;i++)
			{
				*(char *)(vp->addr + 2 * (vp->x+i) + 160 * vp->y) = ' ';
				*(char *)((vp->addr + 2 * (vp->x+i) + 160 * vp->y)+1) =
					vp->text+vp->back;
			}
			vp->x += TAB_SIZE;	// incrementa x di TAB_SIZE
			break;
		default:
		      *(char *)(vp->addr + 2 * vp->x + 160 * vp->y) = ch;
	   *(char *)((vp->addr + 2 * vp->x + 160 * vp->y)+1) = vp->text+vp->back;
			vp->x++;
	}
    
	if (vp->x >= max_w)
	{
		vp->y += vp->x/max_w;  // new line
		vp->x = vp->x%max_w;
	}
	if (vp->y >= max_h)
	{
		scroll_up(vp);
		vp->y = max_h-1;
	}
	update_HW_cursor();
	restore_flags(flags);
}

/* stampa un carattere sulla videopage corrente */
void printc(const char ch)
{
	putc(curpage,ch);
}

void set_xx(unsigned int x)
{
	curpage->x = x;
	update_HW_cursor();
}
void set_yy(unsigned int y)
{
	curpage->y = y;
	update_HW_cursor();
}

unsigned int get_xx(void)
{
	return curpage->x;
}

unsigned int get_yy(void)
{
	return curpage->y;
}
