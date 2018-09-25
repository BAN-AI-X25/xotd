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
extern int isVerbose;
extern pthread_cond_t wait_for_idle;
extern int FNagle;
/*
 * Create a inbound thread for a xot connection
 *
 * ... fix, should cope with failure.
 *
 */

void create_inbound (struct xot *xot) {

	int e;
	
	pthread_attr_t attr;
	
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

	if ((e = pthread_create (&xot->thread, &attr, inbound, xot)) != 0) {
		Printf(0,"pthread_create (in): %s\n", strerror (e));
		/* clean up ... */
		return;
	}
	
	pthread_attr_destroy(&attr);
}

void
clear_xot(struct xot *xot) {
	int len;
	unsigned char tap_packet [6];
	struct xot_device *dev = xot->device;
//    unsigned char *packet = tap_packet + 1;

	//clear:
	//Printf(0,"TCP connection closed (tap inbound), lci=%d.\n", xot->lci);
	Printfe(5,xot->lci,"TCP connection closed (tap in).\n");
	if (xot->cleared != CLEAR_CONFIRMATION) {
		if (xot->cleared == CLEAR_REQUEST) {
			Printfe(5,xot->lci,"send clear confirmation to x.25\n");
			tap_packet [3] = CLEAR_CONFIRMATION;
			len = 4;
		}
		else {
			Printfe(5,xot->lci,"send clear request to x.25\n");
			tap_packet [3] = CLEAR_REQUEST;
			tap_packet [4] = 0;
			tap_packet [5] = 0;
			len = 6;
		}
		tap_packet [0] = 0x00;	/* Data.ind */
		tap_packet [1] = 0x10 + xot->lci / 256;
		tap_packet [2] = xot->lci;
		
		write (xot->device->tap, tap_packet, len);
		//dump_packet(tap_packet, len + 1,TO_TAP);
		Printfe(6,xot->lci,"TCP -> tap %i\n",len);
		hex_dump(8,10,tap_packet,len);
	}


// 	switch (xot->cleared) {
// 		case CLEAR_CONFIRMATION:
// 			/* Nothing to send to X.25 */
// 			break;
// 		case CLEAR_REQUEST:
// 			Printf(3,"send clear confirmation to x.25\n");
// 			tap_packet [3] = CLEAR_CONFIRMATION;
// 			len = 4;
// 			goto send;
// 		
// 		default:
// 			Printf(3,"send clear request to x.25\n");
// 			tap_packet [3] = CLEAR_REQUEST;
// 			tap_packet [4] = 0;
// 			tap_packet [5] = 0;
// 			len = 6;
// 		
// 			send:
// 			tap_packet [0] = 0x00;	/* Data.ind */
// 			tap_packet [1] = 0x10 + xot->lci / 256;
// 			tap_packet [2] = xot->lci;
// 			
// 			write (xot->device->tap, tap_packet, len);
// 			//dump_packet(tap_packet, len + 1,TO_TAP);
// 			Printf(6,"TCP -> tap %i\n",len);
// 			hex_dump(8,10,tap_packet,len);
// 	}

	/* Wait for outbound side to finish work */
	pthread_mutex_lock (&xot->lock);
	
	++xot->closing;
	
	while (xot->busy) {
		Printfe(5,xot->lci,"wait for idle\n");
		pthread_cond_wait (&wait_for_idle, &xot->lock);
	}
	
	pthread_mutex_unlock (&xot->lock);
	pthread_mutex_lock (&dev->lock);
	
	if (dev->xot [xot->lci] == xot) {
		dev->xot [xot->lci] = NULL;
	}
	else {
		Printfe(5,xot->lci,"already zapped (out)\n");
	}
	pthread_mutex_unlock (&dev->lock);

	Printfe(5,xot->lci,"idle\n");
	close (xot->sock);
	pthread_mutex_destroy (&xot->lock);
	free (xot);
	Printfe(3,xot->lci,"inbound done\n");
}

