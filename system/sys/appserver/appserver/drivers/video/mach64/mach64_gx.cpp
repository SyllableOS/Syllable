#include "mach64.h"


/*
 *  ATI Mach64 GX Support
 */

/* Definitions for the ICS 2595 == ATI 18818_1 Clockchip */

#define REF_FREQ_2595       1432 	 /*  14.33 MHz  (exact   14.31818) */
#define REF_DIV_2595          46 		 /* really 43 on ICS 2595 !!!  */
/* ohne Prescaler */
#define MAX_FREQ_2595      15938 	 /* 159.38 MHz  (really 170.486) */
#define MIN_FREQ_2595       8000 	 /*  80.00 MHz  (        85.565) */
 /* mit Prescaler 2, 4, 8 */
#define ABS_MIN_FREQ_2595   1000 	 /*  10.00 MHz  (really  10.697) */
#define N_ADJ_2595           257

#define STOP_BITS_2595     0x1800


#define MIN_N_408		2

#define MIN_N_1703		6

#define MIN_M		2
#define MAX_M		30
#define MIN_N		35
#define MAX_N		255-8

void ATImach64::aty_dac_waste4()
{
	(void)aty_ld_8(DAC_REGS);

	(void)aty_ld_8(DAC_REGS + 2);
	(void)aty_ld_8(DAC_REGS + 2);
	(void)aty_ld_8(DAC_REGS + 2);
	(void)aty_ld_8(DAC_REGS + 2);
}
//-------------------------------------------------------------------
void ATImach64::aty_StrobeClock()
{
	uint8 tmp;

	mach64_delay(26);

	tmp = aty_ld_8(CLOCK_CNTL);
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, tmp | CLOCK_STROBE);

	return;
}


    /*
     *  IBM RGB514 DAC and Clock Chip
     */

void ATImach64::aty_st_514(int offset, uint8 val)
{
	aty_st_8(DAC_CNTL, 1);
	/* right addr byte */
	aty_st_8(DAC_W_INDEX, offset & 0xff);
	/* left addr byte */
	aty_st_8(DAC_DATA, (offset >> 8) & 0xff);
	aty_st_8(DAC_MASK, val);
	aty_st_8(DAC_CNTL, 0);
}
//-------------------------------------------------------------------
int ATImach64::aty_set_dac_514(const union ati_pll *pll, uint32 bpp,
								uint32 accel)
{
	static struct {
		uint8 pixel_dly;
		uint8 misc2_cntl;
		uint8 pixel_rep;
		uint8 pixel_cntl_index;
		uint8 pixel_cntl_v1;
	} tab[3] = {
		{ 0, 0x41, 0x03, 0x71, 0x45 },	/* 8 bpp */
		{ 0, 0x45, 0x04, 0x0c, 0x01 },	/* 555 */
		{ 0, 0x45, 0x06, 0x0e, 0x00 },	/* XRGB */
	};
	int i;

	switch (bpp) {
		case 8:
		default:
			i = 0;
			break;
		case 16:
			i = 1;
			break;
		case 32:
			i = 2;
			break;
	}
	aty_st_514(0x90, 0x00);		/* VRAM Mask Low */
	aty_st_514(0x04, tab[i].pixel_dly);	/* Horizontal Sync Control */
	aty_st_514(0x05, 0x00);		/* Power Management */
	aty_st_514(0x02, 0x01);		/* Misc Clock Control */
	aty_st_514(0x71, tab[i].misc2_cntl);	/* Misc Control 2 */
	aty_st_514(0x0a, tab[i].pixel_rep);	/* Pixel Format */
	aty_st_514(tab[i].pixel_cntl_index, tab[i].pixel_cntl_v1);
			/* Misc Control 2 / 16 BPP Control / 32 BPP Control */
	return 0;
}
//-------------------------------------------------------------------
int ATImach64::aty_var_to_pll_514(uint32 vclk_per, uint8 bpp, union ati_pll *pll)
{
	/*
	*  FIXME: use real calculations instead of using fixed values from the old
	*	       driver
	*/
	static struct {
		uint32 limit;	/* pixlock rounding limit (arbitrary) */
		uint8 m;		/* (df<<6) | vco_div_count */
		uint8 n;		/* ref_div_count */
	} RGB514_clocks[7] = {
		{  8000, (3<<6) | 20, 9 },	/*  7395 ps / 135.2273 MHz */
		{ 10000, (1<<6) | 19, 3 },	/*  9977 ps / 100.2273 MHz */
		{ 13000, (1<<6) |  2, 3 },	/* 12509 ps /  79.9432 MHz */
		{ 14000, (2<<6) |  8, 7 },	/* 13394 ps /  74.6591 MHz */
		{ 16000, (1<<6) | 44, 6 },	/* 15378 ps /  65.0284 MHz */
		{ 25000, (1<<6) | 15, 5 },	/* 17460 ps /  57.2727 MHz */
		{ 50000, (0<<6) | 53, 7 },	/* 33145 ps /  30.1705 MHz */
	};
	uint i;

	for (i = 0; i < sizeof(RGB514_clocks)/sizeof(*RGB514_clocks); i++)
	if (vclk_per <= RGB514_clocks[i].limit) {
		pll->ibm514.m = RGB514_clocks[i].m;
		pll->ibm514.n = RGB514_clocks[i].n;
		return 0;
	}
	return -1;
}
//-------------------------------------------------------------------
void ATImach64::aty_set_pll_514(const union ati_pll *pll)
{
	aty_st_514(0x06, 0x02);	/* DAC Operation */
	aty_st_514(0x10, 0x01);	/* PLL Control 1 */
	aty_st_514(0x70, 0x01);	/* Misc Control 1 */
	aty_st_514(0x8f, 0x1f);	/* PLL Ref. Divider Input */
	aty_st_514(0x03, 0x00);	/* Sync Control */
	aty_st_514(0x05, 0x00);	/* Power Management */
	aty_st_514(0x20, pll->ibm514.m);	/* F0 / M0 */
	aty_st_514(0x21, pll->ibm514.n);	/* F1 / N0 */
}


    /*
     *  ATI 68860-B DAC
     */

