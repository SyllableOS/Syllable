#ifndef _SIS_H
#define _SIS_H

#define MAX_ROM_SCAN              0x10000


/* For 300 series */
#define TURBO_QUEUE_AREA_SIZE     0x80000 /* 512K */

/* For 315 series */
#define COMMAND_QUEUE_AREA_SIZE   0x80000 /* 512K */
#define COMMAND_QUEUE_THRESHOLD   0x1F

/* TW */
#define HW_CURSOR_AREA_SIZE_315   0x4000  /* 16K */
#define HW_CURSOR_AREA_SIZE_300   0x1000  /* 4K */


#define SEQ_ADR                   0x14
#define SEQ_DATA                  0x15
#define DAC_ADR                   0x18
#define DAC_DATA                  0x19
#define CRTC_ADR                  0x24
#define CRTC_DATA                 0x25
#define DAC2_ADR                  (0x16-0x30)
#define DAC2_DATA                 (0x17-0x30)
#define VB_PART1_ADR              (0x04-0x30)
#define VB_PART1_DATA             (0x05-0x30)
#define VB_PART2_ADR              (0x10-0x30)
#define VB_PART2_DATA             (0x11-0x30)
#define VB_PART3_ADR              (0x12-0x30)
#define VB_PART3_DATA             (0x13-0x30)
#define VB_PART4_ADR              (0x14-0x30)
#define VB_PART4_DATA             (0x15-0x30)

#define SISSR			  SiS_Pr.SiS_P3c4
#define SISCR                     SiS_Pr.SiS_P3d4
#define SISDACA                   SiS_Pr.SiS_P3c8
#define SISDACD                   SiS_Pr.SiS_P3c9
#define SISPART1                  SiS_Pr.SiS_Part1Port
#define SISPART2                  SiS_Pr.SiS_Part2Port
#define SISPART3                  SiS_Pr.SiS_Part3Port
#define SISPART4                  SiS_Pr.SiS_Part4Port
#define SISPART5                  SiS_Pr.SiS_Part5Port
#define SISDAC2A                  SISPART5
#define SISDAC2D                  (SISPART5 + 1)
#define SISMISCR                  (SiS_Pr.RelIO + 0x1c)

#define IND_SIS_PASSWORD          0x05  /* SRs */
#define IND_SIS_COLOR_MODE        0x06
#define IND_SIS_RAMDAC_CONTROL    0x07
#define IND_SIS_DRAM_SIZE         0x14
#define IND_SIS_SCRATCH_REG_16    0x16
#define IND_SIS_SCRATCH_REG_17    0x17
#define IND_SIS_SCRATCH_REG_1A    0x1A
#define IND_SIS_MODULE_ENABLE     0x1E
#define IND_SIS_PCI_ADDRESS_SET   0x20
#define IND_SIS_TURBOQUEUE_ADR    0x26
#define IND_SIS_TURBOQUEUE_SET    0x27
#define IND_SIS_POWER_ON_TRAP     0x38
#define IND_SIS_POWER_ON_TRAP2    0x39
#define IND_SIS_CMDQUEUE_SET      0x26
#define IND_SIS_CMDQUEUE_THRESHOLD  0x27

#define IND_SIS_SCRATCH_REG_CR30  0x30  /* CRs */
#define IND_SIS_SCRATCH_REG_CR31  0x31
#define IND_SIS_SCRATCH_REG_CR32  0x32
#define IND_SIS_SCRATCH_REG_CR33  0x33
#define IND_SIS_LCD_PANEL         0x36
#define IND_SIS_SCRATCH_REG_CR37  0x37
#define IND_SIS_AGP_IO_PAD        0x48

#define IND_BRI_DRAM_STATUS       0x63 /* PCI config memory size offset */

#define MMIO_QUEUE_PHYBASE        0x85C0
#define MMIO_QUEUE_WRITEPORT      0x85C4
#define MMIO_QUEUE_READPORT       0x85C8

