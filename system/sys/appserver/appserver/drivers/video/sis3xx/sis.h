#ifndef _SIS_H
#define _SIS_H

#include "osdef.h"
#include "vgatypes.h"
#include "vstruct.h"

#define SIS_IOTYPE1 unsigned char

#define	UNKNOWN_VGA  0
#define	SIS_300_VGA  1
#define	SIS_315_VGA  2

#define MAX_ROM_SCAN              0x10000


/* For 300 series */
#define TURBO_QUEUE_AREA_SIZE     0x80000 /* 512K */
#define HW_CURSOR_AREA_SIZE_300   0x1000  /* 4K */

/* For 315 series */
#define COMMAND_QUEUE_AREA_SIZE   0x80000 /* 512K */
#define COMMAND_QUEUE_THRESHOLD   0x1F
#define HW_CURSOR_AREA_SIZE_315   0x4000  /* 16K */
#define COMMAND_QUEUE_THRESHOLD	0x1F

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

#define SISSR			  		  SiS_Pr.SiS_P3c4
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
#define SISMISCW                  SiS_Pr.SiS_P3c2
#define SISINPSTAT				  (SiS_Pr.RelIO + 0x2a)
#define SISPEL			          SiS_Pr.SiS_P3c6
#define SISVGAENABLE              (SiS_Pr.SiS_Pr.RelIO + 0x13)
#define SISVID					  (SiS_Pr.RelIO + 0x02 - 0x30)
#define SISCAP                    (SiS_Pr.SiS_Pr.RelIO + 0x00 - 0x30 )

#define IND_SIS_PASSWORD          0x05  /* SRs */
#define IND_SIS_COLOR_MODE        0x06
#define IND_SIS_RAMDAC_CONTROL    0x07
#define IND_SIS_DRAM_SIZE         0x14
#define IND_SIS_MODULE_ENABLE     0x1E
#define IND_SIS_PCI_ADDRESS_SET   0x20
#define IND_SIS_TURBOQUEUE_ADR    0x26
#define IND_SIS_TURBOQUEUE_SET    0x27
#define IND_SIS_POWER_ON_TRAP     0x38
#define IND_SIS_POWER_ON_TRAP2    0x39
#define IND_SIS_CMDQUEUE_SET      0x26
#define IND_SIS_CMDQUEUE_THRESHOLD  0x27

#define IND_SIS_AGP_IO_PAD        0x48

#define SIS_CRT2_WENABLE_300 	  0x24  /* Part1 */
#define SIS_CRT2_WENABLE_315 	  0x2F

#define SIS_PASSWORD              0x86  /* SR05 */

#define SIS_INTERLACED_MODE       0x20  /* SR06 */
#define SIS_8BPP_COLOR_MODE       0x0
#define SIS_15BPP_COLOR_MODE      0x1
#define SIS_16BPP_COLOR_MODE      0x2
#define SIS_32BPP_COLOR_MODE      0x4

#define SIS_ENABLE_2D             0x40  /* SR1E */

#define SIS_MEM_MAP_IO_ENABLE     0x01  /* SR20 */
#define SIS_PCI_ADDR_ENABLE       0x80

#define SIS_AGP_CMDQUEUE_ENABLE   0x80  /* 315/330 series SR26 */
#define SIS_VRAM_CMDQUEUE_ENABLE  0x40
#define SIS_MMIO_CMD_ENABLE       0x20
#define SIS_CMD_QUEUE_SIZE_512k   0x00
#define SIS_CMD_QUEUE_SIZE_1M     0x04
#define SIS_CMD_QUEUE_SIZE_2M     0x08
#define SIS_CMD_QUEUE_SIZE_4M     0x0C
#define SIS_CMD_QUEUE_RESET       0x01
#define SIS_CMD_AUTO_CORR	  0x02

#define SIS_CMD_QUEUE_SIZE_Z7_64k	0x00 /* XGI Z7 */
#define SIS_CMD_QUEUE_SIZE_Z7_128k	0x04

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
#define SIS_VB_YPBPR              0x80
#define SIS_VB_TV                 (SIS_VB_COMPOSITE | SIS_VB_SVIDEO | \
                                   SIS_VB_SCART | SIS_VB_HIVISION | SIS_VB_YPBPR)

#define SIS_EXTERNAL_CHIP_MASK    	   0x0E  /* CR37 (< SiS 660) */
#define SIS_EXTERNAL_CHIP_SIS301           0x01  /* in CR37 << 1 ! */
#define SIS_EXTERNAL_CHIP_LVDS             0x02
#define SIS_EXTERNAL_CHIP_TRUMPION         0x03
#define SIS_EXTERNAL_CHIP_LVDS_CHRONTEL    0x04
#define SIS_EXTERNAL_CHIP_CHRONTEL         0x05
#define SIS310_EXTERNAL_CHIP_LVDS          0x02
#define SIS310_EXTERNAL_CHIP_LVDS_CHRONTEL 0x03

#define SIS_AGP_2X		0x20  /* CR48 */


/*
 *  CRT_2 function control register ---------------------------------
 */
#define  Index_CRT2_FC_CONTROL                  0x00
#define  Index_CRT2_FC_SCREEN_HIGH              0x04
#define  Index_CRT2_FC_SCREEN_MID               0x05
#define  Index_CRT2_FC_SCREEN_LOW               0x06
#define  Index_CRT2_FC_ENABLE_WRITE             0x24
#define  Index_CRT2_FC_VR                       0x25
#define  Index_CRT2_FC_VCount                   0x27
#define  Index_CRT2_FC_VCount1                  0x28

#define  Index_310_CRT2_FC_VR                   0x30  /* d[1] = vertical retrace */
#define  Index_310_CRT2_FC_RT			0x33  /* d[7] = retrace in progress */


#define MMIO_QUEUE_PHYBASE        0x85C0
#define MMIO_QUEUE_WRITEPORT      0x85C4
#define MMIO_QUEUE_READPORT       0x85C8

#define IND_SIS_CRT2_WRITE_ENABLE_300 0x24
#define IND_SIS_CRT2_WRITE_ENABLE_315 0x2F


/* vbflags, private entries */
#define VB_CONEXANT		0x00000800	/* 661 series only */
#define VB_TRUMPION		VB_CONEXANT	/* 300 series only */
#define VB_302ELV		0x00004000
#define VB_301			0x00100000	/* Video bridge type */
#define VB_301B			0x00200000
#define VB_302B			0x00400000
#define VB_30xBDH		0x00800000	/* 30xB DH version (w/o LCD support) */
#define VB_LVDS			0x01000000
#define VB_CHRONTEL		0x02000000
#define VB_301LV		0x04000000
#define VB_302LV		0x08000000
#define VB_301C			0x10000000


