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

#ifndef _RADEON_H
#define _RADEON_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/isa_io.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <util/locker.h>

#include <gui/bitmap.h>

#include "../../../server/ddriver.h"
#include "../../../server/sprite.h"
#include "../../../server/bitmap.h"

#include <appserver/pci_graphics.h>

#include "ati_ids.h"
#include "radeon_regs.h"

using namespace os;

/** User-configurable values (deprecated, use configuration file) **/

/* Enable experimental support for R300/R350/RV350 (Radeon 9500-9800) */
#define RADEON_ENABLE_R300

/* Enable experimental support for Mobility & IGP chips */
#define RADEON_ENABLE_MOBILITY

/* Enable experimental support for RV370/R380/R400/R420/R450 & XPRESS (Radeon X300-X850) */
#define RADEON_ENABLE_R400

/* Configuration file */
#define RADEON_CONFIG_FILE "/system/config/drivers/appserver/video/radeon.cfg"


/***************************************************************
 * Most of the definitions here are adapted right from XFree86 *
 ***************************************************************/

/*
 * Chip families. Must fit in the low 16 bits of a long word
 */
enum radeon_family {
	CHIP_FAMILY_UNKNOW,
	CHIP_FAMILY_LEGACY,
	CHIP_FAMILY_RADEON,
	CHIP_FAMILY_RV100,
	CHIP_FAMILY_RS100,    /* U1 (IGP320M) or A3 (IGP320)*/
	CHIP_FAMILY_RV200,
	CHIP_FAMILY_RS200,    /* U2 (IGP330M/340M/350M) or A4 (IGP330/340/345/350), RS250 (IGP 7000) */
	CHIP_FAMILY_R200,
	CHIP_FAMILY_RV250,
	CHIP_FAMILY_RS300,    /* Radeon 9100 IGP */
	CHIP_FAMILY_RV280,
	CHIP_FAMILY_R300,
	CHIP_FAMILY_R350,
	CHIP_FAMILY_RV350,
	CHIP_FAMILY_RV380,    /* RV370/RV380/M22/M24 */
	CHIP_FAMILY_R420,     /* R420/R423/M18 */
	CHIP_FAMILY_LAST,
};

#define IS_RV100_VARIANT(rinfo) (((rinfo)->family == CHIP_FAMILY_RV100)  || \
				 ((rinfo)->family == CHIP_FAMILY_RV200)  || \
				 ((rinfo)->family == CHIP_FAMILY_RS100)  || \
				 ((rinfo)->family == CHIP_FAMILY_RS200)  || \
				 ((rinfo)->family == CHIP_FAMILY_RV250)  || \
				 ((rinfo)->family == CHIP_FAMILY_RV280)  || \
				 ((rinfo)->family == CHIP_FAMILY_RS300))


#define IS_R300_VARIANT(rinfo) (((rinfo)->family == CHIP_FAMILY_R300)  || \
				((rinfo)->family == CHIP_FAMILY_RV350) || \
				((rinfo)->family == CHIP_FAMILY_R350)  || \
				((rinfo)->family == CHIP_FAMILY_RV380) || \
				((rinfo)->family == CHIP_FAMILY_R420))

/*
 * Chip flags
 */
enum radeon_chip_flags {
	CHIP_FAMILY_MASK	= 0x0000ffffUL,
	CHIP_FLAGS_MASK	= 0xffff0000UL,
	CHIP_IS_MOBILITY	= 0x00010000UL,
	CHIP_IS_IGP		= 0x00020000UL,
	CHIP_HAS_CRTC2	= 0x00040000UL,	
};

/*
 * Errata workarounds
 */
enum radeon_errata {
	CHIP_ERRATA_R300_CG		= 0x00000001,
	CHIP_ERRATA_PLL_DUMMYREADS	= 0x00000002,
	CHIP_ERRATA_PLL_DELAY		= 0x00000004,
};

/*
 * Monitor types
 */
enum radeon_montype {
	MT_NONE = 0,
	MT_CRT,		/* CRT */
	MT_LCD,		/* LCD */
	MT_DFP,		/* DVI */
	MT_CTV,		/* composite TV */
	MT_STV			/* S-Video out */
};

typedef struct {
	uint16 reg;
	uint32 val;
} reg_val;

/*
 * DDC i2c ports
 */
enum ddc_type {
	ddc_none,
	ddc_monid,
	ddc_dvi,
	ddc_vga,
	ddc_crt2,
};

/*
 * Connector types
 */