#define IND_SIS_CRT2_WRITE_ENABLE_300 0x24
#define IND_SIS_CRT2_WRITE_ENABLE_315 0x2F

#define SIS_PASSWORD              0x86  /* SR05 */
#define SIS_INTERLACED_MODE       0x20  /* SR06 */
#define SIS_8BPP_COLOR_MODE       0x0 
#define SIS_15BPP_COLOR_MODE      0x1 
#define SIS_16BPP_COLOR_MODE      0x2 
#define SIS_32BPP_COLOR_MODE      0x4 
#define SIS_DRAM_SIZE_MASK        0x3F  /* 300/630/730 SR14 */
#define SIS_DRAM_SIZE_1MB         0x00
#define SIS_DRAM_SIZE_2MB         0x01
#define SIS_DRAM_SIZE_4MB         0x03
#define SIS_DRAM_SIZE_8MB         0x07
#define SIS_DRAM_SIZE_16MB        0x0F
#define SIS_DRAM_SIZE_32MB        0x1F
#define SIS_DRAM_SIZE_64MB        0x3F
#define SIS_DATA_BUS_MASK         0xC0
#define SIS_DATA_BUS_32           0x00
#define SIS_DATA_BUS_64           0x01
#define SIS_DATA_BUS_128          0x02

#define SIS315_DRAM_SIZE_MASK     0xF0  /* 315 SR14 */
#define SIS315_DRAM_SIZE_2MB      0x01
#define SIS315_DRAM_SIZE_4MB      0x02
#define SIS315_DRAM_SIZE_8MB      0x03
#define SIS315_DRAM_SIZE_16MB     0x04
#define SIS315_DRAM_SIZE_32MB     0x05
#define SIS315_DRAM_SIZE_64MB     0x06
#define SIS315_DRAM_SIZE_128MB    0x07
#define SIS315_DATA_BUS_MASK      0x02
#define SIS315_DATA_BUS_64        0x00
#define SIS315_DATA_BUS_128       0x01
#define SIS315_DUAL_CHANNEL_MASK  0x0C
#define SIS315_SINGLE_CHANNEL_1_RANK  	0x0
#define SIS315_SINGLE_CHANNEL_2_RANK  	0x1
#define SIS315_ASYM_DDR		  	0x02
#define SIS315_DUAL_CHANNEL_1_RANK    	0x3

#define SIS550_DRAM_SIZE_MASK     0x3F  /* 550/650/740 SR14 */
#define SIS550_DRAM_SIZE_4MB      0x00
#define SIS550_DRAM_SIZE_8MB      0x01
#define SIS550_DRAM_SIZE_16MB     0x03
#define SIS550_DRAM_SIZE_24MB     0x05
#define SIS550_DRAM_SIZE_32MB     0x07
#define SIS550_DRAM_SIZE_64MB     0x0F
#define SIS550_DRAM_SIZE_96MB     0x17
#define SIS550_DRAM_SIZE_128MB    0x1F
#define SIS550_DRAM_SIZE_256MB    0x3F

#define SIS_SCRATCH_REG_1A_MASK   0x10

#define SIS_ENABLE_2D             0x40  /* SR1E */

#define SIS_MEM_MAP_IO_ENABLE     0x01  /* SR20 */
#define SIS_PCI_ADDR_ENABLE       0x80

#define SIS_AGP_CMDQUEUE_ENABLE   0x80  /* 315/650/740 SR26 */
#define SIS_VRAM_CMDQUEUE_ENABLE  0x40
#define SIS_MMIO_CMD_ENABLE       0x20
#define SIS_CMD_QUEUE_SIZE_512k   0x00
#define SIS_CMD_QUEUE_SIZE_1M     0x04
#define SIS_CMD_QUEUE_SIZE_2M     0x08
#define SIS_CMD_QUEUE_SIZE_4M     0x0C
#define SIS_CMD_QUEUE_RESET       0x01
#define SIS_CMD_AUTO_CORR	  0x02