#define VB_SISBRIDGE		(VB_301|VB_301B|VB_301C|VB_302B|VB_301LV|VB_302LV|VB_302ELV)
#define VB_VIDEOBRIDGE		(VB_SISBRIDGE | VB_LVDS | VB_CHRONTEL | VB_CONEXANT)

/* vbflags, public */
#define CRT2_DEFAULT		0x00000001
#define CRT2_LCD		0x00000002
#define CRT2_TV			0x00000004
#define CRT2_VGA		0x00000008
#define TV_NTSC			0x00000010
#define TV_PAL			0x00000020
#define TV_HIVISION		0x00000040
#define TV_YPBPR		0x00000080
#define TV_AVIDEO		0x00000100
#define TV_SVIDEO		0x00000200
#define TV_SCART		0x00000400
#define TV_PALM			0x00001000
#define TV_PALN			0x00002000
#define TV_NTSCJ		0x00001000
#define TV_CHSCART		0x00008000
#define TV_CHYPBPR525I		0x00010000
#define CRT1_VGA		0x00000000
#define CRT1_LCDA		0x00020000
#define VGA2_CONNECTED          0x00040000
#define VB_DISPTYPE_CRT1	0x00080000	/* CRT1 connected and used */
#define VB_SINGLE_MODE		0x20000000	/* CRT1 or CRT2; determined by DISPTYPE_CRTx */
#define VB_MIRROR_MODE		0x40000000	/* CRT1 + CRT2 identical (mirror mode) */
#define VB_DUALVIEW_MODE	0x80000000	/* CRT1 + CRT2 independent (dual head mode) */

/* vbflags2 (static stuff only!) */
#define VB2_SISUMC		0x00000001
#define VB2_301			0x00000002	/* Video bridge type */
#define VB2_301B		0x00000004
#define VB2_301C		0x00000008
#define VB2_307T		0x00000010
#define VB2_302B		0x00000800
#define VB2_301LV		0x00001000
#define VB2_302LV		0x00002000
#define VB2_302ELV		0x00004000
#define VB2_307LV		0x00008000
#define VB2_30xBDH		0x08000000      /* 30xB DH version (w/o LCD support) */
#define VB2_CONEXANT		0x10000000
#define VB2_TRUMPION		0x20000000
#define VB2_LVDS		0x40000000
#define VB2_CHRONTEL		0x80000000

#define VB2_SISLVDSBRIDGE	(VB2_301LV | VB2_302LV | VB2_302ELV | VB2_307LV)
#define VB2_SISTMDSBRIDGE	(VB2_301   | VB2_301B  | VB2_301C   | VB2_302B | VB2_307T)
#define VB2_SISBRIDGE		(VB2_SISLVDSBRIDGE | VB2_SISTMDSBRIDGE)

#define VB2_SISTMDSLCDABRIDGE	(VB2_301C | VB2_307T)
#define VB2_SISLCDABRIDGE	(VB2_SISTMDSLCDABRIDGE | VB2_301LV | VB2_302LV | VB2_302ELV | VB2_307LV)

#define VB2_SISHIVISIONBRIDGE	(VB2_301  | VB2_301B | VB2_302B)
#define VB2_SISYPBPRBRIDGE	(VB2_301C | VB2_307T | VB2_SISLVDSBRIDGE)
#define VB2_SISYPBPRARBRIDGE	(VB2_301C | VB2_307T | VB2_307LV)
#define VB2_SISTAP4SCALER	(VB2_301C | VB2_307T | VB2_302ELV | VB2_307LV)
#define VB2_SISTVBRIDGE		(VB2_SISHIVISIONBRIDGE | VB2_SISYPBPRBRIDGE)

#define VB2_SISVGA2BRIDGE	(VB2_301 | VB2_301B | VB2_301C | VB2_302B | VB2_307T)

#define VB2_VIDEOBRIDGE		(VB2_SISBRIDGE | VB2_LVDS | VB2_CHRONTEL | VB2_CONEXANT)

#define VB2_30xB		(VB2_301B  | VB2_301C   | VB2_302B  | VB2_307T)
#define VB2_30xBLV		(VB2_30xB  | VB2_SISLVDSBRIDGE)
#define VB2_30xC		(VB2_301C  | VB2_307T)
#define VB2_30xCLV		(VB2_301C  | VB2_307T   | VB2_302ELV| VB2_307LV)
#define VB2_SISEMIBRIDGE	(VB2_302LV | VB2_302ELV | VB2_307LV)
#define VB2_LCD162MHZBRIDGE	(VB2_301C  | VB2_307T)
#define VB2_LCDOVER1280BRIDGE	(VB2_301C  | VB2_307T   | VB2_302LV | VB2_302ELV | VB2_307LV)
#define VB2_LCDOVER1600BRIDGE	(VB2_307T  | VB2_307LV)
#define VB2_RAMDAC202MHZBRIDGE	(VB2_301C  | VB2_307T)

/* Aliases: */
#define CRT2_ENABLE		(CRT2_LCD | CRT2_TV | CRT2_VGA)
#define TV_STANDARD             (TV_NTSC | TV_PAL | TV_PALM | TV_PALN | TV_NTSCJ)
#define TV_INTERFACE            (TV_AVIDEO|TV_SVIDEO|TV_SCART|TV_HIVISION|TV_YPBPR|TV_CHSCART|TV_CHYPBPR525I)

/* Only if TV_YPBPR is set: */
#define TV_YPBPR525I		TV_NTSC
#define TV_YPBPR525P		TV_PAL
#define TV_YPBPR750P		TV_PALM
#define TV_YPBPR1080I		TV_PALN
#define TV_YPBPRALL 		(TV_YPBPR525I | TV_YPBPR525P | TV_YPBPR750P | TV_YPBPR1080I)

#define VB_SISBRIDGE            (VB_301|VB_301B|VB_301C|VB_302B|VB_301LV|VB_302LV|VB_302ELV)
#define VB_SISTVBRIDGE          (VB_301|VB_301B|VB_301C|VB_302B|VB_301LV|VB_302LV)
#define VB_VIDEOBRIDGE		(VB_SISBRIDGE | VB_LVDS | VB_CHRONTEL | VB_CONEXANT)

