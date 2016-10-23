/* errno.h -- Codici di errore per il VFS */
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

#ifndef __FS_ERRNO_H
#define __FS_ERRNO_H

#define VFS_SUCCESS		0	// successo
#define VFS_ERROR		-1	// errore
#define VFS_FAILED		-2	// operazione fallita (errore utente, es: buffer = 0)
#define VFS_NOTFOUND		-3	// file(system) non trovato
#define VFS_NOTMOUNTED		-4	// non montato
#define VFS_FSERROR		-5	// errore nel filesystem
#define VFS_MAXMOUNTS		-6	// troppi montaggi
#define VFS_ACCESSDENIED	-7	// accesso negato a causa del blocco esclusivo da un altro processo
#define VFS_PERMISSIONDENIED	-8	// accesso negato a causa di permessi utente insufficenti
#define VFS_EOFREACHED		-9	// raggiunta la fine del file
#define VFS_INVALIDFD		-10	// descrittore file non valido
#define VFS_DEVERROR		-11	// errore del device
#define VFS_BUSY		-12	// risorsa occupata
#define VFS_NOMEM		-13	// memoria insufficente

#endif
