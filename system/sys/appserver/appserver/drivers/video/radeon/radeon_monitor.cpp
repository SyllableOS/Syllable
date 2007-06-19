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

char* ATIRadeon::GetMonName(int type)
{
	char *pret = NULL;

	switch (type) {
		case MT_NONE:
			pret = "no";
			break;
		case MT_CRT:
			pret = "CRT";
			break;
		case MT_DFP:
			pret = "DFP";
			break;
		case MT_LCD:
			pret = "LCD";
			break;
		case MT_CTV:
			pret = "CTV";
			break;
		case MT_STV:
			pret = "STV";
			break;
	}

	return pret;
}

int ATIRadeon::GetPanelInfoBIOS()
{
	unsigned long tmp, tmp0;
	char stmp[30];
	int i;

	if (!rinfo.bios_seg)
		return 0;

	if (!(tmp = BIOS_IN16(rinfo.fp_bios_start + 0x40))) {
		dbprintf("Radeon :: Failed to detect DFP panel info using BIOS\n");
		rinfo.panel_info.pwr_delay = 200;
		return 0;
	}

	for(i=0; i<24; i++)
		stmp[i] = BIOS_IN8(tmp+i+1);
	stmp[24] = 0;
	dbprintf("Radeon :: panel ID string: %s\n", stmp);
	rinfo.panel_info.xres = BIOS_IN16(tmp + 25);
	rinfo.panel_info.yres = BIOS_IN16(tmp + 27);
	dbprintf("Radeon :: detected LVDS panel size from BIOS: %dx%d\n",
		rinfo.panel_info.xres, rinfo.panel_info.yres);

	rinfo.panel_info.pwr_delay = BIOS_IN16(tmp + 44);
	RTRACE("BIOS provided panel power delay: %d\n", rinfo.panel_info.pwr_delay);
	if (rinfo.panel_info.pwr_delay > 2000 || rinfo.panel_info.pwr_delay < 0)
		rinfo.panel_info.pwr_delay = 2000;

	/*
	 * Some panels only work properly with some divider combinations
	 */
	rinfo.panel_info.ref_divider = BIOS_IN16(tmp + 46);
	rinfo.panel_info.post_divider = BIOS_IN8(tmp + 48);
	rinfo.panel_info.fbk_divider = BIOS_IN16(tmp + 49);
	if (rinfo.panel_info.ref_divider != 0 &&
	    rinfo.panel_info.fbk_divider > 3) {
		rinfo.panel_info.use_bios_dividers = 1;
		dbprintf("Radeon :: BIOS provided dividers will be used\n");
		RTRACE("ref_divider = %x\n", rinfo.panel_info.ref_divider);
		RTRACE("post_divider = %x\n", rinfo.panel_info.post_divider);
		RTRACE("fbk_divider = %x\n", rinfo.panel_info.fbk_divider);
	}
	RTRACE("Scanning BIOS table ...\n");
	for(i=0; i<32; i++) {
		tmp0 = BIOS_IN16(tmp+64+i*2);
		if (tmp0 == 0)
			break;
		RTRACE(" %d x %d\n", BIOS_IN16(tmp0), BIOS_IN16(tmp0+2));
		if ((BIOS_IN16(tmp0) == rinfo.panel_info.xres) &&
		    (BIOS_IN16(tmp0+2) == rinfo.panel_info.yres)) {
			rinfo.panel_info.hblank = (BIOS_IN16(tmp0+17) - BIOS_IN16(tmp0+19)) * 8;
			rinfo.panel_info.hOver_plus = ((BIOS_IN16(tmp0+21) -
							 BIOS_IN16(tmp0+19) -1) * 8) & 0x7fff;
			rinfo.panel_info.hSync_width = BIOS_IN8(tmp0+23) * 8;
			rinfo.panel_info.vblank = BIOS_IN16(tmp0+24) - BIOS_IN16(tmp0+26);
			rinfo.panel_info.vOver_plus = (BIOS_IN16(tmp0+28) & 0x7ff) - BIOS_IN16(tmp0+26);
			rinfo.panel_info.vSync_width = (BIOS_IN16(tmp0+28) & 0xf800) >> 11;
			rinfo.panel_info.clock = BIOS_IN16(tmp0+9);
			/* Assume high active syncs for now until ATI tells me more... maybe we
			 * can probe register values here ?
			 */
			rinfo.panel_info.hAct_high = 1;
			rinfo.panel_info.vAct_high = 1;
			/* Mark panel infos valid */
			rinfo.panel_info.valid = 1;

			RTRACE("Found panel in BIOS table:\n");
			RTRACE("  hblank: %d\n", rinfo.panel_info.hblank);
			RTRACE("  hOver_plus: %d\n", rinfo.panel_info.hOver_plus);
			RTRACE("  hSync_width: %d\n", rinfo.panel_info.hSync_width);
			RTRACE("  vblank: %d\n", rinfo.panel_info.vblank);
			RTRACE("  vOver_plus: %d\n", rinfo.panel_info.vOver_plus);
			RTRACE("  vSync_width: %d\n", rinfo.panel_info.vSync_width);
			RTRACE("  clock: %d\n", rinfo.panel_info.clock);
				
			return 1;
		}
	}
	RTRACE("Didn't find panel in BIOS table !\n");

	return 0;
}

