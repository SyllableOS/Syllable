#ifndef _ATI_INFO_H_
#define _ATI_INFO_H_

    /*
     *  ATI Mach64 features
     */

#define M64_HAS(feature)	((info).features & (M64F_##feature))

#define M64F_RESET_3D		0x00000001
#define M64F_MAGIC_FIFO		0x00000002
#define M64F_GTB_DSP		0x00000004
#define M64F_FIFO_24		0x00000008
#define M64F_SDRAM_MAGIC_PLL	0x00000010
#define M64F_MAGIC_POSTDIV	0x00000020
#define M64F_INTEGRATED		0x00000040
#define M64F_CT_BUS		0x00000080
#define M64F_VT_BUS		0x00000100
#define M64F_MOBIL_BUS		0x00000200
#define M64F_GX			0x00000400
#define M64F_CT			0x00000800
#define M64F_VT			0x00001000
#define M64F_GT			0x00002000
#define M64F_MAGIC_VRAM_SIZE	0x00004000
#define M64F_G3_PB_1_1		0x00008000
#define M64F_G3_PB_1024x768	0x00010000
#define M64F_EXTRA_BRIGHT	0x00020000
#define M64F_LT_SLEEP		0x00040000
#define M64F_XL_DLL			0x00080000


/* Make sure n * PAGE_SIZE is protected at end of Aperture for GUI-regs */
/*  - must be large enough to catch all GUI-Regs   */
/*  - must be aligned to a PAGE boundary           */
#define GUI_RESERVE	(1 * PAGE_SIZE)


struct crtc {
    uint32 vxres;
    uint32 vyres;
    uint32 xoffset;
    uint32 yoffset;
    uint32 bpp;
    uint32 h_tot_disp;
    uint32 h_sync_strt_wid;
    uint32 v_tot_disp;
    uint32 v_sync_strt_wid;
    uint32 off_pitch;
    uint32 gen_cntl;
    uint32 dp_pix_width;	/* acceleration */
    uint32 dp_chain_mask;	/* acceleration */
    
    uint32 lcd_index;
    uint32 config_panel;
    uint32 lcd_gen_ctrl;
    uint32 horz_stretching;
    uint32 vert_stretching;
    uint32 ext_vert_stretch;
};


struct pll_ct {
    uint8 pll_ref_div;
    uint8 pll_gen_cntl;
    uint8 mclk_fb_div;
    uint8 pll_vclk_cntl;
    uint8 vclk_post_div;
    uint8 vclk_fb_div;
    uint8 pll_ext_cntl;
    uint32 dsp_config;	/* Mach64 GTB DSP */
    uint32 dsp_on_off;	/* Mach64 GTB DSP */
    uint8 mclk_post_div_real;
    uint8 vclk_post_div_real;
};

union ati_pll {
    struct pll_ct ct;
};


/*
 *  The Hardware parameters for each card
 */

struct ati_par {
    struct crtc crtc;
    union ati_pll pll;
    uint32 accel_flags;
};

struct ati_lcd {
	int panel_id;
	int clock;
	int horizontal;
	int vertical;
	int hsync_start;
	int hsync_width;
	int hblank_width;
	int vsync_start;
	int vsync_width;
	int vblank_width;
	int blend_fifo_size;
};


struct ati_cursor {
    uint32 offset;
    struct {
        uint16 x, y;
    } pos, hot, size;
    uint8 *ram;
};

struct ati_info {
	uint8 chip;
    unsigned long ati_regbase_phys;
    vuint8 *ati_regbase;
    unsigned long frame_buffer_phys;
    vuint8 *frame_buffer;
    unsigned long clk_wr_offset;
    struct ati_cursor *cursor;
    struct ati_par par;
    struct ati_lcd lcd;
    uint32 features;
    uint32 total_vram;
    uint32 ref_clk_per;
    uint32 pll_per;
    uint32 mclk_per;
    uint8 ram_type;
    uint8 mem_refresh_rate;
    int dac_type;
    int pll_type;
    uint8 auxiliary_aperture;
};

struct ati_features {
	uint8 chip;
    uint16 pci_id, chip_type;
    uint8 rev_mask, rev_val;
    const char *name;
    int pll, mclk;
    uint32 features;
};


#endif

