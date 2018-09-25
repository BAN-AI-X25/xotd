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

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "sighandler.h"
#include "all.h"

extern int Flag_Hup;
static sigset_t Sigset_hup;

void
sig_hup(int signo) {
	//Printf(1,"SigHup\n");
	Flag_Hup = 1;
}

void
setsignal(void) {
	sigemptyset(&Sigset_hup); 
	sigaddset(&Sigset_hup,SIGHUP);
	sigprocmask(SIG_BLOCK,&Sigset_hup,NULL);

	struct	sigaction	act;
	bzero(&act,sizeof(act));
	act.sa_handler = sig_hup;
	sigemptyset(&act.sa_mask);
	//act.sa_flags = SA_RESTART;
	sigaction(SIGHUP,&act,NULL);  /* hangup */

}

void
unblock_hup(void) {
	sigprocmask(SIG_UNBLOCK,&Sigset_hup,NULL);
}

void
block_hup(void) {
	sigprocmask(SIG_BLOCK,&Sigset_hup,NULL);
}