#define SIS_SIMULTANEOUS_VIEW_ENABLE  0x01  /* CR30 */
#define SIS_MODE_SELECT_CRT2      0x02
#define SIS_VB_OUTPUT_COMPOSITE   0x04
#define SIS_VB_OUTPUT_SVIDEO      0x08
#define SIS_VB_OUTPUT_SCART       0x10
#define SIS_VB_OUTPUT_LCD         0x20
#define SIS_VB_OUTPUT_CRT2        0x40
#define SIS_VB_OUTPUT_HIVISION    0x80

#define SIS_VB_OUTPUT_DISABLE     0x20  /* CR31 */
#define SIS_DRIVER_MODE           0x40

#define SIS_VB_COMPOSITE          0x01  /* CR32 */
#define SIS_VB_SVIDEO             0x02
#define SIS_VB_SCART              0x04
#define SIS_VB_LCD                0x08
#define SIS_VB_CRT2               0x10
#define SIS_CRT1                  0x20
#define SIS_VB_HIVISION           0x40
#define SIS_VB_DVI                0x80
#define SIS_VB_TV                 (SIS_VB_COMPOSITE | SIS_VB_SVIDEO | \
                                   SIS_VB_SCART | SIS_VB_HIVISION)

#define SIS_EXTERNAL_CHIP_MASK    	   0x0E  /* CR37 */
#define SIS_EXTERNAL_CHIP_SIS301           0x01  /* in CR37 << 1 ! */
#define SIS_EXTERNAL_CHIP_LVDS             0x02  /* in CR37 << 1 ! */
#define SIS_EXTERNAL_CHIP_TRUMPION         0x03  /* in CR37 << 1 ! */
#define SIS_EXTERNAL_CHIP_LVDS_CHRONTEL    0x04  /* in CR37 << 1 ! */
#define SIS_EXTERNAL_CHIP_CHRONTEL         0x05  /* in CR37 << 1 ! */
#define SIS310_EXTERNAL_CHIP_LVDS          0x02  /* in CR37 << 1 ! */
#define SIS310_EXTERNAL_CHIP_LVDS_CHRONTEL 0x03  /* in CR37 << 1 ! */

#define SIS_AGP_2X                0x20  /* CR48 */

#define BRI_DRAM_SIZE_MASK        0x70  /* PCI bridge config data */
#define BRI_DRAM_SIZE_2MB         0x00
#define BRI_DRAM_SIZE_4MB         0x01
#define BRI_DRAM_SIZE_8MB         0x02
#define BRI_DRAM_SIZE_16MB        0x03
#define BRI_DRAM_SIZE_32MB        0x04
#define BRI_DRAM_SIZE_64MB        0x05

#define HW_DEVICE_EXTENSION	  SIS_HW_DEVICE_INFO
#define PHW_DEVICE_EXTENSION      PSIS_HW_DEVICE_INFO

#define SR_BUFFER_SIZE            5
#define CR_BUFFER_SIZE            5

#define DISPTYPE_CRT1       0x00000008L
#define DISPTYPE_CRT2       0x00000004L
#define DISPTYPE_LCD        0x00000002L
#define DISPTYPE_TV         0x00000001L
#define DISPTYPE_DISP1      DISPTYPE_CRT1
#define DISPTYPE_DISP2      (DISPTYPE_CRT2 | DISPTYPE_LCD | DISPTYPE_TV)
#define DISPMODE_SINGLE	    0x00000020L
#define DISPMODE_MIRROR	    0x00000010L
#define DISPMODE_DUALVIEW   0x00000040L

#define HASVB_NONE      	0x00
#define HASVB_301       	0x01
#define HASVB_LVDS      	0x02
#define HASVB_TRUMPION  	0x04
#define HASVB_LVDS_CHRONTEL	0x10
#define HASVB_302       	0x20
#define HASVB_303       	0x40
#define HASVB_CHRONTEL  	0x80

