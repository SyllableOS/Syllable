/*
 ** Radeon graphics driver for Syllable application server
 *  Copyright (C) 2004 Michael Krueger <invenies@web.de>
 *  Copyright (C) 2003 Arno Klenke <arno_klenke@yahoo.com>
 *  Copyright (C) 1998-2001 Kurt Skauen <kurt@atheos.cx>
 *
 ** This program is free software; you can redistribute it and/or
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
 *
 ** For information about used sources and further copyright notices
 *  see radeon.cpp
 */

#include "radeon.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void ATIRadeon::GetSettings()
{
	int conffile;
	char buffer[1200];
	int actual_length;

	/* Default settings (viable for installation CD) */
	m_bCfgEnableR300 = false;
	m_bCfgEnableMobility = false;
	m_bCfgEnableMirroring = true;
	m_bCfgEnableDebug = false;
	m_bCfgDisableBIOSUsage = false;

	/* Now open the configuration file if available */
	conffile = open( RADEON_CONFIG_FILE, O_RDONLY );
	if( conffile < 0 ) {
		dbprintf("Radeon :: Configuration file %s not available!\n", RADEON_CONFIG_FILE);
		dbprintf("Radeon :: Using default safe settings.\n");
	} else {
		/* Read configuration file */
		dbprintf("Radeon :: Configuration file %s found, reading...\n", RADEON_CONFIG_FILE);
		actual_length = read( conffile, buffer, 1200 );

		/* Close configuration file */
		close( conffile );

		/* Parse it! */
		if( strcasestr( buffer, "\nEnableR300=1") )
			m_bCfgEnableR300 = true;
		if( strcasestr( buffer, "\nEnableMobility=1") )
			m_bCfgEnableMobility = true;
		if( strcasestr( buffer, "\nEnableMirroring=0") )
			m_bCfgEnableMirroring = false;
		if( strcasestr( buffer, "\nEnableDebug=1") )
			m_bCfgEnableDebug = true;
		if( strcasestr( buffer, "\nDisableBIOSUsage=1") )
			m_bCfgDisableBIOSUsage = true;
	}

	/* Print out everything */
	dbprintf("Radeon :: 9500 and higher support: %s\n",
		m_bCfgEnableR300 ? "enabled" : "disabled");
	dbprintf("Radeon :: Mobility/IGP support: %s\n",
		m_bCfgEnableMobility ? "enabled" : "disabled");
	dbprintf("Radeon :: Notebook CRT mirroring support: %s\n",
		m_bCfgEnableMirroring ? "enabled" : "disabled");
	dbprintf("Radeon :: Debugging output: %s\n",
		m_bCfgEnableDebug ? "enabled" : "disabled");
	dbprintf("Radeon :: Card BIOS usage (disabling dangerous): %s\n",
		m_bCfgDisableBIOSUsage ? "disabled" : "enabled");
	return;
}

int ATIRadeon::BytesPerPixel (color_space cs) {
	switch( cs ) {
		case    CS_RGB32:
		case    CS_RGBA32:
			return( 4 );
		case    CS_RGB24:
			return( 3 );
		case    CS_RGB16:
			return( 2 );
		case CS_CMAP8:
		case CS_GRAY8:
			return( 1 );
		default:
			return( 1 );
	}
}

int ATIRadeon::BitsPerPixel (color_space cs) {
	switch( cs ) {
		case    CS_RGB32:
		case    CS_RGBA32:
			return( 32 );
		case    CS_RGB24:
			return( 24 );
		case    CS_RGB16:
			return( 16 );
		case CS_CMAP8:
		case CS_GRAY8:
			return( 8 );
		default:
			return( 8 );
	}
}

/*
 * Set DAC registers for proper directcolor palette mode
 */
void ATIRadeon::InitPalette()
{
	uint32 dac_cntl2, vclk_cntl = 0;

	if (rinfo.is_mobility) {
		vclk_cntl = INPLL(VCLK_ECP_CNTL);
		OUTPLL(VCLK_ECP_CNTL, vclk_cntl & ~PIXCLK_DAC_ALWAYS_ONb);
	}

	/* Make sure we are on first palette */
	if (rinfo.has_CRTC2) {
		dac_cntl2 = INREG(DAC_CNTL2);
		dac_cntl2 &= ~DAC2_PALETTE_ACCESS_CNTL;
		OUTREG(DAC_CNTL2, dac_cntl2);
	}

	OUTREG(PALETTE_INDEX, 0);
	for(int i = 0; i < 256; ++i)
		OUTREG(PALETTE_DATA, (i << 16) | (i << 8) | i);

	if (rinfo.is_mobility)
		OUTPLL(VCLK_ECP_CNTL, vclk_cntl);

	return;
}

