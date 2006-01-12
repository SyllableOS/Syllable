#ifndef _LINUXCOMP_H_
#define _LINUXCOMP_H_

#include <atheos/linux_compat.h>

#define  PCI_HEADER_TYPE_NORMAL 0
#define  PCI_HEADER_TYPE_BRIDGE 1
#define  PCI_HEADER_TYPE_CARDBUS 2

#define	PCI_CB_CAPABILITY_LIST  0x14
#define  PCI_CAP_ID_PM      0x01    /* Power Management */

#define PCI_PM_PMC              2       /* PM Capabilities Register */
#define  PCI_PM_CAP_D1      0x0200  /* D1 power state support */
#define  PCI_PM_CAP_D2      0x0400  /* D2 power state support */
#define PCI_PM_CTRL     4   /* PM control and status register */
#define  PCI_PM_CTRL_STATE_MASK 0x0003  /* Current power state (D0 to D3) */

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
static int pci_set_power_state(PCI_bus_s* psBus, PCI_Info_s *dev, int *current_state,
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
        pmc = psBus->read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
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
        pmcsr = psBus->read_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                                    pm + PCI_PM_CTRL, 2);
        pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
        pmcsr |= state;
    }

    /* enter specified state */
    psBus->write_pci_config( dev->nBus, dev->nDevice, dev->nFunction,
                            pm + PCI_PM_CTRL, 2, pmcsr );

    *current_state = state;

    return 0;
}

#endif	/* _LINUXCOMP_H_ */

