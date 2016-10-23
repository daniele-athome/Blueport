/* mm/config.h -- Configuration parameters for Memory Management */
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

#ifndef __MM_CONFIG_H
#define __MM_CONFIG_H

/* modi possibili per ottenere la grandezza della memoria */
#define	MM_MULTIBOOT	0	// dati dal boot loader (predefinito)
#define MM_CMOS		1	// dati dal cmos
#define MM_COUNT	2	// conteggio della memoria
#define MM_BDA		3	// dati dalla bios data area

/* tipo di probe per il conteggio della memoria */
#define MM_PROBE	MM_MULTIBOOT

#endif
