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

static const char m64n_gx[]  = "mach64GX (ATI888GX00)";
static const char m64n_cx[]  = "mach64CX (ATI888CX00)";
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

struct ati_features aty_chips[] = {
	 /* Mach64 GX */
	{ 0x4758, 0x00d7, 0x00, 0x00, m64n_gx,      135,  50, M64F_GX },
	{ 0x4358, 0x0057, 0x00, 0x00, m64n_cx,      135,  50, M64F_GX },

	/* Mach64 CT */
	{ 0x4354, 0x4354, 0x00, 0x00, m64n_ct,      135,  60, M64F_CT | M64F_INTEGRATED | M64F_CT_BUS | M64F_MAGIC_FIFO },
	{ 0x4554, 0x4554, 0x00, 0x00, m64n_et,      135,  60, M64F_CT | M64F_INTEGRATED | M64F_CT_BUS | M64F_MAGIC_FIFO },

	/* Mach64 VT */
	{ 0x5654, 0x5654, 0xc7, 0x00, m64n_vta3,    170,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_MAGIC_FIFO | M64F_FIFO_24 },
	{ 0x5654, 0x5654, 0xc7, 0x40, m64n_vta4,    200,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_MAGIC_FIFO | M64F_FIFO_24 | M64F_MAGIC_POSTDIV },
	{ 0x5654, 0x5654, 0x00, 0x00, m64n_vtb,     200,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_GTB_DSP | M64F_FIFO_24 },
	{ 0x5655, 0x5655, 0x00, 0x00, m64n_vtb,     200,  67, M64F_VT | M64F_INTEGRATED | M64F_VT_BUS | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL },
	{ 0x5656, 0x5656, 0x00, 0x00, m64n_vt4,     230,  83, M64F_VT | M64F_INTEGRATED | M64F_GTB_DSP },

	/* Mach64 GT (3D RAGE) */
	{ 0x4754, 0x4754, 0x07, 0x00, m64n_gt,      135,  63, M64F_GT | M64F_INTEGRATED | M64F_MAGIC_FIFO | M64F_FIFO_24 | M64F_EXTRA_BRIGHT },
	{ 0x4754, 0x4754, 0x07, 0x01, m64n_gt,      170,  67, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4754, 0x4754, 0x07, 0x02, m64n_gt,      200,  67, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4755, 0x4755, 0x00, 0x00, m64n_gtb,     200,  67, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4756, 0x4756, 0x00, 0x00, m64n_iic_p,   230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4757, 0x4757, 0x00, 0x00, m64n_iic_a,   230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x475a, 0x475a, 0x00, 0x00, m64n_iic_a,   230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_FIFO_24 | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },

	/* Mach64 LT */
	{ 0x4c54, 0x4c54, 0x00, 0x00, m64n_lt,      135,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP },
	{ 0x4c47, 0x4c47, 0x00, 0x00, m64n_ltg,     230,  63, M64F_GT | M64F_INTEGRATED | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT | M64F_LT_SLEEP | M64F_G3_PB_1024x768 },

	/* Mach64 GTC (3D RAGE PRO) */
	{ 0x4742, 0x4742, 0x00, 0x00, m64n_gtc_ba,  230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4744, 0x4744, 0x00, 0x00, m64n_gtc_ba1, 230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4749, 0x4749, 0x00, 0x00, m64n_gtc_bp,  230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT | M64F_MAGIC_VRAM_SIZE },
	{ 0x4750, 0x4750, 0x00, 0x00, m64n_gtc_pp,  230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },
	{ 0x4751, 0x4751, 0x00, 0x00, m64n_gtc_ppl, 230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT },

	/* 3D RAGE XL */
	{ 0x4752, 0x4752, 0x00, 0x00, m64n_xl, 230, /*120*/ 83, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_SDRAM_MAGIC_PLL | M64F_EXTRA_BRIGHT | M64F_XL_DLL },