/*
 * Probe physical connection of a CRT. This code comes from XFree
 * as well and currently is only implemented for the CRT DAC, the
 * code for the TVDAC is commented out in XFree as "non working"
 */
int ATIRadeon::CrtIsConnected(int is_crt_dac)
{
	int	          connected = 0;

	/* the monitor either wasn't connected or it is a non-DDC CRT.
	  * try to probe it
	  */
	if(is_crt_dac) {
		unsigned long ulOrigVCLK_ECP_CNTL;
		unsigned long ulOrigDAC_CNTL;
		unsigned long ulOrigDAC_EXT_CNTL;
		unsigned long ulOrigCRTC_EXT_CNTL;
		unsigned long ulData;
		unsigned long ulMask;

		ulOrigVCLK_ECP_CNTL = INPLL(VCLK_ECP_CNTL);

		ulData              = ulOrigVCLK_ECP_CNTL;
		ulData             &= ~(PIXCLK_ALWAYS_ONb
				| PIXCLK_DAC_ALWAYS_ONb);
		ulMask              = ~(PIXCLK_ALWAYS_ONb
				| PIXCLK_DAC_ALWAYS_ONb);
		OUTPLLP(VCLK_ECP_CNTL, ulData, ulMask);

		ulOrigCRTC_EXT_CNTL = INREG(CRTC_EXT_CNTL);
		ulData              = ulOrigCRTC_EXT_CNTL;
		ulData             |= CRTC_CRT_ON;
		OUTREG(CRTC_EXT_CNTL, ulData);

		ulOrigDAC_EXT_CNTL = INREG(DAC_EXT_CNTL);
		ulData             = ulOrigDAC_EXT_CNTL;
		ulData            &= ~DAC_FORCE_DATA_MASK;
		ulData            |=  (DAC_FORCE_BLANK_OFF_EN
			       |DAC_FORCE_DATA_EN
			       |DAC_FORCE_DATA_SEL_MASK);
		if ((rinfo.family == CHIP_FAMILY_RV250) ||
				(rinfo.family == CHIP_FAMILY_RV280))
			ulData |= (0x01b6 << DAC_FORCE_DATA_SHIFT);
		else
			ulData |= (0x01ac << DAC_FORCE_DATA_SHIFT);

		OUTREG(DAC_EXT_CNTL, ulData);

		ulOrigDAC_CNTL     = INREG(DAC_CNTL);
		ulData             = ulOrigDAC_CNTL;
		ulData            |= DAC_CMP_EN;
		ulData            &= ~(DAC_RANGE_CNTL_MASK
			       | DAC_PDWN);
		ulData            |= 0x2;
		OUTREG(DAC_CNTL, ulData);

		//mdelay(1);
		snooze(1000);

		ulData     = INREG(DAC_CNTL);
		connected =  (DAC_CMP_OUTPUT & ulData) ? 1 : 0;

		ulData    = ulOrigVCLK_ECP_CNTL;
		ulMask    = 0xFFFFFFFFL;
		OUTPLLP(VCLK_ECP_CNTL, ulData, ulMask);

		OUTREG(DAC_CNTL,      ulOrigDAC_CNTL     );
		OUTREG(DAC_EXT_CNTL,  ulOrigDAC_EXT_CNTL );
		OUTREG(CRTC_EXT_CNTL, ulOrigCRTC_EXT_CNTL);
	}

	return connected ? MT_CRT : MT_NONE;
}


