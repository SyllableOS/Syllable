
/*
 *  nVidia GeForce FX driver for Syllable
 *  Copyright (C) 2001  Joseph Artsimovich (joseph_a@mail.ru)
 *  Copyright 1993-2003 NVIDIA, Corporation.  All rights reserved.
 *
 *  Based on the rivafb linux kernel driver and the xfree86 nv driver.
 *  Video overlay code is from the rivatv project.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/vesa_gfx.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <appserver/pci_graphics.h>

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>
#include "fx.h"
#include "fx_dma.h"

static const struct chip_info asChipInfos[] = {
	{0x10DE0100, NV_ARCH_10, "GeForce 256"},
	{0x10DE0101, NV_ARCH_10, "GeForce DDR"},
	{0x10DE0103, NV_ARCH_10, "Quadro"},
	{0x10DE0110, NV_ARCH_10, "GeForce2 MX/MX 400"},
	{0x10DE0111, NV_ARCH_10, "GeForce2 MX 100/200"},
	{0x10DE0112, NV_ARCH_10, "GeForce2 Go"},
	{0x10DE0113, NV_ARCH_10, "Quadro2 MXR/EX/Go"},
	{0x10DE01A0, NV_ARCH_10, "GeForce2 Integrated GPU"},
	{0x10DE0150, NV_ARCH_10, "GeForce2 GTS"},
	{0x10DE0151, NV_ARCH_10, "GeForce2 Ti"},
	{0x10DE0152, NV_ARCH_10, "GeForce2 Ultra"},
	{0x10DE0153, NV_ARCH_10, "Quadro2 Pro"},
	{0x10DE0170, NV_ARCH_10, "GeForce4 MX 460"},
	{0x10DE0171, NV_ARCH_10, "GeForce4 MX 440"},
	{0x10DE0172, NV_ARCH_10, "GeForce4 MX 420"},
	{0x10DE0173, NV_ARCH_10, "GeForce4 MX 440-SE"},
	{0x10DE0174, NV_ARCH_10, "GeForce4 440 Go"},
	{0x10DE0175, NV_ARCH_10, "GeForce4 420 Go"},
	{0x10DE0176, NV_ARCH_10, "GeForce4 420 Go 32M"},
	{0x10DE0177, NV_ARCH_10, "GeForce4 460 Go"},
	{0x10DE0179, NV_ARCH_10, "GeForce4 440 Go 64M"},
	{0x10DE017D, NV_ARCH_10, "GeForce4 410 Go 16M"},
	{0x10DE017C, NV_ARCH_10, "Quadro4 500 GoGL"},
	{0x10DE0178, NV_ARCH_10, "Quadro4 550 XGL"},
	{0x10DE017A, NV_ARCH_10, "Quadro4 NVS"},
	{0x10DE0181, NV_ARCH_10, "GeForce4 MX 440 with AGP8X"},
	{0x10DE0182, NV_ARCH_10, "GeForce4 MX 440SE with AGP8X"},
	{0x10DE0183, NV_ARCH_10, "GeForce4 MX 420 with AGP8X"},
	{0x10DE0185, NV_ARCH_10, "GeForce4 MX 4000"},
	{0x10DE0186, NV_ARCH_10, "GeForce4 448 Go"},
	{0x10DE0187, NV_ARCH_10, "GeForce4 488 Go"},
	{0x10DE0188, NV_ARCH_10, "Quadro4 580 XGL"},
	{0x10DE018A, NV_ARCH_10, "Quadro4 280 NVS"},
	{0x10DE018B, NV_ARCH_10, "Quadro4 380 XGL"},
	{0x10DE01F0, NV_ARCH_10, "GeForce4 MX Integrated GPU"},
	{0x10DE0200, NV_ARCH_20, "GeForce3"},
	{0x10DE0201, NV_ARCH_20, "GeForce3 Ti 200"},
	{0x10DE0202, NV_ARCH_20, "GeForce3 Ti 500"},
	{0x10DE0203, NV_ARCH_20, "Quadro DCC"},
	{0x10DE0250, NV_ARCH_20, "GeForce4 Ti 4600"},
	{0x10DE0251, NV_ARCH_20, "GeForce4 Ti 4400"},
	{0x10DE0252, NV_ARCH_20, "0x0252"},
	{0x10DE0253, NV_ARCH_20, "GeForce4 Ti 4200"},
	{0x10DE0258, NV_ARCH_20, "Quadro4 900 XGL"},
	{0x10DE0259, NV_ARCH_20, "Quadro4 750 XGL"},
	{0x10DE025B, NV_ARCH_20, "Quadro4 700 XGL"},
	{0x10DE0280, NV_ARCH_20, "GeForce4 Ti 4800"},
	{0x10DE0281, NV_ARCH_20, "GeForce4 Ti 4200 with AGP8X"},
	{0x10DE0282, NV_ARCH_20, "GeForce4 Ti 4800 SE"},
	{0x10DE0286, NV_ARCH_20, "GeForce4 4200 Go"},
	{0x10DE028C, NV_ARCH_20, "Quadro4 700 GoGL"},
	{0x10DE0288, NV_ARCH_20, "Quadro4 980 XGL"},
	{0x10DE0289, NV_ARCH_20, "Quadro4 780 XGL"},
	{0x10DE0301, NV_ARCH_30, "GeForce FX 5800 Ultra"},
	{0x10DE0302, NV_ARCH_30, "GeForce FX 5800"},
	{0x10DE0308, NV_ARCH_30, "Quadro FX 2000"},
	{0x10DE0309, NV_ARCH_30, "Quadro FX 1000"},
	{0x10DE0311, NV_ARCH_30, "GeForce FX 5600 Ultra"},
	{0x10DE0312, NV_ARCH_30, "GeForce FX 5600"},
	{0x10DE0314, NV_ARCH_30, "GeForce FX 5600SE"},
	{0x10DE0316, NV_ARCH_30, "0x0316"},
	{0x10DE0317, NV_ARCH_30, "0x0317"},
	{0x10DE031A, NV_ARCH_30, "GeForce FX Go5600"},
	{0x10DE031B, NV_ARCH_30, "GeForce FX Go5650"},
	{0x10DE031C, NV_ARCH_30, "Quadro FX Go700"},
	{0x10DE031D, NV_ARCH_30, "0x031D"},
	{0x10DE031E, NV_ARCH_30, "0x031E"},
	{0x10DE031F, NV_ARCH_30, "0x031F"},
	{0x10DE0320, NV_ARCH_30, "GeForce FX 5200"},
	{0x10DE0321, NV_ARCH_30, "GeForce FX 5200 Ultra"},
	{0x10DE0322, NV_ARCH_30, "GeForce FX 5200"},
	{0x10DE0323, NV_ARCH_30, "GeForce FX 5200SE"},
	{0x10DE0324, NV_ARCH_30, "GeForce FX Go5200"},
	{0x10DE0325, NV_ARCH_30, "GeForce FX Go5250"},
	{0x10DE0326, NV_ARCH_30, "GeForce FX 5500"},
	{0x10DE0327, NV_ARCH_30, "GeForce FX 5100"},
	{0x10DE0328, NV_ARCH_30, "GeForce FX Go5200 32M/64M"},
	{0x10DE032A, NV_ARCH_30, "Quadro NVS 280 PCI"},
	{0x10DE032B, NV_ARCH_30, "Quadro FX 500/600 PCI"},
	{0x10DE032C, NV_ARCH_30, "Quadro FX Go53xx Series"},
	{0x10DE032D, NV_ARCH_30, "Quadro FX Go5100"},
	{0x10DE032F, NV_ARCH_30, "0x032F"},
	{0x10DE0330, NV_ARCH_30, "GeForce FX 5900 Ultra"},
	{0x10DE0331, NV_ARCH_30, "GeForce FX 5900"},
	{0x10DE0332, NV_ARCH_30, "GeForce FX 5900"},
	{0x10DE0333, NV_ARCH_30, "GeForce FX 5950 Ultra"},
	{0x10DE033F, NV_ARCH_30, "Quadro FX 700"},
	{0x10DE0334, NV_ARCH_30, "GeForce FX 5950ZT"},
	{0x10DE0338, NV_ARCH_30, "Quadro FX 3000"},
	{0x10DE0341, NV_ARCH_30, "GeForce FX 5700 Ultra"},
	{0x10DE0342, NV_ARCH_30, "GeForce FX 5700"},
	{0x10DE0343, NV_ARCH_30, "GeForce FX 5700LE"},
	{0x10DE0344, NV_ARCH_30, "GeForce FX 5700VE"},
	{0x10DE0347, NV_ARCH_30, "GeForce FX Go5700"},
	{0x10DE0348, NV_ARCH_30, "GeForce FX Go5700"},
	{0x10DE034C, NV_ARCH_30, "Quadro FX Go1000"},
	{0x10DE034E, NV_ARCH_30, "Quadro FX 1100"},
	{0x10DE0040, NV_ARCH_40, "GeForce 6800 Ultra" },
	{0x10DE0041, NV_ARCH_40, "GeForce 6800" },
	{0x10DE0042, NV_ARCH_40, "GeForce 6800 LE" },
	{0x10DE0043, NV_ARCH_40, "GeForce 6800 XE" },
	{0x10DE0045, NV_ARCH_40, "GeForce 6800 GT" },
	{0x10DE0046, NV_ARCH_40, "GeForce 6800 GT" },
	{0x10DE0047, NV_ARCH_40, "GeForce 6800 GS" },
	{0x10DE0048, NV_ARCH_40, "GeForce 6800 XT" },
	{0x10DE004E, NV_ARCH_40, "Quadro FX 4000" },

	{0x10DE00C0, NV_ARCH_40, "GeForce 6800 GS" },
	{0x10DE00C1, NV_ARCH_40, "GeForce 6800" },
	{0x10DE00C2, NV_ARCH_40, "GeForce 6800 LE" },
	{0x10DE00C3, NV_ARCH_40, "GeForce 6800 XT" },
	{0x10DE00C8, NV_ARCH_40, "GeForce Go 6800" },
	{0x10DE00C9, NV_ARCH_40, "GeForce Go 6800 Ultra" },
	{0x10DE00CC, NV_ARCH_40, "Quadro FX Go1400" },
	{0x10DE00CD, NV_ARCH_40, "Quadro FX 3450/4000 SDI" },
	{0x10DE00CE, NV_ARCH_40, "Quadro FX 1400" },

	{0x10DE0140, NV_ARCH_40, "GeForce 6600 GT" },
	{0x10DE0141, NV_ARCH_40, "GeForce 6600" },
	{0x10DE0142, NV_ARCH_40, "GeForce 6600 LE" },
	{0x10DE0143, NV_ARCH_40, "GeForce 6600 VE" },
	{0x10DE0144, NV_ARCH_40, "GeForce Go 6600" },
	{0x10DE0145, NV_ARCH_40, "GeForce 6610 XL" },
	{0x10DE0146, NV_ARCH_40, "GeForce Go 6600 TE/6200 TE" },
	{0x10DE0147, NV_ARCH_40, "GeForce 6700 XL" },
	{0x10DE0148, NV_ARCH_40, "GeForce Go 6600" },
	{0x10DE0149, NV_ARCH_40, "GeForce Go 6600 GT" },
	{0x10DE014E, NV_ARCH_40, "Quadro FX 540" },
	{0x10DE014F, NV_ARCH_40, "GeForce 6200" },

	{0x10DE0160, NV_ARCH_40, "GeForce 6500" },
	{0x10DE0161, NV_ARCH_40, "GeForce 6200 TurboCache(TM)" },
	{0x10DE0162, NV_ARCH_40, "GeForce 6200SE TurboCache(TM)" },
	{0x10DE0163, NV_ARCH_40, "GeForce 6200 LE" },
	{0x10DE0164, NV_ARCH_40, "GeForce Go 6200" },
	{0x10DE0165, NV_ARCH_40, "Quadro NVS 285" },
	{0x10DE0166, NV_ARCH_40, "GeForce Go 6400" },
	{0x10DE0167, NV_ARCH_40, "GeForce Go 6200" },
	{0x10DE0168, NV_ARCH_40, "GeForce Go 6400" },
	{0x10DE0169, NV_ARCH_40, "GeForce 6250" },

	{0x10DE0211, NV_ARCH_40, "GeForce 6800" },
	{0x10DE0212, NV_ARCH_40, "GeForce 6800 LE" },
	{0x10DE0215, NV_ARCH_40, "GeForce 6800 GT" },
	{0x10DE0218, NV_ARCH_40, "GeForce 6800 XT" },

	{0x10DE0221, NV_ARCH_40, "GeForce 6200" },

	{0x10DE0090, NV_ARCH_40, "GeForce 7800 GTX" },
	{0x10DE0091, NV_ARCH_40, "GeForce 7800 GTX" },
	{0x10DE0092, NV_ARCH_40, "GeForce 7800 GT" },
	{0x10DE0093, NV_ARCH_40, "GeForce 7800 GS" },
	{0x10DE0095, NV_ARCH_40, "GeForce 7800 SLI" },
	{0x10DE0098, NV_ARCH_40, "GeForce Go 7800" },
	{0x10DE0099, NV_ARCH_40, "GeForce Go 7800 GTX" },
	{0x10DE009D, NV_ARCH_40, "Quadro FX 4500" },

	{0x10DE01D0, NV_ARCH_40, "GeForce 7350 LE" },
	{0x10DE01D1, NV_ARCH_40, "GeForce 7300 LE" },
	{0x10DE01D6, NV_ARCH_40, "GeForce Go 7200" },
	{0x10DE01D7, NV_ARCH_40, "GeForce Go 7300" },
	{0x10DE01D8, NV_ARCH_40, "GeForce Go 7400" },
	{0x10DE01DA, NV_ARCH_40, "Quadro NVS 110M" },
	{0x10DE01DB, NV_ARCH_40, "Quadro NVS 120M" },
	{0x10DE01DC, NV_ARCH_40, "Quadro FX 350M" },
	{0x10DE01DE, NV_ARCH_40, "Quadro FX 350" },
	{0x10DE01DF, NV_ARCH_40, "GeForce 7300 GS" },

	{0x10DE0398, NV_ARCH_40, "GeForce Go 7600" },
	{0x10DE0399, NV_ARCH_40, "GeForce Go 7600 GT"},
	{0x10DE039A, NV_ARCH_40, "Quadro NVS 300M" },
	{0x10DE039C, NV_ARCH_40, "Quadro FX 550M" },

	{0x10DE0298, NV_ARCH_40, "GeForce Go 7900 GS" },
	{0x10DE0299, NV_ARCH_40, "GeForce Go 7900 GTX" },
	{0x10DE029A, NV_ARCH_40, "Quadro FX 2500M" },
	{0x10DE029B, NV_ARCH_40, "Quadro FX 1500M" },

	{0x10DE0240, NV_ARCH_40, "GeForce 6150" },
	{0x10DE0241, NV_ARCH_40, "GeForce 6150 LE" },
	{0x10DE0242, NV_ARCH_40, "GeForce 6100" },
	{0x10DE0244, NV_ARCH_40, "GeForce Go 6150" },
	{0x10DE0247, NV_ARCH_40, "GeForce Go 6100" },
	{0x10DE03D1, NV_ARCH_40, "GeForce 6100" },
	
	{ 0x10DE016A, NV_ARCH_40, "GeForce 7100 GS" },
	{ 0x10DE01D3, NV_ARCH_40, "GeForce 7300 SE/7200 GS" },
	
	/*         8 series and other new cards          */
	
	{ 0x10DE0191, NV_ARCH_40, "GeForce 8800 GTX" },
	{ 0x10DE0193, NV_ARCH_40, "GeForce 8800 GTS" },
	{ 0x10DE0194, NV_ARCH_40, "GeForce 8800 Ultra" },
	{ 0x10DE019D, NV_ARCH_40, "Quadro FX 5600" },
	{ 0x10DE019E, NV_ARCH_40, "Quadro FX 4600" },
	{ 0x10DE0400, NV_ARCH_40, "GeForce 8600 GTS" },
	{ 0x10DE0402, NV_ARCH_40, "GeForce 8600 GT" },
	{ 0x10DE0404, NV_ARCH_40, "GeForce 8400 GS" },
	{ 0x10DE0407, NV_ARCH_40, "GeForce 8600M GT" },
	{ 0x10DE0409, NV_ARCH_40, "GeForce 8700M GT" },
	{ 0x10DE040A, NV_ARCH_40, "Quadro FX 370" },
	{ 0x10DE040B, NV_ARCH_40, "Quadro NVS 320M" },
	{ 0x10DE040C, NV_ARCH_40, "Quadro FX 570M" },
	{ 0x10DE040D, NV_ARCH_40, "Quadro FX 1600M" },
	{ 0x10DE040E, NV_ARCH_40, "Quadro FX 570" },
	{ 0x10DE040F, NV_ARCH_40, "Quadro FX 1700" },
	{ 0x10DE0421, NV_ARCH_40, "GeForce 8500 GT" },
	{ 0x10DE0422, NV_ARCH_40, "GeForce 8400 GS" },
	{ 0x10DE0423, NV_ARCH_40, "GeForce 8300 GS" },
	{ 0x10DE0425, NV_ARCH_40, "GeForce 8600M GS" },
	{ 0x10DE0426, NV_ARCH_40, "GeForce 8400M GT" },
	{ 0x10DE0427, NV_ARCH_40, "GeForce 8400M GS" },
	{ 0x10DE0428, NV_ARCH_40, "GeForce 8400M G" },
	{ 0x10DE0429, NV_ARCH_40, "Quadro NVS 140M" },
	{ 0x10DE042A, NV_ARCH_40, "Quadro NVS 130M" },
	{ 0x10DE042B, NV_ARCH_40, "Quadro NVS 135M" },
	{ 0x10DE042D, NV_ARCH_40, "Quadro FX 360M" },
	{ 0x10DE042F, NV_ARCH_40, "Quadro NVS 290" },
	{ 0x10DE042B, NV_ARCH_40, "Quadro NVS 135M" }, 
	{ 0x10DE042D, NV_ARCH_40, "Quadro FX 360M" }, 
	{ 0x10DE042F, NV_ARCH_40, "Quadro NVS 290" }, 
	{ 0x10DE0602, NV_ARCH_40, "GeForce 8800 GT" }, 
	{ 0x10DE0606, NV_ARCH_40, "GeForce 8800 GS" }, 
	{ 0x10DE060D, NV_ARCH_40, "GeForce 8800 GS" }, 
	{ 0x10DE0611, NV_ARCH_40, "GeForce 8800 GT" }, 
	{ 0x10DE061A, NV_ARCH_40, "Quadro FX 3700" },  
	{ 0x10DE06E4, NV_ARCH_40, "GeForce 8400 GS" }, 
	{ 0x10DE0622, NV_ARCH_40, "GeForce 9600 GT" }
};