enum conn_type {
	conn_none,
	conn_proprietary,
	conn_crt,
	conn_DVI_I,
	conn_DVI_D,
};


/*
 * PLL infos
 */
struct pll_info {
	int ppll_max;
	int ppll_min;
	int sclk, mclk;
	int ref_div;
	int ref_clk;
};

/*
 * VRAM infos
 */
struct ram_info {
	int ml;
	int mb;
	int trcd;
	int trp;
	int twr;
	int cl;
	int tr2w;
	int loop_latency;
	int rloop;
};


/*
 * This structure contains the various registers manipulated by this
 * driver for setting or restoring a mode. It's mostly copied from
 * XFree's RADEONSaveRec structure. A few chip settings might still be
 * tweaked without beeing reflected or saved in these registers though
 */
struct radeon_regs {
	/* Common registers */
	uint32		ovr_clr;
	uint32		ovr_wid_left_right;
	uint32		ovr_wid_top_bottom;
	uint32		ov0_scale_cntl;
	uint32		mpp_tb_config;
	uint32		mpp_gp_config;
	uint32		subpic_cntl;
	uint32		viph_control;
	uint32		i2c_cntl_1;
	uint32		gen_int_cntl;
	uint32		cap0_trig_cntl;
	uint32		cap1_trig_cntl;
	uint32		bus_cntl;
	uint32		surface_cntl;
	uint32		bios_5_scratch;

	/* Other registers to save for VT switches or driver load/unload */
	uint32		dp_datatype;
	uint32		rbbm_soft_reset;
	uint32		clock_cntl_index;
	uint32		amcgpio_en_reg;
	uint32		amcgpio_mask;

	/* Surface/tiling registers */
	uint32		surf_lower_bound[8];
	uint32		surf_upper_bound[8];
	uint32		surf_info[8];

	/* CRTC registers */
	uint32		crtc_gen_cntl;
	uint32		crtc_ext_cntl;
	uint32		dac_cntl;
	uint32		crtc_h_total_disp;
	uint32		crtc_h_sync_strt_wid;
	uint32		crtc_v_total_disp;
	uint32		crtc_v_sync_strt_wid;
	uint32		crtc_offset;
	uint32		crtc_offset_cntl;
	uint32		crtc_pitch;
	uint32		disp_merge_cntl;
	uint32		grph_buffer_cntl;
	uint32		crtc_more_cntl;

	/* CRTC2 registers */
	uint32		crtc2_gen_cntl;
	uint32		dac2_cntl;
	uint32		disp_output_cntl;
	uint32		disp_hw_debug;
	uint32		disp2_merge_cntl;
	uint32		grph2_buffer_cntl;
	uint32		crtc2_h_total_disp;
	uint32		crtc2_h_sync_strt_wid;
	uint32		crtc2_v_total_disp;
	uint32		crtc2_v_sync_strt_wid;
	uint32		crtc2_offset;
	uint32		crtc2_offset_cntl;
	uint32		crtc2_pitch;

	/* Flat panel regs */
	uint32 	fp_crtc_h_total_disp;
	uint32		fp_crtc_v_total_disp;
	uint32		fp_gen_cntl;
	uint32		fp2_gen_cntl;
	uint32		fp_h_sync_strt_wid;
	uint32		fp2_h_sync_strt_wid;
	uint32		fp_horz_stretch;
	uint32		fp_panel_cntl;
	uint32		fp_v_sync_strt_wid;
	uint32		fp2_v_sync_strt_wid;
	uint32		fp_vert_stretch;
	uint32		lvds_gen_cntl;
	uint32		lvds_pll_cntl;
	uint32		tmds_crc;
	uint32		tmds_transmitter_cntl;

	/* Computed values for PLL */
	uint32		dot_clock_freq;
	int			feedback_div;
	int			post_div;	

	/* PLL registers */
	uint32		ppll_div_3;
	uint32		ppll_ref_div;
	uint32		vclk_ecp_cntl;
	uint32		clk_cntl_index;	

	/* Computed values for PLL2 */
	uint32		dot_clock_freq_2;
	int			feedback_div_2;
	int			post_div_2;

	/* PLL2 registers */
	uint32		p2pll_ref_div;
	uint32		p2pll_div_0;
	uint32		htotal_cntl2;

       	/* Palette */
	int			palette_valid;
	uint32		palette[256];
	uint32		palette2[256];
};

struct panel_info {
	int xres, yres;
	int valid;
	int clock;
	int hOver_plus, hSync_width, hblank;
	int vOver_plus, vSync_width, vblank;
	int hAct_high, vAct_high, interlaced;
	int pwr_delay;
	int use_bios_dividers;
	int ref_divider;
	int post_divider;
	int fbk_divider;
};

