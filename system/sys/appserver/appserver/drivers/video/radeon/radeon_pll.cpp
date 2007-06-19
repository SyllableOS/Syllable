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

/*
 * Read PLL infos from chip registers - disabled for now -MK
 */
int ATIRadeon::ProbePLLParams()
{
#if 0
	unsigned char ppll_div_sel;
	unsigned Ns, Nm, M;
	unsigned sclk, mclk, tmp, ref_div;
	int hTotal, vTotal, num, denom, m, n;
	unsigned long long hz, vclk;
	long xtal;
	struct timeval start_tv, stop_tv;
	long total_secs, total_usecs;
	int i;

	/* Ugh, we cut interrupts, bad bad bad, but we want some precision
	 * here, so... --BenH
	 */

	/* Flush PCI buffers ? */
	tmp = INREG(DEVICE_ID);

	//local_irq_disable();

	for(i=0; i<1000000; i++)
		if (((INREG(CRTC_VLINE_CRNT_VLINE) >> 16) & 0x3ff) == 0)
			break;

	do_gettimeofday(&start_tv);

	for(i=0; i<1000000; i++)
		if (((INREG(CRTC_VLINE_CRNT_VLINE) >> 16) & 0x3ff) != 0)
			break;

	for(i=0; i<1000000; i++)
		if (((INREG(CRTC_VLINE_CRNT_VLINE) >> 16) & 0x3ff) == 0)
			break;
	
	do_gettimeofday(&stop_tv);
	
	//local_irq_enable();

	total_secs = stop_tv.tv_sec - start_tv.tv_sec;
	if (total_secs > 10)
		return -1;
	total_usecs = stop_tv.tv_usec - start_tv.tv_usec;
	total_usecs += total_secs * 1000000;
	if (total_usecs < 0)
		total_usecs = -total_usecs;
	hz = 1000000/total_usecs;
 
	hTotal = ((INREG(CRTC_H_TOTAL_DISP) & 0x1ff) + 1) * 8;
	vTotal = ((INREG(CRTC_V_TOTAL_DISP) & 0x3ff) + 1);
	vclk = (long long)hTotal * (long long)vTotal * hz;

	switch((INPLL(PPLL_REF_DIV) & 0x30000) >> 16) {
	case 0:
	default:
		num = 1;
		denom = 1;
		break;
	case 1:
		n = ((INPLL(X_MPLL_REF_FB_DIV) >> 16) & 0xff);
		m = (INPLL(X_MPLL_REF_FB_DIV) & 0xff);
		num = 2*n;
		denom = 2*m;
		break;
	case 2:
		n = ((INPLL(X_MPLL_REF_FB_DIV) >> 8) & 0xff);
		m = (INPLL(X_MPLL_REF_FB_DIV) & 0xff);
		num = 2*n;
		denom = 2*m;
        break;
	}

	OUTREG8(CLOCK_CNTL_INDEX, 1);
	ppll_div_sel = INREG8(CLOCK_CNTL_DATA + 1) & 0x3;

	n = (INPLL(PPLL_DIV_0 + ppll_div_sel) & 0x7ff);
	m = (INPLL(PPLL_REF_DIV) & 0x3ff);

	num *= n;
	denom *= m;

	switch ((INPLL(PPLL_DIV_0 + ppll_div_sel) >> 16) & 0x7) {
	case 1:
		denom *= 2;
		break;
	case 2:
		denom *= 4;
		break;
	case 3:
		denom *= 8;
		break;
	case 4:
		denom *= 3;
		break;
	case 6:
		denom *= 6;   
		break;
	case 7:
		denom *= 12;
		break;
	}

	do_div(vclk, 1000);
	xtal = (xtal * denom) / num;

	if ((xtal > 26900) && (xtal < 27100))
		xtal = 2700;
	else if ((xtal > 14200) && (xtal < 14400))
		xtal = 1432;
	else if ((xtal > 29400) && (xtal < 29600))
		xtal = 2950;
	else {
		dbprintf("Radeon :: Warning: xtal calculation failed: %ld\n", xtal);
		return -1;
	}

	tmp = INPLL(X_MPLL_REF_FB_DIV);
	ref_div = INPLL(PPLL_REF_DIV) & 0x3ff;

	Ns = (tmp & 0xff0000) >> 16;
	Nm = (tmp & 0xff00) >> 8;
	M = (tmp & 0xff);
	sclk = round_div((2 * Ns * xtal), (2 * M));
	mclk = round_div((2 * Nm * xtal), (2 * M));

	/* we're done, hopefully these are sane values */
	rinfo.pll.ref_clk = xtal;
	rinfo.pll.ref_div = ref_div;
	rinfo.pll.sclk = sclk;
	rinfo.pll.mclk = mclk;

	return 0;
#else
	return -1;
#endif
}