/* TW: *Never* change the order of the following enum */
typedef enum _SIS_CHIP_TYPE {
	SIS_VGALegacy = 0,
	SIS_300,
	SIS_630,
	SIS_540,
	SIS_730, 
	SIS_315H,
	SIS_315,
	SIS_315PRO,
	SIS_550,
	SIS_650,
	SIS_740,
	SIS_330,
	MAX_SIS_CHIP
} SIS_CHIP_TYPE;

typedef enum _VGA_ENGINE {
	UNKNOWN_VGA = 0,
	SIS_300_VGA,
	SIS_315_VGA,
} VGA_ENGINE;

typedef enum _TVTYPE {
	TVMODE_NTSC = 0,
	TVMODE_PAL,
	TVMODE_HIVISION,
	TVMODE_TOTAL
} SIS_TV_TYPE;

typedef enum _TVPLUGTYPE {
	TVPLUG_Legacy = 0,
	TVPLUG_COMPOSITE,
	TVPLUG_SVIDEO,
	TVPLUG_SCART,
	TVPLUG_TOTAL
} SIS_TV_PLUG;

typedef enum _SIS_CMDTYPE {
	MMIO_CMD = 0,
	AGP_CMD_QUEUE,
	VM_CMD_QUEUE,
} SIS_CMDTYPE;

#define MD_SIS300 SIS_300_VGA
#define MD_SIS315 SIS_315_VGA

/* mode table */
/* NOT const - will be patched for 1280x960 mode number chaos reasons */
struct _sisbios_mode {
	char name[15];
	uint8 mode_no;
	uint16 vesa_mode_no_1;  /* "SiS defined" VESA mode number */
	uint16 vesa_mode_no_2;  /* Real VESA mode numbers */
	uint16 xres;
	uint16 yres;
	uint16 bpp;
	uint16 rate_idx;
	uint16 cols;
	uint16 rows;
	uint8  chipset;
};

extern struct _sisbios_mode sisbios_mode[];


/* TW: CR36 evaluation */
static const unsigned short sis300paneltype[] =
    { LCD_UNKNOWN,   LCD_800x600,  LCD_1024x768,  LCD_1280x1024,
      LCD_1280x960,  LCD_640x480,  LCD_1024x600,  LCD_1152x768,
      LCD_320x480,   LCD_1024x768, LCD_1024x768,  LCD_1024x768,
      LCD_1024x768,  LCD_1024x768, LCD_1024x768,  LCD_1024x768 };

static const unsigned short sis310paneltype[] =
    { LCD_UNKNOWN,   LCD_800x600,  LCD_1024x768,  LCD_1280x1024,
      LCD_640x480,   LCD_1024x600, LCD_1152x864,  LCD_1280x960,
      LCD_1152x768,  LCD_1400x1050,LCD_1280x768,  LCD_1600x1200,
      LCD_320x480,   LCD_1024x768, LCD_1024x768,  LCD_1024x768 };
      
static const struct _sis_crt2type {
	char name[10];
	int type_no;
	int tvplug_no;
} sis_crt2type[] = {
	{"NONE", 	0, 		-1},
	{"LCD",  	DISPTYPE_LCD, 	-1},
	{"TV",   	DISPTYPE_TV, 	-1},
	{"VGA",  	DISPTYPE_CRT2, 	-1},
	{"SVIDEO", 	DISPTYPE_TV, 	TVPLUG_SVIDEO},
	{"COMPOSITE", 	DISPTYPE_TV, 	TVPLUG_COMPOSITE},
	{"SCART", 	DISPTYPE_TV, 	TVPLUG_SCART},
	{"none", 	0, 		-1},
	{"lcd",  	DISPTYPE_LCD, 	-1},
	{"tv",   	DISPTYPE_TV, 	-1},
	{"vga",  	DISPTYPE_CRT2, 	-1},
	{"svideo", 	DISPTYPE_TV, 	TVPLUG_SVIDEO},
	{"composite", 	DISPTYPE_TV, 	TVPLUG_COMPOSITE},
	{"scart", 	DISPTYPE_TV, 	TVPLUG_SCART},
	{"\0",  	-1, 		-1}
};

