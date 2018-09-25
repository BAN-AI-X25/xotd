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
#include "x25trace.h"

// void dump_packet(const unsigned char* pkt, int len,int direction)
// {
// 	int i;
// 	if (direction == TO_TAP) {
// 		printf("TAP Wrote ");
// 	} else {
// 		printf("TAP Read  ");
// 	}
// 	for(i=0;i<len;i++) {
// 		printf("%02X ",pkt[i]);
// 	}
// 	printf("\n");
// 
// }


/*
 * print a pretty description of X.25 packet
 *
 */

void print_x25 (const char *head, const unsigned char *packet, int len) {
	int gfi = *packet;
	
	int extended = (gfi & 0x30) == 0x20;

	int lci = get_lci (packet);
	int pti = packet [2];
	
	if (!(pti & 1)) {	/* Data packet */
		int ps = pti >> 1, pr, m;
		if (extended) {
			pr = packet[3];
			m = pr & 1;
			pr >>= 1;
		}
		else {
			ps &= 0x7;
			m = pti & 0x10;
			pr = pti >> 5;
		}
		//Printf(6,"%s lci=%d DATA (ps=%d, pr=%d%s%s%s)\n",head,lci, ps, pr,m ? ", M" : "",(gfi & 0x80) ? ", Q" : "",(gfi & 0x40) ? ". D" : "");
		Printfe(6,lci,"%s DATA (ps=%d, pr=%d%s%s%s)\n",head, ps, pr,m ? ", M" : "",(gfi & 0x80) ? ", Q" : "",(gfi & 0x40) ? ". D" : "");
	}
	else switch (pti) {
	    case RR(0):
		if (extended) {
			//Printf(6,"%s lci=%d RR (pr=%d)\n", head,lci, packet[3] >> 1);
			Printfe(6,lci,"%s RR (pr=%d)\n", head, packet[3] >> 1);
			break;
		}
	    case RR(1): case RR(2): case RR(3):
	    case RR(4): case RR(5): case RR(6): case RR(7):
		//Printf(6,"%s lci=%d RR (pr=%d)\n", head, lci, pti >> 5);
		Printfe(6,lci,"%s RR (pr=%d)\n", head, pti >> 5);
		break;
	    case RNR(0):
		if (extended) {
			//Printf(6,"%s lci=%d RNR(pr=%d)\n", head, lci, packet[3] >> 1);
			Printfe(6,lci,"%s RNR(pr=%d)\n", head, packet[3] >> 1);
			break;
		}
	    case RNR(1): case RNR(2): case RNR(3):
	    case RNR(4): case RNR(5): case RNR(6): case RNR(7):
		//Printf(6,"%s lci=%d RNR (pr=%d)\n", head, lci, pti >> 5);
		Printfe(6,lci,"%s RNR (pr=%d)\n", head, pti >> 5);
		break;
	    case REJ(0):
		if (extended) {
			//Printf(6,"%s lci=%d REJ (pr=%d)\n", head,lci, packet[3] >> 1);
			Printfe(6,lci,"%s REJ (pr=%d)\n", head, packet[3] >> 1);
			break;
		}
	    case REJ(1): case REJ(2): case REJ(3):
	    case REJ(4): case REJ(5): case REJ(6): case REJ(7):
		//Printf(6,"%s lci=%d REJ (pr=%d)\n", head, lci, pti >> 5);
		Printfe(4,lci,"%s REJ (pr=%d)\n", head, pti >> 5);
		break;
	    case CALL_REQUEST:
		//Printf(6,"%s lci=%d CALL REQUEST\n", head, lci);
		Printfe(4,lci,"%s CALL REQUEST\n", head);
		break;
	    case CALL_ACCEPT:
		//Printf(6,"%s lci=%d CALL ACCEPT\n", head, lci);
		Printfe(4,lci,"%s CALL ACCEPT\n", head);
		break;
	    case CLEAR_REQUEST:
		//Printf(6,"%s lci=%d CLEAR REQUEST\n", head, lci);
		Printfe(4,lci,"%s CLEAR REQUEST\n", head);
		break;
	    case CLEAR_CONFIRMATION:
		//Printf(6,"%s lci=%d CLEAR CONFIRMATION\n", head, lci);
		Printfe(4,lci,"%s CLEAR CONFIRMATION\n", head);
		break;
	    case RESTART_REQUEST:
		//Printf(6,"%s lci=%d RESTART REQUEST\n", head, lci);
		Printfe(4,lci,"%s RESTART REQUEST\n", head);
		break;
	    case RESTART_CONFIRMATION:
		//Printf(6,"%s lci=%d RESTART CONFIRMATION\n", head, lci);
		Printfe(4,lci,"%s RESTART CONFIRMATION\n", head);
		break;
	    default:
		//Printf(6,"%s lci=%d pti=0x%02x\n", head, lci, pti);
		Printfe(6,lci,"%s pti=0x%02x\n", head, pti);
	}
}

/*
 * print debug messages
 *
 */
// void printd(const char *format, ...)
// {
// 	va_list ap;
// 
// 	va_start(ap, format);
// #ifndef DEBUG
// 	vsyslog(LOG_INFO,format,ap);
// #else
// 	{
// 		char buf [BUFSIZ];
// 		char *p = buf;
// 		int left = sizeof buf - 1;
// 		int len;
// 
// 		len = snprintf (p, left, "xotd[%d]:", (int) getpid ());
// 		p += len;
// 		left -= len;
// 
// 		if ((len = vsnprintf (p, left, format, ap)) < 0) len = left;
// 		p += len;
// 
// 		*p++ = '\n';
// 
// //		write (fileno (stderr), buf, p - buf);
// 		write (fileno (stdout), buf, p - buf);
// 	}
// #endif
// 	va_end(ap);
// }
// 
