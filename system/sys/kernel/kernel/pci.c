
/*
 *  The Syllable kernel
 *  PCI busmanager wrapper
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
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>

#include <macros.h>

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sys_raw_read_pci_config( int nBusNum, int nDevFnc, int nOffset, int nSize, uint32 *pnRes )
{
	/* Get PCI busmanager */
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );

	if ( psBus == NULL )
		return ( -EFAULT );

	*pnRes = psBus->read_pci_config( nBusNum, nDevFnc >> 16, nDevFnc & 0xffff, nOffset, nSize );
	return ( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sys_raw_write_pci_config( int nBusNum, int nDevFnc, int nOffset, int nSize, uint32 nValue )
{
	/* Get PCI busmanager */
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );

	if ( psBus == NULL )
		return ( -EFAULT );

	return ( psBus->write_pci_config( nBusNum, nDevFnc >> 16, nDevFnc & 0xffff, nOffset, nSize, nValue ) );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sys_get_pci_info( PCI_Info_s * psInfo, int nIndex )
{
	/* Get PCI busmanager */
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );

	if ( psBus == NULL || psInfo == NULL )
		return ( -EFAULT );

	return ( psBus->get_pci_info( psInfo, nIndex ) );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t get_pci_info( PCI_Info_s * psInfo, int nIndex )
{
	return ( sys_get_pci_info( psInfo, nIndex ) );
}

