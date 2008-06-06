/*
 *  The Syllable kernel
 *  PCI busmanager
 *	Copyright (C) 2008 Dee Sharpe
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  
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

#include <posix/errno.h>
#include <atheos/device.h>
#include <atheos/kernel.h>
#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/pci.h>

#include <macros.h>

static status_t find_io( PCI_Entry_s* psInfo, uintptr_t *pAddress, uintptr_t *pSize, int nReg, int nType );
static status_t find_mem( PCI_Entry_s* psInfo, uintptr_t *pAddress, uintptr_t *pSize, int nReg, int nType );
static status_t find_rom( void );

/** 
 * \par Description: Used PCI info struct to determine the device type
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Dee Sharpe (demetrioussharpe@netscape.net)
 *****************************************************************************/
int get_pci_device_type( PCI_Info_s* psInfo )
{
	if(!psInfo)
		return -ENODEV;
	
	switch(psInfo->nClassBase)
	{
		case PCI_MASS_STORAGE:
				return DEVICE_STORAGE;
		
		case PCI_NETWORK:
				return DEVICE_NET;
		
		case PCI_DISPLAY:
				return DEVICE_VIDEO;
		
		case PCI_MULTIMEDIA:
			switch(psInfo->nClassSub)
			{
				case PCI_VIDEO:
					return DEVICE_VIDEO;
				case PCI_AUDIO:
				case PCI_HDAUDIO:
					return DEVICE_AUDIO;
				case PCI_TELEPHONY:
					return ;
				default:
					return DEVICE_UNKNOWN;
			}			
		
		case PCI_MEMORY:
		case PCI_BRIDGE:
		case PCI_BASE_PERIPHERAL:
			return DEVICE_SYSTEM;
		
		case PCI_SIMPLE_COMMUNICATIONS:
				return DEVICE_UNKNOWN;
		
		case PCI_INPUT:
			return DEVICE_INPUT;
		
		case PCI_DOCKING_STATION:
			return DEVICE_DOCK;
		
		case PCI_PROCESSOR:
			return DEVICE_PROCESSOR;
		
		case PCI_SERIAL_BUS:
		case PCI_WIRELESS:
		case PCI_I2O:
		case PCI_SATCOM:
				return DEVICE_UNKNOWN;
		case PCI_CRYPTO:
			return DEVICE_CRYPTO;
		case PCI_DASP:
		case PCI_UNDEFINED:
		case PCI_EARLY:
		default:
				return DEVICE_UNKNOWN;
	}
}

/** 
 * \par Description: Used PCI info struct to access an address stored in a base address register
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Dee Sharpe (demetrioussharpe@netscape.net)
 *****************************************************************************/
status_t get_bar_info( PCI_Entry_s* psInfo, uintptr_t *pAddress, uintptr_t *pnSize, int nReg, int nType )
{
	if(PCI_BAR_TYPE(nType) == PCI_BAR_TYPE_IO) {
		return (find_io(psInfo, pAddress, pnSize, nReg, nType));
	} else {
		return (find_mem(psInfo, pAddress, pnSize, nReg, nType));
	}
}

status_t find_io( PCI_Entry_s* psInfo, uintptr_t *pAddress, uintptr_t *pSize, int nReg, int nType )
{
	int nAddress, nMask;
	
	if(nReg < PCI_BAR_START || nReg & 3) {
		kerndbg(KERN_WARNING, "PCI: get_bar_info: find_io: bad request\n");
		return -EFAULT;
	}
	
	nAddress = read_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4);
	write_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4, 0xffffffff);
	
	nMask = read_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4);
	write_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4, nAddress);
	
	if( PCI_BAR_MEM_TYPE(nAddress) != PCI_BAR_TYPE_IO) {
		kerndbg(KERN_WARNING, "PCI: get_bar_info: find_io: expected type i/o, found mem\n");
		return -EINVAL;
	}
	
	if(PCI_BAR_IO_SIZE(nMask) == 0) {
		kerndbg(KERN_WARNING, "PCI: get_bar_info: find_io: void region\n");
		return -ENXIO;
	}
	
	if(pAddress != 0)
		*pAddress = PCI_BAR_IO_ADDRESS(nAddress);
	if(pSize != 0)
		*pSize = PCI_BAR_IO_SIZE(nMask);
	
	return 0;
}

