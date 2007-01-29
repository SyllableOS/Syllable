/*
 *  S3 Savage driver for Syllable
 *
 *  Based on the original SavageIX/MX driver for Syllable by Hillary Cheng
 *  and the X.org Savage driver.
 *
 *  X.org driver copyright Tim Roberts (timr@probo.com),
 *  Ani Joshi (ajoshi@unixbox.com) & S. Marineau
 *
 *  Copyright 2005 Kristian Van Der Vliet
 *  Copyright 2003 Hilary Cheng (hilarycheng@yahoo.com)
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

#include <savage_driver.h>
#include <savage_regs.h>
#include <savage_bci.h>
#include <savage_streams.h>

#include <atheos/areas.h>
#include <appserver/pci_graphics.h>

#include <unistd.h>

using namespace os;
using namespace std;

/* Table to map a PCI device ID to a chip family */
struct savage_chips
{
	uint16 nDeviceID;
	savage_type eChip;
};

static struct savage_chips g_sSavageChips[] =
{
	{ PCI_DEVICE_ID_SAVAGE3D, S3_SAVAGE3D },
	{ PCI_DEVICE_ID_SAVAGE3D_MV, S3_SAVAGE3D },
	{ PCI_DEVICE_ID_SAVAGE4, S3_SAVAGE4 },
	{ PCI_DEVICE_ID_SAVAGE2000, S3_SAVAGE2000 },
	{ PCI_DEVICE_ID_SAVAGE_MX_MV, S3_SAVAGE_MX },
	{ PCI_DEVICE_ID_SAVAGE_MX, S3_SAVAGE_MX },
	{ PCI_DEVICE_ID_SAVAGE_IX_MV, S3_SAVAGE_MX },
	{ PCI_DEVICE_ID_SAVAGE_IX, S3_SAVAGE_MX },
	{ PCI_DEVICE_ID_PROSAVAGE_PM, S3_PROSAVAGE },
	{ PCI_DEVICE_ID_PROSAVAGE_KM, S3_PROSAVAGE },
	{ PCI_DEVICE_ID_S3TWISTER_P, S3_TWISTER },
	{ PCI_DEVICE_ID_S3TWISTER_K, S3_TWISTER },
	{ PCI_DEVICE_ID_PROSAVAGE_DDR, S3_PROSAVAGEDDR },
	{ PCI_DEVICE_ID_PROSAVAGE_DDRK, S3_PROSAVAGEDDR },
	{ PCI_DEVICE_ID_SUPSAV_MX128, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_MX64, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_MX64C, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_IX128SDR, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_IX128DDR, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_IX64SDR, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_IX64DDR, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_IXCSDR, S3_SUPERSAVAGE },
	{ PCI_DEVICE_ID_SUPSAV_IXCDDR, S3_SUPERSAVAGE },
	{ 0, S3_UNKNOWN }
};

/* Table to map a chip family to a user-friendly name */
struct savage_name
{
	savage_type cChip;
	char *zName;
};

static struct savage_name g_sSavageNames[] = 
{
	{ S3_UNKNOWN, "Unknown" },
	{ S3_SAVAGE3D, "Savage3D" },
	{ S3_SAVAGE_MX, "MobileSavage" },
	{ S3_SAVAGE4, "Savage4" },
	{ S3_PROSAVAGE, "ProSavage" },
	{ S3_TWISTER, "Twister" },
	{ S3_PROSAVAGEDDR, "ProSavageDDR" },
	{ S3_SUPERSAVAGE, "SuperSavage" },
	{ S3_SAVAGE2000, "Savage2000" }
};

/* S3 Savage driver */

