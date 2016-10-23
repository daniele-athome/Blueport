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
#include <limits.h>
#include <ctype.h>

char *strchr(const char *s, int c)
{
	int p = 0;
	while ((s[p] != c) && (s[p] != '\0'))
		p++;
	if (s[p] == '\0')
		return NULL;
	return (char *) &(s[p]);	/* cast because of CONST char * stuff */
}

char *strcpy(char *s1, const char *s2)
{
	char *p = s1;
	while ((*p++ = *s2++))	/* gcc likes ((...)) :-) */
		;
	return s1;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	char *p = dest;
	int i = 0;
	while ((*p++ = *src++) && (i < n))
		i++;
	for (; i < n; i++)
		*p++ = '\0';
	return dest;
}

char *strcat(char *s1, const char *s2)
{
	char *p = s1;
	while (*p)
		p++;		/* go to the end */
	while ((*p++ = *s2++));	/* copy */
	return s1;
}

int strncmp(const char *s1,size_t n,const char *s2)
{
	char buffer[1024];
	memset((void *)buffer,(char )0,sizeof(char)*1024);	// puliamo il buffer
	strncpy(buffer,s1,n);
	return strcmp(buffer,s2);
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	int i;
	for (i = 0; i < n; i++)
		if (*((char *) s1 + i) != *((char *) s2 + i))
			return *((char *) s1 + i) - *((char *) s2 + i);
	return 0;
}

void reverse(unsigned char s[])
{
	int c, i, j;

	for (i = 0, j = strlen((char *)s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

   /* itoa:  convert n to characters in s */
void itoa(int n, unsigned char s[])
{
	int i, sign;

	if ((sign = n) < 0)	/* record sign */
		n = -n;		/* make n positive */
	i = 0;
	do {			/* generate digits in reverse order */
		s[i++] = n % 10 + '0';	/* get next digit */
	} while ((n /= 10) > 0);	/* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

unsigned char *strstr(unsigned char *orig, unsigned char *pattern)
{
	unsigned char trovato = 0;	//al momento non si è trovato gnente
	unsigned long lung = strlen((char *)pattern);
	unsigned long i;
	if (lung > strlen((char *)orig))
		return NULL;
	while (*orig) {		//finche la stringha contentinitore non è nulla
		if ((*orig) == (*pattern)) {
			trovato = 1;
			i = 1;
			while (*(pattern + i)) {
				if (*(orig + i) != *(pattern + i)) {
					trovato = 0;
					break;	//str non trovata
				}
				i++;
			}
			if (trovato)
				return orig;
		}
		orig++;
	}
	return NULL;
}

long unsigned strtou(char *s, int base, char **scan_end)
{
	int value, overflow = 0;
	long unsigned result = 0, oldresult;
	/* Skip trailing zeros */
	while (*s == '0')
		s++;
	if (*s == 'x' && base == 16) {
		s++;
		while (*s == '0')
			s++;
	}
	/* Convert number */
	while (isnumber(*s, base)) {
		value = tonumber(*s++);
		if (value > base || value < 0)
			return (0);
		oldresult = result;
		result *= base;
		result += value;
		/* Detect overflow */
		if (oldresult > result)
			overflow = 1;
	}
	if (scan_end != 0L)
		*scan_end = s;
	if (overflow)
		result = INT_MAX;
	return (result);
}

inline void * memmove(void * dest,const void * src, size_t n)
{
int d0, d1, d2;
if (dest<src)
__asm__ __volatile__(
	"rep\n\t"
	"movsb"
	: "=&c" (d0), "=&S" (d1), "=&D" (d2)
	:"0" (n),"1" (src),"2" (dest)
	: "memory");
else
__asm__ __volatile__(
	"std\n\t"
	"rep\n\t"
	"movsb\n\t"
	"cld"
	: "=&c" (d0), "=&S" (d1), "=&D" (d2)
	:"0" (n),
	 "1" (n-1+(const char *)src),
	 "2" (n-1+(char *)dest)
	:"memory");
    return dest;
}

inline size_t strlen(const char * s) {
	int d0;
	register int __res;
	__asm__ __volatile__(
	    "repne\n\t"
	    "scasb\n\t"
	    "notl %0\n\t"
	    "decl %0"
	    :"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffff));
	return __res;
}

inline void *memcpy(void * to, const void * from, size_t n)
{
    int d0, d1, d2;
    __asm__ __volatile__(
        "rep ; movsl\n\t"
        "testb $2,%b4\n\t"
        "je 1f\n\t"
        "movsw\n"
        "1:\ttestb $1,%b4\n\t"
        "je 2f\n\t"
        "movsb\n"
        "2:"
        : "=&c" (d0), "=&D" (d1), "=&S" (d2)
        :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
        : "memory");
    return (to);
}

inline void *memset(void * s, char c, size_t count)
{
    int d0, d1;
	__asm__ __volatile__(
        "rep\n\t"
        "stosb"
        : "=&c" (d0), "=&D" (d1)
        :"a" (c),"1" (s),"0" (count)
        :"memory");
    return s;
}

inline int strcmp(const char * cs,const char * ct) {
    int d0, d1;
    register int __res;
__asm__ __volatile__(
    "1:\tlodsb\n\t"
    "scasb\n\t"
    "jne 2f\n\t"
    "testb %%al,%%al\n\t"
    "jne 1b\n\t"
    "xorl %%eax,%%eax\n\t"
    "jmp 3f\n"
    "2:\tsbbl %%eax,%%eax\n\t"
    "orb $1,%%al\n"
    "3:"
    :"=a" (__res), "=&S" (d0), "=&D" (d1)
    :"1" (cs),"2" (ct));
    return __res;
}

/* Visual Basic compatibility functions */

unsigned char *ltrim(unsigned char *string)
{
	while (*string == ' ')
		string++;
	return string;
}

unsigned char *rtrim(unsigned char * string)
{
	/* da sistemare */
	while (*string != ' ')
		string++;
	return string;
}

unsigned char *trim(unsigned char * string)
{
	string = ltrim(string);
	//return rtrim(string);
	return string;
}

int instr(int start,char * string1,char * string2)
{
	int i = start;
	string1 = string1 + i;
	while(*string1 != *string2)
	{
		i++;
		string1++;
	}
	return i;
}

void left(char *dest,const char * string,int lenght)
{
	// TODO
	return;
}

void right(char *dest,const char * string,size_t n)
{
	int i = 0;
	for(i=strlen(string);i>n;i--) strcat(dest,&string[i]);
	return;
}

void mid(char *dest,const char * string,int start,int lenght)
{
	// TODO
	return;
}