/* Queue mode selection for 310 series */
static const struct _sis_queuemode {
	char name[6];
	int type_no;
} sis_queuemode[] = {
	{"AGP",  	AGP_CMD_QUEUE},
	{"VRAM", 	VM_CMD_QUEUE},
	{"MMIO", 	MMIO_CMD},
	{"agp",  	AGP_CMD_QUEUE},
	{"vram", 	VM_CMD_QUEUE},
	{"mmio", 	MMIO_CMD},
	{"\0",   	-1}
};

/* TV standard */
static const struct _sis_tvtype {
	char name[6];
	int type_no;
} sis_tvtype[] = {
	{"PAL",  	1},
	{"NTSC", 	2},
	{"pal", 	1},
	{"ntsc",  	2},
	{"\0",   	-1}
};

static const struct _sis_vrate {
	unsigned idx;
	uint16 xres;
	uint16 yres;
	uint16 refresh;
} sis_vrate[] = {
	{1,  640,  480, 60}, {2,  640,  480,  72}, {3, 640,   480,  75}, {4,  640, 480,  85},
	{5,  640,  480,100}, {6,  640,  480, 120}, {7, 640,   480, 160}, {8,  640, 480, 200},
	{1,  720,  480, 60},
	{1,  720,  576, 58},
	{1,  800,  480, 60}, {2,  800,  480,  75}, {3, 800,   480,  85},
	{1,  800,  600, 56}, {2,  800,  600,  60}, {3, 800,   600,  72}, {4,  800, 600,  75},
	{5,  800,  600, 85}, {6,  800,  600, 100}, {7, 800,   600, 120}, {8,  800, 600, 160},
	{1, 1024,  768, 43}, {2, 1024,  768,  60}, {3, 1024,  768,  70}, {4, 1024, 768,  75},
	{5, 1024,  768, 85}, {6, 1024,  768, 100}, {7, 1024,  768, 120},
	{1, 1024,  576, 60}, {2, 1024,  576,  75}, {3, 1024,  576,  85},
	{1, 1024,  600, 60},
	{1, 1152,  768, 60},
	{1, 1280,  720, 60}, {2, 1280,  720,  75}, {3, 1280,  720,  85},
	{1, 1280,  768, 60},
	{1, 1280, 1024, 43}, {2, 1280, 1024,  60}, {3, 1280, 1024,  75}, {4, 1280, 1024,  85},
	{1, 1280,  960, 70},
	{1, 1400, 1050, 60},
	{1, 1600, 1200, 60}, {2, 1600, 1200,  65}, {3, 1600, 1200,  70}, {4, 1600, 1200,  75},
	{5, 1600, 1200, 85}, {6, 1600, 1200, 100}, {7, 1600, 1200, 120},
	{1, 1920, 1440, 60}, {2, 1920, 1440,  65}, {3, 1920, 1440,  70}, {4, 1920, 1440,  75},
	{5, 1920, 1440, 85}, {6, 1920, 1440, 100},
	{1, 2048, 1536, 60}, {2, 2048, 1536,  65}, {3, 2048, 1536,  70}, {4, 2048, 1536,  75},
	{5, 2048, 1536, 85},
	{0, 0, 0, 0}
};

static const struct _chswtable {
    int subsysVendor;
    int subsysCard;
    char *vendorName;
    char *cardName;
} mychswtable[] = {
        { 0x1631, 0x1002, "Mitachi", "0x1002" },
	{ 0,      0,      ""       , ""       }
};

