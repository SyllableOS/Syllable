/*
 *  Mach64 driver for AtheOS application server
 *  Copyright (C) 2001  Arno Klenke
 *
 *  Based on the atyfb linux kernel driver and the xfree86 ati driver.
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


#include "mach64.h"
#include <gui/bitmap.h>
#include <atheos/kdebug.h>
#include <appserver/pci_graphics.h>

static const char m64n_ct[]  = "mach64CT (ATI264CT)";
static const char m64n_et[]  = "mach64ET (ATI264ET)";
static const char m64n_vta3[]  = "mach64VTA3 (ATI264VT)";
static const char m64n_vta4[]  = "mach64VTA4 (ATI264VT)";
static const char m64n_vtb[]  = "mach64VTB (ATI264VTB)";
static const char m64n_vt4[]  = "mach64VT4 (ATI264VT4)";
static const char m64n_gt[]  = "3D RAGE (GT)";
static const char m64n_gtb[]  = "3D RAGE II+ (GTB)";
static const char m64n_iic_p[]  = "3D RAGE IIC (PCI)";
static const char m64n_iic_a[]  = "3D RAGE IIC (AGP)";
static const char m64n_lt[]  = "3D RAGE LT";
static const char m64n_ltg[]  = "3D RAGE LT-G";
static const char m64n_gtc_ba[]  = "3D RAGE PRO (BGA, AGP)";
static const char m64n_gtc_ba1[]  = "3D RAGE PRO (BGA, AGP, 1x only)";
static const char m64n_gtc_bp[]  = "3D RAGE PRO (BGA, PCI)";
static const char m64n_gtc_pp[]  = "3D RAGE PRO (PQFP, PCI)";
static const char m64n_gtc_ppl[]  = "3D RAGE PRO (PQFP, PCI, limited 3D)";
static const char m64n_xl[]  = "3D RAGE (XL)";
static const char m64n_ltp_a[]  = "3D RAGE LT PRO (AGP)";
static const char m64n_ltp_p[]  = "3D RAGE LT PRO (PCI)";
static const char m64n_mob_p[]  = "3D RAGE Mobility (PCI)";
static const char m64n_mob_a[]  = "3D RAGE Mobility (AGP)";

enum {
	MACH64_CT,
	MACH64_VT,
	MACH64_GT,
	MACH64_LT,
	MACH64_GTC,
	MACH64_XL,
	MACH64_LTPRO,
	MACH64_MOBILITY
};


struct ati_features aty_chips[] = {

	/* Mach64 CT */
	{ MACH64_CT, 0x4354, 0x4354, 0x00, 0x00, m64n_ct,      135,  60, M64F_CT | M64F_INTEGRATED | M64F_CT_BUS | M64F_MAGIC_FIFO },
	{ MACH64_CT, 0x4554, 0x4554, 0x00, 0x00, m64n_et,      135,  60, M64F_CT | M64F_INTEGRATED | M64F_CT_BUS | M64F_MAGIC_FIFO },

	/* Mach64 VT */
	{ MACH64_VT, 0x5654, 0x5654, 0xc7, 0x00, m64n_vta3,    170,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_MAGIC_FIFO | M64F_FIFO_24 },
	{ MACH64_VT, 0x5654, 0x5654, 0xc7, 0x40, m64n_vta4,    200,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_MAGIC_FIFO | M64F_FIFO_24 | M64F_MAGIC_POSTDIV },
	{ MACH64_VT, 0x5654, 0x5654, 0x00, 0x00, m64n_vtb,     200,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_GTB_DSP | M64F_FIFO_24 },
	{ MACH64_VT, 0x5655, 0x5655, 0x00, 0x00, m64n_vtb,     200,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL },
	{ MACH64_VT, 0x5656, 0x5656, 0x00, 0x00, m64n_vt4,     230,  83, M64F_VT | M64F_INTEGRATED | M64F_GTB_DSP },

	/* Mach64 GT (3D RAGE) */
	{ MACH64_GT, 0x4754, 0x4754, 0x07, 0x00, m64n_gt,      135,  63, M64F_GT | M64F_INTEGRATED | M64F_MAGIC_FIFO | M64F_FIFO_24 | M64F_EXTRA_BRIGHT },
	{ MACH64_GT, 0x4754, 0x4754, 0x07, 0x01, m64n_gt,      170,  67, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GT, 0x4754, 0x4754, 0x07, 0x02, m64n_gt,      200,  67, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GT, 0x4755, 0x4755, 0x00, 0x00, m64n_gtb,     200,  67, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GT, 0x4756, 0x4756, 0x00, 0x00, m64n_iic_p,   230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GT, 0x4757, 0x4757, 0x00, 0x00, m64n_iic_a,   230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GT, 0x475a, 0x475a, 0x00, 0x00, m64n_iic_a,   230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },

	/* Mach64 LT */
	{ MACH64_LT, 0x4c54, 0x4c54, 0x00, 0x00, m64n_lt,      135,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP },
	{ MACH64_LT, 0x4c47, 0x4c47, 0x00, 0x00, m64n_ltg,     230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT | M64F_LT_SLEEP | M64F_G3_PB_1024x768 },

	/* Mach64 GTC (3D RAGE PRO) */
	{ MACH64_GTC, 0x4742, 0x4742, 0x00, 0x00, m64n_gtc_ba,  230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GTC, 0x4744, 0x4744, 0x00, 0x00, m64n_gtc_ba1, 230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GTC, 0x4749, 0x4749, 0x00, 0x00, m64n_gtc_bp,  230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT | M64F_MAGIC_VRAM_SIZE },
	{ MACH64_GTC, 0x4750, 0x4750, 0x00, 0x00, m64n_gtc_pp,  230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ MACH64_GTC, 0x4751, 0x4751, 0x00, 0x00, m64n_gtc_ppl, 230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },

	/* 3D RAGE XL */
	{ MACH64_XL, 0x4752, 0x4752, 0x00, 0x00, m64n_xl, 230, /*120*/ 83, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT | M64F_XL_DLL },

	/* Mach64 LT PRO */
	{ MACH64_LTPRO, 0x4c42, 0x4c42, 0x00, 0x00, m64n_ltp_a,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP },
	{ MACH64_LTPRO, 0x4c44, 0x4c44, 0x00, 0x00, m64n_ltp_p,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP },
	{ MACH64_LTPRO, 0x4c49, 0x4c49, 0x00, 0x00, m64n_ltp_p,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_EXTRA_BRIGHT | M64F_G3_PB_1_1 | M64F_G3_PB_1024x768 },
	{ MACH64_LTPRO, 0x4c50, 0x4c50, 0x00, 0x00, m64n_ltp_p,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP },

	 /* 3D RAGE Mobility */
    { MACH64_MOBILITY, 0x4c4d, 0x4c4d, 0x00, 0x00, m64n_mob_p,   230,  50, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_MOBIL_BUS },
    { MACH64_MOBILITY, 0x4c4e, 0x4c4e, 0x00, 0x00, m64n_mob_a,   230,  50, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_MOBIL_BUS },

	{ 0, 0xffff, 0xffff, 0x00, 0x00, NULL, 0, 0, 0 },
};

