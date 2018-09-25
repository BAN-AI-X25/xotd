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

extern int Max_devices;
extern struct xot_device *Devices;

/*
 * Find a xot to handle an incoming call
 *
 */
/* xot must be locked before use */

//static 
void busy_xot (struct xot *xot) {
	++xot->busy;
	Printfe(8,xot->lci,"\t\tbusy (%d)\n", xot->busy);
}
	

struct xot_device *
find_dev_on_ip (struct sockaddr_in *addr) {
	struct xot_device *dev;

	for (dev = Devices; dev < Devices + Max_devices; ++dev) {
		struct sockaddr *a;

		for (a = dev->addr; a < dev->addr + dev->max_addr; ++a) {
			if (((struct sockaddr_in *)a)->sin_addr.s_addr == addr->sin_addr.s_addr)
				return(dev);
				//goto found_device;
		}
	}
	return(NULL);
}

int
find_free_xot(struct xot_device *dev) {
	int lci;
	// Fixme - doesn't allow LCI0  Fix it:
//	for (lci = dev->max_xot - 1;lci; --lci) {
	for (lci = dev->max_xot - 1;lci > 1; --lci) {
		if (!dev->xot [lci]) return(lci);
	}
	return(-1);
}

struct xot *create_new_xot (int fd, struct sockaddr_in *addr) {
	struct xot_device *dev;
	struct xot *xot;
	int lci;

	/* First find the device */

	Printf(2,"Call from %s\n", inet_ntoa (addr->sin_addr));

	if ( (dev = find_dev_on_ip(addr)) == NULL) {
		//Printf(3,"Call from %s\n", inet_ntoa (addr->sin_addr));
		//return NULL;
		//Printf(6,"Accept it!\n");
		dev = Devices;
	}
	//found_device:
	/* Now look for a free lci.  Count down like a DCE */

	pthread_mutex_lock (&dev->lock);
	lci = find_free_xot(dev);
	pthread_mutex_unlock (&dev->lock);

	if (lci == -1) {
		Printf(0,"Too many vc's from %s\n", inet_ntoa (addr->sin_addr));
		return NULL;
	}
    	//found_lci:
	//found free xot
	Printfe(3,lci,"Create new xot.\n");

	xot = calloc (sizeof *xot, 1);
	xot->device = dev;
	xot->sock = fd;
	xot->lci = lci;

	pthread_mutex_init (&xot->lock, NULL);
	dev->xot [lci] = xot;
	pthread_mutex_unlock (&dev->lock);
	
	return xot;
}

void 
force_clear(int tap,unsigned char *tap_packet) {
	//unsigned char *tap_packet = packet - 1;

	Printf(5,"fake clear\n");

	tap_packet[3] = CLEAR_REQUEST;
	tap_packet[4] = 0x05;
	tap_packet[5] = 0;
	
	write (tap, tap_packet, 1 + 5);
	//dump_packet(tap_packet,6,TO_TAP);
	Printf(5,"xotd -> tap %i\n",6);
	hex_dump(8,10,tap_packet,6);
}

/*
 * Find the xot connection for an outbound packet
 *
 */
struct xot *find_xot_for_packet (struct xot_device *dev,unsigned char *packet,int len) {
	struct xot *xot;
	unsigned char *tap_packet = packet - 1;
	
	int lci = get_lci (packet);
	Printfe(6,lci,"\t\tfind xot for packet from tap\n");
	//force_clear0(dev->tap,tap_packet);

	if (lci == 0) {/* ignore this one, see 6.4 in RFC */
		if (packet[2] == RESTART_REQUEST) {
			packet [2] = RESTART_CONFIRMATION;
			write (dev->tap, packet - 1, 1 + 3);
			//dump_packet(packet,4,TO_TAP);
			Printfe(6,lci,"xotd -> tap %i\n",4);
			hex_dump(8,10,packet,4);

		}
		Printf(0,"Drop packet, lci=0\n");
		return NULL;
	}

	if (lci >= dev->max_xot) {
		Printfe(0,lci,"Bad lci - max %d\n", dev->max_xot);
		//goto force_clear;
		force_clear(dev->tap,tap_packet);
		return(NULL);
	}

	pthread_mutex_lock (&dev->lock);
		
	if (!(xot = dev->xot [lci])) {
		/* Not connected */

		switch (packet [2]) {
		    case CLEAR_CONFIRMATION:
			/* discard the message */
			pthread_mutex_unlock (&dev->lock);
			return NULL;

		    default:
			pthread_mutex_unlock (&dev->lock);
			Printfe(0,lci,"no connection and packet not CALL\n");
			//goto force_clear;
			force_clear(dev->tap,tap_packet);
			return(NULL);

		    case CALL_REQUEST:
			break;	
			/* All is good, make the call */
		}
		Printfe(5,lci,"\t\tCreate new xot.\n");

		xot = dev->xot [lci] = calloc (sizeof *xot, 1);
		xot->device = dev;
		xot->sock = -1;
		xot->lci = lci;

		pthread_mutex_init (&xot->lock, NULL);
		pthread_mutex_unlock (&dev->lock);

		/* Save call packet 'till connected to remote */
		if (len > sizeof xot->call) {
			len =  sizeof xot->call;
		}
		memcpy (xot->call, packet, len);
		xot->head.length = htons (len);
		xot->head.version = htons (XOT_VERSION);

		/* Create thread for TCP->X.25, it'll make the call */
		create_inbound (xot);
		return NULL;	/* send call after connected */
	}
	
	//xot with lci exist
	/* DANGER - locked device, then xot - always in that order! */
	
	pthread_mutex_lock (&xot->lock);
	pthread_mutex_unlock (&dev->lock);

	if (xot->sock == -1) {
		/* Not yet connected, only legal thing is CLEAR REQUEST */
		xot->cleared = packet [2];
		pthread_mutex_unlock (&xot->lock);
		return NULL;
	}

	if (xot->closing) {
		/* It's being closed */
		pthread_mutex_unlock (&xot->lock);
		return NULL;
	}

	busy_xot (xot);
	pthread_mutex_unlock (&xot->lock);
			
	if (packet [2] == CALL_REQUEST) {
		Printf(0,"call request on active channel\n");
		idle_xot (xot);
		//goto force_clear;
		force_clear(dev->tap,tap_packet);
		return(NULL);
	}
		
	xot->cleared = packet [2];

	/* Copy GFI, LCI from call packet */
	packet [0] = (packet [0] & 0xa0) | (xot->call [0] & 0x3f); 
	packet [1] = xot->call [1];
	return xot;
}



