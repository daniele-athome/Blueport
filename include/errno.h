/* errno.h -- Error Numbers definitions */
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

#ifndef __ERRNO_H
#define __ERRNO_H

#include <stddef.h>

/* codici di uscita applicazione */
#define EXIT_SUCCESS	0	// uscita normale
#define EXIT_ERROR	1	// uscita con errori
#define EXIT_ABORT	2	// uscita abortita (crash)

/* codici di errore globali */
#define ENOERR		0	// nessun errore
#define ENOMEM		1	// memoria insufficiente
#define EINVAL		2	// argomento non valido
#define EAGAIN		3	// risorsa temporaneamente non disponibile
#define EIO		4	// errore di input/output
#define ENOBUFS		5	// spazio buffer esaurito
#define ENOTSUP		6	// non supportato
#define EOVERFLOW	7	// valore troppo grande per essere memorizzato nel tipo di dato
#define EPERM		8	// operazione non permessa
#define ERANGE		9	// risultato troppo grande
#define EACCESS		10	// accesso negato
#define ECANCELLED	11	// operazione annullata
#define EFAULT		12	// indirizzo non valido

#define MAX_ERROR	EFAULT	// ultimo errore

void perror(char * source);
void get_error_string(char * buf,size_t len);

#endif