int
connect_transmit(struct xot *xot) {
	struct xot_device *dev = xot->device;

	int sock;
	int len;
	struct sockaddr *a;
	int    on = 1;

	/* It's up to us to make the call */
	for (a = dev->addr; a < dev->addr + dev->max_addr; ++a) {
		sock = socket (a->sa_family, SOCK_STREAM, 0);
		if (sock == -1) {
			Perror("socket:");
			//goto clear;
		}
		if (connect (sock, a, sizeof *a) == 0) goto ok;
		Printf(0,"%s: %s\n",inet_ntoa (((struct sockaddr_in *)a)->sin_addr),strerror (errno));
		close (sock);
	}

	/* all call attempts failed; tell X.25 */
	//goto clear;
	//clear_xot(xot);
	return(0);

	ok:
	if (!FNagle) {
		setsockopt(sock,SOL_TCP,TCP_NODELAY,&on,sizeof on);
	}

	pthread_mutex_lock (&xot->lock);
	Printfe(2,xot->lci,"\t\tconnected to %s\n",inet_ntoa (((struct sockaddr_in *)a)->sin_addr));
	
	if (xot->cleared == CLEAR_REQUEST) {
		/* but X.25 has decided to give up. */
		pthread_mutex_unlock (&xot->lock);
		//goto clear;
		//clear_xot(xot);
		return(0);
	}

	xot->sock = sock;
	pthread_mutex_unlock (&xot->lock);

	/* OK, send out the call packet */
	len = ntohs (xot->head.length) + sizeof xot->head;

	Printfe(6,xot->lci,"\t\twrite %d bytes call request to tcp.\n", len);
	//Printf(6,"\t\tgfi=%02x lcn=%02x pti=%02x\n",xot->call[0], xot->call[1], xot->call[2]);
	
	len = writen (xot->sock, (unsigned char *) &xot->head, len);
	if (len < 0)
		Perror("write to tcp socket error:");
	return(1);
}


/*
 * Send data from TCP to X.25
 *
 * One thread per active X.25 connection.
 *
 */