SavageDriver::SavageDriver( int nFd ) : m_cGELock( "savage_ge_lock" )
{
	/* Poison the variables */
	m_bIsInited = false;
	m_hRegisterArea = m_hFramebufferArea = -1;
	m_bVideoOverlayUsed = false;
	m_bEngineDirty = false;

	/* You can change these & recompile if you want to experiment */
	m_bDisableCOB = false;	/* Set true of you're having problems with Savage4 or ProSavage */

	/* Keep all the card info together */
	savage_s *psCard = (savage_s*)calloc( 1, sizeof( *m_psCard ) );
	if( NULL == psCard )
	{
		dbprintf( "Error: Failed to allocate card struct.\n" );
		return;
	}
	m_psCard = psCard;

	/* Get device info from kernel driver */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_psCard->sPCI ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}

	/* Poison the chip info */
	psCard->eChip = S3_UNKNOWN;

	int j = 0;	/* j is used as a general counter throughout */
	while( g_sSavageChips[j].nDeviceID != 0 )
	{
		if( g_sSavageChips[j].nDeviceID == psCard->sPCI.nDeviceID )
		{
			psCard->eChip = g_sSavageChips[j].eChip;
			break;
		}
		j++;
	}

	/* Check that we found a valid device */
	if( S3_UNKNOWN == psCard->eChip )
	{
		dbprintf( "Found an unknown S3 device.  ID 0x%04x\n", psCard->sPCI.nDeviceID );
		return;
	}

	dbprintf( "Found an S3 %s\n", g_sSavageNames[psCard->eChip].zName );

	/* Map MMIO registers */
	if( S3_SAVAGE3D_SERIES( psCard->eChip ) )
	{
		m_nRegisterAddr = ( psCard->sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) + SAVAGE_NEWMMIO_REGBASE_S3;
		m_nFramebufferAddr = ( psCard->sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK );
	}
	else
	{
		m_nRegisterAddr = ( psCard->sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) + SAVAGE_NEWMMIO_REGBASE_S4;
		m_nFramebufferAddr = ( psCard->sPCI.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK );
	}
	//dbprintf( "nRegisterAddr=0x%08lx    nFramebufferAddr=0x%08lx\n", m_nRegisterAddr, m_nFramebufferAddr );

	m_hRegisterArea = create_area( "savage_registers", (void**)&psCard->MapBase, SAVAGE_NEWMMIO_REGSIZE, AREA_FULL_ACCESS, AREA_NO_LOCK );
	if( remap_area( m_hRegisterArea, (void*)m_nRegisterAddr ) != EOK )
	{
		dbprintf( "Error: Failed to map MMIO registers.\n" );
		return;
	}

	psCard->VgaMem = psCard->MapBase + 0x8000;	/* Start of standard VGA registers */
	psCard->BciMem = psCard->MapBase + 0x10000;	/* Start of BCI(XXXKV?) registers */

	//dbprintf( "enabling MMIO\n" );

	/* Enable MMIO, unlock registers */
	int vgaIOBase, vgaCRIndex, vgaCRReg;
	uint8 val, config1, cr66 = 0, tmp;

	val = VGAIN8(0x3c3);
	VGAOUT8(0x3c3, val | 0x01);
	val = VGAIN8(VGA_MISC_OUT_R);
	VGAOUT8(VGA_MISC_OUT_W, val | 0x01);

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

	if( psCard->eChip >= S3_SAVAGE4 )
	{
		VGAOUT8(vgaCRIndex, 0x40);
		val = VGAIN8(vgaCRReg);
		VGAOUT8(vgaCRReg, val | 1);
	}

	//dbprintf( "MMIO enabled\n" );

	/* Unprotect CRTC[0-7] */
	VGAOUT8(vgaCRIndex, 0x11);
	tmp = VGAIN8(vgaCRReg);
	VGAOUT8(vgaCRReg, tmp & 0x7f);

	/* Unlock extended regs */
	VGAOUT16(vgaCRIndex, 0x4838);
	VGAOUT16(vgaCRIndex, 0xa039);
	VGAOUT16(0x3c4, 0x0608);

	VGAOUT8(vgaCRIndex, 0x40);
	tmp = VGAIN8(vgaCRReg);
	VGAOUT8(vgaCRReg, tmp & ~0x01);

	/* Unlock sys regs */
	VGAOUT8(vgaCRIndex, 0x38);
	VGAOUT8(vgaCRReg, 0x48);
    VGAOUT16(vgaCRIndex, 0x4838);

	/* Read config & get available video memory, calculate FB size & map FB */
	VGAOUT8(vgaCRIndex, 0x36);            /* for register CR36 (CONFG_REG1), */
	config1 = VGAIN8(vgaCRReg);           /* get amount of vram installed */

	/* Compute the amount of video memory and offscreen memory. */
	static uint8 anRamSavage3D[] = { 8, 4, 4, 2 };
	static uint8 anRamSavage4[] =  { 2, 4, 8, 12, 16, 32, 64, 32 };
	static uint8 anRamSavageMX[] = { 2, 8, 4, 16, 8, 16, 4, 16 };
	static uint8 anRamSavageNB[] = { 0, 2, 4, 8, 16, 32, 16, 2 };

	uint16 nRamSize;
	switch( psCard->eChip )
	{
		case S3_SAVAGE3D:
		{
			nRamSize = anRamSavage3D[ (config1 & 0xC0) >> 6 ] * 1024;
			break;
		}

		case S3_SAVAGE4:
		{
			/* 
			* The Savage4 has one ugly special case to consider.  On
			* systems with 4 banks of 2Mx32 SDRAM, the BIOS says 4MB
			* when it really means 8MB.  Why do it the same when you
			* can do it different...
			*/
			VGAOUT8(vgaCRIndex, 0x68);	/* memory control 1 */
			if( (VGAIN8(vgaCRReg) & 0xC0) == (0x01 << 6) )
				anRamSavage4[1] = 8;

			/*FALLTHROUGH*/
		}

		case S3_SAVAGE2000:
		{
			nRamSize = anRamSavage4[ (config1 & 0xE0) >> 5 ] * 1024;
			break;
		}

		case S3_SAVAGE_MX:
		case S3_SUPERSAVAGE:
		{
			nRamSize = anRamSavageMX[ (config1 & 0x0E) >> 1 ] * 1024;
			break;
		}

		case S3_PROSAVAGE:
		case S3_PROSAVAGEDDR:
		case S3_TWISTER:
		{
			nRamSize = anRamSavageNB[ (config1 & 0xE0) >> 5 ] * 1024;
			break;
		}

		case S3_UNKNOWN:
		default:
		{
			/* How did we get here? */
			nRamSize = 0;
			break;
		}
	}
	if( nRamSize == 0 )
	{
		dbprintf( "Failed to get video RAM size.\n" );
		return;
	}
	else
		dbprintf( "%uKB video RAM available.\n", nRamSize );

	/* Map the framebuffer */
	psCard->FramebufferSize = nRamSize * 1024;
	m_hFramebufferArea = create_area( "savage_framebuffer", (void**)&psCard->FBBase, psCard->FramebufferSize, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK );
	if( remap_area( m_hFramebufferArea, (void*)m_nFramebufferAddr ) != EOK )
	{
		dbprintf( "Error: Failed to map framebuffer.\n" );
		return;
	}
	//dbprintf( "FBBase=0x%08lx    FramebufferSize=%i\n", psCard->FBBase, psCard->FramebufferSize );

	/* Calculate the location of the Command Overflow Buffer (COB) & HW cursor bitmap */

	/*
	 * If we're running with acceleration, compute the command overflow
	 * buffer location.  The command overflow buffer must END at a
	 * 4MB boundary; for all practical purposes, that means the very
	 * end of the frame buffer.
	 */
	if( ((S3_SAVAGE4_SERIES(psCard->eChip)) || (S3_SUPERSAVAGE == psCard->eChip)) && m_bDisableCOB )
	{
		/*
		 * The Savage4 and ProSavage have COB coherency bugs which render 
		 * the buffer useless.
		 */
		psCard->cobIndex = 0;
		psCard->cobSize = 0;

		psCard->bciThresholdHi = 32;
		psCard->bciThresholdLo = 0;
	}
	else
	{
		/* We use 128kB for the COB on all other chips. */        
		psCard->cobSize = 0x20000;

		if (S3_SAVAGE3D_SERIES(psCard->eChip) || psCard->eChip == S3_SAVAGE2000)
			psCard->cobIndex = 7; /* rev.A savage4 apparently also uses 7 */
		else
			psCard->cobIndex = 2;

		/* max command size: 2560 entries */
		psCard->bciThresholdHi = psCard->cobSize/4 + 32 - 2560;
		psCard->bciThresholdLo = psCard->bciThresholdHi - 2560;
	}

	/* Align cob to 128k */
	psCard->cobOffset = (psCard->FramebufferSize - psCard->cobSize) & ~0x1ffff;

    /* The cursor must be aligned on a 4k boundary. */
	psCard->CursorKByte = (psCard->cobOffset >> 10) - 4;
	psCard->endfb = (psCard->CursorKByte << 10) - 1;

	/* Reset graphics engine to avoid memory corruption */
	VGAOUT8(vgaCRIndex, 0x66);
	cr66 = VGAIN8(vgaCRReg);
	VGAOUT8(vgaCRReg, cr66 | 0x02);
	usleep(10000);

	VGAOUT8(vgaCRIndex, 0x66);
	VGAOUT8(vgaCRReg, cr66 & ~0x02);	/* Clear reset flag */
	usleep(10000);

	/* Set status word positions based on chip type. */
	SavageInitStatus( psCard );

	/* Check for DVI/flat panel */
	bool bDvi = false;
	if(psCard->eChip == S3_SAVAGE4)
	{
		uint8 sr30 = 0x00; 
		VGAOUT8(0x3c4, 0x30);
		/* clear bit 1 */
		VGAOUT8(0x3c5, VGAIN8(0x3c5) & ~0x02);
		sr30 = VGAIN8(0x3c5);
		if (sr30 & 0x02 /*0x04 */)
		{
			bDvi = true;
			dbprintf( "Digital Flat Panel Detected\n");
		}
	}

	if( ( S3_SAVAGE_MOBILE_SERIES(psCard->eChip) || S3_MOBILE_TWISTER_SERIES(psCard->eChip) ) )
		psCard->eDisplayType = MT_LCD;
	else if ( bDvi )
		psCard->eDisplayType = MT_DFP;
	else
		psCard->eDisplayType = MT_CRT;	

	/* XXXKV: We could (should?) get DDC info here */

	/* Check LCD panel information */
	if(psCard->eDisplayType == MT_LCD)
		SavageGetPanelInfo( psCard );

	/* Build a displaymode list, taking into account the panel info if we're not using a CRT */
	VesaDriver::Open();

	int nVBECount = VesaDriver::GetScreenModeCount();
	for( j = 0; j < nVBECount; j++ )
	{
		os::screen_mode sMode;
		if( VesaDriver::GetScreenModeDesc( j, &sMode ) )
		{
			if( sMode.m_nWidth < 640 || sMode.m_nHeight < 480 )
				continue;

			if( psCard->PanelX > 0 && psCard->PanelY > 0 )
				if( sMode.m_nWidth > psCard->PanelX || sMode.m_nHeight > psCard->PanelY )
					continue;

			m_cModes.push_back( sMode );
		}
	}

	/* Calculate available space for off-screen bitmaps */
	uint32 nVRAM, nVidTop;

	nVidTop = ( 1024 * 1024 * 8 );
	if( psCard->FramebufferSize <= nVidTop )
	{
		if( psCard->PanelX > 0 && psCard->PanelY > 0 )
		{
			/* Calculate the max. possible framebuffer size */
			nVidTop = psCard->PanelX * ( psCard->PanelY * 4 );
		}
		else
		{
			/* XXXKV: Cross our fingers and hope.  We could always try to be more sophisticated and
			   limit the max. available video mode if we're this low on RAM */
			nVidTop = ( 1024 * 1024 * 4 );
		}
	}
	nVRAM = psCard->endfb - nVidTop;	/* Total available off-screen memeory */

	//dbprintf( "nVidTop=%d nVRAM=%d endfb=%d off-screen size=%d\n", nVidTop, nVRAM, psCard->endfb, psCard->endfb - nVidTop );

	psCard->nVidTop = nVidTop;
	psCard->nVRAM = nVRAM;

	if( ( nVRAM > ( 1024 * 1024 * 1 ) ) && ( nVRAM < psCard->FramebufferSize ) )
		InitMemory( nVidTop, nVRAM, 4096 - 1, 64 - 1 );

	//dbprintf( "initialisation complete.\n" );

	/* The device is ready to use */
	m_bIsInited = true;
}

