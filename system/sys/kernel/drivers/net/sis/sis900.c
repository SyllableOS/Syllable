/* sis900.c: A SiS 900/7016 PCI Fast Ethernet driver for Linux.
   Copyright 1999 Silicon Integrated System Corporation 
   Revision:	1.06.12 Dec. 30 2003

   Modified from the driver which is originally written by Donald Becker.
   
   This software may be used and distributed according to the terms
   of the GNU Public License (GPL), incorporated herein by reference.
   Drivers based on this skeleton fall under the GPL and must retain
   the authorship (implicit copyright) notice.
   
   References:
   SiS 7016 Fast Ethernet PCI Bus 10/100 Mbps LAN Controller with OnNow Support,
   preliminary Rev. 1.0 Jan. 14, 1998
   SiS 900 Fast Ethernet PCI Bus 10/100 Mbps LAN Single Chip with OnNow Support,
   preliminary Rev. 1.0 Nov. 10, 1998
   SiS 7014 Single Chip 100BASE-TX/10BASE-T Physical Layer Solution,
   preliminary Rev. 1.0 Jan. 18, 1998
   http://www.sis.com.tw/support/databook.htm

   Rev 1.06.12 Mar. 29 2004 Michael Krueger (invenies@web.de) port to Syllable
   Rev 1.06.11 Apr. 25 2002 Mufasa Yang (mufasa@sis.com.tw) added SiS962 support
   Rev 1.06.10 Dec. 18 2001 Hui-Fen Hsu workaround for EDB & RTL8201 PHY
   Rev 1.06.09 Sep. 28 2001 Hui-Fen Hsu update for 630ET & workaround for ICS1893 PHY
   Rev 1.06.08 Mar.  2 2001 Hui-Fen Hsu (hfhsu@sis.com.tw) some bug fix & 635M/B support
   Rev 1.06.07 Jan.  8 2001 Lei-Chun Chang added RTL8201 PHY support
   Rev 1.06.06 Sep.  6 2000 Lei-Chun Chang added ICS1893 PHY support
   Rev 1.06.05 Aug. 22 2000 Lei-Chun Chang (lcchang@sis.com.tw) modified 630E equalier workaroung rule
   Rev 1.06.03 Dec. 23 1999 Ollie Lho Third release
   Rev 1.06.02 Nov. 23 1999 Ollie Lho bug in mac probing fixed
   Rev 1.06.01 Nov. 16 1999 Ollie Lho CRC calculation provide by Joseph Zbiciak (im14u2c@primenet.com)
   Rev 1.06 Nov. 4 1999 Ollie Lho (ollie@sis.com.tw) Second release
   Rev 1.05.05 Oct. 29 1999 Ollie Lho (ollie@sis.com.tw) Single buffer Tx/Rx
   Chin-Shan Li (lcs@sis.com.tw) Added AMD Am79c901 HomePNA PHY support
   Rev 1.05 Aug. 7 1999 Jim Huang (cmhuang@sis.com.tw) Initial release
*/

/* AtheOS includes */
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <atheos/timer.h>
#include <atheos/pci.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/ctype.h>
#include <atheos/device.h>
#include <atheos/bitops.h>
#include <atheos/linux_compat.h>

#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/signal.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/sockios.h>

#include "sis900.h"

static const char *version =
"sis900.c: v1.06.12  03/29/2004\n";

static int max_interrupt_work = 20;
//static int multicast_filter_limit = 128;

#define sis900_debug debug
static int sis900_debug = 0;

/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (4*HZ)

static DeviceOperations_s g_sDevOps;

PCI_bus_s* g_psBus;
#define psBus g_psBus

struct mac_chip_info {
	const char *name;
	uint16	vendor_id, device_id, flags;
	int	io_size;
};

static struct device * sis900_mac_probe (struct mac_chip_info mac, PCI_Info_s pci_dev,
					 int device_handle, int found_cnt);

static struct mac_chip_info  mac_chip_table[] = {
	{ "SiS 900 PCI Fast Ethernet", PCI_VENDOR_ID_SI, PCI_DEVICE_ID_SI_900,
	  PCI_COMMAND_IO|PCI_COMMAND_MASTER, SIS900_TOTAL_SIZE},
	{ "SiS 7016 PCI Fast Ethernet",PCI_VENDOR_ID_SI, PCI_DEVICE_ID_SI_7016,
	  PCI_COMMAND_IO|PCI_COMMAND_MASTER, SIS900_TOTAL_SIZE},
	{0,},					       /* 0 terminated list. */
};

static void sis900_read_mode(struct device *net_dev, int *speed, int *duplex);

static struct mii_chip_info {
	const char * name;
	uint16 phy_id0;
	uint16 phy_id1;
	uint8  phy_types;
#define	HOME 0x0001
#define LAN	0x0002
#define MIX	0x0003
} mii_chip_table[] = {
	{ "SiS 900 Internal MII PHY", 		0x001d, 0x8000, LAN },
	{ "SiS 7014 Physical Layer Solution", 	0x0016, 0xf830, LAN },
	{ "AMD 79C901 10BASE-T PHY",  		0x0000, 0x6B70, LAN },
	{ "AMD 79C901 HomePNA PHY",		0x0000, 0x6B90, HOME},
	{ "ICS LAN PHY",			0x0015, 0xF440, LAN },
	{ "NS  83851 PHY",			0x2000, 0x5C20, MIX },
	{ "Realtek RTL8201 PHY",		0x0000, 0x8200, LAN },
	{0,},
};

struct mii_phy {
	struct mii_phy * next;
	int phy_addr;
	uint16 phy_id0;
	uint16 phy_id1;
	uint16 status;
	uint8  phy_types;
};

typedef struct _BufferDesc {
	uint32	link;
	uint32	cmdsts;
	uint32	bufptr;
} BufferDesc;

struct sis900_private {
	struct device *next_module;
	//struct net_device_stats stats;
	PCI_Info_s * pci_dev;

	struct mac_chip_info * mac;
	struct mii_phy * mii;
        struct mii_phy * first_mii; /* record the first mii structure */
	unsigned int cur_phy;

	//struct timer_list timer; /* Link status detection timer. */
	ktimer_t timer;
	uint8     autong_complete; /* 1: auto-negotiate complete  */

	unsigned int cur_rx, dirty_rx;	/* producer/comsumer pointers for Tx/Rx ring */
	unsigned int cur_tx, dirty_tx;

	/* The saved address of a sent/receive-in-place packet buffer */
	PacketBuf_s *tx_skbuff[NUM_TX_DESC];
	PacketBuf_s *rx_skbuff[NUM_RX_DESC];
	BufferDesc tx_ring[NUM_TX_DESC];
	BufferDesc rx_ring[NUM_RX_DESC];

	unsigned int tx_full;		/* The Tx queue is full.    */
	int LinkOn;
};

#ifdef MODULE
#if LINUX_VERSION_CODE > 0x20115
MODULE_AUTHOR("Jim Huang <cmhuang@sis.com.tw>, Ollie Lho <ollie@sis.com.tw>");
MODULE_DESCRIPTION("SiS 900 PCI Fast Ethernet driver");
MODULE_PARM(multicast_filter_limit, "i");
MODULE_PARM(max_interrupt_work, "i");
MODULE_PARM(debug, "i");
#endif
#endif

static int sis900_open(struct device *net_dev);
static int sis900_mii_probe (struct device * net_dev);
static void sis900_init_rxfilter (struct device * net_dev);
static uint16 read_eeprom(long ioaddr, int location);
static uint16 mdio_read(struct device *net_dev, int phy_id, int location);
static void mdio_write(struct device *net_dev, int phy_id, int location, int val);
static void sis900_timer(unsigned long data);
static void sis900_check_mode (struct device *net_dev, struct mii_phy *mii_phy);
static void sis900_tx_timeout(struct device *net_dev);
static void sis900_init_tx_ring(struct device *net_dev);
static void sis900_init_rx_ring(struct device *net_dev);
static int sis900_start_xmit(PacketBuf_s *skb, struct device *net_dev);
static int sis900_rx(struct device *net_dev);
static void sis900_finish_xmit (struct device *net_dev);
static int sis900_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs);
static int sis900_close(struct device *net_dev);
//static int mii_ioctl(struct device *net_dev, struct ifreq *rq, int cmd);
//static struct enet_statistics *sis900_get_stats(struct device *net_dev);
//static uint16 sis900_compute_hashtable_index(uint8 *addr, uint8 revision);
static void set_rx_mode(struct device *net_dev);
static void sis900_reset(struct device *net_dev);
static void sis630_set_eq(struct device *net_dev, uint8 revision);
static uint16 sis900_default_phy(struct device * net_dev);
static void sis900_set_capability( struct device *net_dev ,struct mii_phy *phy);
static uint16 sis900_reset_phy(struct device *net_dev, int phy_addr);
static void sis900_auto_negotiate(struct device *net_dev, int phy_addr);
static void sis900_set_mode (long ioaddr, int speed, int duplex);