struct cursor_info {
    uint32 offset;
    struct {
        uint16 x, y;
    } pos, hot, size;
    uint8 *ram;
};

struct pci_table
{
	uint16 vendor_id;
	uint16 pci_id;
	uint32 flags;
};

struct VideoMode {
	int		Width;			/* width 						*/
	int		Height;			/* height						*/
	int		Refresh;			/* refresh rate 				*/

	uint32	Clock;      			/* pixel clock					*/
	uint32	left_margin;		/* time from sync to picture	*/
	uint32	right_margin;		/* time from picture to sync	*/
	uint32	upper_margin;		/* time from sync to picture	*/
	uint32	lower_margin;
	uint32	hsync_len;		/* length of horizontal sync	*/
	uint32	vsync_len;		/* length of vertical sync		*/
	uint32	Flags;
};

enum radeon_bios {
	bios_legacy,
	bios_atom
};

struct radeon_info {

    /* From radeonfb_info structure */
    struct radeon_regs 	state;
    struct radeon_regs	init_state;

    char			name[13];
    char			ram_type[12];

    unsigned long 		fb_local_base;

    unsigned long		fb_base_phys;
    unsigned long		mmio_base_phys;

    vuint8			*fb_base;
    vuint8			*mmio_base;

    PCI_Info_s	pdev;

    radeon_bios			bios_type;
    vuint8			*bios_seg;
    int			fp_bios_start;
	int			fp_atom_bios_start;

    int			chipset;
    uint8			family;
    uint8			rev;
	unsigned int		errata;    
    unsigned long		video_ram;

    int			pitch, bpp, depth;

    uint32			dotClock;

    int			has_CRTC2;
    int			is_mobility;
    int			is_IGP;
    int			reversed_DAC;
    int			reversed_TMDS;
    struct panel_info	panel_info;
    int			mon1_type;
    uint8		*mon1_EDID;
    int			mon1_dbsize;
    int			mon2_type;
    uint8	   *mon2_EDID;

    uint32		dp_gui_master_cntl;

    struct pll_info		pll;

    struct ram_info		ram;

    struct cursor_info	cursor;

    int			pm_reg;
    uint32		save_regs[64];
    int			asleep;
    int			lock_blank;

    /* Lock on register access */
    //Spinlock_s		reg_lock;

    /* Timer used for delayed LVDS operations */
    //ktimer_t	lvds_timer;
    uint32			pending_lvds_gen_cntl;
};

// Video mode descriptor
#define V_PHSYNC		0x00000001 // + HSYNC
#define V_NHSYNC		0x00000002 // - HSYNC
#define V_PVSYNC		0x00000004 // + VSYNC
#define V_NVSYNC		0x00000008 // - VSYNC
#define V_INTERLACE	0x00000010
#define V_DBLSCAN	0x00000020
#define V_CLKDIV2		0x00004000

#define VESA_NO_BLANKING   0
#define VESA_VSYNC_SUSPEND 1
#define VESA_HSYNC_SUSPEND 2
#define VESA_POWERDOWN     3

extern VideoMode DefaultVideoModes[];
extern reg_val common_regs[] ;
extern reg_val common_regs_m6[];


class ATIRadeon : public DisplayDriver
{
public:

    ATIRadeon( int nFd );
    ~ATIRadeon();

    uint32 ProbeHardware ( int nFd, PCI_Info_s * PCIInfo );
    bool InitHardware( int nFd );

    area_id	Open();
    virtual void	Close();

    virtual int  GetScreenModeCount();
    virtual bool GetScreenModeDesc( int nIndex, os::screen_mode* psMode );

    virtual int		SetScreenMode( os::screen_mode sMode );
    virtual os::screen_mode GetCurrentScreenMode();
   
    virtual int		    GetFramebufferOffset() { return( 0 ); }

    virtual bool		IsInitialized();

       virtual bool DrawLine(SrvBitmap *psBitMap, const IRect &cClipRect,
        const IPoint &cPnt1, const IPoint &cPnt2, const Color32_s &sColor, int nMode);
    virtual bool		FillRect(SrvBitmap *pcBitmap, const IRect &cRect, const Color32_s &sColor, int nMode);
    virtual bool		BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap, IRect cSrcRect,
								IRect cDstRect, int nMode, int nAlpha);

    virtual bool	CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer );
    virtual bool	RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *pBuffer );
    virtual void	DeleteVideoOverlay( area_id *pBuffer );
