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
#ifndef ALL_H
#define ALL_H
#include <pthread.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <linux/types.h>	/* don't move this one ! */
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <syslog.h>
#include <netdb.h>
#include <fcntl.h>
#include <termio.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <ctype.h>

#include <linux/netlink.h>

#include <netinet/tcp.h>
#include <ulimit.h>

/* see include/linux/netlink.h*/
#define NETLINK_TAPBASE 17

#define XOT_PORT 1998
#define XOT_VERSION 0

//#define MAX_PKT_LEN 1030	/* 1024 bytes of data + PLP header */
#define MAX_PKT_LEN 4100	/* 1024 bytes of data + PLP header */
#define MIN_PKT_LEN 3

#define CALL_REQUEST			0x0B
#define CALL_ACCEPT			0x0F
#define CLEAR_REQUEST			0x13
#define CLEAR_CONFIRMATION		0x17
#define RESTART_REQUEST			0xFB
#define RESTART_CONFIRMATION		0xFF

#define RR(pr)		  		(0x01 + (pr << 5))
#define RNR(pr)				(0x05 + (pr << 5))
#define REJ(pr)				(0x09 + (pr << 5))

#define MAXFILENAME 200

/* #define KEEPLCI0 (NOT IMPLEMENTED) */

/*
 * The famous xot header
 *
 */

struct xot_header {
        u_int16_t version;
        u_int16_t length;
};

//x25 header
#pragma pack(push,1)
struct x25_header
{
	uint8_t LGCN:4;
	uint8_t Modulo:2;
	uint8_t Dbit:1;
	uint8_t Qbit:1;

	uint8_t LCI;

	uint8_t  PTI;
};
#pragma pack(pop)

/*
 * Information kept for each xot connection:
 *
 */

struct xot {
	int sock;			/* socket connected to remote */
	int cleared;			/* True if we've sent CLR REQ */
	struct xot_device *device;	/* backpointer to device */
	int lci;
	pthread_t thread;		/* inbound thread id */
	pthread_mutex_t lock;
	int busy;
	int closing;

	struct xot_header head;		/* Should be contig */
	unsigned char call [211];
};


/*
 * Information for each remote xot device we know
 *
 */

struct xot_device {
	int max_addr;		/* Number of addresses for this one */
	struct sockaddr *addr;
	int tap;		/* The x25tap device it talks to. */
	int max_xot;		/* The biggest LCI it can use */
	struct xot **xot;	/* Table of virtual circuits */
	pthread_t thread;	/* Outbound thread id */
	pthread_mutex_t lock;
};

void usage();

//void daemon_start(void);
//void printd (const char *format, ...);
void print_x25 (const char *head, const unsigned char *buf, int len);
//void dump_packet(const unsigned char* pkt, int len,int direction);

struct xot *create_new_xot(int fd, struct sockaddr_in *addr);
struct xot *find_xot_for_packet (struct xot_device *dev,unsigned char *packet,int len);

int create_outbound (struct xot_device *dev);
void create_inbound (struct xot *xot);

void *outbound(void*);
void *inbound(void*);

//static
int  writen(int fd, unsigned char *ptr, int nbytes);
//static
int  readn(int fd, unsigned char *ptr, int nbytes);
//static inline
int get_lci(const unsigned char *packet);
//static
void busy_xot (struct xot *xot);
//static
//void addrbuf(struct sockaddr *sa,char *buf);

void clear_xot(struct xot *xot);
void go_daemon();
void get_myname(char *nm);

#endif