int ATImach64::aty_set_dac_ATI68860_B(const union ati_pll *pll, uint32 bpp, uint32 AccelMode)
{
	uint32 gModeReg, devSetupRegA, temp, mask;

 	gModeReg = 0;
	devSetupRegA = 0;

	switch (bpp) {
		case 8:
			gModeReg = 0x83;
			devSetupRegA = 0x60 | 0x00 /*(info->mach64DAC8Bit ? 0x00 : 0x01) */;
			break;
		case 15:
			gModeReg = 0xA0;
			devSetupRegA = 0x60;
			break;
		case 16:
			gModeReg = 0xA1;
			devSetupRegA = 0x60;
			break;
		case 24:
			gModeReg = 0xC0;
			devSetupRegA = 0x60;
			break;
		case 32:
		gModeReg = 0xE3;
		devSetupRegA = 0x60;
		break;
	}

	temp = aty_ld_8(DAC_CNTL);
	aty_st_8(DAC_CNTL, (temp & ~DAC_EXT_SEL_RS2) | DAC_EXT_SEL_RS3);

	aty_st_8(DAC_REGS + 2, 0x1D);
	aty_st_8(DAC_REGS + 3, gModeReg);
	aty_st_8(DAC_REGS, 0x02);

	temp = aty_ld_8(DAC_CNTL);
	aty_st_8(DAC_CNTL, temp | DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3);

	if (info.total_vram < MEM_SIZE_1M)
		mask = 0x04;
	else if (info.total_vram == MEM_SIZE_1M)
		mask = 0x08;
	else
		mask = 0x0C;

	 /* The following assumes that the BIOS has correctly set R7 of the
	* Device Setup Register A at boot time.
	*/
#define A860_DELAY_L	0x80

	temp = aty_ld_8(DAC_REGS);
	aty_st_8(DAC_REGS, (devSetupRegA | mask) | (temp & A860_DELAY_L));
	temp = aty_ld_8(DAC_CNTL);
	aty_st_8(DAC_CNTL, (temp & ~(DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3)));

	aty_st_le32(BUS_CNTL, 0x890e20f1);
	aty_st_le32(DAC_CNTL, 0x47052100);

	return 0;
}

    /*
     *  AT&T 21C498 DAC
     */