/*
 * Retreive PLL infos by different means (BIOS, Open Firmware, register probing...)
 */
void ATIRadeon::GetPLLInfo()
{
	/* In the case nothing works, these are defaults; they are mostly
	 * incomplete, however.  It does provide ppll_max and _min values
	 * even for most other methods, however.
	 */
	switch (rinfo.chipset) {
	case PCI_DEVICE_ID_ATI_RADEON_QW:
	case PCI_DEVICE_ID_ATI_RADEON_QX:
		rinfo.pll.ppll_max = 35000;
		rinfo.pll.ppll_min = 12000;
		rinfo.pll.mclk = 23000;
		rinfo.pll.sclk = 23000;
		rinfo.pll.ref_clk = 2700;
		break;
	case PCI_DEVICE_ID_ATI_RADEON_QL:
	case PCI_DEVICE_ID_ATI_RADEON_QN:
	case PCI_DEVICE_ID_ATI_RADEON_QO:
	case PCI_DEVICE_ID_ATI_RADEON_Ql:
	case PCI_DEVICE_ID_ATI_RADEON_BB:
		rinfo.pll.ppll_max = 35000;
		rinfo.pll.ppll_min = 12000;
		rinfo.pll.mclk = 27500;
		rinfo.pll.sclk = 27500;
		rinfo.pll.ref_clk = 2700;
		break;
	case PCI_DEVICE_ID_ATI_RADEON_Id:
	case PCI_DEVICE_ID_ATI_RADEON_Ie:
	case PCI_DEVICE_ID_ATI_RADEON_If:
	case PCI_DEVICE_ID_ATI_RADEON_Ig:
		rinfo.pll.ppll_max = 35000;
		rinfo.pll.ppll_min = 12000;
		rinfo.pll.mclk = 25000;
		rinfo.pll.sclk = 25000;
		rinfo.pll.ref_clk = 2700;
		break;
	case PCI_DEVICE_ID_ATI_RADEON_ND:
	case PCI_DEVICE_ID_ATI_RADEON_NE:
	case PCI_DEVICE_ID_ATI_RADEON_NF:
	case PCI_DEVICE_ID_ATI_RADEON_NG:
		rinfo.pll.ppll_max = 40000;
		rinfo.pll.ppll_min = 20000;
		rinfo.pll.mclk = 27000;
		rinfo.pll.sclk = 27000;
		rinfo.pll.ref_clk = 2700;
		break;
	case PCI_DEVICE_ID_ATI_RADEON_QD:
	case PCI_DEVICE_ID_ATI_RADEON_QE:
	case PCI_DEVICE_ID_ATI_RADEON_QF:
	case PCI_DEVICE_ID_ATI_RADEON_QG:
	default:
		rinfo.pll.ppll_max = 35000;
		rinfo.pll.ppll_min = 12000;
		rinfo.pll.mclk = 16600;
		rinfo.pll.sclk = 16600;
		rinfo.pll.ref_clk = 2700;
		break;
	}
	rinfo.pll.ref_div = INPLL(PPLL_REF_DIV) & PPLL_REF_DIV_MASK;

	
	/*
	 * Check out if we have an X86 which gave us some PLL informations
	 * and if yes, retreive them
	 */
	if (rinfo.bios_seg) {
		if (rinfo.bios_type == bios_atom) {
		    uint16 pll_info_block = BIOS_IN16(rinfo.fp_atom_bios_start + 12);

		    rinfo.pll.ref_clk = BIOS_IN16(pll_info_block + 82);
#if 0
		    rinfo.pll.ref_div = 0; /* Need to derive from existing setting
									  or use a new algorithm to calculate
									  from min_input and max_input */
#else
			rinfo.pll.ref_div = INPLL(PPLL_REF_DIV) & 0x3f;
#endif
		    rinfo.pll.ppll_min = BIOS_IN16(pll_info_block + 78);
		    rinfo.pll.ppll_max = BIOS_IN32(pll_info_block + 32);

			/* XXXKV: The X driver divides both the sclk & mclk values by 100.0,
			   but the legacy BIOS stuff below doesn't do that, so I'm not doing it here either */
		    rinfo.pll.sclk = BIOS_IN32(pll_info_block + 0x08);
		    rinfo.pll.mclk = BIOS_IN32(pll_info_block + 0x0c);
		    if (rinfo.pll.sclk == 0)
				rinfo.pll.sclk = 20000;
		    if (rinfo.pll.mclk == 0)
				rinfo.pll.mclk = 20000;
		
			dbprintf("Radeon :: Retrieved PLL infos from ATOM BIOS\n");
			goto found;
		} else {
			uint16 pll_info_block = BIOS_IN16(rinfo.fp_bios_start + 0x30);

			rinfo.pll.sclk		= BIOS_IN16(pll_info_block + 0x08);
			rinfo.pll.mclk		= BIOS_IN16(pll_info_block + 0x0a);
			rinfo.pll.ref_clk	= BIOS_IN16(pll_info_block + 0x0e);
			rinfo.pll.ref_div	= BIOS_IN16(pll_info_block + 0x10);
			rinfo.pll.ppll_min	= BIOS_IN32(pll_info_block + 0x12);
			rinfo.pll.ppll_max	= BIOS_IN32(pll_info_block + 0x16);

			dbprintf("Radeon :: Retrieved PLL infos from legacy BIOS\n");
			goto found;
		}
	}

	/*
	 * We didn't get PLL parameters from either OF or BIOS, we try to
	 * probe them
	 */
	if (ProbePLLParams() == 0) {
		dbprintf("Radeon :: Retreived PLL infos from registers\n");
		/* FIXME: Max clock may be higher on newer chips */
       		rinfo.pll.ppll_min = 12000;
       		rinfo.pll.ppll_max = 35000;
		goto found;
	}

	dbprintf("Radeon :: Used default PLL infos\n");

found:
	/*
	 * Some methods fail to retreive SCLK and MCLK values, we apply default
	 * settings in this case (200Mhz). If that really happens often, we could
	 * fetch from registers instead...
	 */
	if (rinfo.pll.mclk == 0)
		rinfo.pll.mclk = 20000;
	if (rinfo.pll.sclk == 0)
		rinfo.pll.sclk = 20000;

	dbprintf("Radeon :: Reference=%d.%02d MHz (RefDiv=%d) Memory=%d.%02d Mhz, System=%d.%02d MHz\n",
	       rinfo.pll.ref_clk / 100, rinfo.pll.ref_clk % 100,
	       rinfo.pll.ref_div,
	       rinfo.pll.mclk / 100, rinfo.pll.mclk % 100,
	       rinfo.pll.sclk / 100, rinfo.pll.sclk % 100);
	dbprintf("radeonfb: PLL min %d max %d\n", rinfo.pll.ppll_min, rinfo.pll.ppll_max);
}

