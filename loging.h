/*		This program is free software; you can redistribute it
 *		and/or  modify it under  the terms of  the GNU General
 *		Public  License as  published  by  the  Free  Software
 *		Foundation;  either  version 2 of the License, or  (at
 *		your option) any later version.
 *
 * 		This program is distributed in the hope that it will be useful,
 * 		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *		GNU General Public License for more details.
 */
#ifndef LOGING_H
#define LOGING_H
#include <stdarg.h>

int Printf(int level,char *fmt, ...);
int Printfe(int level,int lci,char *fmt, ...);
void Perror(const char *s);
int openFdlog();
int closeFdlog();


#endif