	/* Mach64 LT PRO */
	{ 0x4c42, 0x4c42, 0x00, 0x00, m64n_ltp_a,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP },
	{ 0x4c44, 0x4c44, 0x00, 0x00, m64n_ltp_p,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP },
	{ 0x4c49, 0x4c49, 0x00, 0x00, m64n_ltp_p,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_EXTRA_BRIGHT | M64F_G3_PB_1_1 | M64F_G3_PB_1024x768 },
	{ 0x4c50, 0x4c50, 0x00, 0x00, m64n_ltp_p,   230, 63, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP },

	/* 3D RAGE Mobility */
	{ 0x4c4d, 0x4c4d, 0x00, 0x00, m64n_mob_p,   230,  50, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_MOBIL_BUS },
	{ 0x4c4e, 0x4c4e, 0x00, 0x00, m64n_mob_a,   230,  50, M64F_GT | M64F_INTEGRATED | M64F_RESET_3D | M64F_GTB_DSP | M64F_MOBIL_BUS },

	{ 0xffff, 0xffff, 0x00, 0x00, NULL, 0, 0, 0 },
};

static const char ram_dram[]  = "DRAM";
static const char ram_vram[]  = "VRAM";
static const char ram_edo[]  = "EDO";
static const char ram_sdram[]  = "SDRAM";
static const char ram_sgram[]  = "SGRAM";
static const char ram_wram[]  = "WRAM";
static const char ram_off[]  = "OFF";
static const char ram_resv[]  = "RESV";

static const char *aty_gx_ram[8]  = {
	ram_dram, ram_vram, ram_vram, ram_dram,
	ram_dram, ram_vram, ram_vram, ram_resv
};
static const char *aty_ct_ram[8]  = {
	ram_off, ram_dram, ram_edo, ram_edo,
	ram_sdram, ram_sgram, ram_wram, ram_resv
};

static uint32 default_vram = 0;
static int default_pll  = 0;
static int default_mclk = 0;


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

static uint32 get_pci_memory_size(const PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource*4;
	uint32 nBase = read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	write_pci_config(nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	write_pci_config(nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return( pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK) );
	} else {
		return( pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff) );
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
bool ATImach64::InitHardware() {

 	// initialize internal instance variables
	memset (&info, 0, sizeof (struct ati_info)); // initialize chip state structure

	 // allocate framebuffer area
	m_hFramebufferArea = create_area ("mach64_fb", (void **)&m_pFramebufferBase,
		get_pci_memory_size(&m_cPCIInfo, 0), AREA_FULL_ACCESS, AREA_NO_LOCK);
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
		get_pci_memory_size(&m_cPCIInfo, 2), AREA_FULL_ACCESS, AREA_NO_LOCK);
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
	tmp=read_pci_config(m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, PCI_COMMAND, 2);
	if( !(tmp & PCI_COMMAND_MEMORY) ) {
		dprintf("Mach64:: Enabling memory-space access\n");
		tmp |= PCI_COMMAND_MEMORY;
		write_pci_config(m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, PCI_COMMAND, 2, tmp);
	}
	info.frame_buffer_phys = m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	info.frame_buffer =m_pFramebufferBase;


	// populate screen mode list
	m_cScreenModeList.push_back (ScreenMode ( 640, 480, 1280, CS_RGB15));  // 640x480 HiColor
	m_cScreenModeList.push_back (ScreenMode ( 640, 480, 1280, CS_RGB16));  // 640x480 HiColor
	m_cScreenModeList.push_back (ScreenMode ( 640, 480, 2560, CS_RGB32)); // 640x480 TrueColor
	m_cScreenModeList.push_back (ScreenMode ( 800, 600, 1600, CS_RGB15));  // 800x600 HiColor
	m_cScreenModeList.push_back (ScreenMode ( 800, 600, 1600, CS_RGB16));  // 800x600 HiColor
	m_cScreenModeList.push_back (ScreenMode ( 800, 600, 3200, CS_RGB32)); // 800x600 TrueColor
	m_cScreenModeList.push_back (ScreenMode (1024, 768, 2048, CS_RGB15));  // 1024x768 HiColor
	m_cScreenModeList.push_back (ScreenMode (1024, 768, 2048, CS_RGB16));  // 1024x768 HiColor
	m_cScreenModeList.push_back (ScreenMode (1024, 768, 4096, CS_RGB32)); // 1024x768 TrueColor
	m_cScreenModeList.push_back (ScreenMode (1152, 864, 2304, CS_RGB15));  // 1152x864 HiColor
	m_cScreenModeList.push_back (ScreenMode (1152, 864, 2304, CS_RGB16));  // 1152x864 HiColor
	m_cScreenModeList.push_back (ScreenMode (1152, 864, 4608, CS_RGB32)); // 1152x864 TrueColor
	m_cScreenModeList.push_back (ScreenMode (1280, 1024, 2560, CS_RGB15));  // 1280x1024 HiColor
	m_cScreenModeList.push_back (ScreenMode (1280, 1024, 2560, CS_RGB16));  // 1280x1024 HiColor
	m_cScreenModeList.push_back (ScreenMode (1280, 1024, 5120, CS_RGB32)); // 1280x1024 TrueColor

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
			info.features = aty_chips[j].features;
			goto found;
		}
	dprintf("Mach64:: Unknown mach64 0x%04x\n", type);
	return false;