status_t find_mem( PCI_Entry_s* psInfo, uintptr_t *pAddress, uintptr_t *pSize, int nReg, int nType )
{
	uint32 nAddress0, nMask0, nAddress1 = 0, nMask1 = 0xffffffff;
	uint64 wAddress, wMask;
	int32 nIsRom, nIs64Bit;
	
	nIs64Bit = (PCI_BAR_MEM_TYPE(nType) == PCI_ADDRESS_TYPE_64);
	nIsRom = (nReg == PCI_BAR_ROM);
	
	if((!nIsRom) && (nReg < PCI_BAR_START || (nReg & 3))) {
		kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: bad request\n");
		return -EINVAL;
	}
	
	if(nIs64Bit && (nReg + 4) >= PCI_BAR_END) {
		kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: bad 64-bit request\n");
		return -EINVAL;
	}
	
	nAddress0 = read_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4);
	write_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4, 0xffffffff);
	
	nMask0 = read_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4);
	write_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg, 4, nAddress0);
	
	if(nIs64Bit) {
		nAddress1 = read_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg + 4, 4);
		write_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg + 4, 4, 0xffffffff);
		
		nMask1 = read_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg + 4, 4);
		write_pci_config(psInfo->nBus, psInfo->nDevice, psInfo->nFunction, nReg + 4, 4, nAddress1);
	}
	
	if(!nIsRom) {
		if(PCI_BAR_TYPE(nAddress0) != PCI_BAR_TYPE_MEM) {
			kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: expected type mem, found i/o\n");
			kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: address type is 0x%x\n", PCI_BAR_TYPE(nAddress0));
		return -EINVAL;
		}
		if(PCI_BAR_MEM_TYPE(nAddress0) != PCI_BAR_MEM_TYPE(nType)) {
			kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: expected mem type %08x, found %08x\n",
			    	PCI_BAR_MEM_TYPE(nType), PCI_BAR_MEM_TYPE(nAddress0));
		return -EINVAL;
		}
	}
	
	wAddress = (uint64)nAddress1 << 32UL | nAddress0;
	wMask = (uint64)nMask1 << 32UL | nMask0;
	
	if((nIs64Bit && PCI_BAR_MEM64_SIZE(wMask) == 0) ||
	   (!nIs64Bit && PCI_BAR_MEM_SIZE(nMask0) == 0)) {
		kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: void region\n");
		return -EFAULT;
	}
	
	switch(PCI_BAR_MEM_TYPE(nAddress0)) {
		case PCI_ADDRESS_TYPE_32:
		case PCI_ADDRESS_TYPE_32_LOW:
			break;
		case PCI_ADDRESS_TYPE_64:
			if(sizeof(uint64) > sizeof(uintptr_t) &&
			  (nAddress1 != 0 || nMask1 != 0xffffffff)) {
				kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: 64-bit memory map which is "
									  "inaccessible on a 32-bit platform\n");
				return -EFAULT;
			} break;
		default:
			{
				kerndbg(KERN_WARNING, "PCI: get_bar_info: find_mem: reserved mapping register type\n");
				return -EFAULT;
			}
	}	
	
	if(sizeof(uint64) > sizeof(uintptr_t)) {
		if(pAddress != 0)
			*pAddress = PCI_BAR_MEM_ADDRESS(nAddress0);
		if(pSize != 0)
			*pSize = PCI_BAR_MEM_SIZE(nMask0);
	} else {
		if(pAddress != 0)
			*pAddress = PCI_BAR_MEM64_ADDRESS(wAddress);
		if(pSize != 0)
			*pSize = PCI_BAR_MEM64_SIZE(wMask);
	}
		
	return 0;
}

status_t find_rom( void )
{
	return 0;
}