int ATImach64::aty_set_dac_ATT21C498(const union ati_pll *pll, uint32 bpp, uint32 accel)
{
	uint32 dotClock;
	int muxmode = 0;
	int DACMask = 0;

	dotClock = 100000000 / pll->ics2595.period_in_ps;

	switch (bpp) {
		case 8:
			if (dotClock > 8000) {
				DACMask = 0x24;
				muxmode = 1;
			} else
				DACMask = 0x04;
			 break;
		case 15:
			DACMask = 0x16;
			break;
		case 16:
			DACMask = 0x36;
			break;
		case 24:
			DACMask = 0xE6;
			break;
		case 32:
	 	 	DACMask = 0xE6;
	 		break;
	}

	if (1 /* info->mach64DAC8Bit */)
		DACMask |= 0x02;

	aty_dac_waste4();
	aty_st_8(DAC_REGS + 2, DACMask);

	aty_st_le32(BUS_CNTL, 0x890e20f1);
	aty_st_le32(DAC_CNTL, 0x00072000);

	return muxmode;
}


    /*
     *  ATI 18818 / ICS 2595 Clock Chip
     */

int ATImach64::aty_var_to_pll_18818(uint32 vclk_per, uint8 bpp, union ati_pll *pll)
{
	uint32 MHz100;		/* in 0.01 MHz */
	int32 program_bits;
	uint32 post_divider;

	/* Calculate the programming word */
	MHz100 = 100000000 / vclk_per;

	program_bits = -1;
	post_divider = 1;

	if (MHz100 > MAX_FREQ_2595) {
		MHz100 = MAX_FREQ_2595;
		return -1;
	} else if (MHz100 < ABS_MIN_FREQ_2595) {
		program_bits = 0;	/* MHz100 = 257 */
		return -1;
	} else {
		while (MHz100 < MIN_FREQ_2595) {
			MHz100 *= 2;
			post_divider *= 2;
		}
	}
	MHz100 *= 1000;
	MHz100 = (REF_DIV_2595 * MHz100) / REF_FREQ_2595;

	MHz100 += 500;    /* + 0.5 round */
	MHz100 /= 1000;

	if (program_bits == -1) {
		program_bits = MHz100 - N_ADJ_2595;
		switch (post_divider) {
			case 1:
				program_bits |= 0x0600;
				break;
			case 2:
				program_bits |= 0x0400;
				break;
			case 4:
				program_bits |= 0x0200;
				break;
			case 8:
			default:
				break;
		}
	}

	program_bits |= STOP_BITS_2595;

	pll->ics2595.program_bits = program_bits;
	pll->ics2595.locationAddr = 0;
	pll->ics2595.post_divider = post_divider;
	pll->ics2595.period_in_ps = vclk_per;

	return 0;
}
//-------------------------------------------------------------------
void ATImach64::aty_ICS2595_put1bit(uint8 data)
{
	uint8 tmp;

	data &= 0x01;
	tmp = aty_ld_8(CLOCK_CNTL);
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, (tmp & ~0x04) | (data << 2));

	tmp = aty_ld_8(CLOCK_CNTL);
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, (tmp & ~0x08) | (0 << 3));

	aty_StrobeClock();

	tmp = aty_ld_8(CLOCK_CNTL);
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, (tmp & ~0x08) | (1 << 3));

	aty_StrobeClock();

	return;
}
//-------------------------------------------------------------------
void ATImach64::aty_set_pll18818(const union ati_pll *pll)
{
	uint32 program_bits;
	uint32 locationAddr;

	uint32 i;

	uint8 old_clock_cntl;
	uint8 old_crtc_ext_disp;

	old_clock_cntl = aty_ld_8(CLOCK_CNTL);
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, 0);

	old_crtc_ext_disp = aty_ld_8(CRTC_GEN_CNTL + 3);
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp | (CRTC_EXT_DISP_EN >> 24));

	mach64_delay(15); /* delay for 50 (15) ms */

	program_bits = pll->ics2595.program_bits;
	locationAddr = pll->ics2595.locationAddr;

	/* Program the clock chip */
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, 0);  /* Strobe = 0 */
	aty_StrobeClock();
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, 1);  /* Strobe = 0 */
	aty_StrobeClock();

	aty_ICS2595_put1bit(1);    /* Send start bits */
	aty_ICS2595_put1bit(0);    /* Start bit */
	aty_ICS2595_put1bit(0);    /* Read / ~Write */

	for (i = 0; i < 5; i++) {	/* Location 0..4 */
		aty_ICS2595_put1bit(locationAddr & 1);
		locationAddr >>= 1;
	}

	for (i = 0; i < 8 + 1 + 2 + 2; i++) {
		aty_ICS2595_put1bit(program_bits & 1);
		program_bits >>= 1;
	}

	mach64_delay(1); /* delay for 1 ms */

	(void)aty_ld_8(DAC_REGS); /* Clear DAC Counter */
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp);
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, old_clock_cntl | CLOCK_STROBE);

	mach64_delay(50); /* delay for 50 (15) ms */
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, ((pll->ics2595.locationAddr & 0x0F) | CLOCK_STROBE));

	return;
}

    /*
     *  STG 1703 Clock Chip
     */

