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
#include "loging.h"

/*
 * Create a new outbound thread, copy x.25 -> tcp
 *
 */

int create_outbound (struct xot_device *dev) {

	int unit;
	struct sockaddr_nl addr;
	int e;

	unit = dev->tap + NETLINK_TAPBASE;

	if ((dev->tap = socket (AF_NETLINK, SOCK_RAW, unit)) == -1) {
		Perror("Error creating netlink socket:");
		return 0;
	}

	memset(&addr, 0, sizeof addr);
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = 1;

	if (bind (dev->tap, (struct sockaddr *) &addr,sizeof addr) == -1) {
		Perror("Error binding netlink socket:");
		close (dev->tap);
		return 0;
	}

	pthread_mutex_init (&dev->lock, NULL);

	if ((e = pthread_create (&dev->thread, NULL, outbound, dev)) != 0) {
		Printf(0,"pthread_create (outbound): %s\n", strerror (e));
		close (dev->tap);
		return 0;
	}

	return 1;
}

/*
 * The outbound thread, read from Linux X.25 and send out
 * to remote xot devices over TCP
 *
 * One thread per local xot device
 *
 */

void *outbound(void *arg)
{
	struct xot_device *dev = arg;

	int nread;
	int nwriten;
	/* Many ways of addressing same data.  UGERLY */

	unsigned char full_packet [sizeof (struct xot_header) + MAX_PKT_LEN];
	struct xot_header *header = (struct xot_header *) full_packet;
	unsigned char *packet = full_packet + sizeof *header;
	unsigned char *tap_packet = packet - 1;

	//block_hup();
	Printf(5,"Start OUT...\n");

//	char buf[] = {0x00  ,0x13 ,0xFE ,0x0B ,0x05 ,0x35 ,0x07 ,0x10 ,0x06 ,0x42 ,0x08 ,0x08 ,0x43 ,0x07 ,0x07 ,0x01 ,0x00 ,0x00 ,0x00};
	char buf[] = {0x00  ,0x10 ,0x00 ,0xFB ,0x03 ,0x03};

	Printf(1,"WRITE to tap \n");
	hex_dump(8,10,buf,sizeof(buf));
	write(dev->tap,buf,sizeof(buf));

//-------------------

	for (;;) {
		struct xot *xot;

		nread = read (dev->tap, tap_packet, MAX_PKT_LEN + 1);
		//struct x25_header *x25h = (struct x25_header *)packet;
		//Printf(3,"out Qbit:%02X Dbit:%02X Modulo:%02X LGCN:%02X Lci:%02X PTI:%02X\n",x25h->Qbit,x25h->Dbit,x25h->Modulo,x25h->LGCN,x25h->LCI,x25h->PTI);

		Printf(6,"\t\treaded from tap %i\n",nread-1);
		hex_dump(8,10,packet,nread-1);

		if (nread < 0) {
			Perror("read from tap error:");
			//break;
			continue;
		}
		if (nread == 0)		/* strange, ignore it!? */
			continue;
		--nread;	/* Ignore NETLINK byte */

		if (*tap_packet == 0x00) {/* data request */
			if (nread < MIN_PKT_LEN)	/* invalid packet size ? */
				//break;
				continue;

			print_x25 ("\t\ttap -> TCP", packet, nread);
			if (!(xot = find_xot_for_packet (dev, packet, nread))) {
				//new xot created
				continue;
			}
			header->version = htons (XOT_VERSION);
			header->length = htons (nread);

			nread += sizeof header;

			Printfe(6,xot->lci,"\t\twrite to TCP %i\n",nread);

			/* Don't care if write fails, read should fail too
				and that'll tell X.25 for us */
			nwriten = writen (xot->sock, (unsigned char *) header, nread);

			if (packet [2]  == CLEAR_CONFIRMATION) {
				/* After this packet this vc is available for use */
				Printfe(5,xot->lci,"\t\ttap->TCP: CLEAR_CONF, out done\n");
				/* This should kill the inbound side */
				shutdown (xot->sock, SHUT_RDWR);
				/* We should not send on this xot */

				pthread_mutex_lock (&dev->lock);
				if (dev->xot [xot->lci] == xot) {
					dev->xot [xot->lci] = NULL;
				}
				else {
					Printfe(5,xot->lci,"\t\talready zapped (in)\n");
				}
				pthread_mutex_unlock (&dev->lock);
			}

			idle_xot (xot);
		}
		else if (*tap_packet == 0x01) { /* Connection request, send back ACK */
			Printf(5,"\t\ttap -> TCP: [conn.req], %d data bytes\n", nread);
			//*tap_packet = 0x01;
			if (write(dev->tap, tap_packet, 1) != 1) {
				Perror("write error:");
				return NULL;
			}
			//dump_packet(tap_packet,1,TO_TAP);
			Printf(5,"xotd -> tap %i\n",1);
			hex_dump(8,10,tap_packet,1);
		}
		else if (*tap_packet == 0x02) { /* Disconnect request */
			Printf(5,"\t\ttap -> TCP: [clr.req], %d data bytes\n", nread);
			//*tap_packet = 0x02;
			if (write(dev->tap, tap_packet, 1) != 1) {
				Perror("write error:");
				return NULL;
			}
			//dump_packet(tap_packet,1,TO_TAP);
			Printf(5,"xotd -> tap %i\n",1);
			hex_dump(8,10,tap_packet,1);
		}
		else if (*tap_packet == 0x03) {
			Printf(5,"\t\ttap -> TCP: [param], %d data bytes\n", nread);
			Printf(5,"changing parameters not supported\n");
		}
		else {
			Printf(0,"read from tap: unknown command %#x\n",*tap_packet);
			break;
		}//end switch
	}//end for

	Printf(5,"exiting tap outbound!\n");
	close (dev->tap);
	return NULL;
}

