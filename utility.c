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
#include "all.h"
#include "utility.h"
extern pthread_cond_t wait_for_idle;

//static 
// void
// addrbuf (struct sockaddr *sa,char *buf) {// Bad! return * to nothing
// 
// 	switch (sa->sa_family) {
// 	    case AF_INET:
// 		strcpy(buf,inet_ntoa (((struct sockaddr_in *)sa)->sin_addr));
// 		return;
// 	    default:
// 		strcpy(buf,"unknown address family");
// 		return;
// 	}
// }

/* xot MUST NOT be locked before use; asymetry rules */

void idle_xot (struct xot *xot) {

	pthread_mutex_lock (&xot->lock);

	Printfe(10,xot->lci,"\t\tidle (busy = %d, closing = %d)\n", xot->busy, xot->closing);
	
	if (!--xot->busy && xot->closing) {
		pthread_cond_broadcast (&wait_for_idle);
	}

	pthread_mutex_unlock (&xot->lock);
}
    


//static
inline 
int get_lci(const unsigned char *packet) {
	return (packet[0] & 0x0F) * 256 + packet[1];
}

/*
 * Write "n" bytes to a descriptor.
 * Use in place of write() when fd is a stream socket.
 */
//static 
int writen(int fd, unsigned char *ptr, int nbytes)
{
	int	nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0)
			return nwritten;		/* error */
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return nbytes - nleft;
}

/*
 * Read "n" bytes from a descriptor.
 * Use in place of read() when fd is a stream socket.
 */
//static 
int readn(int fd, unsigned char *ptr, int nbytes)
{
	int	nread, nleft;

	if (!ptr)
		return -1;

	nleft = nbytes;
	while (nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread <= 0)
			return nread;		/* error */
		nleft -= nread;
		ptr   += nread;
	}
	return nbytes - nleft;
}