using namespace os;

#if 0
inline uint32 pci_size( uint32 base, uint32 mask )
{
	uint32 size = base & mask;

	return size & ~( size - 1 );
}

static uint32 get_pci_memory_size( int nFd, const PCI_Info_s * pcPCIInfo, int nResource )
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource * 4;
	uint32 nBase = pci_gfx_read_config( nFd, nBus, nDev, nFnc, nOffset, 4 );

	pci_gfx_write_config( nFd, nBus, nDev, nFnc, nOffset, 4, ~0 );
	uint32 nSize = pci_gfx_read_config( nFd, nBus, nDev, nFnc, nOffset, 4 );

	pci_gfx_write_config( nFd, nBus, nDev, nFnc, nOffset, 4, nBase );
	if ( !nSize || nSize == 0xffffffff )
		return 0;
	if ( nBase == 0xffffffff )
		nBase = 0;
	if ( !( nSize & PCI_ADDRESS_SPACE ) )
	{
		return pci_size( nSize, PCI_ADDRESS_MEMORY_32_MASK );
	}
	else
	{
		return pci_size( nSize, PCI_ADDRESS_IO_MASK & 0xffff );
	}
}
#endif

FX::FX( int nFd ):m_cGELock( "fx_ge_lock" ), m_hRegisterArea( -1 ), m_hFrameBufferArea( -1 )
{
	m_bIsInitiated = false;
	m_bPaletteEnabled = false;
	m_bEngineDirty = false;
	int j;
	m_pRegisterBase = m_pFrameBufferBase = NULL;

	bool bFound = false;
	int nNrDevs = sizeof( asChipInfos ) / sizeof( chip_info );

	/* Get Info */
	if ( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}


	uint32 ChipsetID = ( m_cPCIInfo.nVendorID << 16 ) | m_cPCIInfo.nDeviceID;

	for ( j = 0; j < nNrDevs; j++ )
	{
		if ( ChipsetID == asChipInfos[j].nDeviceId )
		{
			bFound = true;
			break;
		}
	}
	
	if( !bFound && ( ( ChipsetID & 0xFFF0 ) == 0x00F0 || ( ChipsetID & 0xFFF0 ) == 0x02E0 ) )
	{
		dbprintf( "GeForceFX :: Detected PCI express card\n" );
	} else if( !bFound )
	{
		dbprintf( "GeForceFX :: No supported cards found\n" );
		return;
	} else
		dbprintf( "GeForceFX :: Found %s\n", asChipInfos[j].pzName );

	m_nFd = nFd;

	m_hRegisterArea = create_area( "geforcefx_regs", ( void ** )&m_pRegisterBase, 0x01000000, AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( m_hRegisterArea, ( void * )( m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK ) );

	memset( &m_sHW, 0, sizeof( m_sHW ) );
	
	if( !bFound )
	{
		/* Read PCI id */
		vuint32 *pRegs;
		pRegs = (vuint32*)m_pRegisterBase;
		uint32 nID = pRegs[0x1800/4];
		dbprintf( "Register ID: %x\n", (uint)nID );
		if( ( nID & 0x0000ffff ) == 0x000010DE )
			ChipsetID = 0x10DE0000 | ( nID >> 16 );
		else if( ( nID & 0xffff0000 ) == 0x10DE0000 )
			ChipsetID = 0x10DE0000 | ( ( nID << 8 ) & 0x0000ff00 ) |
                            ( ( nID >> 8 ) & 0x000000ff );
                           
		for ( j = 0; j < nNrDevs; j++ )
		{
			if ( ChipsetID == asChipInfos[j].nDeviceId )
			{
				bFound = true;
				break;
			}
		}	
		if( bFound )
			dbprintf( "GeForceFX :: Found %s\n", asChipInfos[j].pzName );
	}
	
	if( !bFound )
		return;

	m_sHW.Fd = nFd;
	m_sHW.Chipset = asChipInfos[j].nDeviceId;
	m_sHW.alphaCursor = ( ( m_sHW.Chipset & 0x0ff0 ) != 0x0100 );
	m_sHW.Architecture = asChipInfos[j].nArchRev;
	m_sHW.FbAddress = m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
	m_sHW.IOAddress = m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;

	CommonSetup();
	
	m_sHW.FbMapSize = m_sHW.RamAmountKBytes * 1024;
	
	m_hFrameBufferArea = create_area( "geforcefx_framebuffer", ( void ** )&m_pFrameBufferBase, m_sHW.FbMapSize, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK );
	remap_area( m_hFrameBufferArea, ( void * )( m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK ) );

	m_sHW.FbBase = ( uint8 * )m_pFrameBufferBase;
	m_sHW.FbStart = m_sHW.FbBase;

	if( m_sHW.Architecture >= NV_ARCH_40 )
		m_sHW.FbUsableSize = m_sHW.FbMapSize - ( 560 * 1024 );
	else
		m_sHW.FbUsableSize = m_sHW.FbMapSize - ( 128 * 1024 );
	m_sHW.ScratchBufferSize = 16384;
	m_sHW.ScratchBufferStart = m_sHW.FbUsableSize - m_sHW.ScratchBufferSize;

	os::color_space colspace[] =
	{
	CS_RGB16, CS_RGB32};
	int bpp[] = { 2, 4 };
	float rf[] = { 60.0f, 75.0f, 85.0f, 100.0f };
	for ( int i = 0; i < int ( sizeof( bpp ) / sizeof( bpp[0] ) ); i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			m_cScreenModeList.push_back( os::screen_mode( 640, 480, 640 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 800, 600, 800 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 1024, 768, 1024 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 1280, 1024, 1280 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 1600, 1200, 1600 * bpp[i], colspace[i], rf[j] ) );
		}
	}

	m_bVideoOverlayUsed = false;
	m_bIsInitiated = true;
	if( m_sHW.ScratchBufferStart > 1024 * 1024 * 8 )
		InitMemory( 1024 * 1024 * 8, m_sHW.ScratchBufferStart - 1024 * 1024 * 8, PAGE_SIZE - 1, 63 );
}

