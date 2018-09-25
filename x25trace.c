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
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <linux/x25.h>
#include "x25trace.h"

extern int Log_level;

void hex_dump(int level1,int level2,unsigned char *data, int length)
{
	if (Log_level >= level1) {
		int  i, j, length2;
		unsigned char c;
		unsigned char *data2;
		char sbuf[1000];
		int t = 0;
	
		length2 = length;
		data2   = data;
		int fla1str;
		fla1str = (Log_level >= level2)?0:1;
		//Printf(0,"fla1str : %i\n",fla1str);

		for (i = 0; length > 0; i += 16) {
			//Printf(0,"%04X  ", i);
			t = sprintf(sbuf,"%04X  ", i);
		
			for (j = 0; j < 16; j++) {
				c = *data2++;
				length2--;
				
				if (length2 >= 0)
					//Printfe(0,"%2.2X ", c);
					t += sprintf(sbuf + t,"%2.2X ", c);
				else
					t += sprintf(sbuf + t,"   ");
			}
	
			t += sprintf(sbuf + t," | ");
					
			for (j = 0; j < 16 && length > 0; j++) {
				c = *data++;
				length--;
				
				if (c >= 0x20 && c < 0x7F)
					//putc(c, stdout);
					t += sprintf(sbuf + t,"%c",c);
				else
					//putc('.', stdout);
					t += sprintf(sbuf + t,"%c",'.');
			}
			
			//putc('\n', stdout);
			t += sprintf(sbuf + t,"%c",'\n');
			Printf(0,"%s",sbuf);
			if (fla1str == 1) break;
		}
	}
}