/* A list of all installed SiS900 devices, for removing the driver module. */
static struct device *root_sis900_dev = NULL;

#ifndef __ATHEOS__
/* walk through every ethernet PCI devices to see if some of them are matched with our card list*/
int sis900_probe (struct device * net_dev)
{
	int found = 0;
	PCI_Info_s * pci_dev = NULL;


	while ((pci_dev = pci_find_class (PCI_CLASS_NETWORK_ETHERNET << 8, pci_dev)) != NULL) {
		/* pci_dev contains all ethernet devices */
		uint32 pci_io_base;
		struct mac_chip_info * mac;

		for (mac = mac_chip_table; mac->vendor_id; mac++) {
			/* try to match our card list */
			if (pci_dev->vendor == mac->vendor_id &&
			    pci_dev->device == mac->device_id)
				break;
		}

		if (mac->vendor_id == 0)
			/* pci_dev does not match any of our cards */
			continue;

		/* now, pci_dev should be either 900 or 7016 */
		pci_io_base = pci_dev->base_address[0] & PCI_BASE_ADDRESS_IO_MASK;
		if ((mac->flags & PCI_COMMAND_IO ) /* &&
		    check_region(pci_io_base, mac->io_size */ ))
			continue;

		/* PCI master */

		/* do the real low level jobs */
		net_dev = sis900_mac_probe(mac, pci_dev, net_dev);

		if (net_dev != NULL) {
			found++;
		}
		net_dev = NULL;
	}
	return found ? 0 : -ENODEV;
}
#endif /* !__ATHEOS__ */

/* older SiS900 and friends, use EEPROM to store MAC address */
static int sis900_get_mac_addr(PCI_Info_s * pci_dev, struct device *net_dev)
{
	long ioaddr = pci_dev->u.h0.nBase0 & ~3;
	uint16 signature;
	int i;

	/* check to see if we have sane EEPROM */
	signature = (uint16) read_eeprom(ioaddr, EEPROMSignature);    
	if (signature == 0xffff || signature == 0x0000) {
		printk (KERN_INFO "%s: Error EERPOM read %x\n", 
			net_dev->name, signature);
		return 0;
	}

	/* get MAC address from EEPROM */
	for (i = 0; i < 3; i++)
	        ((uint16 *)(net_dev->dev_addr))[i] = read_eeprom(ioaddr, i+EEPROMMACAddr);

	return 1;
}

/* SiS630E model, use APC CMOS RAM to store MAC address */
static int sis630e_get_mac_addr(PCI_Info_s * pci_dev, struct device *net_dev)
{
	PCI_Info_s *isa_bridge = NULL;
	uint8 reg;
	int i;

	if ((isa_bridge = pci_find_device(0x1039, 0x0008, isa_bridge)) == NULL) {
		printk("%s: Can not find ISA bridge\n", net_dev->name);
		return 0;
	}
	pci_read_config_byte(isa_bridge, 0x48, reg);
	pci_write_config_byte(isa_bridge, 0x48, reg | 0x40);

	for (i = 0; i < 6; i++) {
		outb(0x09 + i, 0x70);
		((uint8 *)(net_dev->dev_addr))[i] = inb(0x71); 
	}
	pci_write_config_byte(isa_bridge, 0x48, reg & ~0x40);

	return 1;
}

/* 635 model : set Mac reload bit and get mac address from rfdr */
static int sis635_get_mac_addr(PCI_Info_s * pci_dev, struct device *net_dev)
{
	long ioaddr = net_dev->base_addr;
	uint32 rfcrSave;
	uint32 i;

	rfcrSave = inl(rfcr + ioaddr);

	outl(rfcrSave | RELOAD, ioaddr + cr);
	outl(0, ioaddr + cr);

	/* disable packet filtering before setting filter */
	outl(rfcrSave & ~RFEN, rfcr + ioaddr);

	/* load MAC addr to filter data register */
	for (i = 0 ; i < 3 ; i++) {
		outl((i << RFADDR_shift), ioaddr + rfcr);
		*( ((uint16 *)net_dev->dev_addr) + i) = inw(ioaddr + rfdr);
	}

	/* enable packet filitering */
	outl(rfcrSave | RFEN, rfcr + ioaddr);

	return 1;
}


/**
 *	sis962_get_mac_addr: - Get MAC address for SiS962 model
 *	@pci_dev: the sis900 pci device
 *	@net_dev: the net device to get address for 
 *
 *	SiS962 model, use EEPROM to store MAC address. And EEPROM is shared by
 *	LAN and 1394. When access EEPROM, send EEREQ signal to hardware first 
 *	and wait for EEGNT. If EEGNT is ON, EEPROM is permitted to be access 
 *	by LAN, otherwise is not. After MAC address is read from EEPROM, send
 *	EEDONE signal to refuse EEPROM access by LAN. 
 *	MAC address is read into @net_dev->dev_addr.
 */

static int sis962_get_mac_addr(PCI_Info_s * pci_dev, struct device *net_dev)
{

	long ioaddr = net_dev->base_addr;
	long ee_addr = ioaddr + mear;
	uint32 waittime = 0;
	int ret = 0;
	
	outl(EEREQ, ee_addr);
	while(waittime < 2000) {
		if(inl(ee_addr) & EEGNT) {
			ret = sis900_get_mac_addr(pci_dev, net_dev);
			outl(EEDONE, ee_addr);
			return(ret);
		} else {
			udelay(1);	
			waittime ++;
		}
	}
	outl(EEDONE, ee_addr);
	return 0;
}

static struct device * sis900_mac_probe (struct mac_chip_info mac, PCI_Info_s pci_dev,
					 int device_handle, int found_cnt)
{
	struct device *net_dev;
	struct sis900_private *sis_priv;
	long ioaddr = pci_dev.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
	int	irq = pci_dev.u.h0.nInterruptLine;
	static int did_version = 0;
	uint8 revision;
	int i, ret = 0;
	char node_path[64];

	if (did_version++ == 0)
		printk(KERN_INFO "%s", version);

	net_dev = kmalloc( sizeof(*net_dev), MEMF_KERNEL | MEMF_CLEAR );
	if (!net_dev)
		return NULL;
	net_dev->name = "sis900";

	if ((net_dev->priv = kmalloc(sizeof(struct sis900_private), MEMF_KERNEL)) == NULL) {
		return NULL;
	}

	sis_priv = net_dev->priv;

	/* We do a request_region() to register /proc/ioports info. */
	//request_region(ioaddr, mac->io_size, net_dev->name);
	net_dev->base_addr = ioaddr;
	net_dev->irq = irq;
	net_dev->device_handle = device_handle;
	sis_priv->pci_dev = &pci_dev;
	sis_priv->mac = &mac;

	pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
	if ( revision == SIS630E_900_REV )
		ret = sis630e_get_mac_addr(sis_priv->pci_dev, net_dev);
	else if ((revision > 0x81) && (revision <= 0x90))
		ret = sis635_get_mac_addr(sis_priv->pci_dev, net_dev);
	else if (revision == SIS962_900_REV)
		ret = sis962_get_mac_addr(sis_priv->pci_dev, net_dev);
	else
		ret = sis900_get_mac_addr(sis_priv->pci_dev, net_dev);

	if (ret == 0) {
		//unregister_netdev(net_dev);
		kfree(net_dev);
		kfree(sis_priv);
		return NULL;
	}

	/* print some information about our NIC */
	printk(KERN_INFO "%s: %s at %#lx, IRQ %d, ", net_dev->name, sis_priv->mac->name,
	       ioaddr, irq);
	for (i = 0; i < 5; i++)
		printk("%2.2x:", (uint8) net_dev->dev_addr[i]);
	printk("%2.2x.\n", net_dev->dev_addr[i]);

	/* 630ET : set the mii access mode as software-mode */
	if (revision == SIS630ET_900_REV)
		outl(ACCESSMODE | inl(ioaddr + cr), ioaddr + cr);

	/* probe for mii transceiver */
	if (sis900_mii_probe(net_dev) == 0) {
		kfree(net_dev);
		kfree(sis_priv);
		return NULL;
	}

	sis_priv->next_module = root_sis900_dev;
	root_sis900_dev = net_dev;

	sis_priv->timer = create_timer();

	sprintf( node_path, "net/eth/sis900-%d", found_cnt );
	net_dev->node_handle = create_device_node( device_handle, sis_priv->pci_dev->nHandle,
		node_path, &g_sDevOps, net_dev );
	printk( "sis900_mac_probe() Create node: %s\n", node_path );

	return net_dev;
}

