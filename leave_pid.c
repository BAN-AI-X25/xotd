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
#define _GNU_SOURCE
#include "leave_pid.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static const char *name_pidfile = "/etc/pad_svr/xotd.pid";

extern char *Myname;

void
leave_pid(){
	clear_pid();

	int pidfd;
	if ((pidfd = open(name_pidfile, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
		Printf(0,"Cant create pid file %s\n",name_pidfile);
		Perror("");
		exit(1);
	}

	char *buf;

	asprintf(&buf,"%i",getpid());
	//printf("i have pid %s\n",buf);
	if (write(pidfd,buf,strlen(buf)) < 0) {
		Perror("Cant write pid to pid file.");
		exit(1);
	}
	free(buf);
	if (close(pidfd) != 0) {
		Perror("Cant close pid file.");
		exit(1);
	}
}

void
clear_pid(){
	if (unlink(name_pidfile) != 0 && errno != ENOENT) {
		Printf(0,"Cant unlink pid file %s\n",name_pidfile);
		Perror("\t");
	}
}

//exit kod 0 - process do not running
int
exist_pid(){
	int pidfd;
	if ((pidfd = open(name_pidfile,O_RDONLY)) < 0) {
		//pid file not exist
		return(0);
	}
	char buf[30];
	int c;
	if ( (c = read(pidfd,buf,sizeof(buf))) < 0) {
		Perror("Cant read pid file.");
		exit(1);
	}
	*(buf + c) = 0;
	//printf("Read prev pid %s\n",buf);
	c = atoi(buf);

	if (close(pidfd) != 0) {
		Perror("Cant close pid file.");
		exit(1);
	}

	char *procnm;

	asprintf(&procnm,"/proc/%s/status",buf);
	//printf("file status :%s\n",procnm);
	FILE *stafile;
	if ((stafile = fopen(procnm,"r")) == NULL) {
		//printf("cant open %s\n",procnm);
		//process with pid not exist
		return(0);
	}
	free(procnm);
	if (fgets(buf,sizeof(buf),stafile) == NULL) {
		//empty file
		return(0);
	}
	fclose(stafile);
	//printf ("readed from pid file %s, c: %i\n",buf,c);
	if (strstr(buf,Myname) == NULL) {
		//pid = other process
		//printf("working other process\n");
		return(0);
	}
	return(c);
}










