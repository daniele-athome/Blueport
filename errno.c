/* errno.c -- Error Numbers functions */
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

#include <errno.h>
#include <stddef.h>
#include <kernel.h>
#include <string.h>

/* TODO: esportazione indirizzo errno da task struct (mamma mia...) */

err_t errno = ENOERR;	// errori del kernel

char * errors[MAX_ERROR+1] = {
"operazione completata con successo.",
"memoria insufficiente",
"argomento non valido",
"risorsa temporaneamente non disponibile",
"errore di input/output",
"spazio buffer esaurito",
"non supportato",
"valore troppo grande per essere memorizzato nel tipo di dato",
"operazione non permessa",
"risultato troppo grande",
"accesso negato",
"operazione annullata",
"indirizzo non valido",
};

/* stampa la stringa dell'errore corrente con prima la stringa "<source>: "*/
void perror(char * source)
{
	if(errno > MAX_ERROR) return;
	kprint("%s: %s\n",source,errors[errno]);
}

/* copia la stringa dell'errore corrente in <buf> */
void get_error_string(char * buf,size_t len)
{
	if(errno > MAX_ERROR) return;
	strncpy(buf,errors[errno],len);
}