/* sis900_mii_probe: - Probe MII PHY for sis900 */
static int sis900_mii_probe (struct device * net_dev)
{
	struct sis900_private * sis_priv = (struct sis900_private *)net_dev->priv;
	uint16 poll_bit = MII_STAT_LINK, status = 0;
	unsigned int timeout = jiffies + 5 * HZ;
	int phy_addr;
	uint8 revision;

	sis_priv->mii = NULL;

	/* search for total of 32 possible mii phy addresses */
	for (phy_addr = 0; phy_addr < 32; phy_addr++) {	
	        struct mii_phy * mii_phy = NULL;
		uint16 mii_status;
		int i;

		for(i=0; i<2; i++)
			mii_status = mdio_read(net_dev, phy_addr, MII_STATUS);

		if (mii_status == 0xffff || mii_status == 0x0000)
			/* the mii is not accessable, try next one */
			continue;
		
		if ((mii_phy = kmalloc(sizeof(struct mii_phy), MEMF_KERNEL | MEMF_CLEAR)) == NULL) {
			printk(KERN_INFO "Cannot allocate mem for struct mii_phy\n");
			return 0;
		}
		
		mii_phy->phy_id0 = mdio_read(net_dev, phy_addr, MII_PHY_ID0);
		mii_phy->phy_id1 = mdio_read(net_dev, phy_addr, MII_PHY_ID1);		
		mii_phy->phy_addr = phy_addr;
		mii_phy->status = mii_status;
		mii_phy->next = sis_priv->mii;
		sis_priv->mii = mii_phy;
		sis_priv->first_mii = mii_phy;

		for (i=0; mii_chip_table[i].phy_id1; i++)
			if ( ( mii_phy->phy_id0 == mii_chip_table[i].phy_id0 ) &&
				( (mii_phy->phy_id1 & 0xFFF0) == mii_chip_table[i].phy_id1 )){

				mii_phy->phy_types = mii_chip_table[i].phy_types;
				if(mii_chip_table[i].phy_types == MIX)
					mii_phy->phy_types =
						(mii_status & (MII_STAT_CAN_TX_FDX | MII_STAT_CAN_TX))?LAN:HOME;
				printk(KERN_INFO "%s: %s transceiver found at address %d.\n",
					net_dev->name, mii_chip_table[i].name, phy_addr);
				break;
			}

		if( !mii_chip_table[i].phy_id1 )
			printk(KERN_INFO "%s: Unknown PHY transceiver found at address %d.\n",
				net_dev->name, phy_addr);
	}
	
	if (sis_priv->mii == NULL) {
		printk(KERN_INFO "%s: No MII transceivers found!\n",
		       net_dev->name);
		return 0;
	}

	/* Slect Default PHY to put in sis_priv->mii & sis_priv->cur_phy */
	sis_priv->mii = NULL;
	sis900_default_phy( net_dev );

	/* Reset PHY if default PHY is internal sis900 */
        if( (sis_priv->mii->phy_id0 == 0x001D) &&
       	  ( (sis_priv->mii->phy_id1&0xFFF0) == 0x8000) )
        	status = sis900_reset_phy( net_dev,  sis_priv->cur_phy );
        
        /* workaround for ICS1893 PHY */
        if ((sis_priv->mii->phy_id0 == 0x0015) &&
            ((sis_priv->mii->phy_id1&0xFFF0) == 0xF440))
        	mdio_write(net_dev, sis_priv->cur_phy, 0x0018, 0xD200);

        if( status & MII_STAT_LINK ){
        	while (poll_bit) {
				udelay(1);
        		poll_bit ^= (mdio_read(net_dev, sis_priv->cur_phy, MII_STATUS) & poll_bit);
        		if (jiffies >= timeout) {
        			printk(KERN_WARNING "%s: reset phy and link down\n", net_dev->name);
        			return -ETIME;
        		}
        	}
        }

	pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
	if (revision == SIS630E_900_REV) {
		/* SiS 630E has some bugs on default value of PHY registers */
		mdio_write(net_dev, sis_priv->cur_phy, MII_ANADV, 0x05e1);
		mdio_write(net_dev, sis_priv->cur_phy, MII_CONFIG1, 0x22);
		mdio_write(net_dev, sis_priv->cur_phy, MII_CONFIG2, 0xff00);
		mdio_write(net_dev, sis_priv->cur_phy, MII_MASK, 0xffc0);
		//mdio_write(net_dev, sis_priv->cur_phy, MII_CONTROL, 0x1000);	
	}

	if (sis_priv->mii->status & MII_STAT_LINK)
		sis_priv->LinkOn = TRUE;
	else
		sis_priv->LinkOn = FALSE;

	return 1;
}


/* sis900_default_phy : Select one default PHY for sis900 mac */
static uint16 sis900_default_phy(struct device * net_dev)
{
	struct sis900_private * sis_priv = (struct sis900_private *)net_dev->priv;
 	struct mii_phy *phy = NULL, *phy_home = NULL, *default_phy = NULL;
	uint16 status;

        for( phy=sis_priv->first_mii; phy; phy=phy->next ){
		status = mdio_read(net_dev, phy->phy_addr, MII_STATUS);
		status = mdio_read(net_dev, phy->phy_addr, MII_STATUS);

		/* Link ON & Not select deafalut PHY */
		 if ( (status & MII_STAT_LINK) && !(default_phy) )
		 	default_phy = phy;
		 else{
			status = mdio_read(net_dev, phy->phy_addr, MII_CONTROL);
			mdio_write(net_dev, phy->phy_addr, MII_CONTROL,
				status | MII_CNTL_AUTO | MII_CNTL_ISOLATE);
			if( phy->phy_types == HOME )
				phy_home = phy;
		 }
	}

	if( (!default_phy) && phy_home )
		default_phy = phy_home;
	else if(!default_phy)
		default_phy = sis_priv->first_mii;

	if( sis_priv->mii != default_phy ){
		sis_priv->mii = default_phy;
		sis_priv->cur_phy = default_phy->phy_addr;
		printk(KERN_INFO "%s: Using transceiver found at address %d as default\n", net_dev->name,sis_priv->cur_phy);
	}
	
	status = mdio_read(net_dev, sis_priv->cur_phy, MII_CONTROL);
	status &= (~MII_CNTL_ISOLATE);

	mdio_write(net_dev, sis_priv->cur_phy, MII_CONTROL, status);	
	status = mdio_read(net_dev, sis_priv->cur_phy, MII_STATUS);
	status = mdio_read(net_dev, sis_priv->cur_phy, MII_STATUS);

	return status;	
}
 

/* sis900_set_capability : set the media capability of network adapter */
static void sis900_set_capability( struct device *net_dev , struct mii_phy *phy )
{
	uint16 cap;
	uint16 status;
	
	status = mdio_read(net_dev, phy->phy_addr, MII_STATUS);
	status = mdio_read(net_dev, phy->phy_addr, MII_STATUS);
	
	cap = MII_NWAY_CSMA_CD |
		((phy->status & MII_STAT_CAN_TX_FDX)? MII_NWAY_TX_FDX:0) |
		((phy->status & MII_STAT_CAN_TX)    ? MII_NWAY_TX:0) |
		((phy->status & MII_STAT_CAN_T_FDX) ? MII_NWAY_T_FDX:0)|
		((phy->status & MII_STAT_CAN_T)     ? MII_NWAY_T:0);

	mdio_write( net_dev, phy->phy_addr, MII_ANADV, cap );
}


/* Delay between EEPROM clock transitions. */
#define eeprom_delay()	inl(ee_addr)

/* Read Serial EEPROM through EEPROM Access Register, Note that location is
   in word (16 bits) unit */