void SavageDriver::SavageGetPanelInfo( savage_s *psCard )
{
	unsigned char cr6b;
	int panelX, panelY;
	char * sTechnology = "Unknown";
	enum ACTIVE_DISPLAYS { /* These are the bits in CR6B */
		ActiveCRT = 0x01,
		ActiveLCD = 0x02,
		ActiveTV = 0x04,
		ActiveCRT2 = 0x20,
		ActiveDUO = 0x80
	};

	/* Check LCD panel information */
	cr6b = readCrtc( 0x6b );

	panelX = (readSeq( 0x61 ) + ((readSeq( 0x66 ) & 0x02) << 7) + 1) * 8;
	panelY = readSeq( 0x69 ) + ((readSeq( 0x6e ) & 0x70) << 4) + 1;

	/* OK, I admit it.  I don't know how to limit the max dot clock
	 * for LCD panels of various sizes.  I thought I copied the formula
	 * from the BIOS, but many users have informed me of my folly.
	 *
	 * Instead, I'll abandon any attempt to automatically limit the 
	 * clock, and add an LCDClock option to XF86Config.  Some day,
	 * I should come back to this.
	 */

	if( (readSeq( 0x39 ) & 0x03) == 0 )
		sTechnology = "TFT";
	else if( (readSeq( 0x30 ) & 0x01) == 0 )
		sTechnology = "DSTN";
    else
		sTechnology = "STN";

	dbprintf( "%dx%d %s LCD panel detected %s\n", panelX, panelY, sTechnology, cr6b & ActiveLCD ? "and active" : "but not active");

	if( cr6b & ActiveLCD )
	{
		dbprintf( "- Limiting video mode to %dx%d\n", panelX, panelY );

		psCard->PanelX = panelX;
		psCard->PanelY = panelY;

		if( psCard->LCDClock > 0.0 )
		{
			psCard->maxClock = psCard->LCDClock * 1000.0;
			dbprintf( "- Limiting dot clock to %1.2f MHz\n", psCard->LCDClock );
		}
	}
	else
		psCard->eDisplayType = MT_CRT;
}

