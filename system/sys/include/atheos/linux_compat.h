/*
 *  The Syllable kernel
 *  Copyright (C) 2006 Kristian Van Der Vliet
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

#ifndef	__F_SYLLABLE_LINUX_COMPAT_H__
#define	__F_SYLLABLE_LINUX_COMPAT_H__

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/bitops.h>
#include <posix/errno.h>
#include <net/net.h>
#include <macros.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Linux pre-C99 types */
typedef int8		s8;
typedef uint8		u8;
typedef int16		s16;
typedef uint16		u16;
typedef int32		s32;
typedef uint32		u32;
typedef int64		s64;
typedef uint64		u64;

typedef uint32		dma_addr_t;

#define spinlock_t	SpinLock_s

/* Generic macros */
#define DECLARE_PCI_UNMAP_ADDR(VAR) \
	dma_addr_t VAR;

#define DECLARE_PCI_UNMAP_LEN(VAR) \
	uint64 VAR;

#define assert		kassertw

#define ARRAY_SIZE(x) \
	(sizeof(x) / sizeof((x)[0]))

/* There is no EISA PnP support, some drivers need to know about it */
#define EISA_bus 0

/* Old timing code may use these */
#define jiffies		(get_system_time()/1000LL)
#define HZ			1000
#define JIFF2U(a)	((a)*1000LL)    /* Convert time in JIFFIES to micro sec. */

/* It would be better if these could be mapped to kerndbg macros */
#ifndef NO_DEBUG_STUBS
# ifndef KERN_DEBUG
#  define KERN_DEBUG
# endif
# ifndef KERN_INFO
#  define KERN_INFO
# endif
# ifndef KERN_ERR
#  define KERN_ERR
# endif
# ifndef KERN_WARNING
#  define KERN_WARNING
# endif
# ifndef KERN_NOTICE
#  define KERN_NOTICE
# endif
# ifndef KERN_ALERT
#  define KERN_ALERT
# endif
# ifndef KERN_CRIT
#  define KERN_CRIT
# endif
#endif

/* I/O */
#define FMODE_READ		1	
#define FMODE_WRITE		2

/* Memory */
#define ALIGN( p, m) \
	((typeof(p))(((uint32)(p)+m)&~m))
#define ALIGN8( p ) \
	ALIGN(p,7)
#define ALIGN16( p ) \
	ALIGN(p,15)
#define ALIGN32( p ) \
	ALIGN(p,31)

#define get_unaligned(ptr) \
	(*(ptr))
#define put_unaligned(val, ptr) \
	((void)( *(ptr) = (val) ))

#define virt_to_phys(p)	((uint32)(p))
#define virt_to_bus		virt_to_phys
#define phys_to_virt(p) ((void*)(p))
#define bus_to_virt 	phys_to_virt

#define cpu_to_le64(n)	((uint64)(n))
#define le64_to_cpu(n)	((uint64)(n))
#define cpu_to_le32(n)	((uint32)(n))
#define le32_to_cpu(n)	((uint32)(n))
#define cpu_to_le16(x)	((uint16)(x))
#define le16_to_cpu(x)	((uint16)(x))

#define virt_to_le32desc(addr) \
	cpu_to_le32(virt_to_bus(addr))
#define le32desc_to_virt(addr) \
	bus_to_virt(le32_to_cpu(addr))

#define swab32(val)	\
((uint32)	(((uint32)(val) & (uint32)0xff000000 ) >> 24 ) | \
	   		(((uint32)(val) & (uint32)0x00ff0000 ) >> 8 ) | \
	   		(((uint32)(val) & (uint32)0x0000ff00 ) << 8 ) | \
	   		(((uint32)(val) & (uint32)0x000000ff ) << 24 ) )

#define be32_to_cpu(n)	swab32(n)
#define cpu_to_be32(n)	swab32(n)

