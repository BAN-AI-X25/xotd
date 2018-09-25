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
#ifndef CONFIG_H
#define CONFIG_H
#include "all.h"


void print_usage();
void gopt(int argc,char *argv[]);
int is_key(char *line);
void read_cfg (void);
static int unit_of_devname(const char *devname);
void config_device (char *device_name, char *remote_name, char *circuits);
void print_cfg(void);
#endif