void *inbound(void *arg) {
	struct xot *xot = arg;
	struct xot_device *dev = xot->device;
	
	int nread, len;
	
	struct xot_header header;
	unsigned char tap_packet [MAX_PKT_LEN + 1];
	unsigned char *packet = tap_packet + 1;

	//block_hup();
	Printfe(5,xot->lci,"Start IN.\n");
	
	if (xot->sock == -1) {//we have Call_Req from tap
		if (connect_transmit(xot) == 0) {
			clear_xot(xot);
			return(NULL);
		}
	}

	*tap_packet = 0x00;	/* Data.ind */
	
	do {
	
		nread = readn (xot->sock, (unsigned char*) &header, sizeof header);
		if (nread != sizeof header) {
			if (nread < 0 && ( errno != EPIPE || errno != ENOTCONN))
				Perror("readn error:");
			break;	/* abort */
		}
	
		len = ntohs (header.length);
	
		if (ntohs (header.version) != XOT_VERSION) {
			Printfe(0,xot->lci,"XOT version not supported: %d\n", ntohs(header.version));
			break;	/* and close the connexion */
		}
	
		if (len == 0) {
			Printfe(0,xot->lci,"Zero length packet received!\n");
			continue;	/* discard it */
		}
	
		/*
		* TODO: replace MAX_PKT_LEN by the X.25 device MTU (packet size)
		*/
		
		if (len > MAX_PKT_LEN || len < MIN_PKT_LEN) {
			Printfe(0,xot->lci,"Invalid packet size: %d\n", len);
			break;	/* and close the connexion */
		}
	
		nread = readn (xot->sock, packet, len);	/* read the remainder */
		Printfe(6,xot->lci,"readed from tcp %i\n",nread);
		hex_dump(8,10,packet,nread);

		if (nread != len)
			if (nread < 0) {
				Perror("readn error:");
			break;
		}
		print_x25 ("TCP -> tap", packet, len);
	
		switch (packet[2]) {
	
		case CLEAR_CONFIRMATION:
			
			pthread_mutex_lock (&dev->lock);
	
			if (dev->xot [xot->lci] == xot) {
				dev->xot[xot->lci] = NULL;
			}
	
			pthread_mutex_unlock (&dev->lock);
			
			xot->cleared = CLEAR_CONFIRMATION;
			break;
	
		case CALL_REQUEST:
			/* Should check he doesn't send 2 calls */

			if ((xot->head.length = len) > sizeof xot->call) {
				xot->head.length = sizeof xot->call;
			}
			memcpy (xot->call, packet, xot->head.length);
		}
	
		/* Check gfi, lci of packet */
		
		if ((packet [0] & 0x3f) != (xot->call[0] & 0x3f) ||
		packet [1] != xot->call [1])
		{
			Printfe(0,xot->lci,"Bad GFI/LCN %02x, %02x\n", packet [0], packet [1]);
			break;
		}
		
		/* Fix LCI to be what we want */
		Printfe(6,xot->lci,"Fix LCI from %02X to %02X\n",get_lci(packet),xot->lci);	
		packet [0] = (packet [0] & 0xf0) + xot->lci / 256;
		packet [1] = xot->lci;
		Printfe(6,xot->lci,"write to tap %i\n",nread + 1);

		if (write (dev->tap, tap_packet, nread + 1) != nread + 1) {
			Perror("Tap write error:");
			break;
		}

		//struct x25_header *x25h = (struct x25_header *)packet;
		//Printf(3,"sizeof %i\n",sizeof(struct x25_header));
		//Printf(3,"Qbit:%02X Dbit:%02X Modulo:%02X LGCN:%02X Lci:%02X PTI:%02X\n",x25h->Qbit,x25h->Dbit,x25h->Modulo,x25h->LGCN,x25h->LCI,x25h->PTI);

		//dump_packet(tap_packet, nread +1,TO_TAP);
		/* If we get a clear confirm from remote then we can hang up */
	}
	while (xot->cleared != CLEAR_CONFIRMATION);
	
	clear_xot(xot);

// clear:
// //    if (isVerbose)
// 	Printf(0,"TCP connection closed (tap inbound), lci=%d.\n", xot->lci);
// 
//     switch (xot->cleared) {
// 	case CLEAR_CONFIRMATION:
// 	    /* Nothing to send to X.25 */
// 	    break;
// 	case CLEAR_REQUEST:
// 	    Printf(3,"send clear confirmation to x.25\n");
// 	    packet [2] = CLEAR_CONFIRMATION;
// 	    len = 3;
// 	    goto send;
// 	    
// 	default:
// 	    Printf(3,"send clear request to x.25\n");
// 	    packet [2] = CLEAR_REQUEST;
// 	    packet [3] = 0;
// 	    packet [4] = 0;
// 	    len = 5;
// 
// 	send:
// 	    packet [0] = 0x10 + xot->lci / 256;
// 	    packet [1] = xot->lci;
// 	    
// 	    write (xot->device->tap, tap_packet, len + 1);
// 	    //dump_packet(tap_packet, len + 1,TO_TAP);
// 		Printf(6,"TCP -> tap %i\n",len + 1);
// 		hex_dump(8,10,tap_packet,len + 1);
// 
//     }
// 
//     /* Wait for outbound side to finish work */
//     
//     pthread_mutex_lock (&xot->lock);
//     ++xot->closing;
// 
//     while (xot->busy) {
// 	    Printf(10,"wait for idle\n");
// 	    pthread_cond_wait (&wait_for_idle, &xot->lock);
//     }
// 
//     pthread_mutex_unlock (&xot->lock);
//     pthread_mutex_lock (&dev->lock);
// 
//     if (dev->xot [xot->lci] == xot) {
// 	    dev->xot [xot->lci] = NULL;
//     }
//     else {
// 	    Printf(10,"already zapped (out)\n");
//     }
// 
//     pthread_mutex_unlock (&dev->lock);
//     Printf(10,"idle\n");
//     close (xot->sock);
//     pthread_mutex_destroy (&xot->lock);
//     free (xot);
//     Printf(2,"inbound done\n");

    return NULL;
}

