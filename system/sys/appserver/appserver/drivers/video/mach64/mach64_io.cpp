#include "mach64.h"

uint32 ATImach64::aty_ld_le32(int regindex)
{
	/* Hack for bloc 1, should be cleanly optimized by compiler */
	if( regindex >= 0x400 )
	regindex -= 0x800;

	return( inl (regindex) );
}
//-------------------------------------------------------------------
void ATImach64::aty_st_le32(int regindex, uint32 val)
{
	/* Hack for bloc 1, should be cleanly optimized by compiler */
	if( regindex >= 0x400 )
		regindex -= 0x800;

	outl (val, regindex);
}
//-------------------------------------------------------------------
uint8 ATImach64::aty_ld_8(int regindex)
{
	/* Hack for bloc 1, should be cleanly optimized by compiler */
	if( regindex >= 0x400 )
		regindex -= 0x800;

	return( inb (regindex) );
}
//-------------------------------------------------------------------
void ATImach64::aty_st_8(int regindex, uint8 val)
{
	/* Hack for bloc 1, should be cleanly optimized by compiler */
	if( regindex >= 0x400 )
		regindex -= 0x800;

	outb (val, regindex);
}
//-------------------------------------------------------------------
uint8 ATImach64::aty_ld_lcd( int regindex )
{
	aty_st_8( LCD_INDEX, SetBits( regindex, LCD_INDEX_MASK ) );
	return( aty_ld_8( LCD_DATA ) );
}
//-------------------------------------------------------------------
void ATImach64::aty_st_lcd(int regindex, uint8 val)
{
	aty_st_8( LCD_INDEX, SetBits( regindex, LCD_INDEX_MASK ) );
	aty_st_8( LCD_DATA, val );
}

