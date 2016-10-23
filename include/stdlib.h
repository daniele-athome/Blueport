/* stdlib.h -- standard library definitions */
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

#ifndef __STDLIB_H
#define __STDLIB_H

#define RAND_MAX        2147483647

/* Flag bit settings */
#define RESPECT_WIDTH	1  /* Fixed width wanted 	*/
#define ADD_PLUS	2  /* Add + for positive/floats */
#define SPACE_PAD	4  /* Padding possibility	*/
#define ZERO_PAD	8
#define LEFT_PAD 	16

unsigned abs(int x);

int rand(void);
void srand(unsigned int seed);
int random(int __num);

int max(int a, int b);
int min(int a, int b);

unsigned ecvt(double v, char *buffer, int width, int prec, int flag);
unsigned fcvt(double v, char *buffer, int width, int prec, int flag);
unsigned gcvt(double v, char *buffer, int width, int prec, int flag);
unsigned ucvt(unsigned long v, char *buffer, int base, int width, int flag);
unsigned dcvt(long v, char *buffer, int base, int width, int flag);

#define atof(s)	strtod(s, NULL)
#define atoi(s)	strtoi(s, 10, NULL)
#define atou(s)	strtou(s, 10, NULL)
#define atol(s) strtol(s, 10, NULL)

void itoa(int n, unsigned char s[]);

long strtoi(char *s,int base,char **scan_end);
double strtod(char *s,char **scan_end);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);

#endif