/*
 * This reconfigure the card's internal memory map. In theory, we'd like
 * to setup the card's memory at the same address as it's PCI bus address,
 * and the AGP aperture right after that so that system RAM on 32 bits
 * machines at least, is directly accessible. However, doing so would
 * conflict with the current XFree drivers...
 * Ultimately, I hope XFree, GATOS and ATI binary drivers will all agree
 * on the proper way to set this up and duplicate this here. In the meantime,
 * I put the card's memory at 0 in card space and AGP at some random high
 * local (0xe0000000 for now) that will be changed by XFree/DRI anyway
 */
#define SET_MC_FB_FROM_APERTURE
void ATIRadeon::FixUpMemoryMappings()
{
	uint32 save_crtc_gen_cntl, save_crtc2_gen_cntl = 0;
	uint32 save_crtc_ext_cntl;
	uint32 aper_base, aper_size;
	uint32 agp_base;

	/* First, we disable display to avoid interfering */
	if (rinfo.has_CRTC2) {
		save_crtc2_gen_cntl = INREG(CRTC2_GEN_CNTL);
		OUTREG(CRTC2_GEN_CNTL, save_crtc2_gen_cntl | CRTC2_DISP_REQ_EN_B);
	}
	save_crtc_gen_cntl = INREG(CRTC_GEN_CNTL);
	save_crtc_ext_cntl = INREG(CRTC_EXT_CNTL);
	
	OUTREG(CRTC_EXT_CNTL, save_crtc_ext_cntl | CRTC_DISPLAY_DIS);
	OUTREG(CRTC_GEN_CNTL, save_crtc_gen_cntl | CRTC_DISP_REQ_EN_B);
	snooze(50000);

	aper_base = INREG(CONFIG_APER_0_BASE);
	aper_size = INREG(CONFIG_APER_SIZE);

#ifdef SET_MC_FB_FROM_APERTURE
	/* Set framebuffer to be at the same address as set in PCI BAR */
	OUTREG(MC_FB_LOCATION, 
		((aper_base + aper_size - 1) & 0xffff0000) | (aper_base >> 16));
	rinfo.fb_local_base = aper_base;
#else
	OUTREG(MC_FB_LOCATION, 0x8fff0000);
	rinfo.fb_local_base = 0;
#endif
	agp_base = aper_base + aper_size;
	if (agp_base & 0xf0000000)
		agp_base = (aper_base | 0x0fffffff) + 1;

	/* Set AGP to be just after the framebuffer on a 256Mb boundary. This
	 * assumes the FB isn't mapped to 0xf0000000 or above, but this is
	 * always the case on PPCs afaik.
	 */
#ifdef SET_MC_FB_FROM_APERTURE
	OUTREG(MC_AGP_LOCATION, 0xffff0000 | (agp_base >> 16));
#else
	OUTREG(MC_AGP_LOCATION, 0xfffff000);
#endif

	/* Fixup the display base addresses & engine offsets while we
	 * are at it as well
	 */
#ifdef SET_MC_FB_FROM_APERTURE
	OUTREG(DISPLAY_BASE_ADDR, aper_base);
	if (rinfo.has_CRTC2)
		OUTREG(CRTC2_DISPLAY_BASE_ADDR, aper_base);
#else
	OUTREG(DISPLAY_BASE_ADDR, 0);
	if (rinfo.has_CRTC2)
		OUTREG(CRTC2_DISPLAY_BASE_ADDR, 0);
#endif
	snooze(50000);

	/* Restore display settings */
	OUTREG(CRTC_GEN_CNTL, save_crtc_gen_cntl);
	OUTREG(CRTC_EXT_CNTL, save_crtc_ext_cntl);
	if (rinfo.has_CRTC2)
		OUTREG(CRTC2_GEN_CNTL, save_crtc2_gen_cntl);	

	RTRACE("aper_base: %08x MC_FB_LOC to: %08x, MC_AGP_LOC to: %08x\n",
		(uint)aper_base,
		(uint)(((aper_base + aper_size - 1) & 0xffff0000) | (aper_base >> 16)),
		(uint)(0xffff0000 | (agp_base >> 16)));
}

/*
 * Radeon M6, M7 and M9 Power Management code. This code currently
 * only supports the mobile chips in D2 mode, that is typically what
 * is used on Apple laptops, it's based from some informations provided
 * by ATI along with hours of tracing of MacOS drivers.
 * 
 * New version of this code almost totally rewritten by ATI, many thanks
 * for their support.
 */