static uint16 read_eeprom(long ioaddr, int location)
{
	int i;
	uint16 retval = 0;
	long ee_addr = ioaddr + mear;
	uint32 read_cmd = location | EEread;

	outl(0, ee_addr);
	eeprom_delay();
	outl(EECLK, ee_addr);
	eeprom_delay();

	/* Shift the read command (9) bits out. */
	for (i = 8; i >= 0; i--) {
		uint32 dataval = (read_cmd & (1 << i)) ? EEDI | EECS : EECS;
		outl(dataval, ee_addr);
		eeprom_delay();
		outl(dataval | EECLK, ee_addr);
		eeprom_delay();
	}
	outb(EECS, ee_addr);
	eeprom_delay();

	/* read the 16-bits data in */
	for (i = 16; i > 0; i--) {
		outl(EECS, ee_addr);
		eeprom_delay();
		outl(EECS | EECLK, ee_addr);
		eeprom_delay();
		retval = (retval << 1) | ((inl(ee_addr) & EEDO) ? 1 : 0);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	outl(0, ee_addr);
	eeprom_delay();
	outl(EECLK, ee_addr);

	return (retval);
}

/* Read and write the MII management registers using software-generated
   serial MDIO protocol. Note that the command bits and data bits are
   send out seperately */
#define mdio_delay()	inl(mdio_addr)

static void mdio_idle(long mdio_addr)
{
	outl(MDIO | MDDIR, mdio_addr);
	mdio_delay();
	outl(MDIO | MDDIR | MDC, mdio_addr);
}

/* Syncronize the MII management interface by shifting 32 one bits out. */
static void mdio_reset(long mdio_addr)
{
	int i;

	for (i = 31; i >= 0; i--) {
		outl(MDDIR | MDIO, mdio_addr);
		mdio_delay();
		outl(MDDIR | MDIO | MDC, mdio_addr);
		mdio_delay();
	}
	return;
}

static uint16 mdio_read(struct device *net_dev, int phy_id, int location)
{
	long mdio_addr = net_dev->base_addr + mear;
	int mii_cmd = MIIread|(phy_id << MIIpmdShift)|(location << MIIregShift);
	uint16 retval = 0;
	int i;

	mdio_reset(mdio_addr);
	mdio_idle(mdio_addr);

	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDDIR | MDIO : MDDIR;
		outl(dataval, mdio_addr);
		mdio_delay();
		outl(dataval | MDC, mdio_addr);
		mdio_delay();
	}

	/* Read the 16 data bits. */
	for (i = 16; i > 0; i--) {
		outl(0, mdio_addr);
		mdio_delay();
		retval = (retval << 1) | ((inl(mdio_addr) & MDIO) ? 1 : 0);
		outl(MDC, mdio_addr);
		mdio_delay();
	}
	outl(0x00, mdio_addr);

	return retval;
}

static void mdio_write(struct device *net_dev, int phy_id, int location, int value)
{
	long mdio_addr = net_dev->base_addr + mear;
	int mii_cmd = MIIwrite|(phy_id<<MIIpmdShift)|(location<<MIIregShift);
	int i;

	mdio_reset(mdio_addr);
	mdio_idle(mdio_addr);

	/* Shift the command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDDIR | MDIO : MDDIR;
		outb(dataval, mdio_addr);
		mdio_delay();
		outb(dataval | MDC, mdio_addr);
		mdio_delay();
	}
	mdio_delay();

	/* Shift the value bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (value & (1 << i)) ? MDDIR | MDIO : MDDIR;
		outl(dataval, mdio_addr);
		mdio_delay();
		outl(dataval | MDC, mdio_addr);
		mdio_delay();
	}
	mdio_delay();

	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		outb(0, mdio_addr);
		mdio_delay();
		outb(MDC, mdio_addr);
		mdio_delay();
	}
	outl(0x00, mdio_addr);

	return;
}

static uint16 sis900_reset_phy(struct device *net_dev, int phy_addr)
{
	int i = 0;
	uint16 status;

	while (i++ < 2)
		status = mdio_read(net_dev, phy_addr, MII_STATUS);

	mdio_write( net_dev, phy_addr, MII_CONTROL, MII_CNTL_RESET );
	
	return status;
}

static int
sis900_open(struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	uint8 revision;

	/* Soft reset the chip. */
	sis900_reset(net_dev);
	
	/* Equalizer workaroung Rule */
	pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
	sis630_set_eq(net_dev, revision);
	
	if ((net_dev->irq_handle = request_irq(net_dev->irq, sis900_interrupt, NULL,
			SA_SHIRQ, net_dev->name, net_dev)) < 0) {
		return -EAGAIN;
	}

	//MOD_INC_USE_COUNT;

	sis900_init_rxfilter(net_dev);

	sis900_init_tx_ring(net_dev);
	sis900_init_rx_ring(net_dev);

	set_rx_mode(net_dev);

	net_dev->tbusy = 0;
	net_dev->interrupt = 0;
	net_dev->start = 1;

	/* Workaround for EDB */
	sis900_set_mode(ioaddr, HW_SPEED_10_MBPS, FDX_CAPABLE_HALF_SELECTED);

	/* Enable all known interrupts by setting the interrupt mask. */
	outl((RxSOVR|RxORN|RxERR|RxOK|TxURN|TxERR|TxIDLE), ioaddr + imr);
	outl(RxENA | inl(ioaddr + cr), ioaddr + cr);
	outl(IE, ioaddr + ier);

	sis900_check_mode(net_dev, sis_priv->mii);

	/* Set the timer to switch to check for link beat and perhaps switch
	   to an alternate media type. */
	//init_timer(&sis_priv->timer);
	//sis_priv->timer.expires = jiffies + HZ;
	//sis_priv->timer.data = (unsigned long)net_dev;
	//sis_priv->timer.function = &sis900_timer;
	//add_timer(&sis_priv->timer);

	start_timer(sis_priv->timer, (timer_callback *) &sis900_timer, net_dev, 2000000, true );

	return 0;
}

/* set receive filter address to our MAC address */
static void
sis900_init_rxfilter (struct device * net_dev)
{
	long ioaddr = net_dev->base_addr;
	uint32 rfcrSave;
	uint32 i;

	rfcrSave = inl(rfcr + ioaddr);

	/* disable packet filtering before setting filter */
	outl(rfcrSave & ~RFEN, rfcr + ioaddr);

	/* load MAC addr to filter data register */
	for (i = 0 ; i < 3 ; i++) {
		uint32 w;

		w = (uint32) *((uint16 *)(net_dev->dev_addr)+i);
		outl((i << RFADDR_shift), ioaddr + rfcr);
		outl(w, ioaddr + rfdr);

		if (sis900_debug > 2) {
			printk(KERN_INFO "%s: Receive Filter Addrss[%d]=%x\n",
			       net_dev->name, (int)i, inl(ioaddr + rfdr));
		}
	}

	/* enable packet filitering */
	outl(rfcrSave | RFEN, rfcr + ioaddr);
}

/* Initialize the Tx ring. */
static void
sis900_init_tx_ring(struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	int i;

	sis_priv->tx_full = 0;
	sis_priv->dirty_tx = sis_priv->cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		sis_priv->tx_skbuff[i] = NULL;

		sis_priv->tx_ring[i].link = (uint32) virt_to_bus(&sis_priv->tx_ring[i+1]);
		sis_priv->tx_ring[i].cmdsts = 0;
		sis_priv->tx_ring[i].bufptr = 0;
	}
	sis_priv->tx_ring[i-1].link = (uint32) virt_to_bus(&sis_priv->tx_ring[0]);

	/* load Transmit Descriptor Register */
	outl(virt_to_bus(&sis_priv->tx_ring[0]), ioaddr + txdp);
	if (sis900_debug > 2)
		printk(KERN_INFO "%s: TX descriptor register loaded with: %8.8x\n",
		       net_dev->name, inl(ioaddr + txdp));
}

/* Initialize the Rx descriptor ring, pre-allocate recevie buffers */
static void 
sis900_init_rx_ring(struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	int i;

	sis_priv->cur_rx = 0;
	sis_priv->dirty_rx = 0;

	/* init RX descriptor */
	for (i = 0; i < NUM_RX_DESC; i++) {
		sis_priv->rx_skbuff[i] = NULL;

		sis_priv->rx_ring[i].link = (uint32) virt_to_bus(&sis_priv->rx_ring[i+1]);
		sis_priv->rx_ring[i].cmdsts = 0;
		sis_priv->rx_ring[i].bufptr = 0;
	}
	sis_priv->rx_ring[i-1].link = (uint32) virt_to_bus(&sis_priv->rx_ring[0]);

	/* allocate sock buffers */
	for (i = 0; i < NUM_RX_DESC; i++) {
		PacketBuf_s *skb;

		if ((skb = alloc_pkt_buffer(RX_BUF_SIZE)) == NULL) {
			/* not enough memory for skbuff, this makes a "hole"
			   on the buffer ring, it is not clear how the
			   hardware will react to this kind of degenerated
			   buffer */
			break;
		}
		//skb->dev = net_dev;
		skb->pb_nSize = 0;
		sis_priv->rx_skbuff[i] = skb;
		sis_priv->rx_ring[i].cmdsts = RX_BUF_SIZE;
		sis_priv->rx_ring[i].bufptr = virt_to_bus(skb->pb_pData); /* tail */
	}
	sis_priv->dirty_rx = (unsigned int) (i - NUM_RX_DESC);

	/* load Receive Descriptor Register */
	outl(virt_to_bus(&sis_priv->rx_ring[0]), ioaddr + rxdp);
	if (sis900_debug > 2)
		printk(KERN_INFO "%s: RX descriptor register loaded with: %8.8x\n",
		       net_dev->name, inl(ioaddr + rxdp));
}

