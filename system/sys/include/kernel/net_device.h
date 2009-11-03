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

#ifndef __F_KERNEL_NET_DEVICE_H__
#define __F_KERNEL_NET_DEVICE_H__

#include <net/nettypes.h>
#include <net/if.h>

struct net_device
{
	const char *name;

	/*
	 *  I/O specific fields
	 *  FIXME: Merge these and struct ifmap into one
	 */
	unsigned long rmem_end; /* shmem "recv" end */
	unsigned long rmem_start; /* shmem "recv" start */
	unsigned long mem_end;  /* shared mem end */
	unsigned long mem_start;  /* shared mem start */
	unsigned long base_addr;  /* device I/O address   */
	unsigned int irq;        /* device IRQ number    */

	/* Low-level status flags. */
	volatile unsigned char start;      /* start an operation   */

	/*
	 * These two are just single-bit flags, but due to atomicity
	 * reasons they have to be inside a "unsigned long". However,
	 * they should be inside the SAME unsigned long instead of
	 * this wasteful use of memory..
	 */
	unsigned char if_port;		/* Selectable AUI, TP,..*/
	unsigned char dma;			/* DMA channel    */

	unsigned long interrupt;	/* bitops.. */
	unsigned long tbusy;		/* transmitter busy */

	struct net_device *next;

	/*
	 * This marks the end of the "visible" part of the structure. All
	 * fields hereafter are internal to the system, and may change at
	 * will (read: may be cleaned up at will).
	 */

	/* These may be needed for future network-power-down code. */
	unsigned long trans_start;		/* Time (in jiffies) of last Tx */
	unsigned long last_rx;			/* Time of last Rx      */
	unsigned mtu;					/* interface MTU value    */
	unsigned short type;			/* interface hardware type  */
	unsigned short hard_header_len;	/* hardware hdr length  */   
	unsigned short flags;			/* interface flags (a la BSD) */

	void *priv;	/* pointer to private data  */

	/* Interface address info. */
	unsigned char broadcast[IFHWADDRLEN];	/* hw bcast add */
	unsigned char pad;						/* make dev_addr aligned to 8 bytes */
	unsigned char dev_addr[IFHWADDRLEN];	/* hw address */
	unsigned char perm_addr[IFHWADDRLEN];
	unsigned char addr_len;					/* hardware address length  */    

	/* Pointers to interface service routines */
	int (*hard_start_xmit) (PacketBuf_s *skb, struct net_device *dev);

	int mc_count;	/* Number of installed mcasts	*/

	NetQueue_s* packet_queue;
	int irq_handle;		/* IRQ handler handle */
	int device_handle;	/* device handle from probing */
	int node_handle; 	/* handle of device node in /dev */
};

struct net_device_stats
{
	unsigned long	rx_packets;		/* total packets received	*/
	unsigned long	tx_packets;		/* total packets transmitted	*/
	unsigned long	rx_bytes;		/* total bytes received 	*/
	unsigned long	tx_bytes;		/* total bytes transmitted	*/
	unsigned long	rx_errors;		/* bad packets received		*/
	unsigned long	tx_errors;		/* packet transmit problems	*/
	unsigned long	rx_dropped;		/* no space in linux buffers	*/
	unsigned long	tx_dropped;		/* no space available in linux	*/
	unsigned long	multicast;		/* multicast packets received	*/
	unsigned long	collisions;

	/* detailed rx_errors: */
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;		/* receiver ring buff overflow	*/
	unsigned long	rx_crc_errors;		/* recved pkt with crc error	*/
	unsigned long	rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long	rx_fifo_errors;		/* recv'r fifo overrun		*/
	unsigned long	rx_missed_errors;	/* receiver missed packet	*/

	/* detailed tx_errors */
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	
	/* for cslip etc */
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};

/* Get the private data from the net_device */
#define netdev_priv(dev) \
	(dev)->priv

#endif	/* __F_KERNEL_NET_DEVICE__H__ */