int ATImach64::aty_var_to_pll_1703(uint32 vclk_per,
			       uint8 bpp, union ati_pll *pll)
{
	uint32 mhz100;			/* in 0.01 MHz */
	uint32 program_bits;
	/* uint32 post_divider; */
	uint32 mach64MinFreq, mach64MaxFreq, mach64RefFreq;
	uint32 temp, tempB;
	uint16 remainder, preRemainder;
	short divider = 0, tempA;

	/* Calculate the programming word */
	mhz100 = 100000000 / vclk_per;
	mach64MinFreq = MIN_FREQ_2595;
	mach64MaxFreq = MAX_FREQ_2595;
	mach64RefFreq = REF_FREQ_2595;	/* 14.32 MHz */

	/* Calculate program word */
	if (mhz100 == 0)
		program_bits = 0xE0;
	else {
		if (mhz100 < mach64MinFreq)
			mhz100 = mach64MinFreq;
		if (mhz100 > mach64MaxFreq)
			mhz100 = mach64MaxFreq;

		divider = 0;
		while (mhz100 < (mach64MinFreq << 3)) {
			mhz100 <<= 1;
			divider += 0x20;
		}

		temp = (unsigned int)(mhz100);
		temp = (unsigned int)(temp * (MIN_N_1703 + 2));
		temp -= (short)(mach64RefFreq << 1);

		tempA = MIN_N_1703;
		preRemainder = 0xffff;

		do {
			tempB = temp;
			remainder = tempB % mach64RefFreq;
			tempB = tempB / mach64RefFreq;

			if ((tempB & 0xffff) <= 127 && (remainder <= preRemainder)) {
				preRemainder = remainder;
				divider &= ~0x1f;
				divider |= tempA;
				divider = (divider & 0x00ff) + ((tempB & 0xff) << 8);
			}

			temp += mhz100;
			tempA++;
		} while (tempA <= (MIN_N_1703 << 1));

		program_bits = divider;
	}
	pll->ics2595.program_bits = program_bits;
	pll->ics2595.locationAddr = 0;
	pll->ics2595.post_divider = divider;  /* fuer nix */
	pll->ics2595.period_in_ps = vclk_per;

	return 0;
}
//-------------------------------------------------------------------
void ATImach64::aty_set_pll_1703(const union ati_pll *pll)
{
	uint32 program_bits;
	uint32 locationAddr;

	char old_crtc_ext_disp;

	old_crtc_ext_disp = aty_ld_8(CRTC_GEN_CNTL + 3);
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp | (CRTC_EXT_DISP_EN >> 24));

	program_bits = pll->ics2595.program_bits;
	locationAddr = pll->ics2595.locationAddr;

	/* Program clock */
	aty_dac_waste4();

	(void)aty_ld_8(DAC_REGS + 2);
	aty_st_8(DAC_REGS+2, (locationAddr << 1) + 0x20);
	aty_st_8(DAC_REGS+2, 0);
	aty_st_8(DAC_REGS+2, (program_bits & 0xFF00) >> 8);
	aty_st_8(DAC_REGS+2, (program_bits & 0xFF));

	(void)aty_ld_8(DAC_REGS); /* Clear DAC Counter */
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp);

	return;
}

    /*
     *  Chrontel 8398 Clock Chip
     */