SavageDriver::~SavageDriver()
{
	if( m_hFramebufferArea != -1 )
		delete_area( m_hFramebufferArea );

	if( m_hRegisterArea != -1 )
		delete_area( m_hRegisterArea );

	if( m_psCard )
		free( m_psCard );
}

area_id SavageDriver::Open()
{
	savage_s *psCard = m_psCard;
	uint8 cr66 = 0;

	/* Reset graphics engine to avoid memory corruption */
	OUTREG8(CRT_ADDRESS_REG, 0x66);
	cr66 = INREG8(CRT_DATA_REG);
	OUTREG8(CRT_DATA_REG, cr66 | 0x02);
	usleep(10000);

	/* Clear Reset Flag */
	OUTREG8(CRT_ADDRESS_REG, 0x66);
	OUTREG8(CRT_DATA_REG, cr66 & ~0x02);	/* clear reset flag */
	usleep(10000);

	/* Make sure the start address has been reset to 0 */
	OUTREG16(CRT_ADDRESS_REG, (ushort) 0X0D);
	OUTREG16(CRT_ADDRESS_REG, (ushort) 0X0C);
	OUTREG16(CRT_ADDRESS_REG, (ushort) 0X69);

	/* Taken from SavageInitShadowStatus() */
	if( psCard->eChip == S3_SAVAGE2000 )
		psCard->dwBCIWait2DIdle = 0xc0040000;
	else
		psCard->dwBCIWait2DIdle = 0xc0020000;
    psCard->ShadowCounter = 0;

	return( m_hFramebufferArea );
}

