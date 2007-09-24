/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

// eth_syllable.cc  - Syllable TUN device ethernet pktmover
//
// Syllable only has a TUN only driver, which does not add Ethernet framing.
// This module must strip the Ethernet header on every packet written to the
// device, and add a fake packet header to every packet being read.
// eth_tuntap.c does this for MacOS X in a very hacky manner, and I figured
// there are enough differences between TAP on Linux & BSD and TUN on Syllable
// that it would be better to create a seperate Syllable TUN driver instead
// of adding to the mess in eth_tuntap.cc

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE
 
#define NO_DEVICE_INCLUDES
#include "iodev.h"

#if BX_NETWORKING

#include "eth.h"

#define LOG_THIS bx_devices.pluginNE2kDevice->

#include <signal.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <net/tun.h>
#include <net/if.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

#define BX_ETH_SYLLABLE_LOGGING 0

#define ETHER_HDR_LEN 14

// XXXKV: This class *almost* works, but it apparently causes stack corruption
// *somewhere* that I can not find. This will cause Bochs to crash as it attempts
// to reset the ne2k device. ne2k support is disabled in the Bochs recipe, but just
// in case it is re-enabled...

#warning "NE2K TUN SUPPORT FOR SYLLABLE IS CURRENTLY BROKEN! See iodev/eth_syllable.cc"

// If you're bored (You're reading this, right?), please try to debug & fix this
// class so that we can re-enable ne2k TUN support.

int tun_alloc(char *dev);

//
//  Define the class. This is private to this module
//
class bx_syllable_pktmover_c : public eth_pktmover_c {
public:
  bx_syllable_pktmover_c(const char *netif, const char *macaddr,
		     eth_rx_handler_t rxh,
		     void *rxarg, char *script);
  void sendpkt(void *buf, unsigned io_len);
private:
  int fd;
  int rx_timer_index;
  static void rx_timer_handler(void *);
  void rx_timer ();
  Bit8u guest_macaddr[6];
#if BX_ETH_SYLLABLE_LOGGING
  FILE *txlog, *txlog_txt, *rxlog, *rxlog_txt;
#endif
};


//
//  Define the static class that registers the derived pktmover class,
// and allocates one on request.
//
class bx_syllable_locator_c : public eth_locator_c {
public:
  bx_syllable_locator_c(void) : eth_locator_c("syllable") {}
protected:
  eth_pktmover_c *allocate(const char *netif, const char *macaddr,
			   eth_rx_handler_t rxh,
			   void *rxarg, char *script) {
    return (new bx_syllable_pktmover_c(netif, macaddr, rxh, rxarg, script));
  }
} bx_syllable_match;


//
// Define the methods for the bx_syllable_pktmover derived class
//

// the constructor
bx_syllable_pktmover_c::bx_syllable_pktmover_c(const char *netif, 
				       const char *macaddr,
				       eth_rx_handler_t rxh,
				       void *rxarg,
				       char *script)
{
  int flags;
  char intname[IFNAMSIZ]={0};
  strncpy(intname,netif,IFNAMSIZ);
  fd=tun_alloc(intname);
  if (fd < 0) {
    BX_PANIC (("open failed on %s: %s", netif, strerror (errno)));
    return;
  }

  /* set O_ASYNC flag so that we can poll with read() */
  if ((flags = fcntl( fd, F_GETFL)) < 0) {
    BX_PANIC (("getflags on tun device: %s", strerror (errno)));
  }
  flags |= O_NONBLOCK;
  if (fcntl( fd, F_SETFL, flags ) < 0) {
    BX_PANIC (("set tun device flags: %s", strerror (errno)));
  }

  BX_INFO (("eth_syllable: opened %s device", netif));

  /* Execute the configuration script */
  if((script != NULL)
   &&(strcmp(script, "") != 0)
   &&(strcmp(script, "none") != 0)) {
    if (execute_script(script, intname) < 0)
      BX_ERROR (("execute script '%s' on %s failed", script, intname));
    }

  // Start the rx poll 
  this->rx_timer_index = 
    bx_pc_system.register_timer(this, this->rx_timer_handler, 1000,
				1, 1, "eth_syllable"); // continuous, active

  this->rxh   = rxh;
  this->rxarg = rxarg;
  memcpy(&guest_macaddr[0], macaddr, 6);

#if BX_ETH_SYLLABLE_LOGGING
  // Start the rx poll 
  this->rx_timer_index = 
    bx_pc_system.register_timer(this, this->rx_timer_handler, 1000,
				1, 1, "eth_syllable"); // continuous, active
  this->rxh   = rxh;
  this->rxarg = rxarg;
  // eventually Bryce wants txlog to dump in pcap format so that
  // tcpdump -r FILE can read it and interpret packets.
  txlog = fopen ("ne2k-tx.log", "wb");
  if (!txlog) BX_PANIC (("open ne2k-tx.log failed"));
  txlog_txt = fopen ("ne2k-txdump.txt", "wb");
  if (!txlog_txt) BX_PANIC (("open ne2k-txdump.txt failed"));
  fprintf (txlog_txt, "syllable packetmover readable log file\n");
  fprintf (txlog_txt, "net IF = %s\n", netif);
  fprintf (txlog_txt, "MAC address = ");
  for (int i=0; i<6; i++) 
    fprintf (txlog_txt, "%02x%s", 0xff & macaddr[i], i<5?":" : "");
  fprintf (txlog_txt, "\n--\n");
  fflush (txlog_txt);
#endif
}

