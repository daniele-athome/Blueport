/* path.c -- path manipulation and check functions */
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

#include <fs/path.h>
#include <string.h>

/* restituisce la parte di percorso <part> nel nome di file <ptr> */
void path_get_part(const char * ptr,const unsigned int part,char * buffer)
{
	unsigned long cnt, p = part;
	if (!p)
	{
		*buffer = '/';
		*(buffer+1) = '\0';
		return;
	}
	for (cnt = 0; ptr[cnt] && p; cnt++)
		if (ptr[cnt] == '/') p--;
	if (!ptr[cnt])
	{
		*buffer = '\0';
		return;
	}
	while (ptr[cnt] && ptr[cnt] != '/') *buffer++ = ptr[cnt++];
	*buffer = '\0';
	return;
}

/* restituisce la parte di path con <count> parti a partire da sinistra */
void path_get_left(const char * ptr,const unsigned count,char * buffer)
{
	unsigned int i = 0;
	char part[256];
	char file2[256];
	strcpy(file2,ptr);
	add_slash(file2);
	*buffer = '\0';
	for(i=0;i<=count;i++)
	{
		path_get_part(file2,i,part);
		strcat(buffer,part);
		if(buffer[strlen(buffer)-1] != '/') strcat(buffer,"/");
	}
	if(strcmp(buffer,"/")) buffer[strlen(buffer)-1] = '\0';
}

/* restituisce la parte di path con <count> parti a partire da destra */
void path_get_right(const char * ptr,const unsigned count,char * buffer)
{
	/* TODO */
	*buffer = '\0';
}

/* aggiunge lo slash finale (se non è presente) */
void add_slash(char * filename)
{
	if(filename[strlen(filename)-1] != '/') strcat(filename,"/");
}

/* rimuove lo slash finale (se è presente) */
void remove_slash(char * filename)
{
	if(filename[strlen(filename)-1] == '/') filename[strlen(filename)-1] = '\0';
}

/* conta il numero di parti per un filename */
unsigned int path_count_parts(const char * filename)
{
	unsigned int i = 0;
	unsigned int count = 0;
	char filename2[256];
	strcpy(filename2,filename);
	remove_slash(filename2);
	for(i=0;i<strlen(filename2);i++) if(filename2[i] == '/') count++;
	return count;
}

/* controlla la validità di un path, considerando i caratteri contenuti in <invalid_chars> non validi
 considera non validi anche i caratteri in PATH_INVALID_CHARS
 restituisce 0 se il path è valido, -1 se non lo è */
int path_check_path(const char * path,const char * invalid_chars)
{
	/* TODO */
	return 0;
}