/**
 *	sis630_set_eq: - set phy equalizer value for 630 LAN
 *	@net_dev: the net device to set equalizer value
 *	@revision: 630 LAN revision number
 *
 *	630E equalizer workaround rule(Cyrus Huang 08/15)
 *	PHY register 14h(Test)
 *	Bit 14: 0 -- Automatically dectect (default)
 *		1 -- Manually set Equalizer filter
 *	Bit 13: 0 -- (Default)
 *		1 -- Speed up convergence of equalizer setting
 *	Bit 9 : 0 -- (Default)
 *		1 -- Disable Baseline Wander
 *	Bit 3~7   -- Equalizer filter setting
 *	Link ON: Set Bit 9, 13 to 1, Bit 14 to 0
 *	Then calculate equalizer value
 *	Then set equalizer value, and set Bit 14 to 1, Bit 9 to 0
 *	Link Off:Set Bit 13 to 1, Bit 14 to 0
 *	Calculate Equalizer value:
 *	When Link is ON and Bit 14 is 0, SIS900PHY will auto-dectect proper equalizer value.
 *	When the equalizer is stable, this value is not a fixed value. It will be within
 *	a small range(eg. 7~9). Then we get a minimum and a maximum value(eg. min=7, max=9)
 *	0 <= max <= 4  --> set equalizer to max
 *	5 <= max <= 14 --> set equalizer to max+1 or set equalizer to max+2 if max == min
 *	max >= 15      --> set equalizer to max+5 or set equalizer to max+6 if max == min
 */

static void sis630_set_eq(struct device *net_dev, uint8 revision)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	uint16 reg14h, eq_value, max_value = 0, min_value = 0;
	uint8 host_bridge_rev = 0;
	int i, maxcount = 10;
	PCI_Info_s *dev = NULL;

	if ( !(revision == SIS630E_900_REV || revision == SIS630EA1_900_REV ||
	       revision == SIS630A_900_REV || revision ==  SIS630ET_900_REV) )
		return;

	if ((dev = pci_find_device(SIS630_VENDOR_ID, SIS630_DEVICE_ID, dev)))
		pci_read_config_byte(dev, PCI_CLASS_REVISION, host_bridge_rev);

	if (sis_priv->LinkOn) {
		reg14h=mdio_read(net_dev, sis_priv->cur_phy, MII_RESV);
		mdio_write(net_dev, sis_priv->cur_phy, MII_RESV, (0x2200 | reg14h) & 0xBFFF);
		for (i=0; i < maxcount; i++) {
			eq_value=(0x00F8 & mdio_read(net_dev, sis_priv->cur_phy, MII_RESV)) >> 3;
			if (i == 0)
				max_value=min_value=eq_value;
			max_value = (eq_value > max_value) ? eq_value : max_value;
			min_value = (eq_value < min_value) ? eq_value : min_value;
		}
		/* 630E rule to determine the equalizer value */
		if (revision == SIS630E_900_REV || revision == SIS630EA1_900_REV ||
		    revision == SIS630ET_900_REV) {
			if (max_value < 5)
				eq_value = max_value;
			else if (max_value >= 5 && max_value < 15)
				eq_value = (max_value == min_value) ? max_value + 2 : max_value + 1;
			else if (max_value >= 15)
				eq_value = (max_value == min_value) ? max_value + 6 : max_value + 5;
		}
		/* 630B0&B1 rule to determine the equalizer value */
		if (revision == SIS630A_900_REV && 
		    (host_bridge_rev == SIS630B0 || host_bridge_rev == SIS630B1)) {
			if (max_value == 0)
				eq_value = 3;
			else
				eq_value = (max_value + min_value+1)/2;
		}
		/* write equalizer value and setting */
		reg14h = mdio_read(net_dev, sis_priv->cur_phy, MII_RESV);
		reg14h = (reg14h & 0xFF07) | ((eq_value << 3) & 0x00F8);
		reg14h = (reg14h | 0x6000) & 0xFDFF;
		mdio_write(net_dev, sis_priv->cur_phy, MII_RESV, reg14h);
	}
	else {
		reg14h = mdio_read(net_dev, sis_priv->cur_phy, MII_RESV);
		if (revision == SIS630A_900_REV && 
		    (host_bridge_rev == SIS630B0 || host_bridge_rev == SIS630B1))
			mdio_write(net_dev, sis_priv->cur_phy, MII_RESV, (reg14h | 0x2200) & 0xBFFF);
		else
			mdio_write(net_dev, sis_priv->cur_phy, MII_RESV, (reg14h | 0x2000) & 0xBFFF);
	}
	return;
}


/* on each timer ticks we check two things, Link Status (ON/OFF) and 
   Link Mode (10/100/Full/Half) 
*/
static void sis900_timer(unsigned long data)
{
	struct device *net_dev = (struct device *)data;
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	struct mii_phy *mii_phy = sis_priv->mii;
	//static int next_tick = 5*HZ;
	uint16 status;
	uint8 revision;

	if(!sis_priv->autong_complete){
		int speed, duplex = 0;

		sis900_read_mode(net_dev, &speed, &duplex);
		if(duplex){
			sis900_set_mode(net_dev->base_addr, speed, duplex);
			pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
			sis630_set_eq(net_dev, revision);
		}
		
		//sis_priv->timer.expires = jiffies + HZ;
		start_timer( sis_priv->timer, (timer_callback *) &sis900_timer,
			net_dev, 2000000, true );
		return;
	}

	status = mdio_read(net_dev, sis_priv->cur_phy, MII_STATUS);
	status = mdio_read(net_dev, sis_priv->cur_phy, MII_STATUS);

	/* Link OFF -> ON */
	if ( !sis_priv->LinkOn ) {
LookForLink:
		/* Search for new PHY */
		status = sis900_default_phy( net_dev );
		mii_phy = sis_priv->mii;

		if( status & MII_STAT_LINK ){
			sis900_check_mode(net_dev, mii_phy);
			sis_priv->LinkOn = TRUE;
		}
	}
	/* Link ON -> OFF */
	else {
                if( !(status & MII_STAT_LINK) ){
			sis_priv->LinkOn = FALSE;
                	printk(KERN_INFO "%s: Media Link Off\n", net_dev->name);

                	/* Change mode issue */
                	if( (mii_phy->phy_id0 == 0x001D) && 
                	  ( (mii_phy->phy_id1 & 0xFFF0) == 0x8000 ))
               			sis900_reset_phy( net_dev,  sis_priv->cur_phy );
  
                	pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
			sis630_set_eq(net_dev, revision);
  
                	goto LookForLink;
                }
	}

	//sis_priv->timer.expires = jiffies + next_tick;
	//add_timer(&sis_priv->timer);
	start_timer( sis_priv->timer, (timer_callback *) &sis900_timer,
		net_dev, 2000000, true );
}

static void sis900_check_mode (struct device *net_dev, struct mii_phy *mii_phy)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	int speed, duplex;

	if( mii_phy->phy_types == LAN  ) {
		outl( ~EXD & inl( ioaddr + cfg ), ioaddr + cfg);
		sis900_set_capability(net_dev , mii_phy);
		sis900_auto_negotiate(net_dev, sis_priv->cur_phy);
	} else {
		outl(EXD | inl( ioaddr + cfg ), ioaddr + cfg);
		speed = HW_SPEED_HOME;
		duplex = FDX_CAPABLE_HALF_SELECTED;
		sis900_set_mode(net_dev->base_addr, speed, duplex);
		sis_priv->autong_complete = 1;
	}
}