void SavageDriver::Close()
{
	VesaDriver::Close();
}

int SavageDriver::GetScreenModeCount()
{
	return m_cModes.size();
}

bool SavageDriver::GetScreenModeDesc( int nIndex, screen_mode *psMode )
{
	if( nIndex < 0 || nIndex > (int)m_cModes.size() )
		return false;

	*psMode = m_cModes[nIndex];
	return true;
}

int SavageDriver::SetScreenMode( screen_mode sMode )
{
	int nRet;
	savage_s *psCard = m_psCard;

	/* Store the display configuration for later use */
	psCard->scrn.crtPosH = (int)sMode.m_vHPos;
	psCard->scrn.crtSizeH = (int)sMode.m_vHSize;
 	psCard->scrn.crtPosV = (int)sMode.m_vVPos;
	psCard->scrn.crtSizeV = (int)sMode.m_vVSize;
	psCard->scrn.Width = sMode.m_nWidth;
	psCard->scrn.Height = sMode.m_nHeight;
	psCard->scrn.RefreshRate = sMode.m_vRefreshRate;
	if( sMode.m_eColorSpace == CS_RGB16 )
	{
		psCard->scrn.Bpp = 16;
		psCard->scrnBytes = (psCard->scrn.Width * psCard->scrn.Height) * 2;
	}
	else if( sMode.m_eColorSpace == CS_RGB32 )
	{
		psCard->scrn.Bpp = 32;
		psCard->scrnBytes = (psCard->scrn.Width * psCard->scrn.Height) * 4;
	}
	else
	{
		dbprintf( "Unsupported color space %u requested!\n", sMode.m_eColorSpace );
		return -1;
	}
	psCard->scrn.primStreamBpp = psCard->scrn.Bpp;

	if( psCard->eDisplayType != MT_CRT )
		if( psCard->scrn.Width > psCard->PanelX || psCard->scrn.Height > psCard->PanelY )
		{
			dbprintf( "%u x %u is too large for %u x %u LCD display\n", psCard->scrn.Width, psCard->scrn.Height, psCard->PanelX, psCard->PanelY );
			return -1;
		}

	/* Configure for this video mode */
	int vgaIOBase, vgaCRIndex, vgaCRReg;

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

    if( !S3_SAVAGE_MOBILE_SERIES( psCard->eChip ) )
		SavageInitialize2DEngine( psCard );

	int width;
	unsigned short cr6d;
	unsigned short cr79 = 0;

	/* Set up the mode.  Don't clear video RAM. */
	/* Let the VESA BIOS do the hard work */
	nRet = VesaDriver::SetScreenMode( sMode );

	/* Setup the graphics engine for the new mode */
	m_cGELock.Lock();

	/* Unlock the extended registers. */
	VGAOUT16(vgaCRIndex, 0x4838);
	VGAOUT16(vgaCRIndex, 0xA039);
	VGAOUT16(0x3c4, 0x0608);

	/* Enable linear addressing. */
	VGAOUT16(vgaCRIndex, 0x1358);

	/* Disable old MMIO. */
	VGAOUT8(vgaCRIndex, 0x53);
	VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) & ~0x10);

	/* Disable HW cursor */
	VGAOUT16(vgaCRIndex, 0x0045);

	int cr67 = 0x00;
	switch( psCard->scrn.Bpp )
	{
		case 16:
		{
			if( S3_SAVAGE_MOBILE_SERIES(psCard->eChip) || ((psCard->eChip == S3_SAVAGE2000) && (psCard->scrn.RefreshRate >= 130.0f)) )
				cr67 = 0x50;	/* 16bpp, 2 pixel/clock */
			else
				cr67 = 0x40;	/* 16bpp, 1 pixels/clock */
			break;
		}

		case 32:
		{
			cr67 = 0xd0;
			break;
		}
	}

	/* Set the color mode. */
	VGAOUT8(vgaCRIndex, 0x67);
	VGAOUT8(vgaCRReg, cr67);

	/* Enable gamma correction, set CLUT to 8 bit */
	VGAOUT8(0x3c4, 0x1b);
	VGAOUT8(0x3c5, 0x10 );

	/* Set FIFO fetch delay. */
	VGAOUT8(vgaCRIndex, 0x85);
	VGAOUT8(vgaCRReg, (VGAIN8(vgaCRReg) & 0xf8) | 0x03);

	/* Patch CR79.  These values are magical. */
	if( !S3_SAVAGE_MOBILE_SERIES(psCard->eChip) )
	{
		VGAOUT8(vgaCRIndex, 0x6d);
		cr6d = VGAIN8(vgaCRReg);

		cr79 = 0x04;

		if( psCard->scrn.Width >= 1024 )
		{
			if( psCard->scrn.primStreamBpp == 32 )
			{
				if( psCard->scrn.RefreshRate >= 130.0f )
					cr79 = 0x03;
		    	else if( psCard->scrn.Width >= 1280 )
					cr79 = 0x02;
		    	else if( (psCard->scrn.Width == 1024) && (psCard->scrn.RefreshRate >= 75.0f) )
				{
					if( cr6d && LCD_ACTIVE )
						cr79 = 0x05;
					else
						cr79 = 0x08;
				}
			}
			else if( psCard->scrn.primStreamBpp == 16)
			{
				if( psCard->scrn.Width == 1024 )
				{
					if( cr6d && LCD_ACTIVE )
						cr79 = 0x08;
					else
						cr79 = 0x0e;
				}
			}
		}
	}

	if( (psCard->eChip != S3_SAVAGE2000) && !S3_SAVAGE_MOBILE_SERIES(psCard->eChip) )
		VGAOUT16(vgaCRIndex, (cr79 << 8) | 0x79);

	/* Make sure 16-bit memory access is enabled. */
	VGAOUT16(vgaCRIndex, 0x0c31);

	/* Enable the graphics engine. */
	VGAOUT16(vgaCRIndex, 0x0140);

	/* Handle the pitch. */
	VGAOUT8(vgaCRIndex, 0x50);
	VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) | 0xC1);

	width = (psCard->scrn.Width * (psCard->scrn.primStreamBpp / 8)) >> 3;
	VGAOUT16(vgaCRIndex, ((width & 0xff) << 8) | 0x13 );
	VGAOUT16(vgaCRIndex, ((width & 0x300) << 4) | 0x51 );

	/* Some non-S3 BIOSes enable block write even on non-SGRAM devices. */
	switch( psCard->eChip )
	{
		case S3_SAVAGE2000:
		{
			VGAOUT8(vgaCRIndex, 0x73);
			VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) & 0xdf );
			break;
		}

		case S3_SAVAGE3D:
		case S3_SAVAGE4:
		{
			VGAOUT8(vgaCRIndex, 0x68);
			if( !(VGAIN8(vgaCRReg) & 0x80) )
			{
		    	/* Not SGRAM; disable block write. */
		    	VGAOUT8(vgaCRIndex, 0x88);
		    	VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) | 0x10);
			}
			break;
		}
		default:
			break;
	}

	/* set the correct clock for some BIOSes */
	VGAOUT8(VGA_MISC_OUT_W, 
	VGAIN8(VGA_MISC_OUT_R) | 0x0C);

	/* Some BIOSes turn on clock doubling on non-doubled modes */
	if( psCard->scrn.Bpp < 24 )
	{
		VGAOUT8(vgaCRIndex, 0x67);
		if (!(VGAIN8(vgaCRReg) & 0x10))
		{
			VGAOUT8(0x3c4, 0x15);
			VGAOUT8(0x3c5, VGAIN8(0x3C5) & ~0x10);
			VGAOUT8(0x3c4, 0x18);
			VGAOUT8(0x3c5, VGAIN8(0x3c5) & ~0x80);
		}
	}

	SavageInitialize2DEngine( psCard );

	VGAOUT16(vgaCRIndex, 0x0140);

	m_cGELock.Unlock();

	return nRet;
}