/*
 * Calculate the PLL values for a given mode
 */
void ATIRadeon::CalcPLLRegs(struct radeon_regs *regs, unsigned long freq)
{
	const struct {
		int divider;
		int bitvalue;
	} *post_div,
	  post_divs[] = {
		{ 1,  0 },
		{ 2,  1 },
		{ 4,  2 },
		{ 8,  3 },
		{ 3,  4 },
		{ 16, 5 },
		{ 6,  6 },
		{ 12, 7 },
		{ 0,  0 },
	};
	int fb_div, pll_output_freq = 0;
	int uses_dvo = 0;

	/* Check if the DVO port is enabled and sourced from the primary CRTC. I'm
	 * not sure which model starts having FP2_GEN_CNTL, I assume anything more
	 * recent than an r(v)100...
	 */
	while (rinfo.has_CRTC2) {
		uint32 fp2_gen_cntl = INREG(FP2_GEN_CNTL);
		uint32 disp_output_cntl;
		int source;

		/* FP2 path not enabled */
		if ((fp2_gen_cntl & FP2_ON) == 0)
			break;
		/* Not all chip revs have the same format for this register,
		 * extract the source selection
		 */
		if (rinfo.family == CHIP_FAMILY_R200 ||
		    rinfo.family == CHIP_FAMILY_R300 ||
		    rinfo.family == CHIP_FAMILY_R350 ||
		    rinfo.family == CHIP_FAMILY_RV350) {
			source = (fp2_gen_cntl >> 10) & 0x3;
			/* sourced from transform unit, check for transform unit
			 * own source
			 */
			if (source == 3) {
				disp_output_cntl = INREG(DISP_OUTPUT_CNTL);
				source = (disp_output_cntl >> 12) & 0x3;
			}
		} else
			source = (fp2_gen_cntl >> 13) & 0x1;
		/* sourced from CRTC2 -> exit */
		if (source == 1)
			break;

		/* so we end up on CRTC1, let's set uses_dvo to 1 now */
		uses_dvo = 1;
		break;
	}
	if (freq > (unsigned long)rinfo.pll.ppll_max)
		freq = rinfo.pll.ppll_max;
	if (freq*12 < (unsigned long)rinfo.pll.ppll_min)
		freq = rinfo.pll.ppll_min / 12;

	for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
		pll_output_freq = post_div->divider * freq;
		/* If we output to the DVO port (external TMDS), we don't allow an
		 * odd PLL divider as those aren't supported on this path
		 */
		if (uses_dvo && (post_div->divider & 1))
			continue;
		if (pll_output_freq >= rinfo.pll.ppll_min  &&
		    pll_output_freq <= rinfo.pll.ppll_max)
			break;
	}
	
	/* If we fall through the bottom, try the "default value"
	   given by the terminal post_div->bitvalue */
	if ( !post_div->divider ) {
		post_div = &post_divs[post_div->bitvalue];
		pll_output_freq = post_div->divider * freq;
	}
	RTRACE("ref_div = %d, ref_clk = %d, output_freq = %d\n",
	       rinfo.pll.ref_div, rinfo.pll.ref_clk,
	       pll_output_freq);
	


	fb_div = round_div(rinfo.pll.ref_div*pll_output_freq,
				  rinfo.pll.ref_clk);
	regs->ppll_ref_div = rinfo.pll.ref_div;
	regs->ppll_div_3 = fb_div | (post_div->bitvalue << 16);

	RTRACE("post div = 0x%x\n", post_div->bitvalue);
	RTRACE("fb_div = 0x%x\n", fb_div);
	RTRACE("ppll_div_3 = 0x%x\n", (uint)regs->ppll_div_3);
}