// Eden Chen
static const struct _sis_TV_filter {
	uint8 filter[9][4];
} sis_TV_filter[] = {
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_0 */
	   {0x00,0xE0,0x10,0x60},
	   {0x00,0xEE,0x10,0x44},
	   {0x00,0xF4,0x10,0x38},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0x00,0x00,0x10,0x20},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_1 */
	   {0x00,0xE0,0x10,0x60},
	   {0x00,0xEE,0x10,0x44},
	   {0x00,0xF4,0x10,0x38},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0x00,0x00,0x10,0x20},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_2 */
	   {0xF5,0xEE,0x1B,0x44},
	   {0xF8,0xF4,0x18,0x38},
	   {0xEB,0x04,0x25,0x18},
	   {0xF1,0x05,0x1F,0x16},
	   {0xF6,0x06,0x1A,0x14},
	   {0xFA,0x06,0x16,0x14},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_3 */
	   {0xF1,0x04,0x1F,0x18},
	   {0xEE,0x0D,0x22,0x06},
	   {0xF7,0x06,0x19,0x14},
	   {0xF4,0x0B,0x1C,0x0A},
	   {0xFA,0x07,0x16,0x12},
	   {0xF9,0x0A,0x17,0x0C},
	   {0x00,0x07,0x10,0x12}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_4 */
	   {0x00,0xE0,0x10,0x60},
	   {0x00,0xEE,0x10,0x44},
	   {0x00,0xF4,0x10,0x38},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0x00,0x00,0x10,0x20},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_5 */
	   {0xF5,0xEE,0x1B,0x44},
	   {0xF8,0xF4,0x18,0x38},
	   {0xEB,0x04,0x25,0x18},
	   {0xF1,0x05,0x1F,0x16},
	   {0xF6,0x06,0x1A,0x14},
	   {0xFA,0x06,0x16,0x14},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_6 */
	   {0xEB,0x04,0x25,0x18},
	   {0xE7,0x0E,0x29,0x04},
	   {0xEE,0x0C,0x22,0x08},
	   {0xF6,0x0B,0x1A,0x0A},
	   {0xF9,0x0A,0x17,0x0C},
	   {0xFC,0x0A,0x14,0x0C},
	   {0x00,0x08,0x10,0x10}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* NTSCFilter_7 */
	   {0xEC,0x02,0x24,0x1C},
	   {0xF2,0x04,0x1E,0x18},
	   {0xEB,0x15,0x25,0xF6},
	   {0xF4,0x10,0x1C,0x00},
	   {0xF8,0x0F,0x18,0x02},
	   {0x00,0x04,0x10,0x18},
	   {0x01,0x06,0x0F,0x14}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_0 */
	   {0x00,0xE0,0x10,0x60},
	   {0x00,0xEE,0x10,0x44},
	   {0x00,0xF4,0x10,0x38},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0x00,0x00,0x10,0x20},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_1 */
	   {0x00,0xE0,0x10,0x60},
	   {0x00,0xEE,0x10,0x44},
	   {0x00,0xF4,0x10,0x38},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0x00,0x00,0x10,0x20},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_2 */
	   {0xF5,0xEE,0x1B,0x44},
	   {0xF8,0xF4,0x18,0x38},
	   {0xF1,0xF7,0x01,0x32},
	   {0xF5,0xFB,0x1B,0x2A},
	   {0xF9,0xFF,0x17,0x22},
	   {0xFB,0x01,0x15,0x1E},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_3 */
	   {0xF5,0xFB,0x1B,0x2A},
	   {0xEE,0xFE,0x22,0x24},
	   {0xF3,0x00,0x1D,0x20},
	   {0xF9,0x03,0x17,0x1A},
	   {0xFB,0x02,0x14,0x1E},
	   {0xFB,0x04,0x15,0x18},
	   {0x00,0x06,0x10,0x14}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_4 */
	   {0x00,0xE0,0x10,0x60},
	   {0x00,0xEE,0x10,0x44},
	   {0x00,0xF4,0x10,0x38},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0x00,0x00,0x10,0x20},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_5 */
	   {0xF5,0xEE,0x1B,0x44},
	   {0xF8,0xF4,0x18,0x38},
	   {0xF1,0xF7,0x1F,0x32},
	   {0xF5,0xFB,0x1B,0x2A},
	   {0xF9,0xFF,0x17,0x22},
	   {0xFB,0x01,0x15,0x1E},
	   {0x00,0x04,0x10,0x18}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_6 */
	   {0xF5,0xEE,0x1B,0x2A},
	   {0xEE,0xFE,0x22,0x24},
	   {0xF3,0x00,0x1D,0x20},
	   {0xF9,0x03,0x17,0x1A},
	   {0xFB,0x02,0x14,0x1E},
	   {0xFB,0x04,0x15,0x18},
	   {0x00,0x06,0x10,0x14}, 
	   {0xFF,0xFF,0xFF,0xFF} }},
	{ {{0x00,0x00,0x00,0x40},  /* PALFilter_7 */
	   {0xF5,0xEE,0x1B,0x44},
	   {0xF8,0xF4,0x18,0x38},
	   {0xFC,0xFB,0x14,0x2A},
	   {0xEB,0x05,0x25,0x16},
	   {0xF1,0x05,0x1F,0x16},
	   {0xFA,0x07,0x16,0x12},
	   {0x00,0x07,0x10,0x12}, 
	   {0xFF,0xFF,0xFF,0xFF} }}
};