void
bx_syllable_pktmover_c::sendpkt(void *buf, unsigned io_len)
{
  char *data = ((char*)buf + ETHER_HDR_LEN);
  unsigned int pkt_len = io_len - ETHER_HDR_LEN;
  if (pkt_len > 0) {
    unsigned int size = write (fd, data, pkt_len);
    if (size != pkt_len) {
      BX_PANIC (("write on tun device: %s", strerror (errno)));
    } else {
      BX_DEBUG (("wrote %d bytes on tun (-14 bytes Ethernet header)", pkt_len));
    }
  }

#if BX_ETH_SYLLABLE_LOGGING
  BX_DEBUG (("sendpkt length %u", io_len));
  // dump raw bytes to a file, eventually dump in pcap format so that
  // tcpdump -r FILE can interpret them for us.
  unsigned int n = fwrite (buf, io_len, 1, txlog);
  if (n != 1) BX_ERROR (("fwrite to txlog failed, io_len = %u", io_len));
  // dump packet in hex into an ascii log file
  fprintf (txlog_txt, "NE2K transmitting a packet, length %u\n", io_len);
  Bit8u *charbuf = (Bit8u *)buf;
  for (n=0; n<io_len; n++) {
    if (((n % 16) == 0) && n>0)
      fprintf (txlog_txt, "\n");
    fprintf (txlog_txt, "%02x ", charbuf[n]);
  }
  fprintf (txlog_txt, "\n--\n");
  // flush log so that we see the packets as they arrive w/o buffering
  fflush (txlog);
  fflush (txlog_txt);
#endif
}

void bx_syllable_pktmover_c::rx_timer_handler (void *this_ptr)
{
  bx_syllable_pktmover_c *class_ptr = (bx_syllable_pktmover_c *) this_ptr;
  class_ptr->rx_timer();
}

void bx_syllable_pktmover_c::rx_timer ()
{
  int nbytes;
  Bit8u buf[BX_PACKET_BUFSIZE];
  Bit8u *rxbuf;
  if (fd<0) return;

  // Create a fake Ethernet header
  nbytes = ETHER_HDR_LEN;
  bzero(buf, nbytes);
  buf[0] = buf[6] = 0xFE;
  buf[1] = buf[7] = 0xFD;
  buf[12] = 8;
  nbytes += read (fd, buf+nbytes, sizeof(buf)-nbytes);
  rxbuf=buf;

  // hack: TUN/TAP device likes to create an ethernet header which has
  // the same source and destination address FE:FD:00:00:00:00.
  // Change the dest address to FE:FD:00:00:00:01.
  if (!memcmp(&rxbuf[0], &rxbuf[6], 6)) {
    rxbuf[5] = guest_macaddr[5];
  }

  if (nbytes>ETHER_HDR_LEN)
    BX_DEBUG (("tun read returned %d bytes", nbytes));

  if (nbytes<ETHER_HDR_LEN) {
    if (errno != EAGAIN)
      BX_ERROR (("tun read error: %s", strerror(errno)));
    return;
  }

#if BX_ETH_SYLLABLE_LOGGING
  int io_len = 0;
  Bit8u buf[1];
  bx_syllable_pktmover_c *class_ptr = (bx_syllable_pktmover_c *) this_ptr;
  if (io_len > 0) {
    BX_DEBUG (("receive packet length %u", io_len));
    // dump raw bytes to a file, eventually dump in pcap format so that
    // tcpdump -r FILE can interpret them for us.
    int n = fwrite (buf, io_len, 1, class_ptr->rxlog);
    if (n != 1) BX_ERROR (("fwrite to rxlog failed, io_len = %u", io_len));
    // dump packet in hex into an ascii log file
    fprintf (class_ptr->rxlog_txt, "NE2K transmitting a packet, length %u\n", io_len);
    Bit8u *charbuf = (Bit8u *)buf;
    for (n=0; n<io_len; n++) {
      if (((n % 16) == 0) && n>0)
	fprintf (class_ptr->rxlog_txt, "\n");
      fprintf (class_ptr->rxlog_txt, "%02x ", charbuf[n]);
    }
    fprintf (class_ptr->rxlog_txt, "\n--\n");
    // flush log so that we see the packets as they arrive w/o buffering
    fflush (class_ptr->rxlog);
    fflush (class_ptr->rxlog_txt);
  }
#endif

  BX_DEBUG(("eth_syllable: got packet: %d bytes, dst=%02x:%02x:%02x:%02x:%02x:%02x, src=%02x:%02x:%02x:%02x:%02x:%02x", nbytes, rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3], rxbuf[4], rxbuf[5], rxbuf[6], rxbuf[7], rxbuf[8], rxbuf[9], rxbuf[10], rxbuf[11]));
  if (nbytes < 60) {
    BX_INFO (("packet too short (%d), padding to 60", nbytes));
    nbytes = 60;
  }
  (*rxh)(rxarg, rxbuf, nbytes);
}

#if 0
int tun_alloc(char *dev)
{
  char *ifname;
  int fd, err;

  // split name into device:ifname if applicable, to allow for opening
  // persistent tuntap devices
  for (ifname = dev; *ifname; ifname++) {
	  if (*ifname == ':') {
		  *(ifname++) = '\0';
		  break;
	  }
  }

  if ((fd = open(dev, O_RDWR | O_NONBLOCK)) < 0)
    return -1;
  
  ioctl(fd,TUN_IOC_CREATE_IF,ifname);

  return fd;
}              
#else
int tun_alloc(char *ifname)
{
  int fd, err;

  if ((fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK)) < 0)
    return -1;
  
  ioctl(fd,TUN_IOC_CREATE_IF,ifname);

  return fd;
}              
#endif

#endif /* if BX_NETWORKING */