static void sis900_set_mode (long ioaddr, int speed, int duplex)
{
	uint32 tx_flags = 0, rx_flags = 0;

	if( inl(ioaddr + cfg) & EDB_MASTER_EN ){
		tx_flags = TxATP | (DMA_BURST_64 << TxMXDMA_shift) | (TX_FILL_THRESH << TxFILLT_shift);
		rx_flags = DMA_BURST_64 << RxMXDMA_shift;
	}
	else{
		tx_flags = TxATP | (DMA_BURST_512 << TxMXDMA_shift) | (TX_FILL_THRESH << TxFILLT_shift);
		rx_flags = DMA_BURST_512 << RxMXDMA_shift;
	}

	if (speed == HW_SPEED_HOME || speed == HW_SPEED_10_MBPS ) {
		rx_flags |= (RxDRNT_10 << RxDRNT_shift);
		tx_flags |= (TxDRNT_10 << TxDRNT_shift);
	}
	else {
		rx_flags |= (RxDRNT_100 << RxDRNT_shift);
		tx_flags |= (TxDRNT_100 << TxDRNT_shift);
	}

	if (duplex == FDX_CAPABLE_FULL_SELECTED) {
		tx_flags |= (TxCSI | TxHBI);
		rx_flags |= RxATX;
	}

	outl (tx_flags, ioaddr + txcfg);
	outl (rx_flags, ioaddr + rxcfg);
}


static void sis900_auto_negotiate(struct device *net_dev, int phy_addr)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	int i = 0;
	uint32 status;
	
	while (i++ < 2)
		status = mdio_read(net_dev, phy_addr, MII_STATUS);

	if (!(status & MII_STAT_LINK)){
		printk(KERN_INFO "%s: Media Link Off\n", net_dev->name);
		sis_priv->autong_complete = 1;
		sis_priv->LinkOn = FALSE;
		return;
	}

	/* (Re)start AutoNegotiate */
	mdio_write(net_dev, phy_addr, MII_CONTROL,
			MII_CNTL_AUTO | MII_CNTL_RST_AUTO);
	sis_priv->autong_complete = 0;
}


static void sis900_read_mode(struct device *net_dev, int *speed, int *duplex)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	struct mii_phy *phy = sis_priv->mii;
	int phy_addr = sis_priv->cur_phy;
	uint32 status;
	uint16 autoadv, autorec;
	int i = 0;

	while (i++ < 2)
		status = mdio_read(net_dev, phy_addr, MII_STATUS);

	if (!(status & MII_STAT_LINK))	return;
	
	/* AutoNegotiate completed */
	autoadv = mdio_read(net_dev, phy_addr, MII_ANADV);
	autorec = mdio_read(net_dev, phy_addr, MII_ANLPAR);
	status = autoadv & autorec;

	*speed = HW_SPEED_10_MBPS;
	*duplex = FDX_CAPABLE_HALF_SELECTED;

	if (status & (MII_NWAY_TX | MII_NWAY_TX_FDX))
		*speed = HW_SPEED_100_MBPS;
	if (status & ( MII_NWAY_TX_FDX | MII_NWAY_T_FDX))
		*duplex = FDX_CAPABLE_FULL_SELECTED;

	sis_priv->autong_complete = 1;

	/* Workaround for Realtek RTL8201 PHY issue */
	if((phy->phy_id0 == 0x0000) && ((phy->phy_id1 & 0xFFF0) == 0x8200)){
		if(mdio_read(net_dev, phy_addr, MII_CONTROL) & MII_CNTL_FDX)
			*duplex = FDX_CAPABLE_FULL_SELECTED;
		if(mdio_read(net_dev, phy_addr, 0x0019) & 0x01)
			*speed = HW_SPEED_100_MBPS;
	}

	printk(KERN_INFO "%s: Media Link On %s %s-duplex \n",
	       net_dev->name,
	       *speed == HW_SPEED_100_MBPS ?
	       "100mbps" : "10mbps",
	       *duplex == FDX_CAPABLE_FULL_SELECTED ?
	       "full" : "half");
}


static void sis900_tx_timeout(struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	int i;

	printk(KERN_INFO "%s: Transmit timeout, status %8.8x %8.8x \n",
	       net_dev->name, inl(ioaddr + cr), inl(ioaddr + isr));

	/* Disable interrupts by clearing the interrupt mask. */
	outl(0x0000, ioaddr + imr);

	/* discard unsent packets, should this code section be protected by
	   cli(), sti() ?? */
	sis_priv->dirty_tx = sis_priv->cur_tx = 0;
	for (i = 0; i < NUM_TX_DESC; i++) {
		if (sis_priv->tx_skbuff[i] != NULL) {
			free_pkt_buffer(sis_priv->tx_skbuff[i]);
			sis_priv->tx_skbuff[i] = 0;
			sis_priv->tx_ring[i].cmdsts = 0;
			sis_priv->tx_ring[i].bufptr = 0;
		}
	}
	net_dev->trans_start = jiffies;
	net_dev->tbusy = sis_priv->tx_full = 0;

	/* FIXME: Should we restart the transmission thread here  ?? */
	outl(TxENA | inl(ioaddr + cr), ioaddr + cr);

	/* Enable all known interrupts by setting the interrupt mask. */
	outl((RxSOVR|RxORN|RxERR|RxOK|TxURN|TxERR|TxIDLE), ioaddr + imr);
	return;
}

static int
sis900_start_xmit(PacketBuf_s *skb, struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	unsigned int  entry;

	/* test tbusy to see if we have timeout situation then set it */
	if (test_and_set_bit(0, (void*)&net_dev->tbusy) != 0) {
		if (jiffies - net_dev->trans_start > TX_TIMEOUT)
			sis900_tx_timeout(net_dev);
		return 1;
	}

	/* Calculate the next Tx descriptor entry. */
	entry = sis_priv->cur_tx % NUM_TX_DESC;
	sis_priv->tx_skbuff[entry] = skb;

	/* set the transmit buffer descriptor and enable Transmit State Machine */
	sis_priv->tx_ring[entry].bufptr = virt_to_bus(skb->pb_pData);
	sis_priv->tx_ring[entry].cmdsts = (OWN | skb->pb_nSize);
	outl(TxENA | inl(ioaddr + cr), ioaddr + cr);

	if (++sis_priv->cur_tx - sis_priv->dirty_tx < NUM_TX_DESC) {
		/* Typical path, clear tbusy to indicate more 
		   transmission is possible */
		clear_bit(0, (void*)&net_dev->tbusy);
	} else {
		/* no more transmit descriptor available, tbusy remain set */
		sis_priv->tx_full = 1;
	}

	net_dev->trans_start = jiffies;

	if (sis900_debug > 3)
		printk(KERN_INFO "%s: Queued Tx packet at %p size %d "
		       "to slot %d.\n",
		       net_dev->name, skb->pb_pData, (int)skb->pb_nSize, entry);

	return 0;
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static int sis900_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs)
{
	struct device *net_dev = (struct device *)dev_instance;
	int boguscnt = max_interrupt_work;
	long ioaddr = net_dev->base_addr;
	uint32 status;

#if defined(__i386__)
	/* A lock to prevent simultaneous entry bug on Intel SMP machines. */
	if (test_and_set_bit(0, (void*)&net_dev->interrupt)) {
		printk(KERN_INFO "%s: SMP simultaneous entry of "
		       "an interrupt handler.\n", net_dev->name);
		net_dev->interrupt = 0;		/* Avoid halting machine. */
		return 0;
	}
#else
	if (net_dev->interrupt) {
		printk(KERN_INFO "%s: Re-entering the interrupt handler.\n",
		       net_dev->name);
		return 0;
	}
	net_dev->interrupt = 1;
#endif

	do {
		status = inl(ioaddr + isr);

		if ((status & (HIBERR|TxURN|TxERR|TxIDLE|RxORN|RxERR|RxOK)) == 0)
			/* nothing intresting happened */
			break;

		/* why dow't we break after Tx/Rx case ?? keyword: full-duplex */
		if (status & (RxORN | RxERR | RxOK))
			/* Rx interrupt */
			sis900_rx(net_dev);

		if (status & (TxURN | TxERR | TxIDLE))
			/* Tx interrupt */
			sis900_finish_xmit(net_dev);

		/* something strange happened !!! */
		if (status & HIBERR) {
			printk(KERN_INFO "%s: Abnormal interrupt,"
			       "status %#8.8x.\n", net_dev->name, (int)status);
			break;
		}
		if (--boguscnt < 0) {
			printk(KERN_INFO "%s: Too much work at interrupt, "
			       "interrupt status = %#8.8x.\n",
			       net_dev->name, (int)status);
			break;
		}
	} while (1);

	if (sis900_debug > 3)
		printk(KERN_INFO "%s: exiting interrupt, "
		       "interrupt status = 0x%#8.8x.\n",
		       net_dev->name, inl(ioaddr + isr));
	
#if defined(__i386__)
	clear_bit(0, (void*)&net_dev->interrupt);
#else
	net_dev->interrupt = 0;
#endif
	return 0;
}