/*
 * Parse the "monitor_layout" string if any. This code is mostly
 * copied from XFree's radeon driver
 */
int ATIRadeon::ParseMonitorLayout(const char *monitor_layout)
{
	char s1[5], s2[5];
	int i = 0, second = 0;
	const char *s;

	if (!monitor_layout)
		return 0;

	s = monitor_layout;
	do {
		switch(*s) {
		case ',':
			s1[i] = '\0';
			i = 0;
			second = 1;
			break;
		case ' ':
		case '\0':
			break;
		default:
			if (i > 4)
				break;
			if (second)
				s2[i] = *s;
			else
				s1[i] = *s;
			i++;
		}
	} while (*s++);
	if (second)
		s2[i] = 0;
	else {
		s1[i] = 0;
		s2[0] = 0;
	}
	if (strcmp(s1, "CRT") == 0)
		rinfo.mon1_type = MT_CRT;
	else if (strcmp(s1, "TMDS") == 0)
		rinfo.mon1_type = MT_DFP;
	else if (strcmp(s1, "LVDS") == 0)
		rinfo.mon1_type = MT_LCD;

	if (strcmp(s2, "CRT") == 0)
		rinfo.mon2_type = MT_CRT;
	else if (strcmp(s2, "TMDS") == 0)
		rinfo.mon2_type = MT_DFP;
	else if (strcmp(s2, "LVDS") == 0)
		rinfo.mon2_type = MT_LCD;

	return 1;
}


/*
 * Probe display on both primary and secondary card's connector (if any)
 * by various available techniques (i2c, OF device tree, BIOS, ...) and
 * try to retreive EDID. The algorithm here comes from XFree's radeon
 * driver
 */