private:
    bool		m_bIsInitialized;
    bool		m_bRetrievedInfos;
    Locker		m_cLock; // hardware access lock
    PCI_Info_s m_cPCIInfo;
    struct pci_table m_sRadeonInfo;
    vuint8 * m_pFramebufferBase; // base of the framebuffer
    area_id m_hFramebufferArea; // area_id of the framebuffer
    vuint8 * m_pRegisterBase; // base of the registers
    area_id m_hRegisterArea; // area_id of the registers
    vuint8 * m_pROMBase; // base of the ROM
    area_id m_hROMArea; // area_id of the ROM

    unsigned long m_nRegBasePhys; // physical base of the registers

    radeon_info rinfo;

    bool	m_bCfgEnableR300;
    bool	m_bCfgEnableMobility;
    bool	m_bCfgEnableMirroring;
    bool	m_bCfgEnableDebug;
    bool	m_bCfgDisableBIOSUsage;

    std::vector<os::screen_mode> m_cModeList;
    os::screen_mode	   m_sCurrentMode;
    uint32		m_nFrameBufferSize;
    int		m_nFrameBufferOffset;
    int		mirror;
    bool		m_bFirstRepaint;

   
    // Video Overlay
    Color32_s m_cColorKey;
    bool		m_bVideoOverlayUsed;
    bool		m_bOVToRecreate;

    void SetTransform( os::color_space eFormat, float bright, float cont,
        float sat, float hue, float red_intensity, float green_intensity, 
        float blue_intensity, uint ref);
    void		SetOverlayColorKey( os::Color32_s sColorKey );

    int		BytesPerPixel (color_space cs);
    int		BitsPerPixel (color_space cs);
    void		InitPalette();
    void		FixUpMemoryMappings();
    void		PMEnableDynamicMode();
    void		PMDisableDynamicMode();

    void 		UnmapROM( int nFd, PCI_Info_s *dev );
    int 		MapROM( int nFd, PCI_Info_s *dev );
    int		FindMemVBios();

    void		GetPLLInfo();
    int		ProbePLLParams();
    void		CalcPLLRegs(struct radeon_regs *regs, unsigned long freq);
    void		SaveState(struct radeon_regs *save);
    void 		WritePLLRegs(struct radeon_regs *mode);
    void		WriteMode (struct radeon_regs *mode);

    char *		GetMonName(int type);
    int		GetPanelInfoBIOS();
    int		CrtIsConnected(int is_crt_dac);
    int		ParseMonitorLayout(const char *monitor_layout);
    void		ProbeScreens(const char *monitor_layout, int ignore_edid);
    void		VarToPanelInfo(const struct VideoMode *var);
    void		CreateModeList();
    int		ScreenBlank (int blank);

    void	GetSettings();

    inline void		EngineFlush();
    inline void		FifoWait( int entries );
    inline void		EngineIdle();
    void				EngineReset();
    void				EngineInit();

    inline uint32		pci_size(uint32 base, uint32 mask);
    uint32				get_pci_memory_size(int nFd, PCI_Info_s *pcPCIInfo, int nResource);
    uint32				get_pci_rom_memory_size(int nFd, PCI_Info_s *pcPCIInfo);
    inline uint32 	__INPLL(uint32 addr);
    inline uint32		INPLL(uint32 addr);
    inline void		__OUTPLL(unsigned int index, uint32 val);
    inline void		radeon_pll_errata_after_index();
    inline void		radeon_pll_errata_after_data();    
    inline int round_div(int num, int den) { return (num + (den / 2)) / den; }
    inline uint32	 GetDstBpp(uint16 depth);
};

#define PRIMARY_MONITOR(rinfo)	(rinfo.mon1_type)

#define min(a, b)		(a < b ? a : b)

/*
 * Debugging stuffs
 */


#define RTRACE		if(m_bCfgEnableDebug) dbprintf

#define OUTREG(a, b)       *((vuint32 *)(rinfo.mmio_base + a)) = b
#define INREG(a)           *((vuint32 *)(rinfo.mmio_base + a))
#define OUTREG8(a, b)      *((vuint8 *)(rinfo.mmio_base + a)) = b
#define INREG8(a)          *((vuint8 *)(rinfo.mmio_base + a))


inline void ATIRadeon::__OUTPLL(unsigned int index, uint32 val)
{

	OUTREG8(CLOCK_CNTL_INDEX, (index & 0x0000003f) | 0x00000080);
	radeon_pll_errata_after_index();
	OUTREG(CLOCK_CNTL_DATA, val);
	radeon_pll_errata_after_data();
}

