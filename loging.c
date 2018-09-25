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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
// #define _XOPEN_SOURCE 500
// #include <unistd.h>
#include <unistd.h>

int stat(const char *file_name, struct stat *buf);
int fstat(int filedes, struct stat *buf);

#include "loging.h"
extern char *Name_log_file;

extern int Log_level;
static FILE *Filelog;
static int Fdlog;
extern int Flag_Logopnd;


int openFdlog() {
	setvbuf (stdout, NULL, _IOLBF, 1000);

	Printf(1,"Open log file %s\n",Name_log_file);
//	if ((Fdlog = open(Name_log_file,O_WRONLY | O_APPEND | O_CREAT,00666)) < 0) {
	if ((Filelog = fopen(Name_log_file,"a")) == NULL) {
		Perror("Cannot open logfile.");
		exit(1);
	}

	Fdlog = fileno(Filelog);

// 	if ( (Filelog = fdopen(Fdlog,"a")) == NULL) {
// 		Perror("Cannot open stream to logfile.");
// 		exit(1);
// 	}

	setvbuf (Filelog, NULL, _IOLBF, 1000);

	Flag_Logopnd = 1;
	Printf(1,"Open log file %s\n",Name_log_file);

}

int closeFdlog() {
	Printf(1,"Close log file \n");

	if (Flag_Logopnd) {
		if (fclose(Filelog) != 0) {
//		if (close(Fdlog) != 0) {
			Perror("Cannot close logfile.");
			exit(1);
		}
		Flag_Logopnd = 0;
	}
}

int Printfe(int level,int lci,char *fmt, ...) {
	pid_t pid = getpid();
	va_list ap;
	va_start(ap, fmt);
	//char sbuf[1000];
// 	if (Flag_Hup) {
// 		//freopen(Name_log_file,"a",Filelog);
// 		Flag_Hup = 0;
// 		Flag_Logopnd = 0;
// 		closeFdlog();
// 		openFdlog();
// 	}

	if (Log_level >= level) {
		char tistr[100];
		int c;
		time_t ti;
		time_t ct = time(0);
		struct tm tim;
		localtime_r(&ct,&tim);
	
		c = strftime (tistr,100,"%m/%d %H:%M:%S ",&tim);
		sprintf(tistr + c,"%i %03X %s",pid,lci,fmt);
		if (Flag_Logopnd) {
			vfprintf(Filelog,tistr,ap);
		}
		else {
			vprintf(fmt,ap);
		}
	}
	va_end(ap);
}

int Printf(int level,char *fmt, ...) {
	pid_t pid = getpid();
	va_list ap;
	va_start(ap, fmt);
	//char sbuf[1000];
// 	if (Flag_Hup) {
// 		//freopen(Name_log_file,"a",Filelog);
// 		Flag_Hup = 0;
// 		Flag_Logopnd = 0;
// 		closeFdlog();
// 		openFdlog();
// 	}

	if (Log_level >= level) {
		char tistr[100];
		int c;
		time_t ti;
		time_t ct = time(0);
		struct tm tim;
		localtime_r(&ct,&tim);
	
		c = strftime (tistr,100,"%m/%d %H:%M:%S ",&tim);
		sprintf(tistr + c,"%i %s",pid,fmt);
		if (Flag_Logopnd) {
			//fprintf(Filelog,"%s %i ",tistr,pid);
			//vfprintf(Filelog,fmt,ap);
			vfprintf(Filelog,tistr,ap);

//			fflush(Filelog);
			//fdatasync(Fdlog);

// 			vsprintf(sbuf + c,fmt,ap);
// 			c = sprintf(sbuf,"%s %i ",tistr,pid);
// 			vsprintf(sbuf + c,fmt,ap);
// 			pthread_mutex_lock(&Poslock);
// 			Fdpos += strlen(sbuf);
// 			pthread_mutex_unlock(&Poslock);
// 			//write(Fdlog,sbuf,strlen(sbuf));
// 			pwrite(Fdlog,sbuf,strlen(sbuf),Fdpos);

		}
		else {
			//printf("%s %i ",tistr,pid);
			vprintf(fmt,ap);
		}
	}
	va_end(ap);
}

void Perror(const char *s) {

	Printf(0,"%s %s\n",s,strerror(errno));
}
