/*
 *  helper functions and some pci-related functions not yet found
 *  in atheos kernel
 */


/* Enable probing on EISA bus. I'm not sure what happens when probing for
EISA cards if there's no EISA bus, so it's safe to keep this disabled
unless you have supported EISA card. */
#define EISA_bus 0






#define jiffies     (get_system_time()/1000LL)
#define HZ          1000
#define JIFF2U(a)   ((a)*1000LL)    /* Convert time in JIFFIES to micro sec. */






#define ALIGN( p, m)  (  (typeof(p)) (((uint32)(p)+m)&~m)  )
#define ALIGN8( p )   ALIGN(p,7)
#define ALIGN16( p )  ALIGN(p,15)
#define ALIGN32( p )  ALIGN(p,31)

/* To align packets. */
PacketBuf_s *alloc_pkt_buffer_aligned(int nSize) {
    PacketBuf_s *skb;
        
    skb = alloc_pkt_buffer( nSize+15 );
    if (skb)
        skb->pb_pData = ALIGN16( skb->pb_pData );
    
    return skb;
}


/* Needed because Athe does not have skb, guess atheos never will get it either */
void* skb_put( PacketBuf_s* psBuffer, int nSize )
{
    void* pOldEnd = psBuffer->pb_pHead + psBuffer->pb_nSize;
    
    psBuffer->pb_nSize += nSize;
    return( pOldEnd );
}






#define cpu_to_le32(n)   ( (uint32)(n) )
#define le32_to_cpu(n)   ( (uint32)(n) )

#define virt_to_bus(p)   ( (uint32)(p) )


#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

/* FIXME: I'm not sure, this is correct */
static inline void *ioremap(unsigned long phys_addr, unsigned long size) {
    return (void *)phys_addr;
}








#define PCI_ANY_ID (~0)

struct pci_device_id {
    unsigned int vendor_id, device_id;      /* Vendor and device ID or PCI_ANY_ID */
    unsigned int subvendor, subdevice;  /* Subsystem ID's or PCI_ANY_ID */
    unsigned int class, class_mask;     /* (class,subclass,prog-if) triplet */
    unsigned long driver_data;      /* Data private to the driver */
};





static inline void *request_region(unsigned long start, unsigned long len, char *name) {
    return (void *)1;    // not-NULL value
}

static inline void release_region(unsigned long start, unsigned long len) {
}




#define PCIBIOS_MAX_LATENCY 255

/**
 * pci_set_master - enables bus-mastering for device dev
 * @dev: the PCI device to enable
 *
 * Enables bus-mastering on the device and calls pcibios_set_master()
 * to do the needed arch specific settings.
 */
static void pci_set_master(PCI_Info_s *dev)
{
    int cmd;
    int lat;

    cmd = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction, PCI_COMMAND, 2 );
    if (! (cmd & PCI_COMMAND_MASTER)) {
        printk(KERN_DEBUG PFX "PCI: Enabling bus mastering\n");
        cmd |= PCI_COMMAND_MASTER;
        write_pci_config( dev->nBus, dev->nDevice, dev->nFunction, PCI_COMMAND, 2, cmd );
    }
    
    /* pcibios_set_master() */
    lat = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction, PCI_LATENCY, 1);
    if (lat < 16)
        lat = (64 <= PCIBIOS_MAX_LATENCY) ? 64 : PCIBIOS_MAX_LATENCY;
    else if (lat > PCIBIOS_MAX_LATENCY)
        lat = PCIBIOS_MAX_LATENCY;
    else
        return;
    printk(KERN_DEBUG PFX "PCI: Setting latency timer to %d\n", lat);
    write_pci_config( dev->nBus, dev->nDevice, dev->nFunction, PCI_LATENCY, 1, lat);
}





#define  PCI_STATUS_CAP_LIST    0x10    /* Support Capability List */
#define  PCI_HEADER_TYPE_NORMAL 0
#define  PCI_HEADER_TYPE_BRIDGE 1
#define  PCI_HEADER_TYPE_CARDBUS 2
#define PCI_CB_CAPABILITY_LIST  0x14
#define PCI_CAP_LIST_ID     0   /* Capability ID */
#define PCI_CAP_LIST_NEXT   1   /* Next capability in the list */
#define  PCI_CAP_ID_PM      0x01    /* Power Management */