void ATIRadeon::SaveState(struct radeon_regs *save)
{
	/* CRTC regs */
	save->crtc_gen_cntl = INREG(CRTC_GEN_CNTL);
	save->crtc_ext_cntl = INREG(CRTC_EXT_CNTL);
	save->crtc_more_cntl = INREG(CRTC_MORE_CNTL);
	save->dac_cntl = INREG(DAC_CNTL);
        save->crtc_h_total_disp = INREG(CRTC_H_TOTAL_DISP);
        save->crtc_h_sync_strt_wid = INREG(CRTC_H_SYNC_STRT_WID);
        save->crtc_v_total_disp = INREG(CRTC_V_TOTAL_DISP);
        save->crtc_v_sync_strt_wid = INREG(CRTC_V_SYNC_STRT_WID);
	save->crtc_pitch = INREG(CRTC_PITCH);
	save->surface_cntl = INREG(SURFACE_CNTL);

	/* FP regs */
	save->fp_crtc_h_total_disp = INREG(FP_CRTC_H_TOTAL_DISP);
	save->fp_crtc_v_total_disp = INREG(FP_CRTC_V_TOTAL_DISP);
	save->fp_gen_cntl = INREG(FP_GEN_CNTL);
	save->fp_h_sync_strt_wid = INREG(FP_H_SYNC_STRT_WID);
	save->fp_horz_stretch = INREG(FP_HORZ_STRETCH);
	save->fp_v_sync_strt_wid = INREG(FP_V_SYNC_STRT_WID);
	save->fp_vert_stretch = INREG(FP_VERT_STRETCH);
	save->lvds_gen_cntl = INREG(LVDS_GEN_CNTL);
	save->lvds_pll_cntl = INREG(LVDS_PLL_CNTL);
	save->tmds_crc = INREG(TMDS_CRC);	save->tmds_transmitter_cntl = INREG(TMDS_TRANSMITTER_CNTL);
	save->vclk_ecp_cntl = INPLL(VCLK_ECP_CNTL);

	/* PLL regs */
	save->clk_cntl_index = INREG(CLOCK_CNTL_INDEX) & ~0x3f;
	radeon_pll_errata_after_index();
	save->ppll_div_3 = INPLL(PPLL_DIV_3);
	save->ppll_ref_div = INPLL(PPLL_REF_DIV);	
}