void ATIRadeon::PMDisableDynamicMode()
{

	uint32 sclk_cntl;
	uint32 mclk_cntl;
	uint32 sclk_more_cntl;
	
	uint32 vclk_ecp_cntl;
	uint32 pixclks_cntl;

	/* Mobility chips only, untested on M10/11 */
	if (!rinfo.is_mobility)
		return;
	if (rinfo.family > CHIP_FAMILY_RV280)
		return;
	
	/* Force Core Clocks */
	sclk_cntl = INPLL( pllSCLK_CNTL_M6);
	sclk_cntl |= 	SCLK_CNTL_M6__FORCE_CP|
			SCLK_CNTL_M6__FORCE_HDP|
			SCLK_CNTL_M6__FORCE_DISP1|
			SCLK_CNTL_M6__FORCE_DISP2|
			SCLK_CNTL_M6__FORCE_TOP|
			SCLK_CNTL_M6__FORCE_E2|
			SCLK_CNTL_M6__FORCE_SE|
			SCLK_CNTL_M6__FORCE_IDCT|
			SCLK_CNTL_M6__FORCE_VIP|
			SCLK_CNTL_M6__FORCE_RE|
			SCLK_CNTL_M6__FORCE_PB|
			SCLK_CNTL_M6__FORCE_TAM|
			SCLK_CNTL_M6__FORCE_TDM|
			SCLK_CNTL_M6__FORCE_RB|
			SCLK_CNTL_M6__FORCE_TV_SCLK|
			SCLK_CNTL_M6__FORCE_SUBPIC|
			SCLK_CNTL_M6__FORCE_OV0;
    	OUTPLL( pllSCLK_CNTL_M6, sclk_cntl);
	
	
	
	sclk_more_cntl = INPLL(pllSCLK_MORE_CNTL);
	sclk_more_cntl |= 	SCLK_MORE_CNTL__FORCE_DISPREGS|
				SCLK_MORE_CNTL__FORCE_MC_GUI|
				SCLK_MORE_CNTL__FORCE_MC_HOST;	
	OUTPLL(pllSCLK_MORE_CNTL, sclk_more_cntl);
	
	/* Force Display clocks	*/
	vclk_ecp_cntl = INPLL( pllVCLK_ECP_CNTL);
	vclk_ecp_cntl &= ~(	VCLK_ECP_CNTL__PIXCLK_ALWAYS_ONb |
			 	VCLK_ECP_CNTL__PIXCLK_DAC_ALWAYS_ONb);

	OUTPLL( pllVCLK_ECP_CNTL, vclk_ecp_cntl);
	
	pixclks_cntl = INPLL( pllPIXCLKS_CNTL);
	pixclks_cntl &= ~(	PIXCLKS_CNTL__PIXCLK_GV_ALWAYS_ONb |
			 	PIXCLKS_CNTL__PIXCLK_BLEND_ALWAYS_ONb|
				PIXCLKS_CNTL__PIXCLK_DIG_TMDS_ALWAYS_ONb |
				PIXCLKS_CNTL__PIXCLK_LVDS_ALWAYS_ONb|
				PIXCLKS_CNTL__PIXCLK_TMDS_ALWAYS_ONb|
				PIXCLKS_CNTL__PIX2CLK_ALWAYS_ONb|
				PIXCLKS_CNTL__PIX2CLK_DAC_ALWAYS_ONb);
						
 	OUTPLL( pllPIXCLKS_CNTL, pixclks_cntl);

	/* Force Memory Clocks */
	mclk_cntl = INPLL( pllMCLK_CNTL_M6);
	mclk_cntl &= ~(	MCLK_CNTL_M6__FORCE_MCLKA |  
			MCLK_CNTL_M6__FORCE_MCLKB |
			MCLK_CNTL_M6__FORCE_YCLKA |
			MCLK_CNTL_M6__FORCE_YCLKB );
    	OUTPLL( pllMCLK_CNTL_M6, mclk_cntl);
}