int ATImach64::aty_var_to_pll_8398(uint32 vclk_per,
			       uint8 bpp, union ati_pll *pll)
{

	double tempA, tempB, fOut, longMHz100, diff, preDiff;

	uint32 mhz100;				/* in 0.01 MHz */
	uint32 program_bits;
	/* uint32 post_divider; */
	uint32 mach64MinFreq, mach64MaxFreq, mach64RefFreq;
	uint16 m, n, k=0, save_m, save_n, twoToKth;

	/* Calculate the programming word */
	mhz100 = 100000000 / vclk_per;
	mach64MinFreq = MIN_FREQ_2595;
	mach64MaxFreq = MAX_FREQ_2595;
	mach64RefFreq = REF_FREQ_2595;	/* 14.32 MHz */

	save_m = 0;
	save_n = 0;

	/* Calculate program word */
	if (mhz100 == 0)
		program_bits = 0xE0;
	else
	{
		if (mhz100 < mach64MinFreq)
			mhz100 = mach64MinFreq;
		if (mhz100 > mach64MaxFreq)
			mhz100 = mach64MaxFreq;

		longMHz100 = mhz100 * 256 / 100;   /* 8 bit scale this */

		while (mhz100 < (mach64MinFreq << 3))
		{
			mhz100 <<= 1;
			k++;
		}

		twoToKth = 1 << k;
		diff = 0;
		preDiff = 0xFFFFFFFF;

		for (m = MIN_M; m <= MAX_M; m++)
		{
			for (n = MIN_N; n <= MAX_N; n++)
			{
				tempA = (14.31818 * 65536);
				tempA *= (n + 8);  /* 43..256 */
				tempB = twoToKth * 256;
				tempB *= (m + 2);  /* 4..32 */
				fOut = tempA / tempB;  /* 8 bit scale */

				if (longMHz100 > fOut)
					diff = longMHz100 - fOut;
				else
					diff = fOut - longMHz100;

				if (diff < preDiff)
				{
					save_m = m;
					save_n = n;
					preDiff = diff;
				}
			}
		}
		program_bits = (k << 6) + (save_m) + (save_n << 8);
	}

	pll->ics2595.program_bits = program_bits;
	pll->ics2595.locationAddr = 0;
	pll->ics2595.post_divider = 0;
	pll->ics2595.period_in_ps = vclk_per;

	return 0;
}
//-------------------------------------------------------------------
void ATImach64::aty_set_pll_8398(const union ati_pll *pll)
{
	uint32 program_bits;
	uint32 locationAddr;

	char old_crtc_ext_disp;
	char tmp;

	old_crtc_ext_disp = aty_ld_8(CRTC_GEN_CNTL + 3);
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp | (CRTC_EXT_DISP_EN >> 24));

	program_bits = pll->ics2595.program_bits;
	locationAddr = pll->ics2595.locationAddr;

	/* Program clock */
	tmp = aty_ld_8(DAC_CNTL);
	aty_st_8(DAC_CNTL, tmp | DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3);

	aty_st_8(DAC_REGS, locationAddr);
	aty_st_8(DAC_REGS+1, (program_bits & 0xff00) >> 8);
	aty_st_8(DAC_REGS+1, (program_bits & 0xff));

	tmp = aty_ld_8(DAC_CNTL);
	aty_st_8(DAC_CNTL, (tmp & ~DAC_EXT_SEL_RS2) | DAC_EXT_SEL_RS3);

	(void)aty_ld_8(DAC_REGS); /* Clear DAC Counter */
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp);

	return;
}

    /*
     *  AT&T 20C408 Clock Chip
     */