void ATIRadeon::WritePLLRegs(struct radeon_regs *mode)
{
	int i;
	
	FifoWait(20);

	/* Workaround from XFree */
	if (rinfo.is_mobility) {
	        /* A temporal workaround for the occational blanking on certain laptop panels. 
	           This appears to related to the PLL divider registers (fail to lock?).  
		   It occurs even when all dividers are the same with their old settings.  
	           In this case we really don't need to fiddle with PLL registers. 
	           By doing this we can avoid the blanking problem with some panels.
	        */
		if ((mode->ppll_ref_div == (INPLL(PPLL_REF_DIV) & PPLL_REF_DIV_MASK)) &&
		    (mode->ppll_div_3 == (INPLL(PPLL_DIV_3) &
					  (PPLL_POST3_DIV_MASK | PPLL_FB3_DIV_MASK)))) {
			/* We still have to force a switch to PPLL div 3 thanks to
			 * an XFree86 driver bug which will switch it away in some cases
			 * even when using UseFDev */
			RTRACE("Radeon :: LVDS PLL workaround enabled\n");
			OUTREGP(CLOCK_CNTL_INDEX,
				mode->clk_cntl_index & PPLL_DIV_SEL_MASK,
				~PPLL_DIV_SEL_MASK);
			radeon_pll_errata_after_index();
			radeon_pll_errata_after_data();

       		return;
		}
	}

	/* Swich VCKL clock input to CPUCLK so it stays fed while PPLL updates*/
	OUTPLLP(VCLK_ECP_CNTL, VCLK_SRC_SEL_CPUCLK, ~VCLK_SRC_SEL_MASK);

	/* Reset PPLL & enable atomic update */
	OUTPLLP(PPLL_CNTL,
		PPLL_RESET | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN,
		~(PPLL_RESET | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN));

	/* Switch to PPLL div 3 */
	OUTREGP(CLOCK_CNTL_INDEX,
		mode->clk_cntl_index & PPLL_DIV_SEL_MASK,
		~PPLL_DIV_SEL_MASK);
	radeon_pll_errata_after_index();
	radeon_pll_errata_after_data();
	

	/* Set PPLL ref. div */
	if (rinfo.family == CHIP_FAMILY_R300 ||
	    rinfo.family == CHIP_FAMILY_RS300 ||
	    rinfo.family == CHIP_FAMILY_R350 ||
	    rinfo.family == CHIP_FAMILY_RV350) {
		if (mode->ppll_ref_div & R300_PPLL_REF_DIV_ACC_MASK) {
			/* When restoring console mode, use saved PPLL_REF_DIV
			 * setting.
			 */
			OUTPLLP(PPLL_REF_DIV, mode->ppll_ref_div, 0);
		} else {
			/* R300 uses ref_div_acc field as real ref divider */
			OUTPLLP(PPLL_REF_DIV,
				(mode->ppll_ref_div << R300_PPLL_REF_DIV_ACC_SHIFT), 
				~R300_PPLL_REF_DIV_ACC_MASK);
		}
	} else
		OUTPLLP(PPLL_REF_DIV, mode->ppll_ref_div, ~PPLL_REF_DIV_MASK);

	/* Set PPLL divider 3 & post divider*/
	OUTPLLP(PPLL_DIV_3, mode->ppll_div_3, ~PPLL_FB3_DIV_MASK);
	OUTPLLP(PPLL_DIV_3, mode->ppll_div_3, ~PPLL_POST3_DIV_MASK);

	/* Write update */
	while (INPLL(PPLL_REF_DIV) & PPLL_ATOMIC_UPDATE_R)
		;
	OUTPLLP(PPLL_REF_DIV, PPLL_ATOMIC_UPDATE_W, ~PPLL_ATOMIC_UPDATE_W);

	/* Wait read update complete */
	/* FIXME: Certain revisions of R300 can't recover here.  Not sure of
	   the cause yet, but this workaround will mask the problem for now.
	   Other chips usually will pass at the very first test, so the
	   workaround shouldn't have any effect on them. */
	for (i = 0; (i < 10000 && INPLL(PPLL_REF_DIV) & PPLL_ATOMIC_UPDATE_R); i++)
		;
	
	OUTPLL(HTOTAL_CNTL, 0);

	/* Clear reset & atomic update */
	OUTPLLP(PPLL_CNTL, 0,
		~(PPLL_RESET | PPLL_SLEEP | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN));

	/* We may want some locking ... oh well */
	snooze(5000);

	/* Switch back VCLK source to PPLL */
	OUTPLLP(VCLK_ECP_CNTL, VCLK_SRC_SEL_PPLLCLK, ~VCLK_SRC_SEL_MASK);
}


