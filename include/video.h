/* video.h -- video management definitions */
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

#ifndef __VIDEO_H
#define __VIDEO_H

#define TAB_SIZE	8
#define MAX_VIDEOPAGES	8	// proprio al massimo (se mettiamo 80x25)

/* attributi di testo */
#define TEXT_LIGHTGRAY		0x07

/* attributi di sfondo */
#define BACK_BLACK		0x00

#define	BIOS_DEFAULT_ATTR	TEXT_LIGHTGRAY+BACK_BLACK
#define	BIOS_DEFAULT_TEXT	TEXT_LIGHTGRAY
#define	BIOS_DEFAULT_BACK	BACK_BLACK

/* Video Page structure */
typedef struct
{
	unsigned char * addr;	// indirizzo di base
	unsigned int x;		// coordinata x corrente
	unsigned int y;		// coordinata y corrente
	unsigned char text:4;	// attributo del testo
	unsigned char back:4;	// attributo dello sfondo
} __attribute__((packed)) videopage;

int init_video(unsigned int max_x,unsigned int max_y,unsigned int lasty);
void change_videopage(unsigned int page);
void set_x(videopage * vp,unsigned int x);
void set_y(videopage * vp,unsigned int y);
unsigned int get_x(videopage * vp);
unsigned int get_y(videopage * vp);
void clear_line(videopage * vp,int n);
void scroll_up(videopage * vp);
void new_line(videopage * vp);
void clrscr(videopage * vp);
void putc(videopage * vp,const char ch);
void update_HW_cursor(void);
void make_videopage(unsigned int n);

/* funzioni per la videopage corrente */
void printc(const char ch);
void set_xx(unsigned int x);
void set_yy(unsigned int y);
unsigned int get_xx(void);
unsigned int get_yy(void);

#endif
