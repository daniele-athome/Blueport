/* progress.c -- Progress Video Effects functions */
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

#include <progress.h>
#include <video.h>
#include <kernel.h>
#include <string.h>

int star_current = -1;

/* funzioni di progresso circolare-star */

/* prepara uno star progress
 start: indice del carattere da cui cominciare */
int star_progress_start(unsigned char start)
{
	if(star_current >= 0) return -1;	// star progress in corso
	if(start >= strlen(STAR_PROGRESS)) start = 0;
	star_current = start;
	set_xx(get_xx()+2);
	star_progress_step(0);
	return 0;
}

/* ferma uno star progress */
int star_progress_stop(int newline)
{
	star_current = -1;
	if(newline)
	{
		set_xx(get_xx()-2);
		printc(0x20);
		printc(0x20);
		kprint("\n");	// va a capo
	}
	return 0;
}

/* avanza uno star progress
 step: indica il numero di passi da fare */
int star_progress_step(unsigned int step)
{
	if(star_current < 0) return -1;		// nessun star progress in corso
	star_current += step;
	if(star_current >= strlen(STAR_PROGRESS)) star_current -= strlen(STAR_PROGRESS);
	set_xx(get_xx()-2);
	printc(STAR_PROGRESS[star_current]);
	printc(0x20);	// spazio
	return 0;
}