inline void ATIRadeon::radeon_pll_errata_after_index()
{
	if (!(rinfo.errata & CHIP_ERRATA_PLL_DUMMYREADS))
		return;

	(void)INREG(CLOCK_CNTL_DATA);
	(void)INREG(CRTC_GEN_CNTL);
}

inline void ATIRadeon::radeon_pll_errata_after_data()
{
	if (rinfo.errata & CHIP_ERRATA_PLL_DELAY) {
		/* we can't deal with posted writes here ... */
		snooze( 5000 );
	}
	if (rinfo.errata & CHIP_ERRATA_R300_CG) {
		uint32 save, tmp;
		save = INREG(CLOCK_CNTL_INDEX);
		tmp = save & ~(0x3f | PLL_WR_EN);
		OUTREG(CLOCK_CNTL_INDEX, tmp);
		tmp = INREG(CLOCK_CNTL_DATA);
		OUTREG(CLOCK_CNTL_INDEX, save);
	}
}



inline uint32 ATIRadeon::__INPLL(uint32 addr)
{
	uint32 data;
	OUTREG8(CLOCK_CNTL_INDEX, addr & 0x0000003f);
	radeon_pll_errata_after_index();	
	data = (INREG(CLOCK_CNTL_DATA));
	radeon_pll_errata_after_data();
	return data;
}

inline uint32 ATIRadeon::INPLL(uint32 addr)
{
	uint32 data;

	data = __INPLL(addr);
	return data;
}

#define OUTPLL(addr,val)	\
	do {	\
		__OUTPLL(addr, val); \
	} while(0)

#define OUTPLLP(addr,val,mask)  					\
	do {								\
		unsigned int _tmp;					\
		_tmp  = __INPLL(addr);				\
		_tmp &= (mask);						\
		_tmp |= (val);						\
		__OUTPLL(addr, _tmp);					\
	} while (0)

#define OUTREGP(addr,val,mask)  					\
	do {								\
		unsigned int _tmp;					\
		_tmp = INREG(addr);				       	\
		_tmp &= (mask);						\
		_tmp |= (val);						\
		OUTREG(addr, _tmp);					\
	} while (0)

#define BIOS_IN8(v)  	*((vuint8 *)rinfo.bios_seg + v)
#define BIOS_IN16(v) 	((*((vuint8 *)rinfo.bios_seg + v)) | \
			  (*((vuint8 *)rinfo.bios_seg + v + 1) << 8))
#define BIOS_IN32(v) 	((*((vuint8 *)rinfo.bios_seg + v)) | \
			  (*((vuint8 *)rinfo.bios_seg + v + 1) << 8) | \
			  (*((vuint8 *)rinfo.bios_seg + v + 2) << 16) | \
			  (*((vuint8 *)rinfo.bios_seg + v + 3) << 24))

/*
 * 2D Engine helper routines
 */
inline void ATIRadeon::EngineFlush ()
{
	int i;

	/* initiate flush */
	OUTREGP(RB2D_DSTCACHE_CTLSTAT, RB2D_DC_FLUSH_ALL,
	        ~RB2D_DC_FLUSH_ALL);

	for (i = 0; i < 2000000; i++) {
		if (!(INREG(RB2D_DSTCACHE_CTLSTAT) & RB2D_DC_BUSY))
			return;
	}
	dbprintf( "Radeon :: Error: Flush Timeout !\n" );
}


inline void ATIRadeon::FifoWait (int entries)
{
	int i;

	for (i = 0; i < 2000000; i++) {
		if ((INREG(RBBM_STATUS) & 0x7f) >= (uint)entries)
			return;
	}
	dbprintf("Radeon :: Error: FIFO Timeout !\n");
}


inline void ATIRadeon::EngineIdle ()
{
	int i;

	/* ensure FIFO is empty before waiting for idle */
	FifoWait (64);

	for (i = 0; i < 2000000; i++) {
		if (((INREG(RBBM_STATUS) & GUI_ACTIVE)) == 0) {
			EngineFlush ();
			return;
		}
	}
	dbprintf("Radeon :: Error: Idle Timeout !\n");
}


inline uint32 ATIRadeon::GetDstBpp(uint16 depth)
{
	switch (depth) {
       	case 8:
       		return DST_8BPP;
       	case 15:
       		return DST_15BPP;
       	case 16:
       		return DST_16BPP;
       	case 32:
       		return DST_32BPP;
       	default:
       		return 0;
	}
}

#endif /* _RADEON_H */

