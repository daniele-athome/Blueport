/* string.c -- String manipulation functions */
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

#include <string.h>

const char *strstr(const char *s1, const char *s2)
{
	const char *start_s1 = NULL;
	const char *in_s2 = NULL;

	for(; *s1 != '\0'; s1++)
	{
		if(start_s1 == NULL)
		{
/* first char of match */
			if(*s1 == *s2)
			{
/* remember start of matching substring in s1 */
				start_s1 = s1;
				in_s2 = s2 + 1;
/* done already? */
				if(*in_s2 == '\0')
					return start_s1;
			}
/* continued mis-match */
			else
				/* nothing */;
		}
		else
		{
/* continued match */
			if(*s1 == *in_s2)
			{
				in_s2++;
/* done */
				if(*in_s2 == '\0')
					return start_s1;
			}
			else
/* first char of mis-match */
				start_s1 = NULL;
		}
	}
	return NULL;
}