void SavageDriver::SavageInitStatus( savage_s *psCard )
{
	switch( psCard->eChip )
	{
		case S3_SAVAGE3D:
		case S3_SAVAGE_MX:
		{
			psCard->eWaitQueue = S3_WAIT_3D;
			psCard->eWaitIdle = S3_WAIT_3D;
			psCard->eWaitIdleEmpty = S3_WAIT_3D;
			psCard->bciUsedMask   = 0x1ffff;
			psCard->eventStatusReg= 1;
			break;
		}

		case S3_SAVAGE4:
		case S3_PROSAVAGE:
		case S3_SUPERSAVAGE:
		case S3_PROSAVAGEDDR:
		case S3_TWISTER:
		{
			psCard->eWaitQueue = S3_WAIT_4;
			psCard->eWaitIdle = S3_WAIT_4;
			psCard->eWaitIdleEmpty = S3_WAIT_4;
			psCard->bciUsedMask   = 0x1fffff;
			psCard->eventStatusReg= 1;
			break;
		}

		case S3_SAVAGE2000:
		{
			psCard->eWaitQueue = S3_WAIT_2K;
			psCard->eWaitIdle = S3_WAIT_2K;
			psCard->eWaitIdleEmpty = S3_WAIT_2K;
			psCard->bciUsedMask   = 0xfffff;
			psCard->eventStatusReg= 2;
			break;
		}

		case S3_UNKNOWN:
			break;
	}
}

