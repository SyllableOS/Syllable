
/*  The Syllable application server
 *  Appserver driver <-> PCI kernel driver interface
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_APPSERVER_PCI_GRAPHICS_H__
#define __F_APPSERVER_PCI_GRAPHICS_H__

#include <atheos/types.h>
#include <atheos/device.h>
#ifndef __KERNEL__
#include <sys/ioctl.h>
#endif

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

struct gfx_pci_config
{

#ifdef __cplusplus
	gfx_pci_config()
	{
	}
	gfx_pci_config( int nBus, int nDevice, int nFunction, int nOffset, int nSize )
	{
		m_nBus = nBus;
		m_nDevice = nDevice;
		m_nFunction = nFunction;
		m_nOffset = nOffset;
		m_nSize = nSize;
	}
	gfx_pci_config( int nBus, int nDevice, int nFunction, int nOffset, int nSize, uint32 nValue )
	{
		m_nBus = nBus;
		m_nDevice = nDevice;
		m_nFunction = nFunction;
		m_nOffset = nOffset;
		m_nSize = nSize;
		m_nValue = nValue;
	}
#endif
	int m_nBus;
	int m_nDevice;
	int m_nFunction;
	int m_nOffset;
	int m_nSize;
	uint32 m_nValue;
};

enum
{
	PCI_GFX_GET_PCI_INFO,
	PCI_GFX_READ_PCI_CONFIG,
	PCI_GFX_WRITE_PCI_CONFIG,
	PCI_GFX_LAST_IOCTL
};

static inline uint32 pci_gfx_read_config( int nFd, int nBus, int nDev, int nFnc, int nOffset, int nSize )
{
	struct gfx_pci_config sConfig;

	sConfig.m_nBus = nBus;
	sConfig.m_nDevice = nDev;
	sConfig.m_nFunction = nFnc;
	sConfig.m_nOffset = nOffset;
	sConfig.m_nSize = nSize;

	ioctl( nFd, PCI_GFX_READ_PCI_CONFIG, &sConfig );
	return ( sConfig.m_nValue );
}


static inline status_t pci_gfx_write_config( int nFd, int nBus, int nDev, int nFnc, int nOffset, int nSize, uint32 nVal )
{
	struct gfx_pci_config sConfig;

	sConfig.m_nBus = nBus;
	sConfig.m_nDevice = nDev;
	sConfig.m_nFunction = nFnc;
	sConfig.m_nOffset = nOffset;
	sConfig.m_nSize = nSize;
	sConfig.m_nValue = nVal;

	ioctl( nFd, PCI_GFX_WRITE_PCI_CONFIG, &sConfig );
	return ( sConfig.m_nValue );
}



#ifdef __cplusplus
}
#endif

#endif // __F_APPSERVER_PCI_GRAPHICS_H__


