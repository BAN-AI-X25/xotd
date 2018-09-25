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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "all.h"
#include "leave_pid.h"
#include "config.h"

char *Name_log_file;
char *Name_cfg_file;
char *Bindto;
#define DEF_NAME_LOG "/var/log/xotd.log"
#define DEF_NAME_CFG "/etc/pad_svr/xotd.cfg"

pthread_cond_t wait_for_idle;
int Max_devices;
struct xot_device *Devices;

int Lport = XOT_PORT;
int Rport = XOT_PORT;

int Log_level = 2;

int Flag_Logopnd = 0;
char *Myname;
int Flag_Hup = 0;
int FNagle = 1;
extern int Flag_Hup;

void
set_defaults(void) {
	Name_log_file = malloc(MAXFILENAME);
	Name_cfg_file = malloc(MAXFILENAME);
	if (Name_cfg_file == NULL || Name_log_file == NULL) {
		Perror("Unable alloc memory:");
		exit(1);
	}
	strcpy(Name_log_file,DEF_NAME_LOG);
	strcpy(Name_cfg_file,DEF_NAME_CFG);
	Bindto = NULL;
}

void
open_sock(struct sockaddr_in *paddr,int *psock) {
	int    on = 1;

	if ((*psock = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
		Perror("Error creating socket: %s");
		exit(2);
	}

	setsockopt(*psock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
	if (!FNagle) {
		setsockopt(*psock,SOL_TCP,TCP_NODELAY,&on,sizeof on);
	}


	memset(paddr, 0, sizeof(*paddr));

	paddr->sin_family = AF_INET;
	paddr->sin_port = htons (Lport);

	if (Bindto != NULL) {
		if(inet_aton(Bindto,&(paddr->sin_addr)) == 0) {
		Perror("Error with bind address");
		exit(2);
		}
	} else {
		paddr->sin_addr.s_addr = htonl (INADDR_ANY);
	}

	if (bind (*psock, (struct sockaddr *) paddr, sizeof(*paddr)) == -1) {
		Perror("Error binding socket: %s");
		close (*psock);
		exit(2);
	}

	if (listen (*psock, 8) == -1) {
		Perror("Error listening socket: %s");
		close (*psock);
		exit(2);
	}

}

void
start_out_threads() {
	int    unit=0;

	struct xot_device *dev;
	unit = 0;
	for (dev = Devices; dev < Devices + Max_devices; ++dev) {
		if (create_outbound (dev)) ++unit;
	}
	if (!unit)
		 exit(2);
}

void
get_myname(char *nm) {
	asprintf(&Myname,"%s",nm);
	char *mn;
	if ( (mn = strrchr(Myname,'/')) != NULL) {
		Myname = mn + 1;
	}
}

int main(int argc, char *argv[]) {
	int    sock;
	struct sockaddr_in addr;
	char   c;
//Set ulimits--------
// 	struct rlimit rli;
// 
// // 	getrlimit(,&rli);
// // 	Printf(1,"was %i %i\n",rli.rlim_cur,rli.rlim_max);
// 
// 	getrlimit(RLIMIT_STACK,&rli);
// 	Printf(1,"was %i %i\n",rli.rlim_cur,rli.rlim_max);
// 
// //	rli.rlim_cur = 20971520;//5242880;
// 	rli.rlim_max = 20971520;//5242880;//RLIM_INFINITY;
// 	int kod = setrlimit(RLIMIT_STACK,&rli);
// 	if (kod != 0) {
// 		Printf(1,"setrlimit error\n");
// 	}
// 
// 	getrlimit(RLIMIT_STACK,&rli);
// 	Printf(1,"will %i %i\n",rli.rlim_cur,rli.rlim_max);

//end Set ulimits----

	get_myname(argv[0]);

	if (exist_pid() != 0) {
		printf("Process %s already running.\n",Myname);
		_exit(1);
	}

	set_defaults();
	gopt(argc,argv);
	read_cfg();
	openFdlog();
	Printf(1,"Starting %s...\n",Myname);
	print_cfg();

	if (Max_devices == 0) {
		print_usage ();
		exit(1);
	}
	//printf("log lvl %i\n",Log_level);
	/* Let's become a daemon */
	//daemon_start();
	go_daemon();
	setsignal();
	leave_pid();
	atexit(clear_pid);

	pthread_cond_init (&wait_for_idle, NULL);

	/* Make socket for incoming XOT calls */
	open_sock(&addr,&sock);

	/* Start all the outbound threads, copy x.25 -> tcp */
	start_out_threads();

	Printf(5,"Waiting for connections.\n");

	//unblock_hup();

	for (;;) {

		socklen_t len = sizeof addr;
		int fd;
		struct xot *xot;

		unblock_hup();
		fd = accept (sock, (struct sockaddr *) &addr, &len);
		block_hup();
		if (Flag_Hup == 1) {
			Flag_Hup = 0;
			//Flag_Logopnd = 0;
			closeFdlog();
			openFdlog();
		}

		if (fd == -1) {
			if (errno == EINTR) {
// 				if (Flag_Hup == 1) {
// 					Flag_Hup = 0;
// 					//Flag_Logopnd = 0;
// 					closeFdlog();
// 					openFdlog();
// 				}
				continue;
			}
			else {
				Perror("accept error:");
				exit (1);
			}
		}

		Printf(5,"Accept call\n");
		if (!(xot = create_new_xot(fd, &addr))) {
			close (fd);
			continue;
		}
		create_inbound (xot);
	}
	exit(0);
}

/*
 * Detach a daemon process from login session context.
 */
// void daemon_start(void)
// {
//
// 	if (fork())
// 		exit(0);
// 	chdir("/");
// 	umask(0);
// 	close(0);
// 	close(1);
// 	close(2);
// 	open("/", O_RDONLY);
// 	dup2(0, 1);
// 	dup2(0, 2);
// 	setsid();
// }

void
go_daemon() {
        pid_t pid, sid;

        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(1);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(0);
        }
        umask(0);

        /* Open any logs here */
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                Perror("Create new session and group:");
                exit(1);
        }
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
		Perror("Change current dir:");
                exit(1);
        }
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
	int nullfd = 99;
	nullfd = open("/dev/null", O_RDWR);
	if (nullfd < 0) {
		Perror("Cant open /dev/null (but it very needed) :");
		exit(1);
	}
	dup2(nullfd, STDIN_FILENO);
	dup2(nullfd, STDOUT_FILENO);
	dup2(nullfd, STDERR_FILENO);

	//printf("go daemon %i.",nullfd);
}