#define PCI_PM_PMC              2       /* PM Capabilities Register */
#define  PCI_PM_CAP_D1      0x0200  /* D1 power state support */
#define  PCI_PM_CAP_D2      0x0400  /* D2 power state support */
#define PCI_PM_CTRL     4   /* PM control and status register */
#define  PCI_PM_CTRL_STATE_MASK 0x0003  /* Current power state (D0 to D3) */


/**
 * pci_find_capability - query for devices' capabilities 
 * @dev: PCI device to query
 * @cap: capability code
 *
 * Tell if a device supports a given PCI capability.
 * Returns the address of the requested capability structure within the
 * device's PCI configuration space or 0 in case the device does not
 * support it.  Possible values for @cap:
 *
 *  %PCI_CAP_ID_PM           Power Management 
 *
 *  %PCI_CAP_ID_AGP          Accelerated Graphics Port 
 *
 *  %PCI_CAP_ID_VPD          Vital Product Data 
 *
 *  %PCI_CAP_ID_SLOTID       Slot Identification 
 *
 *  %PCI_CAP_ID_MSI          Message Signalled Interrupts
 *
 *  %PCI_CAP_ID_CHSWP        CompactPCI HotSwap 
 */
static int pci_find_capability(PCI_Info_s *dev, int cap)
{
    int status;
    int pos, id;
    int ttl = 48;

    status = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                PCI_STATUS, 2 );
    if (!(status & PCI_STATUS_CAP_LIST))
        return 0;
    switch (dev->nHeaderType) {
    case PCI_HEADER_TYPE_NORMAL:
    case PCI_HEADER_TYPE_BRIDGE:
        pos = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                PCI_CAPABILITY_LIST, 1 );
        break;
    case PCI_HEADER_TYPE_CARDBUS:
        pos = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                PCI_CB_CAPABILITY_LIST, 1 );
        break;
    default:
        return 0;
    }
    while (ttl-- && pos >= 0x40) {
        pos &= ~3;
        id = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                pos + PCI_CAP_LIST_ID, 1 );
        if (id == 0xff)
            break;
        if (id == cap)
            return pos;
        pos = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                pos + PCI_CAP_LIST_NEXT, 1 );
    }
    return 0;
}




/**
 * pci_set_power_state - Set the power state of a PCI device
 * @dev: PCI device to be suspended
 * @state: Power state we're entering
 *
 * Transition a device to a new power state, using the Power Management 
 * Capabilities in the device's config space.
 *
 * RETURN VALUE: 
 * -EINVAL if trying to enter a lower state than we're already in.
 * 0 if we're already in the requested state.
 * -EIO if device does not support PCI PM.
 * 0 if we can successfully change the power state.
 */
static int pci_set_power_state(PCI_Info_s *dev, int *current_state,
                               int state) {
    int pm;
    int pmcsr;

    /* bound the state we're entering */
    if (state > 3) state = 3;

    /* Validate current state:
     * Can enter D0 from any state, but if we can only go deeper 
     * to sleep if we're already in a low power state
     */
    if (state > 0 && *current_state > state)
        return -EINVAL;
    else if (*current_state == state) 
        return 0;        /* we're already there */

    /* find PCI PM capability in list */
    pm = pci_find_capability(dev, PCI_CAP_ID_PM);
    
    /* abort if the device doesn't support PM capabilities */
    if (!pm) return -EIO; 

    /* check if this device supports the desired state */
    if (state == 1 || state == 2) {
        uint16 pmc;
        pmc = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                pm + PCI_PM_PMC, 2 );
        if (state == 1 && !(pmc & PCI_PM_CAP_D1)) return -EIO;
        else if (state == 2 && !(pmc & PCI_PM_CAP_D2)) return -EIO;
    }

    /* If we're in D3, force entire word to 0.
     * This doesn't affect PME_Status, disables PME_En, and
     * sets PowerState to 0.
     */
    if (*current_state == 3)
        pmcsr = 0;
    else {
        pmcsr = read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                    pm + PCI_PM_CTRL, 2);
        pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
        pmcsr |= state;
    }

    /* enter specified state */
    write_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                            pm + PCI_PM_CTRL, 2, pmcsr );

    *current_state = state;

    return 0;
}