int SavageDriver::WaitQueue( savage_s *psCard, int v )
{
	switch( psCard->eWaitQueue )
	{
		case S3_WAIT_3D:
			return WaitQueue3D( psCard, v );

		case S3_WAIT_4:
			return WaitQueue4( psCard, v );

		case S3_WAIT_2K:
			return WaitQueue2K( psCard, v );

		case S3_WAIT_SHADOW:
			return ShadowWaitQueue( psCard, v );
	}

	return 0;
}

int SavageDriver::WaitIdle( savage_s *psCard )
{
	switch( psCard->eWaitIdle )
	{
		case S3_WAIT_3D:
			return WaitIdle3D( psCard );

		case S3_WAIT_4:
			return WaitIdle4( psCard );

		case S3_WAIT_2K:
			return WaitIdle2K( psCard );

		case S3_WAIT_SHADOW:
			return ShadowWait( psCard );
	}

	return 0;
}

int SavageDriver::WaitIdleEmpty( savage_s *psCard )
{
	switch( psCard->eWaitIdle )
	{
		case S3_WAIT_3D:
			return WaitIdleEmpty3D( psCard );

		case S3_WAIT_4:
			return WaitIdleEmpty4( psCard );

		case S3_WAIT_2K:
			return WaitIdleEmpty2K( psCard );

		case S3_WAIT_SHADOW:
			return ShadowWait( psCard );
	}

	return 0;
}

/* Wait until "v" queue entries are free */

int SavageDriver::WaitQueue3D( savage_s *psCard, int v )
{
	int loop = 0;
	uint32 slots = MAXFIFO - v;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitQueue = S3_WAIT_SHADOW;
		return ShadowWaitQueue( psCard, v);
	}

	loop &= STATUS_WORD0;
	while( ((STATUS_WORD0 & 0x0000ffff) > slots) && (loop++ < MAXLOOP))
		;

	return loop >= MAXLOOP;
}

int SavageDriver::WaitQueue4( savage_s *psCard, int v )
{
	int loop = 0;
	uint32 slots = MAXFIFO - v;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitQueue = S3_WAIT_SHADOW;
		return ShadowWaitQueue( psCard, v);
	}

	while( ((ALT_STATUS_WORD0 & 0x001fffff) > slots) && (loop++ < MAXLOOP))
		;

	return loop >= MAXLOOP;
}

int SavageDriver::WaitQueue2K( savage_s *psCard, int v )
{
	int loop = 0;
	uint32 slots = (MAXFIFO - v) / 4;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitQueue = S3_WAIT_SHADOW;
		return ShadowWaitQueue( psCard, v);
	}
	else
		while( ((ALT_STATUS_WORD0 & 0x000fffff) > slots) && (loop++ < MAXLOOP))
		;

	if( loop >= MAXLOOP )
		ResetBCI2K( psCard );

	return loop >= MAXLOOP;
}

/* Wait until GP is idle and queue is empty */

int SavageDriver::WaitIdleEmpty3D( savage_s *psCard )
{
	int loop = 0;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitIdleEmpty = S3_WAIT_SHADOW;
		return ShadowWait( psCard );
	}

	loop &= STATUS_WORD0;
	while( ((STATUS_WORD0 & 0x0008ffff) != 0x80000) && (loop++ < MAXLOOP) )
		;

	return loop >= MAXLOOP;
}

