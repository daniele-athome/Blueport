/* ktypes.h -- kernel types definitions */
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

#ifndef __KTYPES_H
#define __KTYPES_H

/* hardware datatypes */
typedef unsigned char BYTE;           /* 8-bit byte */
typedef unsigned short WORD;          /* 16-bit word */
typedef unsigned long DWORD;          /* 32-bit dword */
typedef unsigned long ADDR;           /* address that should not be deref'd */

/* integer types */
typedef unsigned char UINT8;          /* 8-bit unsigned integer */
typedef signed char INT8;             /* 8-bit signed integer */
typedef unsigned short UINT16;        /* 16-bit unsigned integer */
typedef signed short INT16;           /* 16-bit signed integer */
typedef unsigned long UINT32;         /* 32-bit unsigned integer */
typedef signed long INT32;            /* 32-bit signed integer */ 
 
/* logical datatypes */
typedef unsigned char CHAR;           /* ISO 8859-1 character */
typedef unsigned char *STRPTR;        /* C-style NUL-terminated string */
typedef enum { FALSE=0,TRUE=1 } BOOL; /* boolean value */

/* useful macros */
#define BITFIELD(name,width) unsigned int name : width
#define HIBYTE(x) ((BYTE)((x) >> 8))
#define LOBYTE(x) ((BYTE)((x) & 0xff))
#define ABS(x) ((x) < 0 ? -(x) : (x))    /* NB: multiple evaluations! */

/* Linux types */

/* unsigned */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long int u32;
typedef unsigned long long int u64;

/* signed */
typedef signed char s8;
typedef signed short int s16;
typedef signed long int s32;
typedef signed long long int s64;

#endif