void ATIRadeon::PMEnableDynamicMode()
{
	uint32 clk_pwrmgt_cntl;
	uint32 sclk_cntl;
	uint32 sclk_more_cntl;
	uint32 clk_pin_cntl;
	uint32 pixclks_cntl;
	uint32 vclk_ecp_cntl;
	uint32 mclk_cntl;
	uint32 mclk_misc;

	/* Mobility chips only, untested on M9+/M10/11 */
	if (!rinfo.is_mobility)
		return;
	if (rinfo.family > CHIP_FAMILY_RV280)
		return;
	
	/* Set Latencies */
	clk_pwrmgt_cntl = INPLL( pllCLK_PWRMGT_CNTL_M6);
	
	clk_pwrmgt_cntl &= ~(	 CLK_PWRMGT_CNTL_M6__ENGINE_DYNCLK_MODE_MASK|
				 CLK_PWRMGT_CNTL_M6__ACTIVE_HILO_LAT_MASK|
				 CLK_PWRMGT_CNTL_M6__DISP_DYN_STOP_LAT_MASK|
				 CLK_PWRMGT_CNTL_M6__DYN_STOP_MODE_MASK);
	/* Mode 1 */
	clk_pwrmgt_cntl = 	CLK_PWRMGT_CNTL_M6__MC_CH_MODE|
				CLK_PWRMGT_CNTL_M6__ENGINE_DYNCLK_MODE | 
				(1<<CLK_PWRMGT_CNTL_M6__ACTIVE_HILO_LAT__SHIFT) |
				(0<<CLK_PWRMGT_CNTL_M6__DISP_DYN_STOP_LAT__SHIFT)|
				(0<<CLK_PWRMGT_CNTL_M6__DYN_STOP_MODE__SHIFT);

	OUTPLL( pllCLK_PWRMGT_CNTL_M6, clk_pwrmgt_cntl);
						

	clk_pin_cntl = INPLL( pllCLK_PIN_CNTL);
	clk_pin_cntl |= CLK_PIN_CNTL__SCLK_DYN_START_CNTL;
	 
	OUTPLL( pllCLK_PIN_CNTL, clk_pin_cntl);

	/* Enable Dyanmic mode for SCLK */

	sclk_cntl = INPLL( pllSCLK_CNTL_M6);	
	sclk_cntl &= SCLK_CNTL_M6__SCLK_SRC_SEL_MASK;
	sclk_cntl |= SCLK_CNTL_M6__FORCE_VIP;		

	OUTPLL( pllSCLK_CNTL_M6, sclk_cntl);


	sclk_more_cntl = INPLL(pllSCLK_MORE_CNTL);
	sclk_more_cntl &= ~(SCLK_MORE_CNTL__FORCE_DISPREGS);
				                    
	OUTPLL(pllSCLK_MORE_CNTL, sclk_more_cntl);

	
	/* Enable Dynamic mode for PIXCLK & PIX2CLK */

	pixclks_cntl = INPLL( pllPIXCLKS_CNTL);
	
	pixclks_cntl|=  PIXCLKS_CNTL__PIX2CLK_ALWAYS_ONb | 
			PIXCLKS_CNTL__PIX2CLK_DAC_ALWAYS_ONb|
			PIXCLKS_CNTL__PIXCLK_BLEND_ALWAYS_ONb|
			PIXCLKS_CNTL__PIXCLK_GV_ALWAYS_ONb|
			PIXCLKS_CNTL__PIXCLK_DIG_TMDS_ALWAYS_ONb|
			PIXCLKS_CNTL__PIXCLK_LVDS_ALWAYS_ONb|
			PIXCLKS_CNTL__PIXCLK_TMDS_ALWAYS_ONb;

	OUTPLL( pllPIXCLKS_CNTL, pixclks_cntl);
		
		
	vclk_ecp_cntl = INPLL( pllVCLK_ECP_CNTL);
	
	vclk_ecp_cntl|=  VCLK_ECP_CNTL__PIXCLK_ALWAYS_ONb | 
			 VCLK_ECP_CNTL__PIXCLK_DAC_ALWAYS_ONb;

	OUTPLL( pllVCLK_ECP_CNTL, vclk_ecp_cntl);


	/* Enable Dynamic mode for MCLK	*/

	mclk_cntl  = INPLL( pllMCLK_CNTL_M6);
	mclk_cntl |= 	MCLK_CNTL_M6__FORCE_MCLKA|  
			MCLK_CNTL_M6__FORCE_MCLKB|	
			MCLK_CNTL_M6__FORCE_YCLKA|
			MCLK_CNTL_M6__FORCE_YCLKB;
			
    	OUTPLL( pllMCLK_CNTL_M6, mclk_cntl);

	mclk_misc = INPLL(pllMCLK_MISC);
	mclk_misc |= 	MCLK_MISC__MC_MCLK_MAX_DYN_STOP_LAT|
			MCLK_MISC__IO_MCLK_MAX_DYN_STOP_LAT|
			MCLK_MISC__MC_MCLK_DYN_ENABLE|
			MCLK_MISC__IO_MCLK_DYN_ENABLE;	
	
	OUTPLL(pllMCLK_MISC, mclk_misc);
}