int SavageDriver::WaitIdleEmpty4( savage_s *psCard )
{
	int loop = 0;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitIdleEmpty = S3_WAIT_SHADOW;
		return ShadowWait( psCard );
	}

	while (((ALT_STATUS_WORD0 & 0x00e1ffff) != 0x00e00000) && (loop++ < MAXLOOP))
		;

	return loop >= MAXLOOP;
}

int SavageDriver::WaitIdleEmpty2K( savage_s *psCard )
{
	int loop = 0;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitIdleEmpty = S3_WAIT_SHADOW;
		return ShadowWait( psCard );
	}

	loop &= ALT_STATUS_WORD0;
	while( ((ALT_STATUS_WORD0 & 0x009fffff) != 0) && (loop++ < MAXLOOP) )
		;

	if( loop >= MAXLOOP )
		ResetBCI2K( psCard );

	return loop >= MAXLOOP;
}

/* Wait until GP is idle */

int SavageDriver::WaitIdle3D( savage_s *psCard )
{
	int loop = 0;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitIdle = S3_WAIT_SHADOW;
		return ShadowWait( psCard );
	}

	while( (!(STATUS_WORD0 & 0x00080000)) && (loop++ < MAXLOOP) )
		;

	return loop >= MAXLOOP;
}

int SavageDriver::WaitIdle4( savage_s *psCard )
{
	int loop = 0;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitIdle = S3_WAIT_SHADOW;
		return ShadowWait( psCard );
	}

	while (((ALT_STATUS_WORD0 & 0x00E00000)!=0x00E00000) && (loop++ < MAXLOOP))
		;

	return loop >= MAXLOOP;
}

int SavageDriver::WaitIdle2K( savage_s *psCard )
{
	int loop = 0;

	if( psCard->ShadowVirtual )
	{
		psCard->eWaitIdle = S3_WAIT_SHADOW;
		return ShadowWait( psCard );
	}

	loop &= ALT_STATUS_WORD0;
	while( (ALT_STATUS_WORD0 & 0x00900000) && (loop++ < MAXLOOP) )
		;

	return loop >= MAXLOOP;
}

void SavageDriver::ResetBCI2K( savage_s *psCard )
{
	uint32 cob = INREG( 0x48c18 );
	/* if BCI is enabled and BCI is busy... */

	if( (cob & 0x00000008) && !(ALT_STATUS_WORD0 & 0x00200000) )
	{
		dbprintf( "Resetting BCI, stat = %08lx...\n",(unsigned long) ALT_STATUS_WORD0);
		/* Turn off BCI */
		OUTREG( 0x48c18, cob & ~8 );
		usleep(10000);
		/* Turn it back on */
		OUTREG( 0x48c18, cob );
		usleep(10000);
	}
}

bool SavageDriver::ShadowWait( savage_s *psCard )
{
	BCI_GET_PTR;
	int loop = 0;

	psCard->ShadowCounter = (psCard->ShadowCounter + 1) & 0xffff;
	if (psCard->ShadowCounter == 0)
		psCard->ShadowCounter++; /* 0 is reserved for the BIOS to avoid confusion in the DRM */
	BCI_SEND( psCard->dwBCIWait2DIdle );
	BCI_SEND( 0x98000000 + psCard->ShadowCounter );

	while((int)(psCard->ShadowVirtual[psCard->eventStatusReg] & 0xffff) != psCard->ShadowCounter && (loop++ < MAXLOOP) )
		;

	return loop >= MAXLOOP;
}

bool SavageDriver::ShadowWaitQueue( savage_s *psCard, int v )
{
	int loop = 0;
	uint32 slots = MAXFIFO - v;

	if (slots >= psCard->bciThresholdHi)
		slots = psCard->bciThresholdHi;
	else
		return ShadowWait( psCard );

	/* Savage 2000 reports only entries filled in the COB, not the on-chip
	 * queue. Also it reports in qword units instead of dwords. */
	if (psCard->eChip == S3_SAVAGE2000)
		slots = (slots - 32) / 4;

	while( ((psCard->ShadowVirtual[0] & psCard->bciUsedMask) >= slots) && (loop++ < MAXLOOP))
		;

	return loop >= MAXLOOP;
}

/* Entry point */
extern "C" DisplayDriver *init_gfx_driver( int nFd )
{
	dbprintf( "Initialise S3 Savage.\n" );

	SavageDriver *pcDriver = new SavageDriver( nFd );
	if( pcDriver->IsInited() == false )
	{
		delete( pcDriver );
		pcDriver = NULL;
	}

	return pcDriver;
}

