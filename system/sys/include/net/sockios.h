/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_NET_SIOCTL_H__
#define __F_NET_SIOCTL_H__

/* Socket-level I/O control calls. */

#define SIO_FIRST_SOCK	0x89000000
#define SIO_LAST_SOCK	0x890fffff

#define FIOSETOWN 	0x89000001
#define SIOCSPGRP	0x89000002
#define FIOGETOWN	0x89000003
#define SIOCGPGRP	0x89000004
#define SIOCATMARK	0x89000005
#define SIOCGSTAMP	0x89000006		/* Get stamp */


/* Routing table calls.  */
#define SIO_FIRST_ROUTE	0x89100000
#define SIO_LAST_ROUTE	0x891fffff
#define SIOCADDRT	0x8910000B		/* add routing table entry	*/
#define SIOCDELRT	0x8910000C		/* delete routing table entry	*/
#define SIOCRTMSG	0x8910000D		/* call to routing system	*/
#define SIOCGETRTAB	0x8910000E		/* get routing table		*/

/* Socket configuration controls. */

/* "global" interface commands */
#define SIO_FIRST_IF		0x89300000
#define SIO_LAST_IF		0x893fffff

#define SIOC_ETH_START		0x89370000
#define SIOC_ETH_STOP		0x89370001

#define SIOCGIFNAME		0x89300000		/* get iface name		*/
#define SIOCGIFINDEX		0x89300001		/* name -> if_index mapping	*/
#define SIOGIFINDEX		SIOCGIFINDEX		/* misprint compatibility :-)	*/
#define SIOCSIFLINK		0x89300002		/* set iface channel		*/
#define SIOCGIFCONF		0x89300003		/* get iface list		*/
#define SIOCGIFCOUNT		0x89300004		/* get number of devices */

/* Commands handled by each interface	*/
#define SIOCGIFFLAGS		0x89300013		/* get flags			*/
#define SIOCSIFFLAGS		0x89300014		/* set flags			*/
#define SIOCGIFADDR		0x89300015		/* get PA address		*/
#define SIOCSIFADDR		0x89300016		/* set PA address		*/
#define SIOCGIFDSTADDR		0x89300017		/* get remote PA address	*/
#define SIOCSIFDSTADDR		0x89300018		/* set remote PA address	*/
#define SIOCGIFBRDADDR		0x89300019		/* get broadcast PA address	*/
#define SIOCSIFBRDADDR		0x8930001a		/* set broadcast PA address	*/
#define SIOCGIFNETMASK		0x8930001b		/* get network PA mask		*/
#define SIOCSIFNETMASK		0x8930001c		/* set network PA mask		*/
#define SIOCGIFMETRIC		0x8930001d		/* get metric			*/
#define SIOCSIFMETRIC		0x8930001e		/* set metric			*/
#define SIOCGIFMEM		0x8930001f		/* get memory address (BSD)	*/
#define SIOCSIFMEM		0x89300020		/* set memory address (BSD)	*/
#define SIOCGIFMTU		0x89300021		/* get MTU size			*/
#define SIOCSIFMTU		0x89300022		/* set MTU size			*/
#define	SIOCSIFHWADDR		0x89300024		/* set hardware address 	*/
#define SIOCGIFENCAP		0x89300025		/* get encapsulations       	*/
#define SIOCSIFENCAP		0x89300026		/* set encapsulations       	*/
#define SIOCGIFHWADDR		0x89300027		/* Get hardware address		*/
#define SIOCGIFSLAVE		0x89300029		/* Driver slaving support	*/
#define SIOCSIFSLAVE		0x89300030
#define SIOCADDMULTI		0x89300031		/* Multicast address lists	*/
#define SIOCDELMULTI		0x89300032
#define SIOCSIFPFLAGS		0x89300034		/* set/get extended flags set	*/
#define SIOCGIFPFLAGS		0x89300035
#define SIOCDIFADDR		0x89300036		/* delete PA address		*/
#define	SIOCSIFHWBROADCAST	0x89300037		/* set hardware broadcast addr	*/

#define SIOCGIFBR		0x89300040		/* Bridging support		*/
#define SIOCSIFBR		0x89300041		/* Set bridging options 	*/

#define SIOCGIFTXQLEN		0x89300042		/* Get the tx queue length	*/
#define SIOCSIFTXQLEN		0x89300043		/* Set the tx queue length 	*/


/* ARP cache control calls. */
#define SIOCDARP	0x89400053		/* delete ARP table entry	*/
#define SIOCGARP	0x89400054		/* get ARP table entry		*/
#define SIOCSARP	0x89400055		/* set ARP table entry		*/

/* RARP cache control calls. */
#define SIOCDRARP	0x89400060		/* delete RARP table entry	*/
#define SIOCGRARP	0x89400061		/* get RARP table entry		*/
#define SIOCSRARP	0x89400062		/* set RARP table entry		*/

#if 1
/* Driver configuration calls */

#define SIOCGIFMAP	0x8970		/* Get device parameters	*/
#define SIOCSIFMAP	0x8971		/* Set device parameters	*/

/* DLCI configuration calls */

#define SIOCADDDLCI	0x8980		/* Create new DLCI device	*/
#define SIOCDELDLCI	0x8981		/* Delete DLCI device		*/

/* Device private ioctl calls.  */

/* These 16 ioctls are available to devices via the do_ioctl() device
   vector.  Each device should include this file and redefine these
   names as their own. Because these are device dependent it is a good
   idea _NOT_ to issue them to random objects and hope.  */

#define SIOCDEVPRIVATE 		0x89F0	/* to 89FF */

/*
 *	These 16 ioctl calls are protocol private
 */

#define SIOCPROTOPRIVATE 0x89E0 /* to 89EF */

#endif /* 0 */

#endif /* __F_NET_SIOCTL_H__ */