//-------------------------------------------------------------------
void ATImach64::wait_for_fifo(uint16 entries)
{
	while( (aty_ld_le32(FIFO_STAT) & 0xffff) >
		((uint32)(0x8000 >> entries)) );
}
//-------------------------------------------------------------------
void ATImach64::wait_for_idle()
{
	wait_for_fifo(16);
	while( (aty_ld_le32(GUI_STAT) & 1)!= 0 );
}
//-------------------------------------------------------------------
void ATImach64::wait_for_vblank()
{
	int i;

    for(i=0; i<2000000; i++)
	if( (aty_ld_le32(CRTC_INT_CNTL)&CRTC_VBLANK)==0 ) break;
    for(i=0; i<2000000; i++)
	if( (aty_ld_le32(CRTC_INT_CNTL)&CRTC_VBLANK) ) break;
}
//-------------------------------------------------------------------
uint8 ATImach64::aty_ld_pll(int offset)
{
	uint8 res;

	/* write addr byte */
	aty_st_8(CLOCK_CNTL + 1, (offset << 2));
	/* read the register value */
	res = aty_ld_8(CLOCK_CNTL + 2);
	return( res );
}
//-------------------------------------------------------------------
void ATImach64::reset_engine()
{
	/* reset engine */
	aty_st_le32(GEN_TEST_CNTL,
		aty_ld_le32(GEN_TEST_CNTL) & ~GUI_ENGINE_ENABLE);
	/* enable engine */
	aty_st_le32(GEN_TEST_CNTL,
		aty_ld_le32(GEN_TEST_CNTL) | GUI_ENGINE_ENABLE);
	/* ensure engine is not locked up by clearing any FIFO or */
	/* HOST errors */
    	aty_st_le32(BUS_CNTL, aty_ld_le32(BUS_CNTL) | BUS_HOST_ERR_ACK |
			  BUS_FIFO_ERR_ACK);
}
//-------------------------------------------------------------------
void ATImach64::reset_GTC_3D_engine()
{
	aty_st_le32(SCALE_3D_CNTL, 0xc0);
	mach64_delay(GTC_3D_RESET_DELAY);
	aty_st_le32(SETUP_CNTL, 0x00);
	mach64_delay(GTC_3D_RESET_DELAY);
	aty_st_le32(SCALE_3D_CNTL, 0x00);
	mach64_delay(GTC_3D_RESET_DELAY);
}
//-------------------------------------------------------------------
void ATImach64::init_engine(const struct ati_par *par)
{
	uint32 pitch_value;

	/* determine modal information from global mode structure */
	pitch_value = par->crtc.vxres;

	if( par->crtc.bpp == 24 ) {
		/* In 24 bpp, the engine is in 8 bpp - this requires that all */
		/* horizontal coordinates and widths must be adjusted */
		pitch_value = pitch_value * 3;
	}

	/* On GTC (RagePro), we need to reset the 3D engine before */
	if( M64_HAS(RESET_3D) )
		reset_GTC_3D_engine();

	/* Reset engine, enable, and clear any engine errors */
	reset_engine();

	aty_st_le32(CONFIG_CNTL, VGA_APERTURE_ENABLE);
	aty_st_le32(MEM_VGA_WP_SEL, 0xffffffff);
	aty_st_le32(MEM_VGA_RP_SEL, 0xffffffff);

	 /* ---- Setup standard engine context ---- */

	/* All GUI registers here are FIFOed - therefore, wait for */
	/* the appropriate number of empty FIFO entries */
	wait_for_fifo(14);

	/* enable all registers to be loaded for context loads */
	aty_st_le32(CONTEXT_MASK, 0xFFFFFFFF);

	/* set destination pitch to modal pitch, set offset to zero */
	aty_st_le32(DST_OFF_PITCH, (pitch_value / 8) << 22);

	/* zero these registers (set them to a known state) */
	aty_st_le32(DST_Y_X, 0);
	aty_st_le32(DST_HEIGHT, 0);
	aty_st_le32(DST_BRES_ERR, 0);
	aty_st_le32(DST_BRES_INC, 0);
	aty_st_le32(DST_BRES_DEC, 0);

	/* set destination drawing attributes */
	aty_st_le32(DST_CNTL, DST_LAST_PEL | DST_Y_TOP_TO_BOTTOM |
			  DST_X_LEFT_TO_RIGHT);

	/* set source pitch to modal pitch, set offset to zero */
	aty_st_le32(SRC_OFF_PITCH, (pitch_value / 8) << 22);

	/* set these registers to a known state */
	aty_st_le32(SRC_Y_X, 0);
	aty_st_le32(SRC_HEIGHT1_WIDTH1, 1);
	aty_st_le32(SRC_Y_X_START, 0);
	aty_st_le32(SRC_HEIGHT2_WIDTH2, 1);

	/* set source pixel retrieving attributes */
	aty_st_le32(SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);

	/* set host attributes */
	wait_for_fifo(13);
	aty_st_le32(HOST_CNTL, 0);

	/* set pattern attributes */
	aty_st_le32(PAT_REG0, 0);
	aty_st_le32(PAT_REG1, 0);
	aty_st_le32(PAT_CNTL, 0);

	/* set scissors to modal size */
	aty_st_le32(SC_LEFT, 0);
	aty_st_le32(SC_TOP, 0);
	aty_st_le32(SC_BOTTOM, par->crtc.vyres - 1);
	aty_st_le32(SC_RIGHT, pitch_value - 1);

	/* set background color to minimum value (usually BLACK) */
	aty_st_le32(DP_BKGD_CLR, 0);

	/* set foreground color to maximum value (usually WHITE) */
	aty_st_le32(DP_FRGD_CLR, 0xFFFFFFFF);

	/* set write mask to effect all pixel bits */
	aty_st_le32(DP_WRITE_MASK, 0xFFFFFFFF);

	/* set foreground mix to overpaint and background mix to */
	/* no-effect */
	aty_st_le32(DP_MIX, FRGD_MIX_S | BKGD_MIX_D);

	/* set primary source pixel channel to foreground color */
	/* register */
	aty_st_le32(DP_SRC, FRGD_SRC_FRGD_CLR);

	/* set compare functionality to false (no-effect on */
	/* destination) */
	wait_for_fifo(3);
	aty_st_le32(CLR_CMP_CLR, 0);
	aty_st_le32(CLR_CMP_MASK, 0xFFFFFFFF);
	aty_st_le32(CLR_CMP_CNTL, 0);

	/* set pixel depth */
	wait_for_fifo(2);
	aty_st_le32(DP_PIX_WIDTH, par->crtc.dp_pix_width);
	aty_st_le32(DP_CHAIN_MASK, par->crtc.dp_chain_mask);

	wait_for_fifo(5);
	aty_st_le32(SCALE_3D_CNTL, 0);
	aty_st_le32(Z_CNTL, 0);
	aty_st_le32(CRTC_INT_CNTL, aty_ld_le32(CRTC_INT_CNTL) & ~0x20);
	aty_st_le32(GUI_TRAJ_CNTL, 0x100023);

	/* insure engine is idle before leaving */
	wait_for_idle();
}