/* Process receive interrupt events, put buffer to higher layer and refill buffer pool 
   Note: This fucntion is called by interrupt handler, don't do "too much" work here */
static int sis900_rx(struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	long ioaddr = net_dev->base_addr;
	unsigned int entry = sis_priv->cur_rx % NUM_RX_DESC;
	uint32 rx_status = sis_priv->rx_ring[entry].cmdsts;

	if (sis900_debug > 3)
		printk(KERN_INFO "sis900_rx, cur_rx:%4.4d, dirty_rx:%4.4d "
		       "status:0x%8.8x\n",
		       sis_priv->cur_rx, sis_priv->dirty_rx, (int)rx_status);

	while (rx_status & OWN) {
		unsigned int rx_size;

		rx_size = (rx_status & DSIZE) - CRC_SIZE;

		if (rx_status & (ABORT|OVERRUN|TOOLONG|RUNT|RXISERR|CRCERR|FAERR)) {
			/* corrupted packet received */
			if (sis900_debug > 3)
				printk(KERN_INFO "%s: Corrupted packet "
				       "received, buffer status = 0x%8.8x.\n",
				       net_dev->name, (int)rx_status);
			/* reset buffer descriptor state */
			sis_priv->rx_ring[entry].cmdsts = RX_BUF_SIZE;
		} else {
			PacketBuf_s * skb;

			/* This situation should never happen, but due to
			   some unknown bugs, it is possible that
			   we are working on NULL sk_buff :-( */
			if (sis_priv->rx_skbuff[entry] == NULL) {
				printk(KERN_INFO "%s: NULL pointer " 
				       "encountered in Rx ring, skipping\n",
				       net_dev->name);
				break;
			}

			/* give the socket buffer to upper layers */
			skb = sis_priv->rx_skbuff[entry];
			skb_put(skb, rx_size);

			if ( net_dev->packet_queue != NULL ) {
				skb->pb_uMacHdr.pRaw = skb->pb_pData;
				enqueue_packet( net_dev->packet_queue, skb );
			} else {
				printk( "Error: sis900_rx() received packet to downed device!\n" );
				free_pkt_buffer( skb );
			}

			/* refill the Rx buffer, what if there is not enought memory for
			   new socket buffer ?? */
			if ((skb = alloc_pkt_buffer(RX_BUF_SIZE)) == NULL) {
				/* not enough memory for skbuff, this makes a "hole"
				   on the buffer ring, it is not clear how the
				   hardware will react to this kind of degenerated
				   buffer */
				printk(KERN_INFO "%s: Memory squeeze,"
				       "deferring packet.\n",
				       net_dev->name);
				sis_priv->rx_skbuff[entry] = NULL;
				/* reset buffer descriptor state */
				sis_priv->rx_ring[entry].cmdsts = 0;
				sis_priv->rx_ring[entry].bufptr = 0;
				break;
			}
			//skb->dev = net_dev;
			skb->pb_nSize = 0;
			sis_priv->rx_skbuff[entry] = skb;
			sis_priv->rx_ring[entry].cmdsts = RX_BUF_SIZE;
			sis_priv->rx_ring[entry].bufptr = virt_to_bus(skb->pb_pData);
			sis_priv->dirty_rx++;
		}
		sis_priv->cur_rx++;
		entry = sis_priv->cur_rx % NUM_RX_DESC;
		rx_status = sis_priv->rx_ring[entry].cmdsts;
	} // while

	/* refill the Rx buffer, what if the rate of refilling is slower than 
	   consuming ?? */
	for (;sis_priv->cur_rx - sis_priv->dirty_rx > 0; sis_priv->dirty_rx++) {
		PacketBuf_s *skb;

		entry = sis_priv->dirty_rx % NUM_RX_DESC;

		if (sis_priv->rx_skbuff[entry] == NULL) {
			if ((skb = alloc_pkt_buffer(RX_BUF_SIZE)) == NULL) {
				/* not enough memory for skbuff, this makes a "hole"
				   on the buffer ring, it is not clear how the
				   hardware will react to this kind of degenerated
				   buffer */
				printk(KERN_INFO "%s: Memory squeeze,"
				       "deferring packet.\n",
				       net_dev->name);
				break;
			}
			//skb->dev = net_dev;
			skb->pb_nSize = 0;
			sis_priv->rx_skbuff[entry] = skb;
			sis_priv->rx_ring[entry].cmdsts = RX_BUF_SIZE;
			sis_priv->rx_ring[entry].bufptr = virt_to_bus(skb->pb_pData);
		}
	}

	/* re-enable the potentially idle receive state matchine */
	outl(RxENA | inl(ioaddr + cr), ioaddr + cr );

	return 0;
}

/* finish up transmission of packets, check for error condition and free skbuff etc.
   Note: This fucntion is called by interrupt handler, don't do "too much" work here */
static void sis900_finish_xmit (struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;

	for (; sis_priv->dirty_tx < sis_priv->cur_tx; sis_priv->dirty_tx++) {
		unsigned int entry;
		uint32 tx_status;

		entry = sis_priv->dirty_tx % NUM_TX_DESC;
		tx_status = sis_priv->tx_ring[entry].cmdsts;

		if (tx_status & OWN) {
			/* The packet is not transmitted yet (owned by hardware) !
			   Note: the interrupt is generated only when Tx Machine
			   is idle, so this is an almost impossible case */
			break;
		}

		if (tx_status & (ABORT | UNDERRUN | OWCOLL)) {
			/* packet unsuccessfully transmitted */
			if (sis900_debug > 3)
				printk(KERN_INFO "%s: Transmit "
				       "error, Tx status %8.8x.\n",
				       net_dev->name, (int)tx_status);
		}
		/* Free the original skb. */
		free_pkt_buffer(sis_priv->tx_skbuff[entry]);
		sis_priv->tx_skbuff[entry] = NULL;
		sis_priv->tx_ring[entry].bufptr = 0;
		sis_priv->tx_ring[entry].cmdsts = 0;
	}

	if (sis_priv->tx_full && net_dev->tbusy && 
	    sis_priv->cur_tx - sis_priv->dirty_tx < NUM_TX_DESC - 4) {
		/* The ring is no longer full, clear tbusy, tx_full and 
		   schedule more transmission by marking NET_BH */
		sis_priv->tx_full = 0;
		clear_bit(0, (void *)&net_dev->tbusy);
	}
}

static int
sis900_close(struct device *net_dev)
{
	long ioaddr = net_dev->base_addr;
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	int i;

	net_dev->start = 0;
	net_dev->tbusy = 1;

	/* Disable interrupts by clearing the interrupt mask. */
	outl(0x0000, ioaddr + imr);
	outl(0x0000, ioaddr + ier);

	/* Stop the chip's Tx and Rx Status Machine */
	outl(RxDIS | TxDIS | inl(ioaddr + cr), ioaddr + cr);

	delete_timer(sis_priv->timer);

	release_irq(net_dev->irq, net_dev->irq_handle);

	/* Free Tx and RX skbuff */
	for (i = 0; i < NUM_RX_DESC; i++) {
		if (sis_priv->rx_skbuff[i] != NULL)
			free_pkt_buffer(sis_priv->rx_skbuff[i]);
		sis_priv->rx_skbuff[i] = 0;
	}
	for (i = 0; i < NUM_TX_DESC; i++) {
		if (sis_priv->tx_skbuff[i] != NULL)
			free_pkt_buffer(sis_priv->tx_skbuff[i]);
		sis_priv->tx_skbuff[i] = 0;
	}

	/* Green! Put the chip in low-power mode. */

	//MOD_DEC_USE_COUNT;

	return 0;
}