static const char ram_dram[]  = "DRAM";
static const char ram_vram[]  = "VRAM";
static const char ram_edo[]  = "EDO";
static const char ram_sdram[]  = "SDRAM";
static const char ram_sgram[]  = "SGRAM";
static const char ram_wram[]  = "WRAM";
static const char ram_off[]  = "OFF";
static const char ram_resv[]  = "RESV";
static const char *aty_ct_ram[8]  = {
	ram_off, ram_dram, ram_edo, ram_edo,
	ram_sdram, ram_sgram, ram_wram, ram_resv
};
// *** mode table ***
// based upon fb.modes

VideoMode DefaultVideoModes [] = {
	 // 640x480 & 60Hz
	{ 640, 480, 60, 39722, 48, 16, 33, 10, 96, 2, 0},
	// 640x480 & 75Hz
	{ 640, 480, 75, 33747, 125, 18, 18, 5, 60, 5, 0},
	// 640x480 & 100Hz
	{ 640, 480, 100, 22272, 48, 32, 17, 22, 128, 12,0},
	// 800x600 & 60Hz
	{ 800, 600, 60, 25000, 88, 40, 23, 1, 128, 4, V_PHSYNC|V_PVSYNC},
	// 800x600 & 75Hz
	{ 800, 600, 75, 20203, 160, 16, 21, 1, 80, 3, V_PHSYNC|V_PVSYNC},
	// 800x600 & 100Hz
	{ 800, 600, 100, 14815, 160, 32, 20, 4, 80, 6, V_PHSYNC|V_PVSYNC},
	// 1024x768 & 60Hz
	{ 1024, 768, 60, 15385, 160, 24, 29, 3, 136, 6, 0},
	// 1024x768 & 75Hz
	{ 1024, 768, 75, 12699, 176, 16, 28, 1, 96, 3, V_PHSYNC|V_PVSYNC},
	// 1024x768 & 100Hz
	{ 1024, 768, 100, 9091, 280, 0, 16, 0, 88, 8, 0},
	// 1152x864 & 60Hz
	{ 1152, 864, 60, 12500, 128, 64, 41, 6, 112, 5, V_PHSYNC|V_PVSYNC},
	// 1152x864 & 75Hz
	{ 1152, 864, 75, 9091, 144, 24, 85, 45, 144, 8, V_PHSYNC|V_PVSYNC},
	// 1280x1024 & 60Hz
	{ 1280, 1024, 60, 9620, 248, 48, 38, 1, 112, 3, V_PHSYNC|V_PVSYNC},
	// 1280x1024 & 75Hz
	{ 1280, 1024, 75, 7408, 248, 16, 38, 1, 144, 3, V_PHSYNC|V_PVSYNC},

	// mode list terminator
	{   -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}


static uint32 get_pci_memory_size(int nFd, const PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource*4;
	uint32 nBase = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}


//=============================================================================
// NAME: ATImach64::ProbeHardware
// DESC: probe specified PCI card
// NOTE: returns 1 if a mach64 card was found
// SEE ALSO:
//-----------------------------------------------------------------------------
uint32 ATImach64::ProbeHardware (PCI_Info_s * PCIInfo ) {
	int nCard = 0;
	 // check all descriptors
	while( aty_chips[nCard].pci_id != 0xffff ) {
		if( PCIInfo->nVendorID == PCI_VENDOR_ID_ATI && PCIInfo->nDeviceID == aty_chips[nCard].pci_id ) {
			return 1;
		}
		nCard++;
	}
	return( 0 );
}
//=============================================================================
// NAME: ATImach64::InitHardware
// DESC: Initialize video hardware
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
bool ATImach64::InitHardware( int nFd ) {

 	// initialize internal instance variables
	memset (&info, 0, sizeof (struct ati_info)); // initialize chip state structure

	 // allocate framebuffer area
	m_hFramebufferArea = create_area ("mach64_fb", (void **)&m_pFramebufferBase,
		get_pci_memory_size(nFd, &m_cPCIInfo, 0), AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK);
	if( m_hFramebufferArea < 0 ) {
		dbprintf ("Mach64:: failed to create framebuffer area (%d)\n", m_hFramebufferArea);
		return false;
	}

	if( remap_area (m_hFramebufferArea, (void *)((m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK))) < 0 ) {
		printf("Mach64:: failed to create framebuffer area (%d)\n", m_hFramebufferArea);
		return false;
	}

	dprintf("Mach64:: Using framebuffer at %x\n", (uint)(m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK));


	// allocate register area
	m_hRegisterArea = create_area ("mach64_register", (void **)&m_pRegisterBase,
		get_pci_memory_size(nFd, &m_cPCIInfo, 2), AREA_FULL_ACCESS, AREA_NO_LOCK);
	if( m_hRegisterArea < 0 ) {
		dbprintf ("Mach64::failed to create register area (%d)\n", m_hRegisterArea);
		return false;
	}

	if( m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK )
	{
		if( remap_area (m_hRegisterArea, (void *)((m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK))) < 0 ) {
			dbprintf("Mach64:: failed to create register area (%d)\n", m_hRegisterArea);
			return false;
		}
		info.ati_regbase_phys = (m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK);
		dprintf("Mach64:: Using MMIO at %x\n", (uint)(m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK));
		info.auxiliary_aperture = 1;
	}
	else
	{
		if( remap_area (m_hRegisterArea, (void *)((m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) + 0x7ff000)) < 0 ) {
			dbprintf("Mach64:: failed to create register area (%d)\n", m_hRegisterArea);
			return false;
		}
		info.ati_regbase_phys = (m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) + 0x7ff000;
		dprintf("Mach64:: Using MMIO at %x\n", (uint)((m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) + 0x7ff000));
		info.auxiliary_aperture = 0;
	}


	info.ati_regbase = m_pRegisterBase + 0xc00 ;
	info.ati_regbase_phys += 0xc00;

	uint32 tmp;
	/*
	 * Enable memory-space accesses using config-space
	 * command register.
	 */
	tmp=pci_gfx_read_config(nFd, m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, PCI_COMMAND, 2);
	if( !(tmp & PCI_COMMAND_MEMORY) ) {
		dprintf("Mach64:: Enabling memory-space access\n");
		tmp |= PCI_COMMAND_MEMORY;
		pci_gfx_write_config(nFd, m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, PCI_COMMAND, 2, tmp);
	}
	info.frame_buffer_phys = m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	info.frame_buffer =m_pFramebufferBase;


	// populate screen mode list
	float rf[] = { 60.0f, 75.0f, 100.0f };
	for( int i = 0; i < 3; i++ ) {
	m_cScreenModeList.push_back (os::screen_mode ( 640, 480, 1280, CS_RGB16, rf[i]));  // 640x480 HiColor
	m_cScreenModeList.push_back (os::screen_mode ( 640, 480, 2560, CS_RGB32, rf[i])); // 640x480 TrueColor
	m_cScreenModeList.push_back (os::screen_mode ( 800, 600, 1600, CS_RGB16, rf[i]));  // 800x600 HiColor
	m_cScreenModeList.push_back (os::screen_mode ( 800, 600, 3200, CS_RGB32, rf[i])); // 800x600 TrueColor
	m_cScreenModeList.push_back (os::screen_mode (1024, 768, 2048, CS_RGB16, rf[i]));  // 1024x768 HiColor
	m_cScreenModeList.push_back (os::screen_mode (1024, 768, 4096, CS_RGB32, rf[i])); // 1024x768 TrueColor
	m_cScreenModeList.push_back (os::screen_mode (1152, 864, 2304, CS_RGB16, rf[i]));  // 1152x864 HiColor
	m_cScreenModeList.push_back (os::screen_mode (1152, 864, 4608, CS_RGB32, rf[i])); // 1152x864 TrueColor
	m_cScreenModeList.push_back (os::screen_mode (1280, 1024, 2560, CS_RGB16, rf[i]));  // 1280x1024 HiColor
	m_cScreenModeList.push_back (os::screen_mode (1280, 1024, 5120, CS_RGB32, rf[i])); // 1280x1024 TrueColor
	}

	uint32 chip_id;
	uint j;
	uint32 i;
	uint16 type;
	uint8 rev;
	const char *chipname = NULL, *ramname = NULL, *xtal;
	int pll, mclk, gtb_memsize;
	uint8 pll_ref_div;


	chip_id = aty_ld_le32(CONFIG_CHIP_ID);
	type = chip_id & CFG_CHIP_TYPE;
	rev = (chip_id & CFG_CHIP_REV)>>24;

	for( j = 0; j < (sizeof(aty_chips)/sizeof(ati_features)); j++ )
		if( type == aty_chips[j].chip_type &&
		(rev & aty_chips[j].rev_mask) == aty_chips[j].rev_val ) {
			chipname = aty_chips[j].name;
			pll = aty_chips[j].pll;
			mclk = aty_chips[j].mclk;
			info.chip = aty_chips[j].chip;
			info.features = aty_chips[j].features;
			goto found;
		}
	dprintf("Mach64:: Unknown mach64 0x%04x\n", type);
	return false;

found:

	dprintf("Mach64:: %s [0x%04x rev 0x%02x]\n ", chipname, type, rev);

	
	info.ram_type = (aty_ld_le32(CONFIG_STAT0) & 0x07);
	ramname = aty_ct_ram[info.ram_type];
	info.dac_type = DAC_INTERNAL;
	info.pll_type = CLK_INTERNAL;
	/* for many chips, the mclk is 67 MHz for SDRAM, 63 MHz otherwise */
	if( mclk == 67 && info.ram_type < SDRAM )
		mclk = 63;
	

	info.ref_clk_per = 1000000000000ULL/14318180;
	xtal = "14.31818";
	if( M64_HAS(GTB_DSP) && (pll_ref_div = aty_ld_pll(PLL_REF_DIV)) ) {
		int diff1, diff2;
		diff1 = 510*14/pll_ref_div-pll;
		diff2 = 510*29/pll_ref_div-pll;
		if (diff1 < 0)
			 diff1 = -diff1;
		if (diff2 < 0)
		diff2 = -diff2;
		if (diff2 < diff1) {
			info.ref_clk_per = 1000000000000ULL/29498928;
			xtal = "29.498928";
		}
	}
	i = aty_ld_le32(MEM_CNTL);
	gtb_memsize = M64_HAS(GTB_DSP);
	if( gtb_memsize )
		switch( i & MEM_SIZE_ALIAS_GTB ) {	
			case MEM_SIZE_512K:
				info.total_vram = 0x80000;
				break;
			case MEM_SIZE_1M:
				info.total_vram = 0x100000;
				break;
			case MEM_SIZE_2M_GTB:
				info.total_vram = 0x200000;
				break;
			case MEM_SIZE_4M_GTB:
				info.total_vram = 0x400000;
				break;
			case MEM_SIZE_6M_GTB:
				info.total_vram = 0x600000;
				break;
			case MEM_SIZE_8M_GTB:
				info.total_vram = 0x800000;
			break;
			default:
				info.total_vram = 0x80000;
		}
	else
		switch( i & MEM_SIZE_ALIAS ) {
			case MEM_SIZE_512K:
				info.total_vram = 0x80000;
				break;
			case MEM_SIZE_1M:
				info.total_vram = 0x100000;
				break;
			case MEM_SIZE_2M:
				info.total_vram = 0x200000;
				break;
			case MEM_SIZE_4M:
				info.total_vram = 0x400000;
				break;
			case MEM_SIZE_6M:
				info.total_vram = 0x600000;
				break;
			case MEM_SIZE_8M:
				info.total_vram = 0x800000;
				break;
			default:
				info.total_vram = 0x80000;
		}

	if( M64_HAS(MAGIC_VRAM_SIZE) ) {
		if( aty_ld_le32(CONFIG_STAT1) & 0x40000000 )
			info.total_vram += 0x400000;
	}


	dbprintf("Mach64:: %d%c %s, %s MHz XTAL, %d MHz PLL, %d Mhz MCLK\n",
		info.total_vram == 0x80000 ? 512 : (int)(info.total_vram >> 20),
		info.total_vram == 0x80000 ? 'K' : 'M', ramname, xtal, pll, mclk);

	if( mclk < 44 )
		info.mem_refresh_rate = 0;	/* 000 = 10 Mhz - 43 Mhz */
	else if( mclk < 50 )
		info.mem_refresh_rate = 1;	/* 001 = 44 Mhz - 49 Mhz */
	else if( mclk < 55 )
		info.mem_refresh_rate = 2;	/* 010 = 50 Mhz - 54 Mhz */
	else if( mclk < 66 )
		info.mem_refresh_rate = 3;	/* 011 = 55 Mhz - 65 Mhz */
	else if( mclk < 75 )
		info.mem_refresh_rate = 4;	/* 100 = 66 Mhz - 74 Mhz */
	else if( mclk < 80 )
		info.mem_refresh_rate = 5;	/* 101 = 75 Mhz - 79 Mhz */
	else if( mclk < 100 )
		info.mem_refresh_rate = 6;	/* 110 = 80 Mhz - 100 Mhz */
	else
		info.mem_refresh_rate = 7;	/* 111 = 100 Mhz and above */
	info.pll_per = 1000000/pll;
	info.mclk_per = 1000000/mclk;
	
	/* Use VESA if necessary */
	m_bVESA = false;
	if( info.chip == MACH64_LT || info.chip == MACH64_XL || info.chip == MACH64_LTPRO || info.chip == MACH64_MOBILITY )
	{
		dbprintf( "Using VESA for mode switching\n" );
		M64VesaDriver::InitModes();
		m_bVESA = true;
		goto vesa;
	}
	
	/* LCD */
	info.lcd.panel_id = -1;
	if( info.chip == MACH64_LTPRO )
		info.lcd.blend_fifo_size = 800;
	if( info.chip == MACH64_XL || info.chip == MACH64_MOBILITY )
		info.lcd.blend_fifo_size = 1024;
	
	if( info.chip == MACH64_LTPRO || info.chip == MACH64_XL || info.chip == MACH64_MOBILITY ) {
		i = aty_ld_le32(CONFIG_STAT0);
		info.lcd.panel_id = GetBits( i, CFG_PANEL_ID );
		info.par.crtc.lcd_index = aty_ld_le32( LCD_INDEX );
		info.par.crtc.horz_stretching = aty_ld_lcd( HORZ_STRETCHING );
		info.lcd.horizontal = GetBits( info.par.crtc.horz_stretching, HORZ_PANEL_SIZE );
		if( info.lcd.horizontal )
			if( info.lcd.horizontal == MaxBits( HORZ_PANEL_SIZE ) )
				info.lcd.horizontal = 0;
			else
				info.lcd.horizontal = ( info.lcd.horizontal + 1 ) << 3;
		info.par.crtc.ext_vert_stretch = aty_ld_lcd( EXT_VERT_STRETCH );
		info.lcd.vertical = GetBits( info.par.crtc.ext_vert_stretch, VERT_PANEL_SIZE );
		if( info.lcd.vertical )
			if( info.lcd.vertical == MaxBits( VERT_PANEL_SIZE ) )
				info.lcd.vertical = 0;
			else
				info.lcd.vertical++;
		info.par.crtc.vert_stretching = aty_ld_lcd( VERT_STRETCHING );
		info.par.crtc.lcd_gen_ctrl = aty_ld_lcd( LCD_GEN_CTRL );
		aty_st_le32( LCD_INDEX, info.par.crtc.lcd_index);
		
		dbprintf("Mach64:: LCD panel with size %ix%i detected\n", info.lcd.horizontal,
				info.lcd.vertical );
	}
	
vesa:

	/*
	*  Last page of 8 MB aperture can be MMIO
	*/
	if( info.total_vram == 0x800000 && info.auxiliary_aperture == 0 )
		info.total_vram -= GUI_RESERVE;

	wait_for_idle();


	/* Initialize video overlay */
	m_bSupportsYUV = false;
	m_bVideoOverlayUsed = false;
	aty_st_le32( SCALER_BUF0_OFFSET_U, -1 );
	wait_for_vblank();
	wait_for_idle();
	wait_for_fifo(2);
	if( aty_ld_le32(SCALER_BUF0_OFFSET_U) ) m_bSupportsYUV = true;

	return( true );
}

//=============================================================================
// NAME: ATImach64::ATImach64 ()
// DESC: ATImach64 driver class constructor
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
ATImach64::ATImach64( int nFd ) : M64VesaDriver( nFd ), m_cLock ("mach64_hardware_lock") {

	m_bIsInitialized = false;
	m_pFramebufferBase = m_pRegisterBase = NULL;
	
	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	
	if( ProbeHardware (&m_cPCIInfo) ) { m_bIsInitialized = true; }

	if( m_bIsInitialized == false ) {
		dbprintf("Mach64:: No supported cards found\n");
		return;
	}

	// OK. card found. now initialize it.
	if( !InitHardware ( nFd ) ) {
		m_bIsInitialized = false;
		return;
	}
}

//=============================================================================
// NAME: ATImach64::~ATImach64 ()
// DESC: ATImach64 driver class destructor
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
ATImach64::~ATImach64 () {

}

//=============================================================================
// NAME: ATImach64::SetScreenMode ()
// DESC:
// NOTE: <!> nPosH, nPosV, nSizeH and nSizeV ignored at this time
// SEE ALSO:
//-----------------------------------------------------------------------------
int ATImach64::SetScreenMode( os::screen_mode sMode ) {

	if( sMode.m_eColorSpace == CS_CMAP8 || sMode.m_eColorSpace == CS_RGB24 ) {
		dbprintf ("Mach64:: 8BPP or 24BPP modes are not supported!\n");
		return( -1 );
	}
	
	if( m_bVESA )
	{
		int nReturn = M64VesaDriver::SetScreenMode( sMode );
		info.par.crtc.vxres = sMode.m_nWidth;
		info.par.crtc.vyres = sMode.m_nHeight;
		info.par.crtc.bpp = BitsPerPixel( sMode.m_eColorSpace );
		m_sCurrentMode = sMode;
		if( sMode.m_eColorSpace == CS_RGB16 ) {
			info.par.crtc.dp_pix_width = HOST_16BPP | SRC_16BPP | DST_16BPP |
				BYTE_ORDER_LSB_TO_MSB;
			info.par.crtc.dp_chain_mask = 0x8410;
		} else {
			info.par.crtc.dp_pix_width = HOST_32BPP | SRC_32BPP | DST_32BPP |
				BYTE_ORDER_LSB_TO_MSB;
			info.par.crtc.dp_chain_mask = 0x8080;
		}
		init_engine(&info.par);
		wait_for_idle();
		return( nReturn );
	}

	 /* find display mode */
	VideoMode * vmode = DefaultVideoModes;
	VideoMode * bestVMode = NULL;
	while( vmode->Width > 0 ) {
		if( vmode->Width == sMode.m_nWidth &&
		vmode->Height == sMode.m_nHeight &&
		vmode->Refresh <= int (sMode.m_vRefreshRate) ) {
			// we have found mode with given size and compatible refresh
			if( bestVMode == NULL )
				bestVMode = vmode;
			else if( (int (sMode.m_vRefreshRate)-vmode->Refresh) <
			(int (sMode.m_vRefreshRate)-bestVMode->Refresh) )
				bestVMode = vmode;
		}
		vmode++;
	}

	if( !bestVMode ) {
		dbprintf ("Mach64:: Unable to find video mode %dx%d %dhz\n", sMode.m_nWidth, sMode.m_nHeight, int (sMode.m_vRefreshRate));
		return( -1 );
	}

	vmode = bestVMode;

	int err;
	uint32 i;
	uint32 lcdindex;

	/* Save some LCD information */
	if( info.lcd.panel_id >= 0 ) {
		lcdindex = aty_ld_le32( LCD_INDEX );
		info.par.crtc.lcd_index = lcdindex & ~( LCD_INDEX_MASK |
			LCD_DISPLAY_DIS | LCD_SRC_SEL | CRTC2_DISPLAY_DIS );
		if( info.chip != MACH64_XL )
			info.par.crtc.lcd_index |= CRTC2_DISPLAY_DIS;
		info.par.crtc.config_panel = aty_ld_lcd( CONFIG_PANEL ) | DONT_SHADOW_HEND;
		info.par.crtc.lcd_gen_ctrl = aty_ld_lcd( LCD_GEN_CTRL ) | ~CRTC_RW_SELECT;
		aty_st_le32( LCD_INDEX, lcdindex );
		
		info.par.crtc.lcd_gen_ctrl &= ~( HORZ_DIVBY2_EN | DIS_HOR_CRT_DIVBY2 | MCLK_PM_EN |
                      VCLK_DAC_PM_EN | USE_SHADOWED_VEND |
                      USE_SHADOWED_ROWCUR | SHADOW_EN | SHADOW_RW_EN );
		
		info.par.crtc.lcd_gen_ctrl |= DONT_SHADOW_VPAR | LOCK_8DOT;; 
		info.par.crtc.lcd_gen_ctrl |= CRT_ON | LCD_ON;
		
	}

	if( (err = aty_var_to_crtc(vmode, BitsPerPixel(sMode.m_eColorSpace), &info.par.crtc)) )
		return( err );
	
	err = aty_var_to_pll_ct(vmode->Clock, info.par.crtc.bpp, &info.par.pll);
		
	if( err ) return err;
	
	/* Setup LCDs */
	if( info.lcd.panel_id >= 0 ) {
		info.par.crtc.gen_cntl &= ~( CRTC2_EN | CRTC2_PIX_WIDTH_MASK );
		lcdindex = aty_ld_le32( LCD_INDEX );
		info.par.crtc.horz_stretching = aty_ld_lcd( HORZ_STRETCHING );
		info.par.crtc.ext_vert_stretch = aty_ld_lcd( EXT_VERT_STRETCH ) &
		 ~( AUTO_VERT_RATIO | VERT_STRETCH_MODE | VERT_STRETCH_RATIO3 );
		 if( (int)info.par.crtc.vxres <= info.lcd.blend_fifo_size &&
		 	(int)info.par.crtc.vyres < info.lcd.vertical )
		 	info.par.crtc.ext_vert_stretch |= VERT_STRETCH_MODE;
		 aty_st_le32( LCD_INDEX, lcdindex );
		 
		 info.par.crtc.horz_stretching &= ~( HORZ_STRETCH_RATIO | HORZ_STRETCH_LOOP | AUTO_HORZ_RATIO |
              HORZ_STRETCH_MODE | HORZ_STRETCH_EN );
		 
		if( (int)info.par.crtc.vxres < info.lcd.horizontal )
		 	info.par.crtc.horz_stretching |= ( HORZ_STRETCH_MODE | HORZ_STRETCH_EN |
		 	SetBits((info.par.crtc.vxres * (MaxBits(HORZ_STRETCH_BLEND) + 1)) /
                        info.lcd.horizontal, HORZ_STRETCH_BLEND ) );
		
		if( (int)info.par.crtc.vyres < info.lcd.vertical )
			info.par.crtc.vert_stretching = ( VERT_STRETCH_USE0 | VERT_STRETCH_EN )
				| SetBits((info.par.crtc.vyres * (MaxBits(VERT_STRETCH_RATIO0) + 1)) /
                        info.lcd.vertical, VERT_STRETCH_RATIO0);
        else
        	info.par.crtc.vert_stretching = 0;
	}
	
	/* Load LCD registers */
	if( info.lcd.panel_id >= 0 ) {
		aty_st_le32( CRTC_GEN_CNTL, info.par.crtc.gen_cntl & ~( CRTC_EXT_DISP_EN | CRTC_ENABLE )) ;
		
		aty_st_lcd( CONFIG_PANEL, info.par.crtc.config_panel );
		aty_st_lcd( LCD_GEN_CTRL, info.par.crtc.lcd_gen_ctrl & ~( CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN ) );
		aty_st_lcd( HORZ_STRETCHING, info.par.crtc.horz_stretching & ~
					( HORZ_STRETCH_MODE | HORZ_STRETCH_EN ) );
		aty_st_lcd( VERT_STRETCHING, info.par.crtc.vert_stretching &~
					( VERT_STRETCH_RATIO1 | VERT_STRETCH_RATIO2 |
                      VERT_STRETCH_USE0 | VERT_STRETCH_EN ) );
	}

	aty_set_crtc(&info.par.crtc);
	
	/* Load LCD registers */
	if( info.lcd.panel_id >= 0 ) {
		
		aty_st_lcd( LCD_GEN_CTRL, ( info.par.crtc.lcd_gen_ctrl & ~CRTC_RW_SELECT ) | ( SHADOW_EN | 
		SHADOW_RW_EN ) );
		 
		
		aty_st_le32(CRTC_H_TOTAL_DISP, info.par.crtc.h_tot_disp);
		aty_st_le32(CRTC_H_SYNC_STRT_WID, info.par.crtc.h_sync_strt_wid);
		aty_st_le32(CRTC_V_TOTAL_DISP, info.par.crtc.v_tot_disp);
		aty_st_le32(CRTC_V_SYNC_STRT_WID, info.par.crtc.v_sync_strt_wid);
		
		aty_st_lcd( LCD_GEN_CTRL, info.par.crtc.lcd_gen_ctrl );
		aty_st_lcd( HORZ_STRETCHING, info.par.crtc.horz_stretching );
		aty_st_lcd( VERT_STRETCHING, info.par.crtc.vert_stretching );
		aty_st_lcd( EXT_VERT_STRETCH, info.par.crtc.ext_vert_stretch );
		aty_st_le32( LCD_INDEX, info.par.crtc.lcd_index );
		
	}

	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, 0);
	/* better call aty_StrobeClock ?? */
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, CLOCK_STROBE);


	aty_set_pll_ct(&info.par.pll);
	
	
	i = aty_ld_le32(MEM_CNTL) & 0xf00fffff;
	if( !M64_HAS(MAGIC_POSTDIV) )
		i |= info.mem_refresh_rate << 20;
	switch ( info.par.crtc.bpp ) {
		case 8:
		case 24:
			i |= 0x00000000;
		break;
	 	case 15:
		case 16:
			i |= 0x04000000;
		break;
		case 32:
			i |= 0x08000000;
		break;
	}
	if( M64_HAS(CT_BUS) ) {
		aty_st_le32(DAC_CNTL, 0x87010184);
		aty_st_le32(BUS_CNTL, 0x680000f9);
	} else if( M64_HAS(VT_BUS) ) {
	 	aty_st_le32(DAC_CNTL, 0x87010184);
		aty_st_le32(BUS_CNTL, 0x680000f9);
	}  else if( M64_HAS(MOBIL_BUS) ) {
		aty_st_le32(DAC_CNTL, 0x80010102);
		aty_st_le32(BUS_CNTL, 0x7b33a040);
	} else {
		/* GT */
		aty_st_le32(DAC_CNTL, 0x86010102);
		aty_st_le32(BUS_CNTL, 0x7b23a040);
		aty_st_le32(EXT_MEM_CNTL, aty_ld_le32(EXT_MEM_CNTL) | 0x5000001);
	}
	aty_st_le32(MEM_CNTL, i);

	aty_st_8(DAC_MASK, 0xff);

	/* Color registers */
	int index;

	for( index = 0; index < 256; index++ )
	{
		i = aty_ld_8(DAC_CNTL) & 0xfc;
		if( M64_HAS(EXTRA_BRIGHT) )
			i |= 0x2;	// DAC_CNTL|0x2 turns off the extra brightness for gt
		aty_st_8(DAC_CNTL, i);
		aty_st_8(DAC_W_INDEX, index);
		aty_st_8(DAC_DATA, index);
		aty_st_8(DAC_DATA, index);
		aty_st_8(DAC_DATA, index);

	}

	init_engine(&info.par);

	wait_for_idle();
	
	m_sCurrentMode.m_nWidth = sMode.m_nWidth;
	m_sCurrentMode.m_nHeight = sMode.m_nHeight;
	m_sCurrentMode.m_eColorSpace = sMode.m_eColorSpace;
	m_sCurrentMode.m_vRefreshRate = bestVMode->Refresh;
	m_sCurrentMode.m_nBytesPerLine = sMode.m_nWidth * BytesPerPixel( sMode.m_eColorSpace );

	return 0;
}


// ----------------------------------------------------------------
area_id ATImach64::Open() {
	return( m_hFramebufferArea );
}
// ----------------------------------------------------------------
void ATImach64::Close() {
}
//------------------------------------------------------------------

bool ATImach64::IsInitialized() {
	return( m_bIsInitialized );
}
//-----------------------------------------------------------------------------

extern "C"
DisplayDriver* init_gfx_driver( int nFd ) {

	try {
		ATImach64* pcDriver = new ATImach64( nFd );
		if (pcDriver->IsInitialized()) {
			return pcDriver;
		} else {
			delete pcDriver;
			return NULL ;
		}
	} catch (std::exception&  cExc) {
		dbprintf( "Mach64:: Got exception\n" );
		return NULL ;
	}
}

//-----------------------------------------------------------------------------
// *** end of file ***