#define VB_DISPTYPE_DISP2	CRT2_ENABLE
#define VB_DISPTYPE_CRT2	CRT2_ENABLE
#define VB_DISPTYPE_DISP1	VB_DISPTYPE_CRT1
#define VB_DISPMODE_SINGLE	VB_SINGLE_MODE
#define VB_DISPMODE_MIRROR	VB_MIRROR_MODE
#define VB_DISPMODE_DUAL	VB_DUALVIEW_MODE
#define VB_DISPLAY_MODE       	(SINGLE_MODE | MIRROR_MODE | DUALVIEW_MODE)


enum _SIS_LCD_TYPE {
    LCD_INVALID = 0,
    LCD_800x600,
    LCD_1024x768,
    LCD_1280x1024,
    LCD_1280x960,
    LCD_640x480,
    LCD_1600x1200,
    LCD_1920x1440,
    LCD_2048x1536,
    LCD_320x240,	/* FSTN */
    LCD_1400x1050,
    LCD_1152x864,
    LCD_1152x768,
    LCD_1280x768,
    LCD_1024x600,
    LCD_320x240_2,	/* DSTN */
    LCD_320x240_3,	/* DSTN */
    LCD_848x480,
    LCD_1280x800,
    LCD_1680x1050,
    LCD_1280x720,
    LCD_1280x854,
    LCD_CUSTOM,
    LCD_UNKNOWN
};

enum _SIS_CMDTYPE {
    MMIO_CMD = 0,
    AGP_CMD_QUEUE,
    VM_CMD_QUEUE,
};

typedef unsigned int SIS_CMDTYPE;

#define MD_SIS300 1
#define MD_SIS315 2