/*
 * Apply a video mode. This will apply the whole register set, including
 * the PLL registers, to the card
 */
void ATIRadeon::WriteMode (struct radeon_regs *mode)
{
	int i;
	int primary_mon = PRIMARY_MONITOR(rinfo);

	ScreenBlank(VESA_POWERDOWN);

	FifoWait(31);
	for (i=0; i<10; i++)
		OUTREG(common_regs[i].reg, common_regs[i].val);

	/* Apply surface registers */
	for (i=0; i<8; i++) {
		OUTREG(SURFACE0_LOWER_BOUND + 0x10*i, mode->surf_lower_bound[i]);
		OUTREG(SURFACE0_UPPER_BOUND + 0x10*i, mode->surf_upper_bound[i]);
		OUTREG(SURFACE0_INFO + 0x10*i, mode->surf_info[i]);
	}

	OUTREG(CRTC_GEN_CNTL, mode->crtc_gen_cntl);
	OUTREGP(CRTC_EXT_CNTL, mode->crtc_ext_cntl,
		~(CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_DISPLAY_DIS));
	OUTREG(CRTC_MORE_CNTL, mode->crtc_more_cntl);
	OUTREGP(DAC_CNTL, mode->dac_cntl, DAC_RANGE_CNTL | DAC_BLANKING);
	OUTREG(CRTC_H_TOTAL_DISP, mode->crtc_h_total_disp);
	OUTREG(CRTC_H_SYNC_STRT_WID, mode->crtc_h_sync_strt_wid);
	OUTREG(CRTC_V_TOTAL_DISP, mode->crtc_v_total_disp);
	OUTREG(CRTC_V_SYNC_STRT_WID, mode->crtc_v_sync_strt_wid);
	OUTREG(CRTC_OFFSET, 0);
	OUTREG(CRTC_OFFSET_CNTL, 0);
	OUTREG(CRTC_PITCH, mode->crtc_pitch);
	OUTREG(SURFACE_CNTL, mode->surface_cntl);

	WritePLLRegs(mode);

	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD)) {
		OUTREG(FP_CRTC_H_TOTAL_DISP, mode->fp_crtc_h_total_disp);
		OUTREG(FP_CRTC_V_TOTAL_DISP, mode->fp_crtc_v_total_disp);
		OUTREG(FP_H_SYNC_STRT_WID, mode->fp_h_sync_strt_wid);
		OUTREG(FP_V_SYNC_STRT_WID, mode->fp_v_sync_strt_wid);
		OUTREG(FP_HORZ_STRETCH, mode->fp_horz_stretch);
		OUTREG(FP_VERT_STRETCH, mode->fp_vert_stretch);
		OUTREG(FP_GEN_CNTL, mode->fp_gen_cntl);
		OUTREG(TMDS_CRC, mode->tmds_crc);
		OUTREG(TMDS_TRANSMITTER_CNTL, mode->tmds_transmitter_cntl);
	}

	//RTRACE("lvds_gen_cntl: %08x\n", (uint)INREG(LVDS_GEN_CNTL));

	ScreenBlank(VESA_NO_BLANKING);

	FifoWait(2);
	OUTPLL(VCLK_ECP_CNTL, mode->vclk_ecp_cntl);
	
	return;
}