/* Information shared between the c++ syllable
	driver and the c sis driver */

typedef struct
{
	PCI_Info_s pci_dev;
	uint32	device_id;
	char	name[30];

	uint32	io_base;
	uint8	*framebuffer;
	vuint32 *regs;
	void 	*rom;
	
	SIS_CHIP_TYPE chip;
	VGA_ENGINE	vga_engine;
	uint32		mem_size;
	
	unsigned long disp_state;
	unsigned char hasVB;
	unsigned char TV_type;
	unsigned char TV_plug;
	int	CRT2_enable;
	uint32 AccelDepth;
	uint16 DstColor;
	uint32 CmdReg;
	int filter;
	unsigned char filter_tb;
} shared_info;


/* Useful macros */
#define inSISREG(base)          inb(base)
#define outSISREG(base,val)     outb(val,base)
#define orSISREG(base,val)      do { \
                                  unsigned char __Temp = inb(base); \
                                  outSISREG(base, __Temp | (val)); \
                                } while (0)
#define andSISREG(base,val)     do { \
                                  unsigned char __Temp = inb(base); \
                                  outSISREG(base, __Temp & (val)); \
                                } while (0)
#define inSISIDXREG(base,idx,var)   do { \
                                      outb(idx,base); var=inb((base)+1); \
                                    } while (0)
#define outSISIDXREG(base,idx,val)  do { \
                                      outb(idx,base); outb((val),(base)+1); \
                                    } while (0)
#define orSISIDXREG(base,idx,val)   do { \
                                      unsigned char __Temp; \
                                      outb(idx,base);   \
                                      __Temp = inb((base)+1)|(val); \
                                      outSISIDXREG(base,idx,__Temp); \
                                    } while (0)
#define andSISIDXREG(base,idx,and)  do { \
                                      unsigned char __Temp; \
                                      outb(idx,base);   \
                                      __Temp = inb((base)+1)&(and); \
                                      outSISIDXREG(base,idx,__Temp); \
                                    } while (0)
#define setSISIDXREG(base,idx,and,or)   do { \
                                          unsigned char __Temp; \
                                          outb(idx,base);   \
                                          __Temp = (inb((base)+1)&(and))|(or); \
                                          outSISIDXREG(base,idx,__Temp); \
                                        } while (0)
      
/* Some global stuff */                                  
                                        
extern SiS_Private SiS_Pr;
extern HW_DEVICE_EXTENSION sishw_ext;
extern shared_info si;

/* sis.c */

void sis_pre_setmode( uint8 rate_idx );
void sis_post_setmode( int bpp, int xres );
int sis_init();

#endif