FX::~FX()
{
	if ( m_hRegisterArea != -1 )
	{
		delete_area( m_hRegisterArea );
	}
	if ( m_hFrameBufferArea != -1 )
	{
		delete_area( m_hFrameBufferArea );
	}
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

area_id FX::Open()
{
	return m_hFrameBufferArea;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::Close()
{
}

int FX::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::GetScreenModeDesc( int nIndex, os::screen_mode * psMode )
{
	if ( nIndex < 0 || uint ( nIndex ) >= m_cScreenModeList.size() )
	{
		return false;
	}
	*psMode = m_cScreenModeList[nIndex];
	return true;
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

int FX::SetScreenMode( os::screen_mode sMode )
{
	vga_regs newmode;
	FXRegPtr nvReg = &m_sHW.ModeReg;
	memset( &newmode, 0, sizeof( struct vga_regs ) );
	
	NVLockUnlock( &m_sHW, 1);

	float vHPeriodEst = ( ( ( 1.0 / sMode.m_vRefreshRate ) - ( 550.0 / 1000000.0 ) ) / ( ( float )sMode.m_nHeight + 1 ) * 1000000.0 );
	float vVSyncPlusBp = rint( 550.0 / vHPeriodEst );
	float vVTotal = ( float )sMode.m_nHeight + vVSyncPlusBp + 1;
	float vVFieldRateEst = 1.0 / vHPeriodEst / vVTotal * 1000000.0;
	float vHPeriod = vHPeriodEst / ( sMode.m_vRefreshRate / vVFieldRateEst );

	//float vVFieldRate = 1.0 / vHPeriod / vVTotal * 1000000.0;
	//float vVFrameRate = vVFieldRate;
	float vIdealDutyCycle = ( ( ( 40.0 - 20.0 ) * 128.0 / 256.0 ) + 20.0 ) - ( ( 128.0 / 256.0 * 600.0 ) * vHPeriod / 1000.0 );
	float vHBlank = rint( ( float )sMode.m_nWidth * vIdealDutyCycle / ( 100.0 - vIdealDutyCycle ) / ( 2.0 * 8 ) ) * ( 2.0 * 8 );
	float vHTotal = ( float )sMode.m_nWidth + vHBlank;
	float vPixClock = vHTotal / vHPeriod;
	float vHSync = rint( 8.0 / 100.0 * vHTotal / 8 ) * 8;
	float vHFrontPorch = ( vHBlank / 2.0 ) - vHSync;
	float vVOddFrontPorchLines = 1;
	
	int nBpp = ( sMode.m_eColorSpace == CS_RGB32 ) ? 4 : 2;

	int nHTotal = ( int )rint( vHTotal );
	int nVTotal = ( int )rint( vVTotal );
	int nPixClock = ( int )rint( vPixClock ) * 1000;
	int nHSyncStart = sMode.m_nWidth + ( int )rint( vHFrontPorch );
	int nHSyncEnd = nHSyncStart + ( int )rint( vHSync );
	int nVSyncStart = sMode.m_nHeight + ( int )rint( vVOddFrontPorchLines );
	int nVSyncEnd = ( int )nVSyncStart + 3;

	int horizTotal = nHTotal / 8 - 5;
	int horizDisplay = sMode.m_nWidth / 8 - 1;
	int horizStart = nHSyncStart / 8 - 1;
	int horizEnd = nHSyncEnd / 8 - 1;
	int horizBlankStart = sMode.m_nWidth / 8 - 1;
	int horizBlankEnd = nHTotal / 8 - 1;
	int vertTotal = nVTotal - 2;
	int vertDisplay = sMode.m_nHeight - 1;
	int vertStart = nVSyncStart - 1;
	int vertEnd = nVSyncEnd - 1;
	int vertBlankStart = sMode.m_nHeight - 1;
	int vertBlankEnd = nVTotal - 1;

	if ( m_sHW.FlatPanel == 1 )
	{
		vertStart = vertTotal - 3;
		vertEnd = vertTotal - 2;
		vertBlankStart = vertStart;
		horizStart = horizTotal - 5;
		horizEnd = horizTotal - 2;
		horizBlankEnd = horizTotal + 4;
	}


	newmode.crtc[0x00] = Set8Bits( horizTotal );
	newmode.crtc[0x01] = Set8Bits( horizDisplay );
	newmode.crtc[0x02] = Set8Bits( horizBlankStart );
      newmode.crtc[0x03] = SetBitField( horizBlankEnd, 4: 0, 4:0 ) | SetBit( 7 );
	newmode.crtc[0x04] = Set8Bits( horizStart );
      newmode.crtc[0x05] = SetBitField( horizBlankEnd, 5: 5, 7: 7 ) | SetBitField( horizEnd, 4: 0, 4:0 );
      newmode.crtc[0x06] = SetBitField( vertTotal, 7: 0, 7:0 );
      newmode.crtc[0x07] = SetBitField( vertTotal, 8: 8, 0: 0 ) | SetBitField( vertDisplay, 8: 8, 1: 1 ) | SetBitField( vertStart, 8: 8, 2: 2 ) | SetBitField( vertBlankStart, 8: 8, 3: 3 ) | SetBit( 4 ) | SetBitField( vertTotal, 9: 9, 5: 5 ) | SetBitField( vertDisplay, 9: 9, 6: 6 ) | SetBitField( vertStart, 9: 9, 7:7 );
      newmode.crtc[0x09] = SetBitField( vertBlankStart, 9: 9, 5:5 ) | SetBit( 6 );
	newmode.crtc[0x10] = Set8Bits( vertStart );
      newmode.crtc[0x11] = SetBitField( vertEnd, 3: 0, 3:0 ) | SetBit( 5 );
	newmode.crtc[0x12] = Set8Bits( vertDisplay );
	newmode.crtc[0x13] = Set8Bits( ( sMode.m_nWidth / 8 ) * nBpp );
	newmode.crtc[0x15] = Set8Bits( vertBlankStart );
	newmode.crtc[0x16] = Set8Bits( vertBlankEnd );
	newmode.crtc[0x17] = 0xe3;
	newmode.crtc[0x18] = 0xff;
	newmode.crtc[0x28] = 0x40;

	newmode.gra[0x05] = 0x40;
	newmode.gra[0x06] = 0x05;
	newmode.gra[0x07] = 0x0f;
	newmode.gra[0x08] = 0xff;

	for ( int i = 0; i < 16; i++ )
	{
		newmode.attr[i] = ( uint8 )i;
	}
	newmode.attr[0x10] = 0x41;
	newmode.attr[0x11] = 0xff;
	newmode.attr[0x12] = 0x0f;
	newmode.attr[0x13] = 0x00;
	newmode.attr[0x14] = 0x00;

	if ( m_sHW.Television )
		newmode.attr[0x11] = 0x00;

	newmode.seq[0x00] = 0x03;
	newmode.seq[0x01] = 0x01;
	newmode.seq[0x02] = 0x0f;
	newmode.seq[0x03] = 0x00;
	newmode.seq[0x04] = 0x0e;

	nvReg->bpp = nBpp * 8;
	nvReg->width = sMode.m_nWidth;
	nvReg->height = sMode.m_nHeight;


      nvReg->screen = SetBitField( horizBlankEnd, 6: 6, 4: 4 ) | SetBitField( vertBlankStart, 10: 10, 3: 3 ) | SetBitField( vertStart, 10: 10, 2: 2 ) | SetBitField( vertDisplay, 10: 10, 1: 1 ) | SetBitField( vertTotal, 10: 10, 0:0 );

      nvReg->horiz = SetBitField( horizTotal, 8: 8, 0: 0 ) | SetBitField( horizDisplay, 8: 8, 1: 1 ) | SetBitField( horizBlankStart, 8: 8, 2: 2 ) | SetBitField( horizStart, 8: 8, 3:3 );

      nvReg->extra = SetBitField( vertTotal, 11: 11, 0: 0 ) | SetBitField( vertDisplay, 11: 11, 2: 2 ) | SetBitField( vertStart, 11: 11, 4: 4 ) | SetBitField( vertBlankStart, 11: 11, 6:6 );

	nvReg->interlace = 0xff;	/* interlace off */

	newmode.misc_output = 0x2b;

	m_sHW.CURSOR = ( U032 * )( m_sHW.FbStart + m_sHW.CursorStart );

	NVCalcStateExt( &m_sHW, nvReg, nBpp * 8, sMode.m_nWidth, sMode.m_nWidth, sMode.m_nHeight, nPixClock, 0 );

	nvReg->scale = m_sHW.PRAMDAC[0x00000848 / 4] & 0xfff000ff;
	if ( m_sHW.FlatPanel == 1 )
	{
		nvReg->pixel |= ( 1 << 7 );
		if ( !m_sHW.fpScaler || ( m_sHW.fpWidth <= sMode.m_nWidth ) || ( m_sHW.fpHeight <= sMode.m_nHeight ) )
		{
			nvReg->scale |= ( 1 << 8 );
		}
		nvReg->crtcSync = m_sHW.PRAMDAC[0x0828/4];
	}

	nvReg->vpll = nvReg->pll;
	nvReg->vpll2 = nvReg->pll;
	nvReg->vpllB = nvReg->pllB;
	nvReg->vpll2B = nvReg->pllB;
	
	VGA_WR08( &m_sHW.PCIO, 0x03D4, 0x1C );
	nvReg->fifo = VGA_RD08( &m_sHW.PCIO, 0x03D5 ) & ~( 1<<5 );

	
	if ( m_sHW.CRTCnumber )
	{
		nvReg->head = m_sHW.PCRTC0[0x00000860 / 4] & ~0x00001000;
		nvReg->head2 = m_sHW.PCRTC0[0x00002860 / 4] | 0x00001000;
		nvReg->crtcOwner = 3;
		nvReg->pllsel |= 0x20000800;
		nvReg->vpll = m_sHW.PRAMDAC0[0x0508 / 4];
		if ( m_sHW.twoStagePLL )
			nvReg->vpllB = m_sHW.PRAMDAC0[0x0578 / 4];
	}
	else if ( m_sHW.twoHeads )
	{
		nvReg->head = m_sHW.PCRTC0[0x00000860 / 4] | 0x00001000;
		nvReg->head2 = m_sHW.PCRTC0[0x00002860 / 4] & ~0x00001000;
		nvReg->crtcOwner = 0;
		nvReg->vpll2 = m_sHW.PRAMDAC0[0x00000520 / 4];
		if ( m_sHW.twoStagePLL )
			nvReg->vpll2B = m_sHW.PRAMDAC0[0x057C / 4];
	}
	nvReg->cursorConfig = 0x00000100;
	if ( m_sHW.alphaCursor )
	{
		if ( ( m_sHW.Chipset & 0x0ff0 ) != 0x0110 )
			nvReg->cursorConfig |= 0x04011000;
		else
			nvReg->cursorConfig |= 0x14011000;
		nvReg->general |= ( 1 << 29 );
	}
	else
		nvReg->cursorConfig |= 0x02000000;

	if ( m_sHW.twoHeads )
	{
		if ( ( m_sHW.Chipset & 0x0ff0 ) == 0x0110 )
			nvReg->dither = m_sHW.PRAMDAC[0x0528 / 4] & ~0x00010000;
		else
			nvReg->dither = m_sHW.PRAMDAC[0x083C / 4] & ~1;
	}

	nvReg->timingH = 0;
	nvReg->timingV = 0;
	nvReg->displayV = sMode.m_nHeight;

	NVLockUnlock( &m_sHW, 0 );
	
	if ( m_sHW.twoHeads )
	{
		VGA_WR08( &m_sHW.PCIO, 0x03D4, 0x44 );
		VGA_WR08( &m_sHW.PCIO, 0x03D5, nvReg->crtcOwner );
		NVLockUnlock( &m_sHW, 0 );
	}
	
	VGAProtect( true );

	/* Write registers */
	NVLoadStateExt( &m_sHW, nvReg );
	LoadVGAState( &newmode );

	/* Load color registers */
	for ( int i = 0; i < 256; i++ )
	{
		VGA_WR08( m_sHW.PDIO, 0x3c8, i );
		VGA_WR08( m_sHW.PDIO, 0x3c9, i );
		VGA_WR08( m_sHW.PDIO, 0x3c9, i );
		VGA_WR08( m_sHW.PDIO, 0x3c9, i );
	}


	m_sCurrentMode = sMode;
	m_sCurrentMode.m_nBytesPerLine = sMode.m_nWidth * nBpp;

	//NVSetStartAddress( &m_sHW, 0 /*m_sCurrentMode.m_nBytesPerLine * m_sCurrentMode.m_nHeight */  );

	/* Init acceleration */
	SetupAccel();
	
	VGAProtect( false );
	
	return 0;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

os::screen_mode FX::GetCurrentScreenMode()
{

	return m_sCurrentMode;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst )
{
	#if 0
	if( ( pcDstBitmap->m_bVideoMem == false && ( pcSrcBitmap == NULL || pcSrcBitmap->m_bVideoMem == false ) ) || m_bEngineDirty == false )
		return;
	m_cGELock.Lock();
	WaitForIdle();	
	m_cGELock.Unlock();
	m_bEngineDirty = false;
	#endif
}


//-----------------------------------------------------------------------------
//                          DMA Functions
//-----------------------------------------------------------------------------

void DmaKickoff( NVPtr pNv )
{
	if ( pNv->dmaCurrent != pNv->dmaPut )
	{
		pNv->dmaPut = pNv->dmaCurrent;
		WRITE_PUT( pNv, pNv->dmaPut );
	}
}

/* There is a HW race condition with videoram command buffers.
   You can't jump to the location of your put offset.  We write put
   at the jump offset + SKIPS dwords with noop padding in between
   to solve this problem */
#define SKIPS  8

void DmaWait( NVPtr pNv, uint32 size )
{
	uint32 dmaGet;

	size++;
	while ( pNv->dmaFree < size )
	{
		dmaGet = READ_GET( pNv );
		if ( pNv->dmaPut >= dmaGet )
		{
			pNv->dmaFree = pNv->dmaMax - pNv->dmaCurrent;
			if ( pNv->dmaFree < size )
			{
				DmaNext( pNv, 0x20000000 );
				if ( dmaGet <= SKIPS )
				{
					if ( pNv->dmaPut <= SKIPS )	/* corner case - idle */
						WRITE_PUT( pNv, SKIPS + 1 );
					do
					{
						dmaGet = READ_GET( pNv );
					}
					while ( dmaGet <= SKIPS );
				}
				WRITE_PUT( pNv, SKIPS );
				pNv->dmaCurrent = pNv->dmaPut = SKIPS;
				pNv->dmaFree = dmaGet - ( SKIPS + 1 );
			}
		}
		else
			pNv->dmaFree = dmaGet - pNv->dmaCurrent - 1;
	}
}


//-----------------------------------------------------------------------------
//                          Accelerated Functions
//-----------------------------------------------------------------------------


void FX::WaitForIdle()
{
	bigtime_t nTimeOut = get_system_time() + 5000;

	while ( READ_GET( &m_sHW ) != m_sHW.dmaPut && ( get_system_time() < nTimeOut ) );

	while ( m_sHW.PGRAPH[0x0700 / 4] && ( get_system_time() < nTimeOut ) );

	/*if( get_system_time() > nTimeOut ) {
	   dbprintf( "GeForce FX :: Error: Engine timed out\n" );

	   } */
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
bool FX::DrawLine( SrvBitmap * pcBitMap, const IRect & cClipRect, const IPoint & cPnt1, const IPoint & cPnt2, const Color32_s & sColor, int nMode )
{
	if ( pcBitMap->m_bVideoMem == false || nMode != DM_COPY )
	{
		return DisplayDriver::DrawLine( pcBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode );
	}

	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;

	if ( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false )
	{
		return false;
	}

	uint32 nColor;

	if ( pcBitMap->m_eColorSpc == CS_RGB32 )
	{
		nColor = COL_TO_RGB32( sColor );
	}
	else
	{
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16( sColor );
	}

	m_cGELock.Lock();

	DmaStart( &m_sHW, SURFACE_PITCH, 3 );
	DmaNext( &m_sHW, pcBitMap->m_nBytesPerLine | ( pcBitMap->m_nBytesPerLine << 16 ) );
	DmaNext( &m_sHW, pcBitMap->m_nVideoMemOffset );
	DmaNext( &m_sHW, pcBitMap->m_nVideoMemOffset );
	
	DmaStart( &m_sHW, LINE_COLOR, 1 );
	DmaNext( &m_sHW, nColor );
	DmaStart( &m_sHW, LINE_LINES( 0 ), 4 );
	DmaNext( &m_sHW, ( ( y1 << 16 ) | ( x1 & 0xffff ) ) );
	DmaNext( &m_sHW, ( ( y2 << 16 ) | ( x2 & 0xffff ) ) );
	DmaNext( &m_sHW, ( ( y2 << 16 ) | ( x2 & 0xffff ) ) );
	DmaNext( &m_sHW, ( ( ( y2 + 1 ) << 16 ) | ( x2 & 0xffff ) ) );
	DmaKickoff( &m_sHW );
	WaitForIdle();

	m_bEngineDirty = true;
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::FillRect( SrvBitmap * pcBitMap, const IRect & cRect, const Color32_s & sColor, int nMode )
{
	if ( pcBitMap->m_bVideoMem == false || nMode != DM_COPY )
	{
		return DisplayDriver::FillRect( pcBitMap, cRect, sColor, nMode );
	}

	int nWidth = cRect.Width() + 1;
	int nHeight = cRect.Height() + 1;
	uint32 nColor;

	if ( pcBitMap->m_eColorSpc == CS_RGB32 )
	{
		nColor = COL_TO_RGB32( sColor );
	}
	else
	{
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16( sColor );
	}

	m_cGELock.Lock();
	
	DmaStart( &m_sHW, SURFACE_PITCH, 3 );
	DmaNext( &m_sHW, pcBitMap->m_nBytesPerLine | ( pcBitMap->m_nBytesPerLine << 16 ) );
	DmaNext( &m_sHW, pcBitMap->m_nVideoMemOffset );
	DmaNext( &m_sHW, pcBitMap->m_nVideoMemOffset );

	DmaStart( &m_sHW, RECT_SOLID_COLOR, 1 );
	DmaNext( &m_sHW, nColor );
	DmaStart( &m_sHW, RECT_SOLID_RECTS( 0 ), 2 );
	DmaNext( &m_sHW, ( ( cRect.left << 16 ) | cRect.top ) );
	DmaNext( &m_sHW, ( ( nWidth << 16 ) | nHeight ) );
	DmaKickoff( &m_sHW );
	WaitForIdle();

	m_bEngineDirty = true;
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::BltBitmap( SrvBitmap * pcDstBitMap, SrvBitmap * pcSrcBitMap, IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha )
{
	#if 0
	if( nMode == DM_SELECT )
	{
		uint32 nArea = pcSrcBitMap->m_hArea;
		ioctl( m_nFd, 100, &nArea );
//			dbprintf( "Got DMA address %x\n", (uint)nArea );
		vuint32* pNotifier = (vuint32*)( m_pFrameBufferBase + m_sHW.FbUsableSize + 128 * 1024 );
		memset( (void*)pNotifier, 0xff, 0x100 );
		//dbprintf( "DMA %i!!!\n", pNotifier[3]  );
		m_cGELock.Lock();
		DmaStart(&m_sHW, MEMFORMAT_NOTIFY, 1);
        DmaNext (&m_sHW, 0);
        DmaStart(&m_sHW, MEMFORMAT_OFFSET_IN, 8);
        DmaNext (&m_sHW, nArea + cSrcRect.top * pcSrcBitMap->m_nBytesPerLine + cSrcRect.left * 4 );
        DmaNext (&m_sHW, pcDstBitMap->m_nVideoMemOffset + cDstRect.top * pcDstBitMap->m_nBytesPerLine + cDstRect.left * 4 );
        DmaNext (&m_sHW, pcSrcBitMap->m_nBytesPerLine );
        DmaNext (&m_sHW, pcDstBitMap->m_nBytesPerLine);
        DmaNext (&m_sHW, ( cSrcRect.Width() + 1 ) * 4 );
        DmaNext (&m_sHW, cSrcRect.Height() + 1);
        DmaNext (&m_sHW, 0x101);
        DmaNext (&m_sHW, 0);
        DmaKickoff(&m_sHW);
        m_cGELock.Unlock();
       //	dbprintf("Notify start!\n");
       	int count = 0;
        while (1) {
        	uint32 a = pNotifier[0];
        	uint32 b = pNotifier[1];
        	uint32 c = pNotifier[2];
        	uint32 status = pNotifier[3]; 
        	
        	count++;       	        	
        	
        	if( status == 0xffffffff )
        		continue;
        	if( !status )
        	{
        		//dbprintf("Finished %i!\n", status);
        		break;
        	}
        	if( status & 0xffff ) {
        		dbprintf("Error!\n");
        		break;
        	}
        	snooze( 10 );
       	}
       	return true;
       	//dbprintf("Notify finished %i!\n", count);
	}
	#endif
	
	if ( pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY || cSrcRect.Size() != cDstRect.Size() )
	{
		return DisplayDriver::BltBitmap( pcDstBitMap, pcSrcBitMap, cSrcRect, cDstRect, nMode, nAlpha );
	}

	int nWidth = cSrcRect.Width() + 1;
	int nHeight = cSrcRect.Height() + 1;
	IPoint cDstPos = cDstRect.LeftTop();

	m_cGELock.Lock();
	
	DmaStart( &m_sHW, SURFACE_PITCH, 3 );
	DmaNext( &m_sHW, pcSrcBitMap->m_nBytesPerLine | ( pcDstBitMap->m_nBytesPerLine << 16 ) );
	DmaNext( &m_sHW, pcSrcBitMap->m_nVideoMemOffset );
	DmaNext( &m_sHW, pcDstBitMap->m_nVideoMemOffset );

	DmaStart( &m_sHW, BLIT_POINT_SRC, 3 );
	DmaNext( &m_sHW, ( ( cSrcRect.top << 16 ) | cSrcRect.left ) );
	DmaNext( &m_sHW, ( ( cDstPos.y << 16 ) | cDstPos.x ) );
	DmaNext( &m_sHW, ( ( nHeight << 16 ) | nWidth ) );
	DmaKickoff( &m_sHW );
	WaitForIdle();
	
	m_bEngineDirty = true;
	m_cGELock.Unlock();
	return true;
}


//-----------------------------------------------------------------------------
//                          Video Overlay Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::CreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if( !( ( m_sHW.Architecture <= NV_ARCH_30 ) || 
            ( ( m_sHW.Chipset & 0xfff0 ) == 0x0040 ) ) )
		return( false );
	if ( eFormat == CS_YUV422 && !m_bVideoOverlayUsed )
	{
		/* Calculate offset */
		uint32 pitch = 0;
		uint32 totalSize = 0;

		
		pitch = ( ( cSize.x << 1 ) + 0xff ) & ~0xff;
		totalSize = pitch * cSize.y; 
		uint32 offset;
		
		if( AllocateMemory( totalSize, &offset ) != 0 )
			return( false );

		//uint32 offset = PAGE_ALIGN( m_sHW.ScratchBufferStart - totalSize - PAGE_SIZE );

		*pBuffer = create_area( "geforcefx_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, ( void * )( ( m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK ) + offset ) );


		/* Start video */

		switch ( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
		{
		case 16:
			m_nColorKey = COL_TO_RGB16( sColorKey );
			break;
		case 32:
			m_nColorKey = COL_TO_RGB32( sColorKey );
			break;

		default:
			delete_area( *pBuffer );
			return ( false );
		}

		/* NV30 Overlay */
		m_sHW.PMC[0x00008910 / 4] = 4096;
		m_sHW.PMC[0x00008914 / 4] = 4096;
		m_sHW.PMC[0x00008918 / 4] = 4096;
		m_sHW.PMC[0x0000891C / 4] = 4096;
		m_sHW.PMC[0x00008b00 / 4] = m_nColorKey & 0xffffff;

		m_sHW.PMC[0x8900 / 4] = offset;
		m_sHW.PMC[0x8928 / 4] = ( cSize.y << 16 ) | cSize.x;
		m_sHW.PMC[0x8930 / 4] = 0;
		m_sHW.PMC[0x8938 / 4] = ( cSize.x << 20 ) / ( cDst.Width() + 1 );
		m_sHW.PMC[0x8940 / 4] = ( cSize.y << 20 ) / ( cDst.Height() + 1 );
		m_sHW.PMC[0x8948 / 4] = ( cDst.top << 16 ) | cDst.left;
		m_sHW.PMC[0x8950 / 4] = ( ( cDst.Height() + 1 ) << 16 ) | ( cDst.Width() + 1 );

		uint32 dstPitch = ( pitch ) | 1 << 20;
		if( eFormat == CS_YUV12 )
			dstPitch |= 1 << 16;

		m_sHW.PMC[0x8958 / 4] = dstPitch;
		m_sHW.PMC[0x8704 / 4] = 0;
		m_sHW.PMC[0x8700 / 4] = 1;

		m_bVideoOverlayUsed = true;
		m_cVideoSize = cSize;
		m_nVideoOffset = offset;

		return ( true );
	}
	return ( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::RecreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, area_id *pBuffer )
{
	if( !( ( m_sHW.Architecture <= NV_ARCH_30 ) || 
            ( ( m_sHW.Chipset & 0xfff0 ) == 0x0040 ) ) )
		return( false );
	if ( eFormat == CS_YUV422  )
	{
		delete_area( *pBuffer );
		FreeMemory( m_nVideoOffset );
		/* Calculate offset */
		uint32 pitch = 0;
		uint32 totalSize = 0;

		pitch = ( ( cSize.x << 1 ) + 0xff ) & ~0xff;
		totalSize = pitch * cSize.y;
		uint32 offset;
		
		if( AllocateMemory( totalSize, &offset ) != 0 )
			return( false );

		//uint32 offset = PAGE_ALIGN( m_sHW.ScratchBufferStart - totalSize - PAGE_SIZE );

		*pBuffer = create_area( "geforcefx_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, ( void * )( ( m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK ) + offset ) );

		m_sHW.PMC[0x00008910 / 4] = 4096;
		m_sHW.PMC[0x00008914 / 4] = 4096;
		m_sHW.PMC[0x00008918 / 4] = 4096;
		m_sHW.PMC[0x0000891C / 4] = 4096;
		m_sHW.PMC[0x00008b00 / 4] = m_nColorKey & 0xffffff;

		m_sHW.PMC[0x8900 / 4] = offset;
		m_sHW.PMC[0x8928 / 4] = ( cSize.y << 16 ) | cSize.x;
		m_sHW.PMC[0x8930 / 4] = 0;
		m_sHW.PMC[0x8938 / 4] = ( cSize.x << 20 ) / ( cDst.Width() + 1 );
		m_sHW.PMC[0x8940 / 4] = ( cSize.y << 20 ) / ( cDst.Height() + 1 );
		m_sHW.PMC[0x8948 / 4] = ( cDst.top << 16 ) | cDst.left;
		m_sHW.PMC[0x8950 / 4] = ( ( cDst.Height() + 1 ) << 16 ) | ( cDst.Width() + 1 );

		uint32 dstPitch = ( pitch ) | 1 << 20;
		if( eFormat == CS_YUV12 )
			dstPitch |= 1 << 16;

		m_sHW.PMC[0x8958 / 4] = dstPitch;
		m_sHW.PMC[0x8704 / 4] = 0;
		m_sHW.PMC[0x8700 / 4] = 1;


		m_bVideoOverlayUsed = true;
		m_cVideoSize = cSize;
		m_nVideoOffset = offset;
		return ( true );
	}
	return ( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::DeleteVideoOverlay( area_id *pBuffer )
{
	if ( m_bVideoOverlayUsed )
	{
		delete_area( *pBuffer );
		FreeMemory( m_nVideoOffset );

		m_sHW.PMC[0x00008704 / 4] = 1;

	}
	m_bVideoOverlayUsed = false;
}


//-----------------------------------------------------------------------------
//                          Private Functions
//-----------------------------------------------------------------------------

void FX::LoadVGAState( struct vga_regs *regs )
{
	MISCout( regs->misc_output );

	for ( int i = 1; i < NUM_SEQ_REGS; i++ )
	{
		SEQout( i, regs->seq[i] );
	}

	CRTCout( 17, regs->crtc[17] & ~0x80 );
	
	for ( int i = 0; i < NUM_CRT_REGS; i++) {
		switch (i) {
		case 0x19:
		case 0x20 ... 0x40:
			break;
		default:
			CRTCout( i, regs->crtc[i] );
		}
	}

	for ( int i = 0; i < NUM_GRC_REGS; i++ )
	{
		GRAout( i, regs->gra[i] );
	}

//      DisablePalette();       
	for ( int i = 0; i < NUM_ATC_REGS; i++ )
	{
		ATTRout( i, regs->attr[i] );
	}

}


//-----------------------------------------------------------------------------

void FX::SetupAccel()
{
	m_sHW.dmaBase = ( uint32 * )( &m_sHW.FbStart[m_sHW.FbUsableSize] );
	uint32 surfaceFormat, patternFormat, rectFormat, lineFormat;
	int pitch = m_sCurrentMode.m_nWidth * ( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) >> 3 );

	for ( uint i = 0; i < SKIPS; i++ )
		m_sHW.dmaBase[i] = 0x00000000;

	/* Initialize engine objects */
	m_sHW.dmaBase[0x0 + SKIPS] = 0x00040000;
	m_sHW.dmaBase[0x1 + SKIPS] = 0x80000010;
	m_sHW.dmaBase[0x2 + SKIPS] = 0x00042000;
	m_sHW.dmaBase[0x3 + SKIPS] = 0x80000011;
	m_sHW.dmaBase[0x4 + SKIPS] = 0x00044000;
	m_sHW.dmaBase[0x5 + SKIPS] = 0x80000012;
	m_sHW.dmaBase[0x6 + SKIPS] = 0x00046000;
	m_sHW.dmaBase[0x7 + SKIPS] = 0x80000013;
	m_sHW.dmaBase[0x8 + SKIPS] = 0x00048000;
	m_sHW.dmaBase[0x9 + SKIPS] = 0x80000014;
	m_sHW.dmaBase[0xA + SKIPS] = 0x0004A000;
	m_sHW.dmaBase[0xB + SKIPS] = 0x80000015;
	m_sHW.dmaBase[0xC + SKIPS] = 0x0004C000;
	m_sHW.dmaBase[0xD + SKIPS] = 0x80000016;
	m_sHW.dmaBase[0xE + SKIPS] = 0x0004E000;
	m_sHW.dmaBase[0xF + SKIPS] = 0x80000017;
	m_sHW.dmaPut = 0;
	m_sHW.dmaCurrent = 16 + SKIPS;
	m_sHW.dmaMax = 8191;
	m_sHW.dmaFree = m_sHW.dmaMax - m_sHW.dmaCurrent;
	switch ( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
	{
	case 32:
	case 24:
		surfaceFormat = SURFACE_FORMAT_DEPTH24;
		patternFormat = PATTERN_FORMAT_DEPTH24;
		rectFormat = RECT_FORMAT_DEPTH24;
		lineFormat = LINE_FORMAT_DEPTH24;
		break;
	case 16:
	case 15:
		surfaceFormat = SURFACE_FORMAT_DEPTH16;
		patternFormat = PATTERN_FORMAT_DEPTH16;
		rectFormat = RECT_FORMAT_DEPTH16;
		lineFormat = LINE_FORMAT_DEPTH16;
		break;
	default:
		surfaceFormat = SURFACE_FORMAT_DEPTH8;
		patternFormat = PATTERN_FORMAT_DEPTH8;
		rectFormat = RECT_FORMAT_DEPTH8;
		lineFormat = LINE_FORMAT_DEPTH8;
		break;
	}
	DmaStart( &m_sHW, SURFACE_FORMAT, 4 );
	DmaNext( &m_sHW, surfaceFormat );
	DmaNext( &m_sHW, pitch | ( pitch << 16 ) );
	DmaNext( &m_sHW, 0 );
	DmaNext( &m_sHW, 0 );
	DmaStart( &m_sHW, PATTERN_FORMAT, 1 );
	DmaNext( &m_sHW, patternFormat );
	DmaStart( &m_sHW, RECT_FORMAT, 1 );
	DmaNext( &m_sHW, rectFormat );
	DmaStart( &m_sHW, LINE_FORMAT, 1 );
	DmaNext( &m_sHW, lineFormat );
	m_sHW.currentRop = ~0;	/* set to something invalid */

	/* Set solid color mode */
	DmaStart( &m_sHW, PATTERN_COLOR_0, 4 );
	DmaNext( &m_sHW, ~0 );
	DmaNext( &m_sHW, ~0 );
	DmaNext( &m_sHW, ~0 );
	DmaNext( &m_sHW, ~0 );
	DmaStart( &m_sHW, ROP_SET, 1 );
	DmaNext( &m_sHW, 0xCA );
	DmaKickoff( &m_sHW );
	WaitForIdle();
}


//-----------------------------------------------------------------------------

bool FX::IsConnected( int output )
{
	volatile U032 *PRAMDAC = m_sHW.PRAMDAC0;
	CARD32 reg52C, reg608, dac0_reg608;
	bool present;

	dbprintf( "GeForce FX :: Probing for analog device on output %s...\n", output ? "B" : "A" );
	if ( output ) {
        dac0_reg608 = PRAMDAC[0x0608/4];
        PRAMDAC += 0x800;
    }
	reg52C = PRAMDAC[0x052C / 4];
	reg608 = PRAMDAC[0x0608 / 4];
	PRAMDAC[0x0608 / 4] = reg608 & ~0x00010000;
	PRAMDAC[0x052C / 4] = reg52C & 0x0000FEEE;
	snooze( 1000 );
	PRAMDAC[0x052C / 4] |= 1;
	m_sHW.PRAMDAC0[0x0610 / 4] = 0x94050140;
	m_sHW.PRAMDAC0[0x0608 / 4] |= 0x00001000;
	snooze( 1000 );
	present = ( PRAMDAC[0x0608 / 4] & ( 1 << 28 ) ) ? true : false;
	if ( present )
		dbprintf( "GeForce FX ::  ...found one\n" );

	else
		dbprintf( "GeForce FX ::  ...can't find one\n" );
    if(output)
        m_sHW.PRAMDAC0[0x0608/4] = dac0_reg608;

	PRAMDAC[0x052C / 4] = reg52C;
	PRAMDAC[0x0608 / 4] = reg608;
	return present;
}


//-----------------------------------------------------------------------------
void FX::CommonSetup()
{
	uint16 nImplementation = m_sHW.Chipset & 0x0ff0;
	bool bMobile = false;
	int FlatPanel = -1;
	bool Television = false;
	bool tvA = false;
	bool tvB = false;

	/*
	 * Register mappings
	 */

	m_sHW.REGS = ( unsigned * )( m_pRegisterBase );
	m_sHW.PRAMIN = ( unsigned * )( m_pRegisterBase + 0x00710000 );
	m_sHW.PCRTC0 = ( unsigned * )( m_pRegisterBase + 0x00600000 );
	m_sHW.PRAMDAC0 = ( unsigned * )( m_pRegisterBase + 0x00680000 );
	m_sHW.PFB = ( unsigned * )( m_pRegisterBase + 0x00100000 );

	m_sHW.PFIFO = ( unsigned * )( m_pRegisterBase + 0x00002000 );
	m_sHW.PGRAPH = ( unsigned * )( m_pRegisterBase + 0x00400000 );
	m_sHW.PEXTDEV = ( unsigned * )( m_pRegisterBase + 0x00101000 );
	m_sHW.PTIMER = ( unsigned * )( m_pRegisterBase + 0x00009000 );
	m_sHW.PMC = ( unsigned * )( m_pRegisterBase + 0x00000000 );
	m_sHW.FIFO = ( unsigned * )( m_pRegisterBase + 0x00800000 );
	m_sHW.PCIO0 = ( U008 * )( m_pRegisterBase + 0x00601000 );
	m_sHW.PDIO0 = ( U008 * )( m_pRegisterBase + 0x00681000 );
	m_sHW.PVIO = ( U008 * )( m_pRegisterBase + 0x000C0000 );

	m_sHW.twoHeads = ( nImplementation != 0x0100 ) && ( nImplementation != 0x0150 ) && ( nImplementation != 0x01A0 ) && ( nImplementation != 0x0200 );
	m_sHW.fpScaler = ( m_sHW.twoHeads && ( nImplementation != 0x0110 ) );

	m_sHW.twoStagePLL = ( nImplementation == 0x0310 ) || ( nImplementation == 0x0340 ) || ( m_sHW.Architecture >= NV_ARCH_40 );

	switch ( m_sHW.Chipset )
	{
	case 0x0112:
	case 0x0174:
	case 0x0175:
	case 0x0176:
	case 0x0177:
	case 0x0179:
	case 0x017C:
	case 0x017D:
	case 0x0186:
	case 0x0187:
	case 0x0189:
	case 0x0286:
	case 0x028C:
	case 0x0316:
	case 0x0317:
	case 0x031A:
	case 0x031B:
	case 0x031C:
	case 0x031D:
	case 0x031E:
	case 0x031F:
	case 0x0324:
	case 0x0325:
	case 0x0328:
	case 0x0329:
	case 0x032C:
	case 0x032D:
	 case 0x0347:
    case 0x0348:
    case 0x0349:
    case 0x034B:
    case 0x034C:
    case 0x0160:
    case 0x0166:
    case 0x0169:
    case 0x016B:
    case 0x016C:
    case 0x016D:
    case 0x00C8:
    case 0x00CC:
    case 0x0144:
    case 0x0146:
    case 0x0148:
    case 0x0098:
    case 0x0099:
		bMobile = true;
		break;
	default:
		break;
	}


	/* Get memory size */
	if ( nImplementation == 0x01a0 )
	{
		int nAmt = pci_gfx_read_config( m_sHW.Fd, 0, 0, 1, 0x7C, 4 );

		m_sHW.RamAmountKBytes = ( ( ( nAmt >> 6 ) & 31 ) + 1 ) * 1024;
	}
	else if ( nImplementation == 0x01f0 )
	{
		int nAmt = pci_gfx_read_config( m_sHW.Fd, 0, 0, 1, 0x84, 4 );

		m_sHW.RamAmountKBytes = ( ( ( nAmt >> 4 ) & 127 ) + 1 ) * 1024;
	}
	else
	{
		m_sHW.RamAmountKBytes = ( m_sHW.PFB[0x020C / 4] & 0xFFF00000 )>>10;
	}
	
	if(m_sHW.RamAmountKBytes > 256*1024)
        m_sHW.RamAmountKBytes = 256*1024;
	
	m_sHW.CrystalFreqKHz = ( m_sHW.PEXTDEV[0x0000 / 4] & ( 1 << 6 ) ) ? 14318 : 13500;
	if ( m_sHW.twoHeads && ( nImplementation != 0x0110 ) )
	{
		if ( m_sHW.PEXTDEV[0x0000 / 4] & ( 1 << 22 ) )
			m_sHW.CrystalFreqKHz = 27000;
	}
	m_sHW.CursorStart = ( m_sHW.RamAmountKBytes - 96 ) * 1024;
	m_sHW.CURSOR = NULL;	/* can't set this here */
	m_sHW.MinVClockFreqKHz = 12000;
	m_sHW.MaxVClockFreqKHz = m_sHW.twoStagePLL ? 400000 : 350000;

	/* Select first head */
	m_sHW.PCIO = m_sHW.PCIO0;
	m_sHW.PCRTC = m_sHW.PCRTC0;
	m_sHW.PRAMDAC = m_sHW.PRAMDAC0;
	m_sHW.PDIO = m_sHW.PDIO0;
	NVLockUnlock( &m_sHW, 0 );

	m_sHW.Television = false;
	m_sHW.FlatPanel = -1;
	m_sHW.CRTCnumber = -1;

	if ( !m_sHW.twoHeads )
	{
		m_sHW.CRTCnumber = 0;
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x28 );
		if ( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x80 )
		{
			VGA_WR08( m_sHW.PCIO, 0x03D4, 0x33 );
			if ( !( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x01 ) )
				Television = true;
			FlatPanel = 1;
		}
		else
		{
			FlatPanel = 0;
		}
		dbprintf( "GeForce FX :: %s connected\n", FlatPanel ? ( Television ? "TV" : "LCD" ) : "CRT" );
		m_sHW.FlatPanel = FlatPanel;
		m_sHW.Television = Television;
	}
	else
	{
		uint8 outputAfromCRTC, outputBfromCRTC;
		int CRTCnumber = -1;
		uint8 slaved_on_A, slaved_on_B;
		bool analog_on_A, analog_on_B;
		uint32 oldhead;
		uint8 cr44;

		if ( nImplementation != 0x0110 )
		{
			if ( m_sHW.PRAMDAC0[0x0000052C / 4] & 0x100 )
				outputAfromCRTC = 1;
			else
				outputAfromCRTC = 0;
			if ( m_sHW.PRAMDAC0[0x0000252C / 4] & 0x100 )
				outputBfromCRTC = 1;
			else
				outputBfromCRTC = 0;
			analog_on_A = IsConnected( 0 );
			analog_on_B = IsConnected( 1 );
		}
		else
		{
			outputAfromCRTC = 0;
			outputBfromCRTC = 1;
			analog_on_A = false;
			analog_on_B = false;
		}
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x44 );
		cr44 = VGA_RD08( m_sHW.PCIO, 0x03D5 );
		VGA_WR08( m_sHW.PCIO, 0x03D5, 3 );

		/* Select second head */
		m_sHW.PCIO = m_sHW.PCIO0 + 0x2000;
		m_sHW.PCRTC = m_sHW.PCRTC0 + 0x800;
		m_sHW.PRAMDAC = m_sHW.PRAMDAC0 + 0x800;
		m_sHW.PDIO = m_sHW.PDIO0 + 0x2000;
		NVLockUnlock( &m_sHW, 0 );

		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x28 );
		slaved_on_B = VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x80;
		if ( slaved_on_B )
		{
			VGA_WR08( m_sHW.PCIO, 0x03D4, 0x33 );
			tvB = !( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x01 );
		}
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x44 );
		VGA_WR08( m_sHW.PCIO, 0x03D5, 0 );

		/* Select first head */
		m_sHW.PCIO = m_sHW.PCIO0;
		m_sHW.PCRTC = m_sHW.PCRTC0;
		m_sHW.PRAMDAC = m_sHW.PRAMDAC0;
		m_sHW.PDIO = m_sHW.PDIO0;
		NVLockUnlock( &m_sHW, 0 );

		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x28 );
		slaved_on_A = VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x80;
		if ( slaved_on_A )
		{
			VGA_WR08( m_sHW.PCIO, 0x03D4, 0x33 );
			tvA = !( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x01 );
		}
		oldhead = m_sHW.PCRTC0[0x00000860 / 4];
		m_sHW.PCRTC0[0x00000860 / 4] = oldhead | 0x00000010;

		if ( slaved_on_A && !tvA )
		{
			CRTCnumber = 0;
			FlatPanel = 1;
			dbprintf( "GeForce FX :: CRTC 0 is currently programmed for DFP\n" );
		}
		else if ( slaved_on_B && !tvB )
		{
			CRTCnumber = 1;
			FlatPanel = 1;
			dbprintf( "GeForce FX :: CRTC 1 is currently programmed for DFP\n" );
		}
		else if ( analog_on_A )
		{
			CRTCnumber = outputAfromCRTC;
			FlatPanel = 0;
			dbprintf( "GeForce FX :: CRTC %i appears to have a CRT attached\n", CRTCnumber );
		}
		else if ( analog_on_B )
		{
			CRTCnumber = outputBfromCRTC;
			FlatPanel = 0;
			dbprintf( "GeForce FX :: CRTC %i appears to have a CRT attached\n", CRTCnumber );
		}
		else if ( slaved_on_A )
		{
			CRTCnumber = 0;
			FlatPanel = 1;
			Television = 1;
			dbprintf( "GeForce FX :: CRTC 0 is currently programmed for TV\n" );
		}
		else if ( slaved_on_B )
		{
			CRTCnumber = 1;
			FlatPanel = 1;
			Television = 1;
			dbprintf( "GeForce FX :: CRTC 1 is currently programmed for TV\n" );
		}
		if ( m_sHW.FlatPanel == -1 )
		{
			if ( FlatPanel != -1 )
			{
				m_sHW.FlatPanel = FlatPanel;
				m_sHW.Television = Television;
			}
			else
			{
				dbprintf( "GeForce FX :: Unable to detect display type...\n" );
				if ( bMobile )
				{
					dbprintf( "GeForce FX :: ...On a laptop, assuming DFP\n" );
					m_sHW.FlatPanel = 1;
				}
				else
				{
					dbprintf( "GeForce FX :: ...Using default of CRT\n" );
					m_sHW.FlatPanel = 0;
				}
			}
		}
		if ( m_sHW.CRTCnumber == -1 )
		{
			if ( CRTCnumber != -1 )
				m_sHW.CRTCnumber = CRTCnumber;

			else
			{
				dbprintf( "GeForce FX :: Unable to detect which CRTCNumber...\n" );
				if ( m_sHW.FlatPanel )
					m_sHW.CRTCnumber = 1;

				else
					m_sHW.CRTCnumber = 0;
				dbprintf( "GeForce FX ::...Defaulting to CRTCNumber %i\n", m_sHW.CRTCnumber );
			}
		}
		else
		{
			dbprintf( "GeForce FX :: Forcing CRTCNumber %i as specified\n", m_sHW.CRTCnumber );
		}
		if ( nImplementation == 0x0110 )
			cr44 = m_sHW.CRTCnumber * 0x3;
		m_sHW.PCRTC0[0x00000860 / 4] = oldhead;
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x44 );
		VGA_WR08( m_sHW.PCIO, 0x03D5, cr44 );
		if ( m_sHW.CRTCnumber == 1 )
		{
			/* Select second head */
			m_sHW.PCIO = m_sHW.PCIO0 + 0x2000;
			m_sHW.PCRTC = m_sHW.PCRTC0 + 0x800;
			m_sHW.PRAMDAC = m_sHW.PRAMDAC0 + 0x800;
			m_sHW.PDIO = m_sHW.PDIO0 + 0x2000;
		}
		else
		{
			/* Select first head */
			m_sHW.PCIO = m_sHW.PCIO0;
			m_sHW.PCRTC = m_sHW.PCRTC0;
			m_sHW.PRAMDAC = m_sHW.PRAMDAC0;
			m_sHW.PDIO = m_sHW.PDIO0;
		}

	}

	dbprintf( "Using %s on CRTC %i\n", m_sHW.FlatPanel ? ( m_sHW.Television ? "TV" : "DFP" ) : "CRT", m_sHW.CRTCnumber );

	if ( m_sHW.FlatPanel && !m_sHW.Television )
	{
		m_sHW.fpWidth = m_sHW.PRAMDAC[0x0820 / 4] + 1;
		m_sHW.fpHeight = m_sHW.PRAMDAC[0x0800 / 4] + 1;
		m_sHW.fpSyncs = m_sHW.PRAMDAC[0x0848 / 4] & 0x30000033;
		dbprintf( "GeForce FX :: Panel size is %i x %i\n", m_sHW.fpWidth, m_sHW.fpHeight );
	}

	m_sHW.FPDither = false;
	
	m_sHW.LVDS = false;
    if(m_sHW.FlatPanel && m_sHW.twoHeads) {
        m_sHW.PRAMDAC0[0x08B0/4] = 0x00010004;
        if(m_sHW.PRAMDAC0[0x08B4/4] & 1)
           m_sHW.LVDS = true;
        dbprintf( "GeForce FX :: Panel is %s\n", 
                   m_sHW.LVDS ? "LVDS" : "TMDS");
    }
}


//-----------------------------------------------------------------------------
extern "C" DisplayDriver * init_gfx_driver( int nFd )
{
	dbprintf( "geforcefx attempts to initialize\n" );

	try
	{
		FX *pcDriver = new FX( nFd );

		if ( pcDriver->IsInitiated() )
		{
			return pcDriver;
		}
		return NULL;
	}
	catch( std::exception & cExc )
	{
		dbprintf( "Got exception\n" );
		return NULL;
	}
}
