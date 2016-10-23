/* driver.c -- Gestore/Caricatore dei driver di periferica */
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

#include <driver.h>
#include <kernel.h>

int init_driver(void)
{
/* guardare nella initial ramdisk se c'è qualche modulo da inizializzare */
	kprint("drv: nessun driver da caricare dalla ramdisk di boot.\n");
	return 0;
}

int register_driver(void /* da mettere gli argomenti */)
{
	return 0;
}

int load_driver(const char * filename /* altri argomenti */)
{
	return 0;
}