int ATImach64::aty_var_to_pll_408(uint32 vclk_per,
			       uint8 bpp, union ati_pll *pll)
{
	uint32 mhz100;		/* in 0.01 MHz */
	uint32 program_bits;
	/* uint32 post_divider; */
	uint32 mach64MinFreq, mach64MaxFreq, mach64RefFreq;
	uint32 temp, tempB;
	uint16 remainder, preRemainder;
	short divider = 0, tempA;

	/* Calculate the programming word */
	mhz100 = 100000000 / vclk_per;
	mach64MinFreq = MIN_FREQ_2595;
	mach64MaxFreq = MAX_FREQ_2595;
	mach64RefFreq = REF_FREQ_2595;	/* 14.32 MHz */

	/* Calculate program word */
	if (mhz100 == 0)
		program_bits = 0xFF;
	else {
		if (mhz100 < mach64MinFreq)
			mhz100 = mach64MinFreq;
		if (mhz100 > mach64MaxFreq)
			mhz100 = mach64MaxFreq;

		while (mhz100 < (mach64MinFreq << 3)) {
		mhz100 <<= 1;
		divider += 0x40;
		}

		temp = (unsigned int)mhz100;
		temp = (unsigned int)(temp * (MIN_N_408 + 2));
		temp -= ((short)(mach64RefFreq << 1));

		tempA = MIN_N_408;
		preRemainder = 0xFFFF;

		do {
			tempB = temp;
			remainder = tempB % mach64RefFreq;
			tempB = tempB / mach64RefFreq;
			if (((tempB & 0xFFFF) <= 255) && (remainder <= preRemainder)) {
				preRemainder = remainder;
				divider &= ~0x3f;
				divider |= tempA;
				divider = (divider & 0x00FF) + ((tempB & 0xFF) << 8);
			}
			temp += mhz100;
			tempA++;
		} while(tempA <= 32);

		program_bits = divider;
	}

	pll->ics2595.program_bits = program_bits;
	pll->ics2595.locationAddr = 0;
	pll->ics2595.post_divider = divider;	/* fuer nix */
	pll->ics2595.period_in_ps = vclk_per;

	return 0;
}
//-------------------------------------------------------------------
void ATImach64::aty_set_pll_408(const union ati_pll *pll)
{
	uint32 program_bits;
	uint32 locationAddr;

	uint8 tmpA, tmpB, tmpC;
	char old_crtc_ext_disp;

	old_crtc_ext_disp = aty_ld_8(CRTC_GEN_CNTL + 3);
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp | (CRTC_EXT_DISP_EN >> 24));

	program_bits = pll->ics2595.program_bits;
	locationAddr = pll->ics2595.locationAddr;

	/* Program clock */
	aty_dac_waste4();
	tmpB = aty_ld_8(DAC_REGS + 2) | 1;
	aty_dac_waste4();
	aty_st_8(DAC_REGS + 2, tmpB);

	tmpA = tmpB;
	tmpC = tmpA;
	tmpA |= 8;
	tmpB = 1;

	aty_st_8(DAC_REGS, tmpB);
	aty_st_8(DAC_REGS + 2, tmpA);

	mach64_delay(4); /* delay for 400 us */

	locationAddr = (locationAddr << 2) + 0x40;
	tmpB = locationAddr;
	tmpA = program_bits >> 8;

	aty_st_8(DAC_REGS, tmpB);
	aty_st_8(DAC_REGS + 2, tmpA);

	tmpB = locationAddr + 1;
	tmpA = (uint8)program_bits;

	aty_st_8(DAC_REGS, tmpB);
	aty_st_8(DAC_REGS + 2, tmpA);

	tmpB = locationAddr + 2;
	tmpA = 0x77;

	aty_st_8(DAC_REGS, tmpB);
	aty_st_8(DAC_REGS + 2, tmpA);

	mach64_delay(4); /* delay for 400 us */
	tmpA = tmpC & (~(1 | 8));
	tmpB = 1;

	aty_st_8(DAC_REGS, tmpB);
	aty_st_8(DAC_REGS + 2, tmpA);

	(void)aty_ld_8(DAC_REGS); /* Clear DAC Counter */
	aty_st_8(CRTC_GEN_CNTL + 3, old_crtc_ext_disp);

	return;
}

   /*
     *  Unsupported DAC and Clock Chip
     */

int ATImach64::aty_set_dac_unsupported(const union ati_pll *pll, uint32 bpp,
				   uint32 accel)
{
	aty_st_le32(BUS_CNTL, 0x890e20f1);
	aty_st_le32(DAC_CNTL, 0x47052100);
	aty_st_le32(BUS_CNTL, 0x590e10ff);
	aty_st_le32(DAC_CNTL, 0x47012100);
	return 0;
}