found:

	dprintf("Mach64:: %s [0x%04x rev 0x%02x]\n ", chipname, type, rev);

	if( !M64_HAS(INTEGRATED) ) {
		uint32 stat0;
		uint8 dac_type, dac_subtype, clk_type;
		stat0 = aty_ld_le32(CONFIG_STAT0);
		info.bus_type = (stat0 >> 0) & 0x07;
		info.ram_type = (stat0 >> 3) & 0x07;
		ramname = aty_gx_ram[info.ram_type];
		/* FIXME: clockchip/RAMDAC probing? */
		dac_type = (aty_ld_le32(DAC_CNTL) >> 16) & 0x07;

		dac_type = DAC_IBMRGB514;
		dac_subtype = DAC_IBMRGB514;
		clk_type = CLK_IBMRGB514;

		switch( dac_subtype ) {
			case DAC_IBMRGB514:
				info.dac_type = DAC_IBMRGB514;
				break;
			case DAC_ATI68860_B:
			case DAC_ATI68860_C:
				info.dac_type = DAC_ATI68860_C;
				break;
			case DAC_ATT20C408:
			case DAC_ATT21C498:
				info.dac_type = DAC_ATT21C498;
				break;
			default:
				dprintf("Mach64:: DAC type not implemented yet!\n");
				info.pll_type = DAC_UNSUPPORTED;
			break;
		}
		switch ( clk_type ) {
			case CLK_ATI18818_1:
				info.pll_type = CLK_ATI18818_1;
				break;
			case CLK_STG1703:
				info.pll_type = CLK_STG1703;
				break;
			case CLK_CH8398:
				info.pll_type = CLK_CH8398;
				break;
			case CLK_ATT20C408:
				info.pll_type = CLK_ATT20C408;
				break;
			case CLK_IBMRGB514:
				info.pll_type = CLK_IBMRGB514;
				break;
			default:
				dprintf("Mach64:: CLK type not implemented yet!");
				info.pll_type = CLK_UNSUPPORTED;
			break;
		}
	}

	if( M64_HAS(INTEGRATED) ) {
		info.bus_type = PCI;
		info.ram_type = (aty_ld_le32(CONFIG_STAT0) & 0x07);
		ramname = aty_ct_ram[info.ram_type];
		info.dac_type = DAC_INTERNAL;
		info.pll_type = CLK_INTERNAL;
		/* for many chips, the mclk is 67 MHz for SDRAM, 63 MHz otherwise */
		if( mclk == 67 && info.ram_type < SDRAM )
			mclk = 63;
	}

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
		switch( i & 0xF ) {	/* 0xF used instead of MEM_SIZE_ALIAS */
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

	if ( default_vram ) {
		info.total_vram = default_vram *1024;
		i = i & ~(gtb_memsize ? 0xF : MEM_SIZE_ALIAS);
		if (info.total_vram <= 0x80000)
			i |= MEM_SIZE_512K;
		else if (info.total_vram <= 0x100000)
			i |= MEM_SIZE_1M;
		else if (info.total_vram <= 0x200000)
			i |= gtb_memsize ? MEM_SIZE_2M_GTB : MEM_SIZE_2M;
		else if (info.total_vram <= 0x400000)
			i |= gtb_memsize ? MEM_SIZE_4M_GTB : MEM_SIZE_4M;
		else if (info.total_vram <= 0x600000)
			i |= gtb_memsize ? MEM_SIZE_6M_GTB : MEM_SIZE_6M;
		else
			i |= gtb_memsize ? MEM_SIZE_8M_GTB : MEM_SIZE_8M;
		aty_st_le32(MEM_CNTL, i);
	}
	if( default_pll )
		pll = default_pll;
 	if( default_mclk )
		mclk = default_mclk;

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

	/*
	*  Last page of 8 MB aperture can be MMIO
	*/
	if( info.total_vram == 0x800000 && info.auxiliary_aperture == 0 )
		info.total_vram -= GUI_RESERVE;

	wait_for_idle();


	/* Try to enable hardware cursor */
	HWcursor = false;
	if( M64_HAS(INTEGRATED) )
		aty_hw_cursor_init();

	return( true );
}