void ATIRadeon::ProbeScreens(const char *monitor_layout, int ignore_edid)
{
	int tmp, i;

	if (ParseMonitorLayout(monitor_layout)) {

		/*
		 * If user specified a monitor_layout option, use it instead
		 * of auto-detecting. Maybe we should only use this argument
		 * on the first radeon card probed or provide a way to specify
		 * a layout for each card ?
		 */

		RTRACE("Radeon :: Using specified monitor layout: %s", monitor_layout);

		if (rinfo.mon1_type == MT_NONE) {
			if (rinfo.mon2_type != MT_NONE) {
				rinfo.mon1_type = rinfo.mon2_type;
				rinfo.mon1_EDID = rinfo.mon2_EDID;
			} else {
				rinfo.mon1_type = MT_CRT;
				dbprintf("Radeon :: No valid monitor, assuming CRT on first port\n");
			}
			rinfo.mon2_type = MT_NONE;
			rinfo.mon2_EDID = NULL;
		}
	} else {

		/*
		 * Auto-detecting display type (well... trying to ...)
		 */
		
		RTRACE("Radeon :: Starting monitor auto detection...\n");

		/*
		 * Old single head cards
		 */
		if (!rinfo.has_CRTC2) {
			if (rinfo.mon1_type == MT_NONE)
				rinfo.mon1_type = MT_CRT;
			goto bail;
		}

		/*
		 * Check for cards with reversed DACs or TMDS controllers using BIOS
		 */
		if (rinfo.bios_seg &&
		    (tmp = BIOS_IN16(rinfo.fp_bios_start + 0x50))) {
			for (i = 1; i < 4; i++) {
				unsigned int tmp0;

				if (!BIOS_IN8(tmp + i*2) && i > 1)
					break;
				tmp0 = BIOS_IN16(tmp + i*2);
				if ((!(tmp0 & 0x01)) && (((tmp0 >> 8) & 0x0f) == ddc_dvi)) {
					rinfo.reversed_DAC = 1;
					dbprintf("Radeon :: Reversed DACs detected\n");
				}
				if ((((tmp0 >> 8) & 0x0f) == ddc_dvi) && ((tmp0 >> 4) & 0x01)) {
					rinfo.reversed_TMDS = 1;
					dbprintf("Radeon :: Reversed TMDS detected\n");
				}
			}
		}

		/*
		 * Probe primary head (DVI or laptop internal panel)
		 */
		if (rinfo.mon1_type == MT_NONE && rinfo.is_mobility &&
		    ((rinfo.bios_seg && (INREG(BIOS_4_SCRATCH) & 4))
		     || (INREG(LVDS_GEN_CNTL) & LVDS_ON))) {
			rinfo.mon1_type = MT_LCD;
			dbprintf("Radeon :: non-DDC laptop panel detected\n");
		}
		if (rinfo.mon1_type == MT_NONE)
			rinfo.mon1_type = CrtIsConnected(rinfo.reversed_DAC);

		/*
		 * Probe secondary head (mostly VGA, can be DVI)
		 */
		if (rinfo.mon2_type == MT_NONE)
			rinfo.mon2_type = CrtIsConnected(!rinfo.reversed_DAC);

		/*
		 * If we only detected port 2, we swap them, if none detected,
		 * assume CRT (maybe fallback to old BIOS_SCRATCH stuff ? or look
		 * at FP registers ?)
		 */
		if (rinfo.mon1_type == MT_NONE) {
			if (rinfo.mon2_type != MT_NONE) {
				rinfo.mon1_type = rinfo.mon2_type;
				rinfo.mon1_EDID = rinfo.mon2_EDID;
			} else
				rinfo.mon1_type = MT_CRT;
			rinfo.mon2_type = MT_NONE;
			rinfo.mon2_EDID = NULL;
		}

		/*
		 * Deal with reversed TMDS
		 */
		if (rinfo.reversed_TMDS) {
			/* Always keep internal TMDS as primary head */
			if (rinfo.mon1_type == MT_DFP || rinfo.mon2_type == MT_DFP) {
				int tmp_type = rinfo.mon1_type;
				uint8 *tmp_EDID = rinfo.mon1_EDID;
				rinfo.mon1_type = rinfo.mon2_type;
				rinfo.mon1_EDID = rinfo.mon2_EDID;
				rinfo.mon2_type = tmp_type;
				rinfo.mon2_EDID = tmp_EDID;
				if (rinfo.mon1_type == MT_CRT || rinfo.mon2_type == MT_CRT)
					rinfo.reversed_DAC ^= 1;
			}
		}
	}
	if (ignore_edid) {
		//if (rinfo.mon1_EDID)
			//kfree(rinfo.mon1_EDID);
		rinfo.mon1_EDID = NULL;
		//if (rinfo.mon2_EDID)
			//kfree(rinfo.mon2_EDID);
		rinfo.mon2_EDID = NULL;
	}

 bail:
	dbprintf("Radeon :: Monitor 1 type %s found\n",
	       GetMonName(rinfo.mon1_type));
	if (rinfo.mon1_EDID)
		dbprintf("Radeon :: EDID probed\n");
	if (!rinfo.has_CRTC2)
		return;
	dbprintf("Radeon :: Monitor 2 type %s found\n",
	       GetMonName(rinfo.mon2_type));
	if (rinfo.mon2_EDID)
		dbprintf("Radeon :: EDID probed\n");
}

/*
 * Fill up panel infos from a mode definition, either returned by the EDID
 * or from the default mode when we can't do any better
 */
void ATIRadeon::VarToPanelInfo(const struct VideoMode *var)
{
	rinfo.panel_info.xres = var->Width;
	rinfo.panel_info.yres = var->Height;
	rinfo.panel_info.clock = 100000000 / var->Clock;
	rinfo.panel_info.hOver_plus = var->right_margin;
	rinfo.panel_info.hSync_width = var->hsync_len;
       	rinfo.panel_info.hblank = var->left_margin +
		(var->right_margin + var->hsync_len);
	rinfo.panel_info.vOver_plus = var->lower_margin;
	rinfo.panel_info.vSync_width = var->vsync_len;
       	rinfo.panel_info.vblank = var->upper_margin +
		(var->lower_margin + var->vsync_len);
	rinfo.panel_info.hAct_high =
		(var->Flags & V_PHSYNC) != 0;
	rinfo.panel_info.vAct_high =
		(var->Flags & V_PVSYNC) != 0;
	rinfo.panel_info.valid = 1;
	/* We use a default of 200ms for the panel power delay, 
	 * I need to have a real schedule() instead of mdelay's in the panel code.
	 * we might be possible to figure out a better power delay either from
	 * MacOS OF tree or from the EDID block (proprietary extensions ?)
	 */
	rinfo.panel_info.pwr_delay = 200;
}

