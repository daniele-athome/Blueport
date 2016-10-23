/* path.h -- Path manipulation and checking definitions */
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

#ifndef __PATH_H
#define __PATH_H

//#define PATH_INVALID_CHARS	"/"	// TODO

/* Path Manipulation */
void path_get_part(const char * ptr, const unsigned int part, char * buffer);
void path_get_left(const char * ptr,const unsigned count,char * buffer);
void path_get_right(const char * ptr,const unsigned count,char * buffer);
void add_slash(char * filename);
void remove_slash(char * filename);
unsigned int path_count_parts(const char * filename);

/* Path Checking */
int path_check_path(const char * path,const char * invalid_chars);

#endif