#ifndef __ATHEOS__
static int mii_ioctl(struct device *net_dev, struct ifreq *rq, int cmd)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	uint16 *data = (uint16 *)&rq->ifr_data;

	switch(cmd) {
	case SIOCDEVPRIVATE:			/* Get the address of the PHY in use. */
		data[0] = sis_priv->mii->phy_addr;
		/* Fall Through */
	case SIOCDEVPRIVATE+1:			/* Read the specified MII register. */
		data[3] = mdio_read(net_dev, data[0] & 0x1f, data[1] & 0x1f);
		return 0;
	case SIOCDEVPRIVATE+2:			/* Write the specified MII register */
		if (!suser())
			return -EPERM;
		mdio_write(net_dev, data[0] & 0x1f, data[1] & 0x1f, data[2]);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static struct enet_statistics *
sis900_get_stats(struct device *net_dev)
{
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;

	return &sis_priv->stats;
}

/* SiS 900 uses the most sigificant 7 bits to index a 128 bits multicast
 * hash table, which makes this function a little bit different from other drivers
 * SiS 900 B0 & 635 M/B uses the most significat 8 bits to index 256 bits
 * multicast hash table. 
 */
static uint16 sis900_compute_hashtable_index(uint8 *addr, uint8 revision)
{

/* what is the correct value of the POLYNOMIAL ??
   Donald Becker use 0x04C11DB7U
   Joseph Zbiciak im14u2c@primenet.com gives me the
   correct answer, thank you Joe !! */
#define POLYNOMIAL 0x04C11DB7L
	uint32 crc = 0xffffffff, msb;
	int  i, j;
	uint32 byte;

	for (i = 0; i < 6; i++) {
		byte = *addr++;
		for (j = 0; j < 8; j++) {
			msb = crc >> 31;
			crc <<= 1;
			if (msb ^ (byte & 1)) {
				crc ^= POLYNOMIAL;
			}
			byte >>= 1;
		}
	}

	/* leave 8 or 7 most siginifant bits */
	if((revision >= SIS635A_900_REV) || (revision == SIS900B_900_REV))
		return ((int)(crc >> 24));
	else
		return ((int)(crc >> 25));
}
#endif /* !__ATHEOS__ */

static void set_rx_mode(struct device *net_dev)
{
	long ioaddr = net_dev->base_addr;
	struct sis900_private * sis_priv = (struct sis900_private *)net_dev->priv;
	uint16 mc_filter[16] = {0};	/* 256/128 bits multicast hash table */
	int i, table_entries;
	uint32 rx_mode;
	uint8 revision;

	/* 635 Hash Table entires = 256(2^16) */
	pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
	if((revision >= SIS635A_900_REV) || (revision == SIS900B_900_REV))
		table_entries = 16;
	else
		table_entries = 8;

#ifndef __ATHEOS__
	if (net_dev->flags & IFF_PROMISC) {
#endif
		/* Accept any kinds of packets */
		rx_mode = RFPromiscuous;
		for (i = 0; i < table_entries; i++)
			mc_filter[i] = 0xffff;
#ifndef __ATHEOS__
	} else if ((net_dev->mc_count > multicast_filter_limit) ||
		   (net_dev->flags & IFF_ALLMULTI)) {
		/* too many multicast addresses or accept all multicast packets */
		rx_mode = RFAAB | RFAAM;
		for (i = 0; i < table_entries; i++)
			mc_filter[i] = 0xffff;
	} else {
		/* Accept Broadcast packets, destination addresses match our MAC address,
		   use Receive Filter to reject unwanted MCAST packets */
		struct dev_mc_list *mclist;
		rx_mode = RFAAB;
		for (i = 0, mclist = net_dev->mc_list; mclist && i < net_dev->mc_count;
		     i++, mclist = mclist->next)
			set_bit(sis900_compute_hashtable_index(mclist->dmi_addr, revision),
				mc_filter);
	}
#endif

	/* update Multicast Hash Table in Receive Filter */
	for (i = 0; i < table_entries; i++) {
		/* why plus 0x04 ??, That makes the correct value for hash table. */
		outl((uint32)(0x00000004+i) << RFADDR_shift, ioaddr + rfcr);
		outl(mc_filter[i], ioaddr + rfdr);
	}

	outl(RFEN | rx_mode, ioaddr + rfcr);

#ifndef __ATHEOS__
	/* sis900 is capable of looping back packet at MAC level for debugging purpose */
	if (net_dev->flags & IFF_LOOPBACK) {
		uint32 cr_saved;
		/* We must disable Tx/Rx before setting loopback mode */
		cr_saved = inl(ioaddr + cr);
		outl(cr_saved | TxDIS | RxDIS, ioaddr + cr);
		/* enable loopback */
		outl(inl(ioaddr + txcfg) | TxMLB, ioaddr + txcfg);
		outl(inl(ioaddr + rxcfg) | RxATX, ioaddr + rxcfg);
		/* restore cr */
		outl(cr_saved, ioaddr + cr);
	}
#endif

	return;
}

static void sis900_reset(struct device *net_dev)
{
	long ioaddr = net_dev->base_addr;
	struct sis900_private *sis_priv = (struct sis900_private *)net_dev->priv;
	int i = 0;
	uint8 revision;
	uint32 status = TxRCMP | RxRCMP;

	outl(0, ioaddr + ier);
	outl(0, ioaddr + imr);
	outl(0, ioaddr + rfcr);

	outl(RxRESET | TxRESET | RESET | inl(ioaddr + cr), ioaddr + cr);
	
	/* Check that the chip has finished the reset. */
	while (status && (i++ < 1000)) {
		status ^= (inl(isr + ioaddr) & status);
	}

	pci_read_config_byte(sis_priv->pci_dev, PCI_CLASS_REVISION, revision);
	if( (revision >= SIS635A_900_REV) || (revision == SIS900B_900_REV) )
		outl(PESEL | RND_CNT, ioaddr + cfg);
	else
		outl(PESEL, ioaddr + cfg);
}

int sis900_probe( int device_handle )
{
	int cards_found = 0;
	struct device* dev;
	
    int i;
    PCI_Info_s sInfo;
   
    for ( i = 0 ; g_psBus->get_pci_info( &sInfo, i ) == 0 ; ++i ) {
		int chip_idx;
		int ioaddr;
		int irq;
		int pci_command;
		int new_command;
		
		for ( chip_idx = 0 ; mac_chip_table[chip_idx].vendor_id ; chip_idx++ ) {
			if ( sInfo.nVendorID == mac_chip_table[chip_idx].vendor_id &&
				 sInfo.nDeviceID == mac_chip_table[chip_idx].device_id) {
				break;
			}
		}
		if (mac_chip_table[chip_idx].vendor_id == 0) { 		/* Compiled out! */
			continue;
		}
		
		if( claim_device( device_handle, sInfo.nHandle, "SiS 900/7016", DEVICE_NET ) != 0 )
			continue;
		
		printk( "sis900_probe() found NIC\n" );
		ioaddr = sInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
		irq = sInfo.u.h0.nInterruptLine;

		pci_command = g_psBus->read_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2 );
		new_command = pci_command | (mac_chip_table[chip_idx].flags & 7);
		if (pci_command != new_command) {
			printk(KERN_INFO "  The PCI BIOS has not enabled the"
				   " device at %d/%d/%d!  Updating PCI command %4.4x->%4.4x.\n",
				   sInfo.nBus, sInfo.nDevice, sInfo.nFunction, pci_command, new_command );
			g_psBus->write_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2, new_command);
		}

		dev = sis900_mac_probe( mac_chip_table[chip_idx], sInfo, device_handle, cards_found );
		dev = 0;
		cards_found++;
		break;
	}
	if( !cards_found )
		disable_device( device_handle );
	return cards_found ? 0 : -ENODEV;
}

static status_t sis900_open_dev( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

static status_t sis900_close_dev( void* pNode, void* pCookie )
{
    return( 0 );
}

static status_t sis900_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct device* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psDev->packet_queue = pArgs;
			sis900_open( psDev );
			break;
		}
		case SIOC_ETH_STOP:
		{
			sis900_close( psDev );
			psDev->packet_queue = NULL;
			break;
		}
		case SIOCSIFHWADDR:
			nError = -ENOSYS;
			break;
		case SIOCGIFHWADDR:
			memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, psDev->dev_addr, 6 );
			break;
		default:
			printk( "Error: sis900_ioctl() unknown command %d\n", (int)nCommand );
			nError = -ENOSYS;
			break;
    }

    return( nError );
}

static int sis900_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}

static int sis900_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct device* dev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL ) {
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		sis900_start_xmit( psBuffer, dev );
	}
	return( nSize );
}


static DeviceOperations_s g_sDevOps = {
    sis900_open_dev,
    sis900_close_dev,
    sis900_ioctl,
    sis900_read,
    sis900_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};


status_t device_init( int nDeviceID )
{
	/* Get PCI bus */
    g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
    
    if( g_psBus == NULL )
    	return( -1 );
	return( sis900_probe( nDeviceID ) );
}

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}