/*
 * Create mode list for Syllable (!! no checking of validaty !!)
 * Takes data from the initial DefaultVideoModes table
 * Experimental LCD mode checking for Mobility chipsets
 */
void ATIRadeon::CreateModeList()
{
	int maxWidth = 1600, maxHeight = 1200;
	int curWidth = 0, curHeight = 0;
	int rfCount = 4;
	float rf[] = { 60.0f, 75.0f, 85.0f, 100.0f };

	/*
	 * First check out what BIOS has to say
	 */
	if (rinfo.mon1_type == MT_LCD)
		GetPanelInfoBIOS();

	/*
	 * If we have some valid panel infos, we set some limits to the mode list
	 */
	if (rinfo.mon1_type != MT_CRT && rinfo.panel_info.valid)
	{
		maxWidth = rinfo.panel_info.xres;
		maxHeight = rinfo.panel_info.yres;
		rfCount = 2; /* 60Hz & 75Hz only */
		dbprintf("Radeon :: LCD panel %dx%d detected\n", (uint) maxWidth, (uint) maxHeight);
	}

	/*
	 * Populate screen mode list
	 */
	for( int j = 0; DefaultVideoModes[j].Width != -1; j++)
	{
		if (DefaultVideoModes[j].Width != curWidth ||
				DefaultVideoModes[j].Height != curHeight)
		{
			curWidth = DefaultVideoModes[j].Width;
			curHeight = DefaultVideoModes[j].Height;
			if (curWidth <= maxWidth && curHeight <= maxHeight)
			{
				for( int i = 0; i < rfCount; i++ ) {
					//m_cModeList.push_back (os::screen_mode (curWidth, curHeight, curWidth * 2, CS_RGB15, rf[i]));
					m_cModeList.push_back (os::screen_mode (curWidth, curHeight, curWidth * 2, CS_RGB16, rf[i]));
					m_cModeList.push_back (os::screen_mode (curWidth, curHeight, curWidth * 4, CS_RGB32, rf[i]));
				}
				RTRACE("Radeon :: Video mode %dx%d registered\n", (uint) curWidth, (uint) curHeight);
			}
		}
	}
}

/*
 * Blank screen per registers
 */
int ATIRadeon::ScreenBlank (int blank)
{
	uint32 val = INREG(CRTC_EXT_CNTL);
	int unblank = 0;
	
	val &= ~(CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS |
                 CRTC_VSYNC_DIS);
	switch (blank) {
		case VESA_NO_BLANKING:
			unblank = 1;
			break;
		case VESA_VSYNC_SUSPEND:
			val |= (CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS);
			break;
		case VESA_HSYNC_SUSPEND:
			val |= (CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS);
			break;
		case VESA_POWERDOWN:
			val |= (CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS | 
				CRTC_HSYNC_DIS);
			break;
	}
	OUTREG(CRTC_EXT_CNTL, val);
	
	switch(rinfo.mon1_type)
	{
		case MT_DFP:
			if (unblank)
				OUTREGP(FP_GEN_CNTL, (FP_FPON | FP_TMDS_EN),
					~(FP_FPON | FP_TMDS_EN));
			break;
		case MT_LCD:
			val = INREG(LVDS_GEN_CNTL);
			if (unblank) {
				uint32 target_val = (val & ~LVDS_DISPLAY_DIS) | LVDS_BLON | LVDS_ON
				| LVDS_EN | (rinfo.init_state.lvds_gen_cntl
					     & (LVDS_DIGON | LVDS_BL_MOD_EN));
				if ((val ^ target_val) == LVDS_DISPLAY_DIS)
					OUTREG(LVDS_GEN_CNTL, target_val);
				else if ((val ^ target_val) != 0) {
					OUTREG(LVDS_GEN_CNTL, target_val
				       & ~(LVDS_ON | LVDS_BL_MOD_EN));
					rinfo.init_state.lvds_gen_cntl &= ~LVDS_STATE_MASK;
					rinfo.init_state.lvds_gen_cntl |= target_val & LVDS_STATE_MASK;
				
					snooze(rinfo.panel_info.pwr_delay*1000);
					OUTREG(LVDS_GEN_CNTL, target_val);
				}
			} else {
				val |= LVDS_DISPLAY_DIS;
				OUTREG(LVDS_GEN_CNTL, val);
			}
			break;
	}

	return 0;
}