/* Mode table */
static const struct _sisbios_mode {
	char name[15];
	u8  mode_no[2];
	u16 vesa_mode_no_1;  /* "SiS defined" VESA mode number */
	u16 vesa_mode_no_2;  /* Real VESA mode numbers */
	u16 xres;
	u16 yres;
	u16 bpp;
	u16 rate_idx;
	u16 cols;
	u16 rows;
	u8  chipset;
} sisbios_mode[] = {
/*0*/	{"none",         {0xff,0xff}, 0x0000, 0x0000,    0,    0,  0, 0,   0,  0, MD_SIS300|MD_SIS315},
	{"320x200x8",    {0x59,0x59}, 0x0138, 0x0000,  320,  200,  8, 1,  40, 12, MD_SIS300|MD_SIS315},
	{"320x200x16",   {0x41,0x41}, 0x010e, 0x0000,  320,  200, 16, 1,  40, 12, MD_SIS300|MD_SIS315},
	{"320x200x24",   {0x4f,0x4f}, 0x0000, 0x0000,  320,  200, 32, 1,  40, 12, MD_SIS300|MD_SIS315},  /* That's for people who mix up color- and fb depth */
	{"320x200x32",   {0x4f,0x4f}, 0x0000, 0x0000,  320,  200, 32, 1,  40, 12, MD_SIS300|MD_SIS315},
	{"320x240x8",    {0x50,0x50}, 0x0132, 0x0000,  320,  240,  8, 1,  40, 15, MD_SIS300|MD_SIS315},
	{"320x240x16",   {0x56,0x56}, 0x0135, 0x0000,  320,  240, 16, 1,  40, 15, MD_SIS300|MD_SIS315},
	{"320x240x24",   {0x53,0x53}, 0x0000, 0x0000,  320,  240, 32, 1,  40, 15, MD_SIS300|MD_SIS315},
	{"320x240x32",   {0x53,0x53}, 0x0000, 0x0000,  320,  240, 32, 1,  40, 15, MD_SIS300|MD_SIS315},
	{"320x240x8",    {0x5a,0x5a}, 0x0132, 0x0000,  320,  480,  8, 1,  40, 30,           MD_SIS315},  /* FSTN */
/*10*/	{"320x240x16",   {0x5b,0x5b}, 0x0135, 0x0000,  320,  480, 16, 1,  40, 30,           MD_SIS315},  /* FSTN */
	{"400x300x8",    {0x51,0x51}, 0x0133, 0x0000,  400,  300,  8, 1,  50, 18, MD_SIS300|MD_SIS315},
	{"400x300x16",   {0x57,0x57}, 0x0136, 0x0000,  400,  300, 16, 1,  50, 18, MD_SIS300|MD_SIS315},
	{"400x300x24",   {0x54,0x54}, 0x0000, 0x0000,  400,  300, 32, 1,  50, 18, MD_SIS300|MD_SIS315},
	{"400x300x32",   {0x54,0x54}, 0x0000, 0x0000,  400,  300, 32, 1,  50, 18, MD_SIS300|MD_SIS315},
	{"512x384x8",    {0x52,0x52}, 0x0000, 0x0000,  512,  384,  8, 1,  64, 24, MD_SIS300|MD_SIS315},
	{"512x384x16",   {0x58,0x58}, 0x0000, 0x0000,  512,  384, 16, 1,  64, 24, MD_SIS300|MD_SIS315},
	{"512x384x24",   {0x5c,0x5c}, 0x0000, 0x0000,  512,  384, 32, 1,  64, 24, MD_SIS300|MD_SIS315},
	{"512x384x32",   {0x5c,0x5c}, 0x0000, 0x0000,  512,  384, 32, 1,  64, 24, MD_SIS300|MD_SIS315},
	{"640x400x8",    {0x2f,0x2f}, 0x0000, 0x0000,  640,  400,  8, 1,  80, 25, MD_SIS300|MD_SIS315},
/*20*/	{"640x400x16",   {0x5d,0x5d}, 0x0000, 0x0000,  640,  400, 16, 1,  80, 25, MD_SIS300|MD_SIS315},
	{"640x400x24",   {0x5e,0x5e}, 0x0000, 0x0000,  640,  400, 32, 1,  80, 25, MD_SIS300|MD_SIS315},
	{"640x400x32",   {0x5e,0x5e}, 0x0000, 0x0000,  640,  400, 32, 1,  80, 25, MD_SIS300|MD_SIS315},
	{"640x480x8",    {0x2e,0x2e}, 0x0101, 0x0101,  640,  480,  8, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"640x480x16",   {0x44,0x44}, 0x0111, 0x0111,  640,  480, 16, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"640x480x24",   {0x62,0x62}, 0x013a, 0x0112,  640,  480, 32, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"640x480x32",   {0x62,0x62}, 0x013a, 0x0112,  640,  480, 32, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"720x480x8",    {0x31,0x31}, 0x0000, 0x0000,  720,  480,  8, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x480x16",   {0x33,0x33}, 0x0000, 0x0000,  720,  480, 16, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x480x24",   {0x35,0x35}, 0x0000, 0x0000,  720,  480, 32, 1,  90, 30, MD_SIS300|MD_SIS315},
/*30*/	{"720x480x32",   {0x35,0x35}, 0x0000, 0x0000,  720,  480, 32, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x576x8",    {0x32,0x32}, 0x0000, 0x0000,  720,  576,  8, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"720x576x16",   {0x34,0x34}, 0x0000, 0x0000,  720,  576, 16, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"720x576x24",   {0x36,0x36}, 0x0000, 0x0000,  720,  576, 32, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"720x576x32",   {0x36,0x36}, 0x0000, 0x0000,  720,  576, 32, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"768x576x8",    {0x5f,0x5f}, 0x0000, 0x0000,  768,  576,  8, 1,  96, 36, MD_SIS300|MD_SIS315},
	{"768x576x16",   {0x60,0x60}, 0x0000, 0x0000,  768,  576, 16, 1,  96, 36, MD_SIS300|MD_SIS315},
	{"768x576x24",   {0x61,0x61}, 0x0000, 0x0000,  768,  576, 32, 1,  96, 36, MD_SIS300|MD_SIS315},
	{"768x576x32",   {0x61,0x61}, 0x0000, 0x0000,  768,  576, 32, 1,  96, 36, MD_SIS300|MD_SIS315},
	{"800x480x8",    {0x70,0x70}, 0x0000, 0x0000,  800,  480,  8, 1, 100, 30, MD_SIS300|MD_SIS315},
/*40*/	{"800x480x16",   {0x7a,0x7a}, 0x0000, 0x0000,  800,  480, 16, 1, 100, 30, MD_SIS300|MD_SIS315},
	{"800x480x24",   {0x76,0x76}, 0x0000, 0x0000,  800,  480, 32, 1, 100, 30, MD_SIS300|MD_SIS315},
	{"800x480x32",   {0x76,0x76}, 0x0000, 0x0000,  800,  480, 32, 1, 100, 30, MD_SIS300|MD_SIS315},
#define DEFAULT_MODE              43 /* index for 800x600x8 */
#define DEFAULT_LCDMODE           43 /* index for 800x600x8 */
#define DEFAULT_TVMODE            43 /* index for 800x600x8 */
	{"800x600x8",    {0x30,0x30}, 0x0103, 0x0103,  800,  600,  8, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"800x600x16",   {0x47,0x47}, 0x0114, 0x0114,  800,  600, 16, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"800x600x24",   {0x63,0x63}, 0x013b, 0x0115,  800,  600, 32, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"800x600x32",   {0x63,0x63}, 0x013b, 0x0115,  800,  600, 32, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"848x480x8",    {0x39,0x39}, 0x0000, 0x0000,  848,  480,  8, 2, 106, 30, MD_SIS300|MD_SIS315},
	{"848x480x16",   {0x3b,0x3b}, 0x0000, 0x0000,  848,  480, 16, 2, 106, 30, MD_SIS300|MD_SIS315},
	{"848x480x24",   {0x3e,0x3e}, 0x0000, 0x0000,  848,  480, 32, 2, 106, 30, MD_SIS300|MD_SIS315},
/*50*/	{"848x480x32",   {0x3e,0x3e}, 0x0000, 0x0000,  848,  480, 32, 2, 106, 30, MD_SIS300|MD_SIS315},
	{"856x480x8",    {0x3f,0x3f}, 0x0000, 0x0000,  856,  480,  8, 2, 107, 30, MD_SIS300|MD_SIS315},
	{"856x480x16",   {0x42,0x42}, 0x0000, 0x0000,  856,  480, 16, 2, 107, 30, MD_SIS300|MD_SIS315},
	{"856x480x24",   {0x45,0x45}, 0x0000, 0x0000,  856,  480, 32, 2, 107, 30, MD_SIS300|MD_SIS315},
	{"856x480x32",   {0x45,0x45}, 0x0000, 0x0000,  856,  480, 32, 2, 107, 30, MD_SIS300|MD_SIS315},
	{"960x540x8",    {0x1d,0x1d}, 0x0000, 0x0000,  960,  540,  8, 1, 120, 33,           MD_SIS315},
	{"960x540x16",   {0x1e,0x1e}, 0x0000, 0x0000,  960,  540, 16, 1, 120, 33,           MD_SIS315},
	{"960x540x24",   {0x1f,0x1f}, 0x0000, 0x0000,  960,  540, 32, 1, 120, 33,           MD_SIS315},
	{"960x540x32",   {0x1f,0x1f}, 0x0000, 0x0000,  960,  540, 32, 1, 120, 33,           MD_SIS315},
	{"960x600x8",    {0x20,0x20}, 0x0000, 0x0000,  960,  600,  8, 1, 120, 37,           MD_SIS315},
/*60*/	{"960x600x16",   {0x21,0x21}, 0x0000, 0x0000,  960,  600, 16, 1, 120, 37,           MD_SIS315},
	{"960x600x24",   {0x22,0x22}, 0x0000, 0x0000,  960,  600, 32, 1, 120, 37,           MD_SIS315},
	{"960x600x32",   {0x22,0x22}, 0x0000, 0x0000,  960,  600, 32, 1, 120, 37,           MD_SIS315},
	{"1024x576x8",   {0x71,0x71}, 0x0000, 0x0000, 1024,  576,  8, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x576x16",  {0x74,0x74}, 0x0000, 0x0000, 1024,  576, 16, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x576x24",  {0x77,0x77}, 0x0000, 0x0000, 1024,  576, 32, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x576x32",  {0x77,0x77}, 0x0000, 0x0000, 1024,  576, 32, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x600x8",   {0x20,0x20}, 0x0000, 0x0000, 1024,  600,  8, 1, 128, 37, MD_SIS300          },
	{"1024x600x16",  {0x21,0x21}, 0x0000, 0x0000, 1024,  600, 16, 1, 128, 37, MD_SIS300          },
	{"1024x600x24",  {0x22,0x22}, 0x0000, 0x0000, 1024,  600, 32, 1, 128, 37, MD_SIS300          },
/*70*/	{"1024x600x32",  {0x22,0x22}, 0x0000, 0x0000, 1024,  600, 32, 1, 128, 37, MD_SIS300          },
	{"1024x768x8",   {0x38,0x38}, 0x0105, 0x0105, 1024,  768,  8, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1024x768x16",  {0x4a,0x4a}, 0x0117, 0x0117, 1024,  768, 16, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1024x768x24",  {0x64,0x64}, 0x013c, 0x0118, 1024,  768, 32, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1024x768x32",  {0x64,0x64}, 0x013c, 0x0118, 1024,  768, 32, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1152x768x8",   {0x23,0x23}, 0x0000, 0x0000, 1152,  768,  8, 1, 144, 48, MD_SIS300          },
	{"1152x768x16",  {0x24,0x24}, 0x0000, 0x0000, 1152,  768, 16, 1, 144, 48, MD_SIS300          },
	{"1152x768x24",  {0x25,0x25}, 0x0000, 0x0000, 1152,  768, 32, 1, 144, 48, MD_SIS300          },
	{"1152x768x32",  {0x25,0x25}, 0x0000, 0x0000, 1152,  768, 32, 1, 144, 48, MD_SIS300          },
	{"1152x864x8",   {0x29,0x29}, 0x0000, 0x0000, 1152,  864,  8, 1, 144, 54, MD_SIS300|MD_SIS315},
/*80*/	{"1152x864x16",  {0x2a,0x2a}, 0x0000, 0x0000, 1152,  864, 16, 1, 144, 54, MD_SIS300|MD_SIS315},
	{"1152x864x24",  {0x2b,0x2b}, 0x0000, 0x0000, 1152,  864, 32, 1, 144, 54, MD_SIS300|MD_SIS315},
	{"1152x864x32",  {0x2b,0x2b}, 0x0000, 0x0000, 1152,  864, 32, 1, 144, 54, MD_SIS300|MD_SIS315},
	{"1280x720x8",   {0x79,0x79}, 0x0000, 0x0000, 1280,  720,  8, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x720x16",  {0x75,0x75}, 0x0000, 0x0000, 1280,  720, 16, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x720x24",  {0x78,0x78}, 0x0000, 0x0000, 1280,  720, 32, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x720x32",  {0x78,0x78}, 0x0000, 0x0000, 1280,  720, 32, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x768x8",   {0x55,0x23}, 0x0000, 0x0000, 1280,  768,  8, 1, 160, 48, MD_SIS300|MD_SIS315},
	{"1280x768x16",  {0x5a,0x24}, 0x0000, 0x0000, 1280,  768, 16, 1, 160, 48, MD_SIS300|MD_SIS315},
	{"1280x768x24",  {0x5b,0x25}, 0x0000, 0x0000, 1280,  768, 32, 1, 160, 48, MD_SIS300|MD_SIS315},
/*90*/	{"1280x768x32",  {0x5b,0x25}, 0x0000, 0x0000, 1280,  768, 32, 1, 160, 48, MD_SIS300|MD_SIS315},
	{"1280x800x8",   {0x14,0x14}, 0x0000, 0x0000, 1280,  800,  8, 1, 160, 50,           MD_SIS315},
	{"1280x800x16",  {0x15,0x15}, 0x0000, 0x0000, 1280,  800, 16, 1, 160, 50,           MD_SIS315},
	{"1280x800x24",  {0x16,0x16}, 0x0000, 0x0000, 1280,  800, 32, 1, 160, 50,           MD_SIS315},
	{"1280x800x32",  {0x16,0x16}, 0x0000, 0x0000, 1280,  800, 32, 1, 160, 50,           MD_SIS315},
	{"1280x960x8",   {0x7c,0x7c}, 0x0000, 0x0000, 1280,  960,  8, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x960x16",  {0x7d,0x7d}, 0x0000, 0x0000, 1280,  960, 16, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x960x24",  {0x7e,0x7e}, 0x0000, 0x0000, 1280,  960, 32, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x960x32",  {0x7e,0x7e}, 0x0000, 0x0000, 1280,  960, 32, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x1024x8",  {0x3a,0x3a}, 0x0107, 0x0107, 1280, 1024,  8, 2, 160, 64, MD_SIS300|MD_SIS315},
/*100*/	{"1280x1024x16", {0x4d,0x4d}, 0x011a, 0x011a, 1280, 1024, 16, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1280x1024x24", {0x65,0x65}, 0x013d, 0x011b, 1280, 1024, 32, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1280x1024x32", {0x65,0x65}, 0x013d, 0x011b, 1280, 1024, 32, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1360x768x8",   {0x48,0x48}, 0x0000, 0x0000, 1360,  768,  8, 1, 170, 48, MD_SIS300|MD_SIS315},
	{"1360x768x16",  {0x4b,0x4b}, 0x0000, 0x0000, 1360,  768, 16, 1, 170, 48, MD_SIS300|MD_SIS315},
	{"1360x768x24",  {0x4e,0x4e}, 0x0000, 0x0000, 1360,  768, 32, 1, 170, 48, MD_SIS300|MD_SIS315},
	{"1360x768x32",  {0x4e,0x4e}, 0x0000, 0x0000, 1360,  768, 32, 1, 170, 48, MD_SIS300|MD_SIS315},
	{"1360x1024x8",  {0x67,0x67}, 0x0000, 0x0000, 1360, 1024,  8, 1, 170, 64, MD_SIS300          },
	{"1360x1024x16", {0x6f,0x6f}, 0x0000, 0x0000, 1360, 1024, 16, 1, 170, 64, MD_SIS300          },
	{"1360x1024x24", {0x72,0x72}, 0x0000, 0x0000, 1360, 1024, 32, 1, 170, 64, MD_SIS300          },
/*110*/	{"1360x1024x32", {0x72,0x72}, 0x0000, 0x0000, 1360, 1024, 32, 1, 170, 64, MD_SIS300          },
	{"1400x1050x8",  {0x26,0x26}, 0x0000, 0x0000, 1400, 1050,  8, 1, 175, 65,           MD_SIS315},
	{"1400x1050x16", {0x27,0x27}, 0x0000, 0x0000, 1400, 1050, 16, 1, 175, 65,           MD_SIS315},
	{"1400x1050x24", {0x28,0x28}, 0x0000, 0x0000, 1400, 1050, 32, 1, 175, 65,           MD_SIS315},
	{"1400x1050x32", {0x28,0x28}, 0x0000, 0x0000, 1400, 1050, 32, 1, 175, 65,           MD_SIS315},
	{"1600x1200x8",  {0x3c,0x3c}, 0x0130, 0x011c, 1600, 1200,  8, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1600x1200x16", {0x3d,0x3d}, 0x0131, 0x011e, 1600, 1200, 16, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1600x1200x24", {0x66,0x66}, 0x013e, 0x011f, 1600, 1200, 32, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1600x1200x32", {0x66,0x66}, 0x013e, 0x011f, 1600, 1200, 32, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1680x1050x8",  {0x17,0x17}, 0x0000, 0x0000, 1680, 1050,  8, 1, 210, 65,           MD_SIS315},
/*120*/	{"1680x1050x16", {0x18,0x18}, 0x0000, 0x0000, 1680, 1050, 16, 1, 210, 65,           MD_SIS315},
	{"1680x1050x24", {0x19,0x19}, 0x0000, 0x0000, 1680, 1050, 32, 1, 210, 65,           MD_SIS315},
	{"1680x1050x32", {0x19,0x19}, 0x0000, 0x0000, 1680, 1050, 32, 1, 210, 65,           MD_SIS315},
	{"1920x1080x8",  {0x2c,0x2c}, 0x0000, 0x0000, 1920, 1080,  8, 1, 240, 67,           MD_SIS315},
	{"1920x1080x16", {0x2d,0x2d}, 0x0000, 0x0000, 1920, 1080, 16, 1, 240, 67,           MD_SIS315},
	{"1920x1080x24", {0x73,0x73}, 0x0000, 0x0000, 1920, 1080, 32, 1, 240, 67,           MD_SIS315},
	{"1920x1080x32", {0x73,0x73}, 0x0000, 0x0000, 1920, 1080, 32, 1, 240, 67,           MD_SIS315},
	{"1920x1440x8",  {0x68,0x68}, 0x013f, 0x0000, 1920, 1440,  8, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"1920x1440x16", {0x69,0x69}, 0x0140, 0x0000, 1920, 1440, 16, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"1920x1440x24", {0x6b,0x6b}, 0x0141, 0x0000, 1920, 1440, 32, 1, 240, 75, MD_SIS300|MD_SIS315},
/*130*/	{"1920x1440x32", {0x6b,0x6b}, 0x0141, 0x0000, 1920, 1440, 32, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"2048x1536x8",  {0x6c,0x6c}, 0x0000, 0x0000, 2048, 1536,  8, 1, 256, 96,           MD_SIS315},
	{"2048x1536x16", {0x6d,0x6d}, 0x0000, 0x0000, 2048, 1536, 16, 1, 256, 96,           MD_SIS315},
	{"2048x1536x24", {0x6e,0x6e}, 0x0000, 0x0000, 2048, 1536, 32, 1, 256, 96,           MD_SIS315},
	{"2048x1536x32", {0x6e,0x6e}, 0x0000, 0x0000, 2048, 1536, 32, 1, 256, 96,           MD_SIS315},
	{"\0", {0x00,0x00}, 0, 0, 0, 0, 0, 0, 0}
};


#define SIS_LCD_NUMBER 18
static struct _sis_lcd_data {
	u32 lcdtype;
	u16 xres;
	u16 yres;
	u8  default_mode_idx;
} sis_lcd_data[] = {
	{ LCD_640x480,    640,  480,  23 },
	{ LCD_800x600,    800,  600,  43 },
	{ LCD_1024x600,  1024,  600,  67 },
	{ LCD_1024x768,  1024,  768,  71 },
	{ LCD_1152x768,  1152,  768,  75 },
	{ LCD_1152x864,  1152,  864,  79 },
	{ LCD_1280x720,  1280,  720,  83 },
	{ LCD_1280x768,  1280,  768,  87 },
	{ LCD_1280x800,  1280,  800,  91 },
	{ LCD_1280x854,  1280,  854,  95 },
	{ LCD_1280x960,  1280,  960,  99 },
	{ LCD_1280x1024, 1280, 1024, 103 },
	{ LCD_1400x1050, 1400, 1050, 115 },
	{ LCD_1680x1050, 1680, 1050, 123 },
	{ LCD_1600x1200, 1600, 1200, 119 },
	{ LCD_320x240_2,  320,  240,   9 },
	{ LCD_320x240_3,  320,  240,   9 },
	{ LCD_320x240,    320,  240,   9 },
};

/* CR36 evaluation */
static unsigned short sis300paneltype[] = {
	LCD_UNKNOWN,   LCD_800x600,   LCD_1024x768,  LCD_1280x1024,
	LCD_1280x960,  LCD_640x480,   LCD_1024x600,  LCD_1152x768,
	LCD_UNKNOWN,   LCD_UNKNOWN,   LCD_UNKNOWN,   LCD_UNKNOWN,
	LCD_UNKNOWN,   LCD_UNKNOWN,   LCD_UNKNOWN,   LCD_UNKNOWN
};

static unsigned short sis310paneltype[] = {
	LCD_UNKNOWN,   LCD_800x600,   LCD_1024x768,  LCD_1280x1024,
	LCD_640x480,   LCD_1024x600,  LCD_1152x864,  LCD_1280x960,
	LCD_1152x768,  LCD_1400x1050, LCD_1280x768,  LCD_1600x1200,
	LCD_320x240_2, LCD_320x240_3, LCD_UNKNOWN,   LCD_UNKNOWN
};

static unsigned short sis661paneltype[] = {
	LCD_UNKNOWN,   LCD_800x600,   LCD_1024x768,  LCD_1280x1024,
	LCD_640x480,   LCD_1024x600,  LCD_1152x864,  LCD_1280x960,
	LCD_1280x854,  LCD_1400x1050, LCD_1280x768,  LCD_1600x1200,
	LCD_1280x800,  LCD_1680x1050, LCD_1280x720,  LCD_UNKNOWN
};

static const struct _sis_vrate {
	u16 idx;
	u16 xres;
	u16 yres;
	u16 refresh;
	BOOLEAN SiS730valid32bpp;
} sis_vrate[] = {
	{1,  320,  200,  70,  TRUE},
	{1,  320,  240,  60,  TRUE},
	{1,  320,  480,  60,  TRUE},
	{1,  400,  300,  60,  TRUE},
	{1,  512,  384,  60,  TRUE},
	{1,  640,  400,  72,  TRUE},
	{1,  640,  480,  60,  TRUE}, {2,  640,  480,  72,  TRUE}, {3,  640,  480,  75,  TRUE},
	{4,  640,  480,  85,  TRUE}, {5,  640,  480, 100,  TRUE}, {6,  640,  480, 120,  TRUE},
	{7,  640,  480, 160,  TRUE}, {8,  640,  480, 200,  TRUE},
	{1,  720,  480,  60,  TRUE},
	{1,  720,  576,  58,  TRUE},
	{1,  768,  576,  58,  TRUE},
	{1,  800,  480,  60,  TRUE}, {2,  800,  480,  75,  TRUE}, {3,  800,  480,  85,  TRUE},
	{1,  800,  600,  56,  TRUE}, {2,  800,  600,  60,  TRUE}, {3,  800,  600,  72,  TRUE},
	{4,  800,  600,  75,  TRUE}, {5,  800,  600,  85,  TRUE}, {6,  800,  600, 105,  TRUE},
	{7,  800,  600, 120,  TRUE}, {8,  800,  600, 160,  TRUE},
	{1,  848,  480,  39,  TRUE}, {2,  848,  480,  60,  TRUE},
	{1,  856,  480,  39,  TRUE}, {2,  856,  480,  60,  TRUE},
	{1,  960,  540,  60,  TRUE},
	{1,  960,  600,  60,  TRUE},
	{1, 1024,  576,  60,  TRUE}, {2, 1024,  576,  75,  TRUE}, {3, 1024,  576,  85,  TRUE},
	{1, 1024,  600,  60,  TRUE},
	{1, 1024,  768,  43,  TRUE}, {2, 1024,  768,  60,  TRUE}, {3, 1024,  768,  70, FALSE},
	{4, 1024,  768,  75, FALSE}, {5, 1024,  768,  85,  TRUE}, {6, 1024,  768, 100,  TRUE},
	{7, 1024,  768, 120,  TRUE},
	{1, 1152,  768,  60,  TRUE},
	{1, 1152,  864,  60,  TRUE}, {1, 1152,  864,  75,  TRUE}, {2, 1152,  864,  84,  TRUE},
	{1, 1280,  720,  60,  TRUE}, {2, 1280,  720,  75,  TRUE}, {3, 1280,  720,  85,  TRUE},
	{1, 1280,  768,  60,  TRUE},
	{1, 1280,  800,  60,  TRUE},
	{1, 1280,  960,  60,  TRUE}, {2, 1280,  960,  85,  TRUE},
	{1, 1280, 1024,  43,  TRUE}, {2, 1280, 1024,  60,  TRUE}, {3, 1280, 1024,  75,  TRUE},
	{4, 1280, 1024,  85,  TRUE},
	{1, 1360,  768,  60,  TRUE},
	{1, 1360, 1024,  59,  TRUE},
	{1, 1400, 1050,  60,  TRUE}, {2, 1400, 1050,  75,  TRUE},
	{1, 1600, 1200,  60,  TRUE}, {2, 1600, 1200,  65,  TRUE}, {3, 1600, 1200,  70,  TRUE},
	{4, 1600, 1200,  75,  TRUE}, {5, 1600, 1200,  85,  TRUE}, {6, 1600, 1200, 100,  TRUE},
	{7, 1600, 1200, 120,  TRUE},
	{1, 1680, 1050,  60,  TRUE},
	{1, 1920, 1080,  30,  TRUE},
	{1, 1920, 1440,  60,  TRUE}, {2, 1920, 1440,  65,  TRUE}, {3, 1920, 1440,  70,  TRUE},
	{4, 1920, 1440,  75,  TRUE}, {5, 1920, 1440,  85,  TRUE}, {6, 1920, 1440, 100,  TRUE},
	{1, 2048, 1536,  60,  TRUE}, {2, 2048, 1536,  65,  TRUE}, {3, 2048, 1536,  70,  TRUE},
	{4, 2048, 1536,  75,  TRUE}, {5, 2048, 1536,  85,  TRUE},
	{0,    0,    0,   0, FALSE}
};

static const struct _sisfbddcsmodes {
	u32 mask;
	u16 h;
	u16 v;
	u32 d;
} sisfb_ddcsmodes[] = {
	{ 0x10000, 67, 75, 108000},
	{ 0x08000, 48, 72,  50000},
	{ 0x04000, 46, 75,  49500},
	{ 0x01000, 35, 43,  44900},
	{ 0x00800, 48, 60,  65000},
	{ 0x00400, 56, 70,  75000},
	{ 0x00200, 60, 75,  78800},
	{ 0x00100, 80, 75, 135000},
	{ 0x00020, 31, 60,  25200},
	{ 0x00008, 38, 72,  31500},
	{ 0x00004, 37, 75,  31500},
	{ 0x00002, 35, 56,  36000},
	{ 0x00001, 38, 60,  40000}
};

static const struct _sisfbddcfmodes {
	u16 x;
	u16 y;
	u16 v;
	u16 h;
	u32 d;
} sisfb_ddcfmodes[] = {
       { 1280, 1024, 85, 92, 157500},
       { 1600, 1200, 60, 75, 162000},
       { 1600, 1200, 65, 82, 175500},
       { 1600, 1200, 70, 88, 189000},
       { 1600, 1200, 75, 94, 202500},
       { 1600, 1200, 85, 107,229500},
       { 1920, 1440, 60, 90, 234000},
       { 1920, 1440, 75, 113,297000}
};

static struct _chswtable {
    uint16  subsysVendor;
    uint16  subsysCard;
    char *vendorName;
    char *cardName;
} mychswtable[] = {
        { 0x1631, 0x1002, "Mitachi", "0x1002" },
	{ 0x1071, 0x7521, "Mitac"  , "7521P"  },
	{ 0,      0,      ""       , ""       }
};

static struct _customttable {
    u16   chipID;
    char  *biosversion;
    char  *biosdate;
    u32   bioschksum;
    u16   biosFootprintAddr[5];
    u8    biosFootprintData[5];
    u16   pcisubsysvendor;
    u16   pcisubsyscard;
    char  *vendorName;
    char  *cardName;
    u32   SpecialID;
    char  *optionName;
} mycustomttable[] = {
	{ SIS_630, "2.00.07", "09/27/2002-13:38:25",
	  0x3240A8,
	  { 0x220, 0x227, 0x228, 0x229, 0x0ee },
	  {  0x01,  0xe3,  0x9a,  0x6a,  0xef },
	  0x1039, 0x6300,
	  "Barco", "iQ R200L/300/400", CUT_BARCO1366, "BARCO_1366"
	},
	{ SIS_630, "2.00.07", "09/27/2002-13:38:25",
	  0x323FBD,
	  { 0x220, 0x227, 0x228, 0x229, 0x0ee },
	  {  0x00,  0x5a,  0x64,  0x41,  0xef },
	  0x1039, 0x6300,
	  "Barco", "iQ G200L/300/400/500", CUT_BARCO1024, "BARCO_1024"
	},
	{ SIS_650, "", "",
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x0e11, 0x083c,
	  "Inventec (Compaq)", "3017cl/3045US", CUT_COMPAQ12802, "COMPAQ_1280"
	},
	{ SIS_650, "", "",
	  0,
	  { 0x00c, 0, 0, 0, 0 },
	  { 'e'  , 0, 0, 0, 0 },
	  0x1558, 0x0287,
	  "Clevo", "L285/L287 (Version 1)", CUT_CLEVO1024, "CLEVO_L28X_1"
	},
	{ SIS_650, "", "",
	  0,
	  { 0x00c, 0, 0, 0, 0 },
	  { 'y'  , 0, 0, 0, 0 },
	  0x1558, 0x0287,
	  "Clevo", "L285/L287 (Version 2)", CUT_CLEVO10242, "CLEVO_L28X_2"
	},
	{ SIS_650, "", "",
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1558, 0x0400,  /* possibly 401 and 402 as well; not panelsize specific (?) */
	  "Clevo", "D400S/D410S/D400H/D410H", CUT_CLEVO1400, "CLEVO_D4X0"
	},
	{ SIS_650, "", "",
	  0,	/* Shift LCD in LCD-via-CRT1 mode */
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1558, 0x2263,
	  "Clevo", "D22ES/D27ES", CUT_UNIWILL1024, "CLEVO_D2X0ES"
	},
	{ SIS_650, "", "",
	  0,	/* Shift LCD in LCD-via-CRT1 mode */
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1734, 0x101f,
	  "Uniwill", "N243S9", CUT_UNIWILL1024, "UNIWILL_N243S9"
	},
	{ SIS_650, "", "",
	  0,	/* Shift LCD in LCD-via-CRT1 mode */
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1584, 0x5103,
	  "Uniwill", "N35BS1", CUT_UNIWILL10242, "UNIWILL_N35BS1"
	},
	{ SIS_650, "1.09.2c", "",  /* Other versions, too? */
	  0,	/* Shift LCD in LCD-via-CRT1 mode */
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1019, 0x0f05,
	  "ECS", "A928", CUT_UNIWILL1024, "ECS_A928"
	},
	{ SIS_740, "1.11.27a", "",
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1043, 0x1612,
	  "Asus", "L3000D/L3500D", CUT_ASUSL3000D, "ASUS_L3X00"
	},
	{ SIS_650, "1.10.9k", "",
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1025, 0x0028,
	  "Acer", "Aspire 1700", CUT_ACER1280, "ACER_ASPIRE1700"
	},
	{ SIS_650, "1.10.7w", "",
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x14c0, 0x0012,
	  "Compal", "??? (V1)", CUT_COMPAL1400_1, "COMPAL_1400_1"
	},
	{ SIS_650, "1.10.7x", "",
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x14c0, 0x0012,
	  "Compal", "??? (V2)", CUT_COMPAL1400_2, "COMPAL_1400_2"
	},
	{ SIS_650, "1.10.8o", "",
	  0,	/* For EMI (unknown) */
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1043, 0x1612,
	  "Asus", "A2H (V1)", CUT_ASUSA2H_1, "ASUS_A2H_1"
	},
	{ SIS_650, "1.10.8q", "",
	  0,	/* For EMI */
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0x1043, 0x1612,
	  "Asus", "A2H (V2)", CUT_ASUSA2H_2, "ASUS_A2H_2"
	},
	{ 4321, "", "",			/* never autodetected */
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0, 0,
	  "Generic", "LVDS/Parallel 848x480", CUT_PANEL848, "PANEL848x480"
	},
	{ 4322, "", "",			/* never autodetected */
	  0,
	  { 0, 0, 0, 0, 0 },
	  { 0, 0, 0, 0, 0 },
	  0, 0,
	  "Generic", "LVDS/Parallel 856x480", CUT_PANEL856, "PANEL856x480"
	},
	{ 0, "", "",
	  0,
	  { 0, 0, 0, 0 },
	  { 0, 0, 0, 0 },
	  0, 0,
	  "", "", CUT_NONE, ""
	}
};

/* Information shared between the c++ syllable
	driver and the c sis driver */

typedef struct
{
	int fd;
	PCI_Info_s pci_dev;
	PCI_Info_s bridge;
	uint32	device_id;
	uint16	subsys_vendor_id;
	uint16	subsys_device_id;
	char	name[30];

	uint32	io_base;
	uint8	*framebuffer;
	vuint32 *regs;
	void 	*rom;
	
	enum _SIS_CHIP_TYPE chip;
	uint8		revision_id;
	int		sisvga_engine;
	uint32		video_size;
	int newrom;
	unsigned long UMAsize, LFBsize;
	
	uint32 vbflags;
	uint32 currentvbflags;
	uint32 vbflags2;
	uint32 CRT2LCDType;
	int	CRT2_write_enable;
	int crt1off;
	int lcdxres;
	int lcdyres;
	int	lcddefmodeidx, tvdefmodeidx, defmodeidx;
	uint8 detectedpdc;
	uint8 detectedpdca;
	uint8 detectedlcda;
	uint8 p2_1f,p2_20,p2_2b,p2_42,p2_43,p2_01,p2_02;
	int chronteltype;
	int	tvx, tvy;
	int mode_idx;
	int rate_idx;
	unsigned short video_width;
	unsigned short video_height;
	uint8 bpp;
	uint32 AccelDepth;
	uint16 DstColor;
	uint32 CmdReg;
	SISPortPrivRec video_port;
	
    unsigned int	*cmdQueueBase;
    unsigned int	cmdQueueOffset;
    unsigned int	cmdQueueSize;
    unsigned int	cmdQueueSizeMask;
    unsigned int	cmdQ_SharedWritePort;
    unsigned int	cmdQueueSize_div2;
    unsigned int	cmdQueueSize_div4;
    unsigned int	cmdQueueSize_4_3;
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
#define andSISIDXREG(base,idx,a)  do { \
                                      unsigned char __Temp; \
                                      outb(idx,base);   \
                                      __Temp = inb((base)+1)&(a); \
                                      outSISIDXREG(base,idx,__Temp); \
                                    } while (0)
#define setSISIDXREG(base,idx,a,o)   do { \
                                          unsigned char __Temp; \
                                          outb(idx,base);   \
                                          __Temp = (inb((base)+1)&(a))|(o); \
                                          outSISIDXREG(base,idx,__Temp); \
                                        } while (0)
      
/* Some global stuff */                                  
                                        
extern struct SiS_Private SiS_Pr;
extern shared_info si;

/* sis.c */

void sis_pre_setmode( uint8 rate_idx );
void sis_post_setmode();
int sis_init();
extern BOOLEAN  SiSSetMode(struct SiS_Private *SiS_Pr, unsigned short ModeNo);
void sis_set_pitch( int pitch );

#endif








