//=============================================================================
// NAME: ATImach64::ATImach64 ()
// DESC: ATImach64 driver class constructor
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
ATImach64::ATImach64() : m_cLock ("mach64_hardware_lock") {

	m_bIsInitialized = false;

	// scan all PCI devices
	for( int i = 0 ; get_pci_info( &m_cPCIInfo, i ) == 0 ; i++ ) {
		if( ProbeHardware (&m_cPCIInfo) ) { m_bIsInitialized = true; break; }
	}

	if( m_bIsInitialized == false ) {
		dbprintf("Mach64:: No supported cards found\n");
		return;
	}

	// OK. card found. now initialize it.
	if( !InitHardware () ) {
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
int ATImach64::SetScreenMode(int nWidth, int nHeight,
                          color_space eColorSpc,
                          int nPosH, int nPosV,
                          int nSizeH, int nSizeV,
                          float vRefreshRate) {

	m_nCurrentMode = -1;
	m_vCurrentRefresh = 0.0;

	dbprintf ("Mach64:: New video mode : %dx%d %dhz\n",
		nWidth, nHeight, (int)vRefreshRate);

	if( eColorSpc == CS_CMAP8 || eColorSpc == CS_RGB24 ) {
		dbprintf ("Mach64:: 8BPP or 24BPP modes are not supported!\n");
		return( -1 );
	}

	 /* find display mode */
	VideoMode * vmode = DefaultVideoModes;
	VideoMode * bestVMode = NULL;
	while( vmode->Width > 0 ) {
		if( vmode->Width  == nWidth &&
		vmode->Height == nHeight &&
		vmode->Refresh <= int (vRefreshRate) ) {
			// we have found mode with given size and compatible refresh
			if( bestVMode == NULL )
				bestVMode = vmode;
			else if( (int (vRefreshRate)-vmode->Refresh) <
			(int (vRefreshRate)-bestVMode->Refresh) )
				bestVMode = vmode;
		}
		vmode++;
	}

	if( !bestVMode ) {
		dbprintf ("Mach64:: Unable to find video mode %dx%d %dhz\n", nWidth, nHeight, int (vRefreshRate));
		return( -1 );
	}

	vmode = bestVMode;

	struct ati_par par;
	int err;
	uint32 i;

	memset (&par, 0, sizeof (ati_par));

	if( (err = aty_var_to_crtc(vmode, BitsPerPixel(eColorSpc), &par.crtc)) )
		return( err );

	switch( info.pll_type )
	{
		case CLK_IBMRGB514:
			err = aty_var_to_pll_514(vmode->Clock, par.crtc.bpp, &par.pll);
			break;
		case CLK_ATI18818_1:
			err = aty_var_to_pll_18818(vmode->Clock, par.crtc.bpp, &par.pll);
			break;
		case CLK_STG1703:
			err = aty_var_to_pll_1703(vmode->Clock, par.crtc.bpp, &par.pll);
			break;
		case CLK_CH8398:
			err = aty_var_to_pll_8398(vmode->Clock, par.crtc.bpp, &par.pll);
			break;
		case CLK_ATT20C408:
			err = aty_var_to_pll_408(vmode->Clock, par.crtc.bpp, &par.pll);
			break;
		case CLK_UNSUPPORTED:
			break;
		case CLK_INTERNAL:
			err = aty_var_to_pll_ct(vmode->Clock, par.crtc.bpp, &par.pll);
			break;
		default:
			return( -1 );
			break;
	}
	if( err ) return err;

	info.current_par = par;
	aty_set_crtc(&par.crtc);

	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, 0);
	/* better call aty_StrobeClock ?? */
	aty_st_8(CLOCK_CNTL + info.clk_wr_offset, CLOCK_STROBE);


	switch( info.dac_type )
	{
		case DAC_IBMRGB514:
			err = aty_set_dac_514(&par.pll, par.crtc.bpp, 0);
			break;
		case DAC_ATI68860_C:
			err = aty_set_dac_ATI68860_B(&par.pll, par.crtc.bpp, 0);
			break;
		case DAC_ATT21C498:
			err = aty_set_dac_ATT21C498(&par.pll, par.crtc.bpp, 0);
			break;
		case DAC_UNSUPPORTED:
			err = aty_set_dac_unsupported(&par.pll, par.crtc.bpp, 0);
			break;
		case DAC_INTERNAL:
			break;
		default:
			return( -1 );
			break;
	}
	if( err ) return err;

	switch( info.pll_type )
	{
		case CLK_IBMRGB514:
			aty_set_pll_514(&par.pll);
			break;
		case CLK_ATI18818_1:
			aty_set_pll18818(&par.pll);
			break;
		case CLK_STG1703:
			aty_set_pll_1703(&par.pll);
			break;
		case CLK_CH8398:
			aty_set_pll_8398(&par.pll);
			break;
		case CLK_ATT20C408:
			aty_set_pll_408(&par.pll);
			break;
		case CLK_INTERNAL:
			aty_set_pll_ct(&par.pll);
			break;
		default:
			return( -1 );
			break;
	}

	if( !M64_HAS(INTEGRATED) ) {
		/* Don't forget MEM_CNTL */
		i = aty_ld_le32(MEM_CNTL) & 0xf0ffffff;
		switch( par.crtc.bpp ) {
			case 8:
				i |= 0x02000000;
			break;
			case 15:
			case 16:
				i |= 0x03000000;
			break;
			case 32:
				i |= 0x06000000;
			break;
		}
		aty_st_le32(MEM_CNTL, i);
	} else {
		i = aty_ld_le32(MEM_CNTL) & 0xf00fffff;
		if( !M64_HAS(MAGIC_POSTDIV) )
			i |= info.mem_refresh_rate << 20;
		switch ( par.crtc.bpp ) {
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
	}

	aty_st_8(DAC_MASK, 0xff);

	/* Color registers */
	uint32 j;
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

	init_engine(&par);

	wait_for_idle();

	info.current_par = par;

	/* initialize current video mode variables */
	for( j = 0; j < m_cScreenModeList.size (); j++ ) {
		if( m_cScreenModeList[j].m_nWidth == nWidth &&
			m_cScreenModeList[j].m_nHeight == nHeight &&
			m_cScreenModeList[j].m_eColorSpace == eColorSpc ) {
			// found video mode
			m_nCurrentMode = j;
			m_vCurrentRefresh = vRefreshRate;
			break;
		}
	}
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
DisplayDriver* init_gfx_driver() {

	try {
		ATImach64* pcDriver = new ATImach64();
		if (pcDriver->IsInitialized()) {
			return pcDriver;
		} else {
			delete pcDriver;
			return NULL ;
		}
	} catch (exception&  cExc) {
		dbprintf( "Mach64:: Got exception\n" );
		return NULL ;
	}
}

//-----------------------------------------------------------------------------
// *** end of file ***