static inline void * ioremap( unsigned long phys_addr, unsigned long size )
{
	area_id hArea;
	void *pAddr = NULL;

	hArea = create_area( "mmio", &pAddr, size, size, AREA_KERNEL|AREA_ANY_ADDRESS|AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( hArea, (void *) ( phys_addr & PCI_ADDRESS_IO_MASK ) );

	return pAddr;
}

#define copy_to_user(x, y, z) \
	memcpy_to_user(x, y, z)
#define copy_from_user(x, y, z) \
	memcpy_from_user(x, y, z)

#define put_user(x, y) \
	do { *y = x; } while(0)
#define get_user(x, y) \
	do { x = *y; } while(0)

/* Are these valid? */
static inline void * request_region(unsigned long start, unsigned long len, char *name)
{
	return (void *)1;
}

static inline void release_region(unsigned long start, unsigned long len)
{
	/* EMPTY */
}

/* Semaphores & IPC */
#define spin_lock(x) \
	spinlock(x)
#define spin_unlock(x) \
	spinunlock(x)

#define spin_lock_irq(x) \
	spinlock(x)
#define spin_unlock_irq(x) \
	spinunlock(x)

#define spin_lock_irqsave(x, y) \
	spinlock_cli(x, y)
#define spin_unlock_irqrestore(x, y) \
	spinunlock_restore(x, y)

#define up(x)	UNLOCK(x)
#define down(x)	LOCK(x)

/* PCI devices */
static inline void* pci_alloc_consistent( PCI_Info_s *psDev, size_t nSize, dma_addr_t *hDma )
{
	void *p = kmalloc( nSize, MEMF_KERNEL );
	*hDma = (uint32)p;
	return p;
}

#define pci_free_consistent( cookie, size, ptr, dma_ptr ) \
	kfree( ptr )
#define pci_map_single( cookie, address, size, dir ) \
	virt_to_bus( address )
#define pci_unmap_single( cookie, address, size, dir ) \
	{/* EMPTY */}
#define pci_unmap_page( cookie, address, size, dir ) \
	{/* EMPTY */}

/* The following do nothing */
#define pci_unmap_addr( s, m )	(0)
#define pci_unmap_len( s, m )	(0)

/* psBus must be available & be a valid PCI bus instance */
#define pci_read_config_byte(dev, address, val) \
	val = psBus->read_pci_config((dev)->nBus, (dev)->nDevice, (dev)->nFunction, address, sizeof(val))
#define pci_read_config_word(dev, address, val) \
	val = psBus->read_pci_config((dev)->nBus, (dev)->nDevice, (dev)->nFunction, address, sizeof(val))
#define pci_read_config_dword(dev, address, val) \
	val = psBus->read_pci_config((dev)->nBus, (dev)->nDevice, (dev)->nFunction, address, sizeof(val))
#define pci_write_config_byte(dev, address, val) \
	psBus->write_pci_config((dev)->nBus, (dev)->nDevice, (dev)->nFunction, address, sizeof(val), (uint32)val)
#define pci_write_config_word(dev, address, val) \
	psBus->write_pci_config((dev)->nBus, (dev)->nDevice, (dev)->nFunction, address, sizeof(val), (uint32)val)
#define pci_write_config_dword(dev, address, val) \
	psBus->write_pci_config((dev)->nBus, (dev)->nDevice, (dev)->nFunction, address, sizeof(val), (uint32)val)

#define pci_set_master(dev) \
	psBus->enable_pci_master((dev)->nBus, (dev)->nDevice, (dev)->nFunction)

#define pci_find_capability(dev, cap) \
	psBus->get_pci_capability((dev)->nBus, (dev)->nDevice, (dev)->nFunction, cap)

static inline PCI_Info_s * pci_find_device( int vendor_id, int device_id, PCI_Info_s * pci_dev )
{
	int i;
	static PCI_Info_s  sInfo;
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return NULL;

	for( i = 0 ; psBus->get_pci_info( &sInfo, i ) == 0 ; ++i )
		if ( sInfo.nVendorID == vendor_id && sInfo.nDeviceID == device_id)
			return &sInfo;

	return NULL;
}

static inline uint32 pci_resource_len( PCI_Info_s * pci_dev, int resource )
{
	int nOffset;
	uint32 nBase, nSize;

	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return 0;

	nOffset = PCI_BASE_REGISTERS + resource * 4;
	nBase = psBus->read_pci_config( pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction, nOffset, 4 );

	psBus->write_pci_config( pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction, nOffset, 4, ~0 );
	nSize = psBus->read_pci_config( pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction, nOffset, 4 );
	psBus->write_pci_config( pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction, nOffset, 4, nBase );
	if( nSize == 0 || nSize == ~0 )
		return 0;
	if( nBase == ~0 )
		nBase = 0;
	if( ( nSize & PCI_ADDRESS_SPACE ) == 0 )
	{
		nSize &= PCI_ADDRESS_MEMORY_32_MASK;
		nSize &= ~( nSize - 1 );
	}
	else
	{
		nSize &= ( PCI_ADDRESS_IO_MASK & 0xffff );
		nSize &= ~( nSize - 1 );
	}
	return nSize;
}

static inline int pci_save_state( PCI_Info_s *dev, uint32 *buf )
{
	int i;
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return -1;

	if( buf )
		for( i = 0; i < 16; i++ )
			buf[i] = psBus->read_pci_config(dev->nBus, dev->nDevice, dev->nFunction, i * 4, 4);

	return 0;
}

static inline int pci_restore_state( PCI_Info_s *dev, uint32 *buf )
{
	int i;
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return -1;

	if( buf )
		for( i = 0; i < 16; i++ )
			psBus->write_pci_config(dev->nBus, dev->nDevice, dev->nFunction, i * 4, 4, buf[i]);

	return 0;
}

/* No implementation possible until the PCI bus manager supports MSI */
static inline int pci_disable_msi( PCI_Info_s *dev )
{
	kerndbg( KERN_WARNING, "%s not implemented!\n", __FUNCTION__ );
	return 1;
}

static inline int pci_enable_msi( PCI_Info_s *dev )
{
	kerndbg( KERN_WARNING, "%s not implemented!\n", __FUNCTION__ );
	return 1;
}

/**
 * pci_set_power_state - Set the power state of a PCI device
 * @dev: PCI device to be suspended
 * @state: Power state we're entering
 *
 * Transition a device to a new power state, using the Power Management 
 * Capabilities in the device's config space.
 *
 * It would be preferable to have this as a function of the PCI bus
 * manager, but that won't be possible until we can get the ACPI handle
 * from a PCI device.
 *
 * RETURN VALUE: 
 * -EINVAL if trying to enter a lower state than we're already in.
 * -EIO if device does not support PCI PM.
 * 0 if we can successfully change the power state.
 */
static inline pci_set_power_state( PCI_Info_s *dev, int state )
{
	int pm;
	int pmcsr;
	int current_state;
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return -EIO;

	/* bound the state we're entering */
	if (state > 3)
		state = 3;

	current_state = psBus->read_pci_config( dev->nBus, dev->nDevice, dev->nFunction, pm + PCI_PM_CTRL, 2 );

	/* Validate current state:
	 * Can enter D0 from any state, but if we can only go deeper 
	 * to sleep if we're already in a low power state
	 */
	if (state > 0 && current_state > state)
		return -EINVAL;

	/* find PCI PM capability in list */
	pm = pci_find_capability(dev, PCI_CAP_ID_PM);

	/* abort if the device doesn't support PM capabilities */
	if (!pm)
		return -EIO; 

	/* check if this device supports the desired state */
	if (state == 1 || state == 2) {
		uint16 pmc;
		pmc = psBus->read_pci_config( dev->nBus, dev->nDevice, dev->nFunction, pm + PCI_PM_PMC, 2 );
		if (state == 1 && !(pmc & PCI_PM_CAP_D1))
			return -EIO;
		else if (state == 2 && !(pmc & PCI_PM_CAP_D2))
			return -EIO;
	}

	/* If we're in D3, force entire word to 0.
	 * This doesn't affect PME_Status, disables PME_En, and
	 * sets PowerState to 0.
	 */
	if (current_state == 3)
		pmcsr = 0;
	else {
		pmcsr = psBus->read_pci_config( dev->nBus, dev->nDevice, dev->nFunction, pm + PCI_PM_CTRL, 2);
		pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
		pmcsr |= state;
	}

	/* enter specified state */
	psBus->write_pci_config( dev->nBus, dev->nDevice, dev->nFunction, pm + PCI_PM_CTRL, 2, pmcsr );

	return 0;
}

/* prefetch() may not be needed at all: check */
#define prefetch(x);

#define PCI_ANY_ID (~0)
#define PCIBIOS_MAX_LATENCY 255
#define PCI_CLASS_REVISION 0x08

typedef int pci_power_t;

#define PCI_D0		((pci_power_t) 0)
#define PCI_D1		((pci_power_t) 1)
#define PCI_D2		((pci_power_t) 2)
#define PCI_D3hot	((pci_power_t) 3)
#define PCI_D3cold	((pci_power_t) 4)
#define PCI_UNKNOWN	((pci_power_t) 5)
#define PCI_POWER_ERROR	((pci_power_t) -1)

struct pci_device_id
{
	unsigned int vendor_id, device_id;
	unsigned int subvendor, subdevice;
	unsigned int class, class_mask;
	unsigned long driver_data;
};

#define	disable_irq	disable_irq_nosync

/* Net */
static inline PacketBuf_s* alloc_pkt_buffer_aligned(int nSize)
{
	PacketBuf_s *psBuffer;

	psBuffer = alloc_pkt_buffer( nSize+15 );
	if( psBuffer )
		psBuffer->pb_pData = ALIGN16( psBuffer->pb_pData );

	return psBuffer;
}

static inline void* skb_put( PacketBuf_s* psBuffer, int nSize )
{
	void* pOldEnd = psBuffer->pb_pHead + psBuffer->pb_nSize;

	psBuffer->pb_nSize += nSize;
	return( pOldEnd );
}

#define MAX_SKB_FRAGS	(65536/PAGE_SIZE + 2)

#define MAX_ADDR_LEN IFHWADDRLEN

#define IFF_RUNNING 0x40

static inline is_valid_ether_addr( uint8 * addr )
{
	return !( addr[0] & 1 ) && memcmp( addr, "\0\0\0\0\0\0", 6 );
}

/* The following macros assume that dev has the fields tbusy, start & flags */
#define netif_wake_queue(dev) \
	do { clear_bit(0, (void*)&dev->tbusy); } while(0)
#if 0
#define netif_start_queue(dev) \
	clear_bit(0, (void*)&dev->tbusy)
#else
#define netif_start_queue(dev) \
	do { clear_bit(0, (void*)&dev->tbusy); dev->start = 1; } while(0)
#endif

#define netif_start_tx_queue(dev) \
	do { (dev)->tbusy = 0; dev->start = 1; } while(0)
#define netif_stop_tx_queue(dev) \
	do { (dev)->tbusy = 1; dev->start = 0; } while(0)
#define netif_stop_queue(dev) \
	set_bit(0, (void*)&dev->tbusy)
#define netif_queue_paused(dev) \
	((dev)->tbusy != 0)
#define netif_pause_tx_queue(dev) \
	(test_and_set_bit(0, (void*)&dev->tbusy))
#define netif_unpause_tx_queue(dev) \
	do { clear_bit(0, (void*)&dev->tbusy); } while(0)
#define netif_resume_tx_queue(dev) \
	do { clear_bit(0, (void*)&dev->tbusy); } while(0)
#define netif_running(dev) \
	((dev)->start != 0)
#define netif_mark_up(dev) \
	do { (dev)->start = 1; } while (0)
#define netif_mark_down(dev) \
	do { (dev)->start = 0; } while (0)
#define netif_queue_stopped(dev) \
	((dev)->tbusy)
#define netif_link_down(dev) \
	(dev)->flags &= ~ IFF_RUNNING
#define netif_link_up(dev) \
	(dev)->flags |= IFF_RUNNING
#define netif_carrier_on(dev) \
	netif_link_up(dev)
#define netif_carrier_off(dev) \
	netif_link_down(dev)
#define netif_carrier_ok(dev) \
	((dev)->flags & IFF_RUNNING)

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_LINUX_COMPAT_H__ */

