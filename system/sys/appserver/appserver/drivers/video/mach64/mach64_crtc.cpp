#include "mach64.h"


int ATImach64::aty_var_to_crtc(const struct VideoMode *var, int var_bpp, 
			   struct crtc *crtc)
{
	uint32 xres, yres, vxres, vyres, xoffset, yoffset, bpp;
	uint32 left, right, upper, lower, hslen, vslen, sync, flags;
	uint32 h_total, h_disp, h_sync_strt, h_sync_dly, h_sync_wid, h_sync_pol;
	uint32 v_total, v_disp, v_sync_strt, v_sync_wid, v_sync_pol, c_sync;
	uint32 pix_width = 0, dp_pix_width = 0, dp_chain_mask = 0;

	/* input */
	xres = var->Width;
	yres = var->Height;
	vxres = var->Width;
	vyres = var->Height;
	xoffset = 0;
	yoffset = 0;
	bpp = var_bpp;
	left = var->left_margin;
	right = var->right_margin;
	upper = var->upper_margin;
	lower = var->lower_margin;
	hslen = var->hsync_len;
	vslen = var->vsync_len;
	sync = 0;
	flags = var->Flags;

	/* convert (and round up) and validate */
	xres = (xres+7) & ~7;
	xoffset = (xoffset+7) & ~7;
	vxres = (vxres+7) & ~7;
	if( vxres < xres+xoffset )
		vxres = xres+xoffset;
	h_disp = xres/8-1;
	if( h_disp > 0xff )
		dprintf("h_disp too large");
	h_sync_strt = h_disp+(right/8);
	if( h_sync_strt > 0x1ff )
		dprintf("h_sync_start too large");
	h_sync_dly = right & 7;
	h_sync_wid = (hslen+7)/8;
	if( h_sync_wid > 0x1f )
		dprintf("h_sync_wid too large");
	h_total = h_sync_strt+h_sync_wid+(h_sync_dly+left+7)/8;
	if( h_total > 0x1ff )
		dprintf("h_total too large");
	h_sync_pol = flags & V_PHSYNC ? 0 : 1;

	if( vyres < yres+yoffset )
		vyres = yres+yoffset;
	v_disp = yres-1;
	if( v_disp > 0x7ff )
		dprintf("v_disp too large");
	v_sync_strt = v_disp+lower;
	if( v_sync_strt > 0x7ff )
		dprintf("v_sync_strt too large");
	v_sync_wid = vslen;
	if( v_sync_wid > 0x1f )
		dprintf("v_sync_wid too large");
	v_total = v_sync_strt+v_sync_wid+upper;
	if( v_total > 0x7ff )
		dprintf("v_total too large");
	v_sync_pol = flags & V_PVSYNC ? 0 : 1;

	c_sync = 0;

	if( bpp <= 8 ) {
		bpp = 8;
		pix_width = CRTC_PIX_WIDTH_8BPP;
		dp_pix_width = HOST_8BPP | SRC_8BPP | DST_8BPP | BYTE_ORDER_LSB_TO_MSB;
		dp_chain_mask = 0x8080;
	} else if( bpp <= 15 ) {
		bpp = 16;
		pix_width = CRTC_PIX_WIDTH_15BPP;
		dp_pix_width = HOST_15BPP | SRC_15BPP | DST_15BPP |
			BYTE_ORDER_LSB_TO_MSB;
		dp_chain_mask = 0x4210;
	} else if( bpp <= 16 ) {
		bpp = 16;
		pix_width = CRTC_PIX_WIDTH_16BPP;
		dp_pix_width = HOST_16BPP | SRC_16BPP | DST_16BPP |
		       BYTE_ORDER_LSB_TO_MSB;
		dp_chain_mask = 0x8410;
	} else if( (bpp <= 24) && M64_HAS(INTEGRATED) ) {
		bpp = 24;
		pix_width = CRTC_PIX_WIDTH_24BPP;
		dp_pix_width = HOST_8BPP | SRC_8BPP | DST_8BPP | BYTE_ORDER_LSB_TO_MSB;
		dp_chain_mask = 0x8080;
	} else if( bpp <= 32 ) {
		bpp = 32;
		pix_width = CRTC_PIX_WIDTH_32BPP;
		dp_pix_width = HOST_32BPP | SRC_32BPP | DST_32BPP |
			BYTE_ORDER_LSB_TO_MSB;
		dp_chain_mask = 0x8080;
	} else
		dprintf("invalid bpp");

	if( vxres * vyres * bpp / 8 > info.total_vram )
		dprintf("not enough video RAM");


	/* output */
	crtc->vxres = vxres;
	crtc->vyres = vyres;
	crtc->xoffset = xoffset;
	crtc->yoffset = yoffset;
	crtc->bpp = bpp;
	crtc->h_tot_disp = h_total | (h_disp<<16);
	crtc->h_sync_strt_wid = (h_sync_strt & 0xff) | (h_sync_dly<<8) |
			 ((h_sync_strt & 0x100)<<4) | (h_sync_wid<<16) |
			 (h_sync_pol<<21);
	crtc->v_tot_disp = v_total | (v_disp<<16);
	crtc->v_sync_strt_wid = v_sync_strt | (v_sync_wid<<16) | (v_sync_pol<<21);
	crtc->off_pitch = ((yoffset*vxres+xoffset)*bpp/64) | (vxres<<19);
	crtc->gen_cntl = aty_ld_le32(CRTC_GEN_CNTL) &
		~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
		CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_CSYNC_EN |
		CRTC_PIX_BY_2_EN | CRTC_DISPLAY_DIS | CRTC_VGA_XOVERSCAN |
		CRTC_PIX_WIDTH_MASK | CRTC_BYTE_PIX_ORDER | CRTC_FIFO_LWM |
		VGA_128KAP_PAGING | VFC_SYNC_TRISTATE |
		CRTC_LOCK_REGS |              /* Already off, but ... */
		CRTC_SYNC_TRISTATE | CRTC_DISP_REQ_ENB |
		VGA_TEXT_132 | VGA_CUR_B_TEST);
	crtc->gen_cntl = pix_width | VGA_XCRT_CNT_EN | CRTC_EXT_DISP_EN | CRTC_ENABLE | VGA_ATI_LINEAR;
	if (M64_HAS(MAGIC_FIFO)) {
		/* Not VTB/GTB */
		/* FIXME: magic FIFO values */
		crtc->gen_cntl |= aty_ld_le32(CRTC_GEN_CNTL) & 0x000e0000;
	}
	crtc->dp_pix_width = dp_pix_width;
	crtc->dp_chain_mask = dp_chain_mask;

	return( 0 );
}
//-------------------------------------------------------------------
void ATImach64::aty_set_crtc(const struct crtc *crtc)
{
	aty_st_le32(CRTC_H_TOTAL_DISP, crtc->h_tot_disp);
	aty_st_le32(CRTC_H_SYNC_STRT_WID, crtc->h_sync_strt_wid);
	aty_st_le32(CRTC_V_TOTAL_DISP, crtc->v_tot_disp);
	aty_st_le32(CRTC_V_SYNC_STRT_WID, crtc->v_sync_strt_wid);
	aty_st_le32(OVR_CLR, 0);
	aty_st_le32(OVR_WID_LEFT_RIGHT, 0);
	aty_st_le32(OVR_WID_TOP_BOTTOM, 0);
	aty_st_le32(CRTC_OFF_PITCH, crtc->off_pitch);
	aty_st_le32(CRTC_VLINE_CRNT_VLINE, 0);
	aty_st_le32(CRTC_GEN_CNTL, crtc->gen_cntl);
}
