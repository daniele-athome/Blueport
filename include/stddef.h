/* stddef.h -- standard datatype definitions */
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

#ifndef __STDDEF_H
#define __STDDEF_H

typedef unsigned int size_t;
typedef int diff_t;
typedef diff_t ptrdiff_t;
typedef void* ptr_t;
typedef unsigned int pid_t;
typedef unsigned char sig_t;
typedef unsigned int err_t;

typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned int dword;
typedef unsigned long long qword; //8 byte

#ifndef NULL
//#define NULL	((void *)0)	m'hanno detto che è sbagliato...
#define NULL	0
#endif /* NULL */

#endif
