/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/vstruct.h,v 1.0 2001/06/15 21:23:00 dawes Exp $ */
/*
 * General structure definitions for universal mode switching modules
 *
 * Copyright 2002, 2003 by Thomas Winischhofer, Vienna, Austria
 *
 * If distributed as part of the linux kernel, the contents of this file
 * is entirely covered by the GPL.
 *
 * Otherwise, the following terms apply:
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holder not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holder makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: 	Thomas Winischhofer <thomas@winischhofer.net>
 *              Silicon Integrated Systems
 *
 */
 
#ifdef _INIT_
#define EXTERN
#else
#define EXTERN extern
#endif /* _INIT_ */

#ifndef _VSTRUCT_
#define _VSTRUCT_

typedef struct _SiS_PanelDelayTblStruct
{
 	UCHAR timer[2];
} SiS_PanelDelayTblStruct;

typedef struct _SiS_LCDDataStruct
{
	USHORT RVBHCMAX;
	USHORT RVBHCFACT;
	USHORT VGAHT;
	USHORT VGAVT;
	USHORT LCDHT;
	USHORT LCDVT;
} SiS_LCDDataStruct;

typedef struct _SiS_TVDataStruct
{
	USHORT RVBHCMAX;
	USHORT RVBHCFACT;
	USHORT VGAHT;
	USHORT VGAVT;
	USHORT TVHDE;
	USHORT TVVDE;
	USHORT RVBHRS;
	UCHAR  FlickerMode;
	USHORT HALFRVBHRS;
	UCHAR  RY1COE;
	UCHAR  RY2COE;
	UCHAR  RY3COE;
	UCHAR  RY4COE;
} SiS_TVDataStruct;

typedef struct _SiS_LVDSDataStruct
{
	USHORT VGAHT;
	USHORT VGAVT;
	USHORT LCDHT;
	USHORT LCDVT;
} SiS_LVDSDataStruct;

typedef struct _SiS_LVDSDesStruct
{
	USHORT LCDHDES;
	USHORT LCDVDES;
} SiS_LVDSDesStruct;

typedef struct _SiS_LVDSCRT1DataStruct
{
	UCHAR  CR[15];
} SiS_LVDSCRT1DataStruct;

typedef struct _SiS_LCDACRT1DataStruct
{
	UCHAR  CR[17];
} SiS_LCDACRT1DataStruct;

typedef struct _SiS_CHTVRegDataStruct
{
	UCHAR  Reg[16];
} SiS_CHTVRegDataStruct;

typedef struct _SiS_StStruct
{
	UCHAR  St_ModeID;
	USHORT St_ModeFlag;
	UCHAR  St_StTableIndex;
	UCHAR  St_CRT2CRTC;
	UCHAR  St_ResInfo;
	UCHAR  VB_StTVFlickerIndex;
	UCHAR  VB_StTVEdgeIndex;
	UCHAR  VB_StTVYFilterIndex;
} SiS_StStruct;

typedef struct _SiS_VBModeStruct
{
	UCHAR  ModeID;
	UCHAR  VB_TVDelayIndex;
	UCHAR  VB_TVFlickerIndex;
	UCHAR  VB_TVPhaseIndex;
	UCHAR  VB_TVYFilterIndex;
	UCHAR  VB_LCDDelayIndex;
	UCHAR  _VB_LCDHIndex;
	UCHAR  _VB_LCDVIndex;
} SiS_VBModeStruct;

typedef struct _SiS_StandTableStruct
{
	UCHAR  CRT_COLS;
	UCHAR  ROWS;
	UCHAR  CHAR_HEIGHT;
	USHORT CRT_LEN;
	UCHAR  SR[4];
	UCHAR  MISC;
	UCHAR  CRTC[0x19];
	UCHAR  ATTR[0x14];
	UCHAR  GRC[9];
} SiS_StandTableStruct;

typedef struct _SiS_ExtStruct
{
	UCHAR  Ext_ModeID;
	USHORT Ext_ModeFlag;
	USHORT Ext_ModeInfo;
	USHORT Ext_VESAID;
	UCHAR  Ext_RESINFO;
	UCHAR  VB_ExtTVFlickerIndex;
	UCHAR  VB_ExtTVEdgeIndex;
	UCHAR  VB_ExtTVYFilterIndex;
	UCHAR  REFindex;
} SiS_ExtStruct;

typedef struct _SiS_Ext2Struct
{
	USHORT Ext_InfoFlag;
	UCHAR  Ext_CRT1CRTC;
	UCHAR  Ext_CRTVCLK;
	UCHAR  Ext_CRT2CRTC;
	UCHAR  ModeID;
	USHORT XRes;
	USHORT YRes;
} SiS_Ext2Struct;

typedef struct _SiS_Part2PortTblStruct
{
 	UCHAR  CR[12];
} SiS_Part2PortTblStruct;

typedef struct _SiS_CRT1TableStruct
{
	UCHAR  CR[17];
} SiS_CRT1TableStruct;

typedef struct _SiS_MCLKDataStruct
{
	UCHAR  SR28,SR29,SR2A;
	USHORT CLOCK;
} SiS_MCLKDataStruct;

typedef struct _SiS_ECLKDataStruct
{
	UCHAR  SR2E,SR2F,SR30;
	USHORT CLOCK;
} SiS_ECLKDataStruct;

typedef struct _SiS_VCLKDataStruct
{
	UCHAR  SR2B,SR2C;
	USHORT CLOCK;
} SiS_VCLKDataStruct;

typedef struct _SiS_VBVCLKDataStruct
{
	UCHAR  Part4_A,Part4_B;
	USHORT CLOCK;
} SiS_VBVCLKDataStruct;

typedef struct _SiS_StResInfoStruct
{
	USHORT HTotal;
	USHORT VTotal;
} SiS_StResInfoStruct;

typedef struct _SiS_ModeResInfoStruct
{
	USHORT HTotal;
	USHORT VTotal;
	UCHAR  XChar;
	UCHAR  YChar;
} SiS_ModeResInfoStruct;

typedef UCHAR DRAM4Type[4];

/* Defines for SiS_Customt */
#define CUT_NONE        0
#define CUT_FORCENONE   1
#define CUT_BARCO1366   2
#define CUT_BARCO1024   3
#define CUT_COMPAQ1280  4
#define CUT_COMPAQ12802 5
#define CUT_PANEL848    6

typedef struct _SiS_Private
{
#ifdef SYLLABLE
        USHORT RelIO;
#endif
	USHORT SiS_P3c4;
	USHORT SiS_P3d4;
	USHORT SiS_P3c0;
	USHORT SiS_P3ce;
	USHORT SiS_P3c2;
	USHORT SiS_P3ca;
	USHORT SiS_P3c6;
	USHORT SiS_P3c7;
	USHORT SiS_P3c8;
	USHORT SiS_P3c9;
	USHORT SiS_P3cb;
	USHORT SiS_P3cd;
	USHORT SiS_P3da;
	USHORT SiS_Part1Port;
	USHORT SiS_Part2Port;
	USHORT SiS_Part3Port;
	USHORT SiS_Part4Port;
	USHORT SiS_Part5Port;
	USHORT SiS_VidCapt;
	USHORT SiS_VidPlay;
	USHORT SiS_IF_DEF_LVDS;
	USHORT SiS_IF_DEF_TRUMPION;
	USHORT SiS_IF_DEF_DSTN;
	USHORT SiS_IF_DEF_FSTN;
	USHORT SiS_IF_DEF_CH70xx;
	USHORT SiS_IF_DEF_HiVision;
	USHORT SiS_SysFlags;
	UCHAR  SiS_VGAINFO;
#ifndef LINUX_KERNEL
        USHORT SiS_CP1, SiS_CP2, SiS_CP3, SiS_CP4;
#endif
	BOOLEAN SiS_UseROM;
	int    SiS_CHOverScan;
	BOOLEAN SiS_CHSOverScan;
	BOOLEAN SiS_ChSW;
	BOOLEAN SiS_UseLCDA;
	int    SiS_UseOEM;
	ULONG  SiS_CustomT;
	USHORT SiS_Backup70xx;
	USHORT SiS_CRT1Mode;
	USHORT SiS_flag_clearbuffer;
	int    SiS_RAMType;
	UCHAR  SiS_ChannelAB;
	UCHAR  SiS_DataBusWidth;
	USHORT SiS_ModeType;
	USHORT SiS_VBInfo;
	USHORT SiS_LCDResInfo;
	USHORT SiS_LCDTypeInfo;
	USHORT SiS_LCDInfo;
	USHORT SiS_VBType;
	USHORT SiS_VBExtInfo;
	USHORT SiS_HiVision;
	USHORT SiS_SelectCRT2Rate;
	USHORT SiS_SetFlag;
	USHORT SiS_RVBHCFACT;
	USHORT SiS_RVBHCMAX;
	USHORT SiS_RVBHRS;
	USHORT SiS_VGAVT;
	USHORT SiS_VGAHT;
	USHORT SiS_VT;
	USHORT SiS_HT;
	USHORT SiS_VGAVDE;
	USHORT SiS_VGAHDE;
	USHORT SiS_VDE;
	USHORT SiS_HDE;
	USHORT SiS_NewFlickerMode;
	USHORT SiS_RY1COE;
	USHORT SiS_RY2COE;
	USHORT SiS_RY3COE;
	USHORT SiS_RY4COE;
	USHORT SiS_LCDHDES;
	USHORT SiS_LCDVDES;
	USHORT SiS_DDC_Port;
	USHORT SiS_DDC_Index;
	USHORT SiS_DDC_Data;
	USHORT SiS_DDC_Clk;
	USHORT SiS_DDC_DataShift;
	USHORT SiS_DDC_DeviceAddr;
	USHORT SiS_DDC_ReadAddr;
	USHORT SiS_DDC_SecAddr;
	USHORT SiS_Panel800x600;
	USHORT SiS_Panel1024x768;
	USHORT SiS_Panel1280x1024;
	USHORT SiS_Panel1600x1200;
	USHORT SiS_Panel1280x960;
	USHORT SiS_Panel1400x1050;
	USHORT SiS_Panel320x480;
	USHORT SiS_Panel1152x768;
	USHORT SiS_Panel1280x768;
	USHORT SiS_Panel1024x600;
	USHORT SiS_Panel640x480;
	USHORT SiS_Panel640x480_2;
	USHORT SiS_Panel640x480_3;
	USHORT SiS_Panel1152x864;
	USHORT SiS_PanelCustom;
	USHORT SiS_PanelBarco1366;
	USHORT SiS_PanelMax;
	USHORT SiS_PanelMinLVDS;
	USHORT SiS_PanelMin301;
	USHORT SiS_ChrontelInit;
	
	const SiS_StStruct          *SiS_SModeIDTable;
	SiS_StandTableStruct        *SiS_StandTable;
	const SiS_ExtStruct         *SiS_EModeIDTable;
	const SiS_Ext2Struct        *SiS_RefIndex;
	const SiS_VBModeStruct      *SiS_VBModeIDTable;
	const SiS_CRT1TableStruct   *SiS_CRT1Table;
	const SiS_MCLKDataStruct    *SiS_MCLKData_0;
	const SiS_MCLKDataStruct    *SiS_MCLKData_1;
	const SiS_ECLKDataStruct    *SiS_ECLKData;
	const SiS_VCLKDataStruct    *SiS_VCLKData;
	const SiS_VBVCLKDataStruct  *SiS_VBVCLKData;
	const SiS_StResInfoStruct   *SiS_StResInfo;
	const SiS_ModeResInfoStruct *SiS_ModeResInfo;
	const UCHAR                 *SiS_ScreenOffset;

	const UCHAR                 *pSiS_OutputSelect;
	const UCHAR                 *pSiS_SoftSetting;

	const DRAM4Type *SiS_SR15; /* pointer : point to array */
#ifndef LINUX_XF86
	UCHAR *pSiS_SR07;
	const DRAM4Type *SiS_CR40; /* pointer : point to array */
	UCHAR *SiS_CR49;
	UCHAR *SiS_SR25;
	UCHAR *pSiS_SR1F;
	UCHAR *pSiS_SR21;
	UCHAR *pSiS_SR22;
	UCHAR *pSiS_SR23;
	UCHAR *pSiS_SR24;
	UCHAR *pSiS_SR31;
	UCHAR *pSiS_SR32;
	UCHAR *pSiS_SR33;
	UCHAR *pSiS_CRT2Data_1_2;
	UCHAR *pSiS_CRT2Data_4_D;
	UCHAR *pSiS_CRT2Data_4_E;
	UCHAR *pSiS_CRT2Data_4_10;
	const USHORT *pSiS_RGBSenseData;
	const USHORT *pSiS_VideoSenseData;
	const USHORT *pSiS_YCSenseData;
	const USHORT *pSiS_RGBSenseData2;
	const USHORT *pSiS_VideoSenseData2;
	const USHORT *pSiS_YCSenseData2;
#endif
	const UCHAR *SiS_NTSCPhase;
	const UCHAR *SiS_PALPhase;
	const UCHAR *SiS_NTSCPhase2;
	const UCHAR *SiS_PALPhase2;
	const UCHAR *SiS_PALMPhase;
	const UCHAR *SiS_PALNPhase;
	const UCHAR *SiS_PALMPhase2;
	const UCHAR *SiS_PALNPhase2;
	const UCHAR *SiS_SpecialPhase;
	const SiS_LCDDataStruct  *SiS_StLCD1024x768Data;
	const SiS_LCDDataStruct  *SiS_ExtLCD1024x768Data;
	const SiS_LCDDataStruct  *SiS_St2LCD1024x768Data;
	const SiS_LCDDataStruct  *SiS_StLCD1280x1024Data;
	const SiS_LCDDataStruct  *SiS_ExtLCD1280x1024Data;
	const SiS_LCDDataStruct  *SiS_St2LCD1280x1024Data;
	const SiS_LCDDataStruct  *SiS_NoScaleData1024x768;
	const SiS_LCDDataStruct  *SiS_NoScaleData1280x1024;
	const SiS_LCDDataStruct  *SiS_LCD1280x960Data;
	const SiS_LCDDataStruct  *SiS_NoScaleData1400x1050;
	const SiS_LCDDataStruct  *SiS_NoScaleData1600x1200;
	const SiS_LCDDataStruct  *SiS_NoScaleData1280x768;
	const SiS_LCDDataStruct  *SiS_StLCD1400x1050Data;
	const SiS_LCDDataStruct  *SiS_StLCD1600x1200Data;
	const SiS_LCDDataStruct  *SiS_StLCD1280x768Data;
	const SiS_LCDDataStruct  *SiS_ExtLCD1400x1050Data;
	const SiS_LCDDataStruct  *SiS_ExtLCD1600x1200Data;
	const SiS_LCDDataStruct  *SiS_ExtLCD1280x768Data;
	const SiS_LCDDataStruct  *SiS_NoScaleData;
	const SiS_TVDataStruct   *SiS_StPALData;
	const SiS_TVDataStruct   *SiS_ExtPALData;
	const SiS_TVDataStruct   *SiS_StNTSCData;
	const SiS_TVDataStruct   *SiS_ExtNTSCData;
	const SiS_TVDataStruct   *SiS_St2HiTVData;
	const SiS_TVDataStruct   *SiS_ExtHiTVData;
	const UCHAR *SiS_NTSCTiming;
	const UCHAR *SiS_PALTiming;
	const UCHAR *SiS_HiTVExtTiming;
	const UCHAR *SiS_HiTVSt1Timing;
	const UCHAR *SiS_HiTVSt2Timing;
	const UCHAR *SiS_HiTVTextTiming;
	const UCHAR *SiS_HiTVGroup3Data;
	const UCHAR *SiS_HiTVGroup3Simu;
	const UCHAR *SiS_HiTVGroup3Text;
	const SiS_PanelDelayTblStruct *SiS_PanelDelayTbl;
	const SiS_PanelDelayTblStruct *SiS_PanelDelayTblLVDS;
	const SiS_LVDSDataStruct  *SiS_LVDS800x600Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS800x600Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1024x768Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1024x768Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1280x1024Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1280x1024Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1280x960Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1280x960Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1400x1050Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1400x1050Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1600x1200Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1600x1200Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1280x768Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1280x768Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1024x600Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1024x600Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS1152x768Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS1152x768Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS640x480Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS640x480Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS320x480Data_1;
	const SiS_LVDSDataStruct  *SiS_LCDA1400x1050Data_1;
	const SiS_LVDSDataStruct  *SiS_LCDA1400x1050Data_2;
	const SiS_LVDSDataStruct  *SiS_LCDA1600x1200Data_1;
	const SiS_LVDSDataStruct  *SiS_LCDA1600x1200Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDSXXXxXXXData_1;
	const SiS_LVDSDataStruct  *SiS_LVDSBARCO1366Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDSBARCO1366Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDSBARCO1024Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDSBARCO1024Data_2;
	const SiS_LVDSDataStruct  *SiS_LVDS848x480Data_1;
	const SiS_LVDSDataStruct  *SiS_LVDS848x480Data_2;
	const SiS_LVDSDataStruct  *SiS_CHTVUNTSCData;
	const SiS_LVDSDataStruct  *SiS_CHTVONTSCData;
	const SiS_LVDSDataStruct  *SiS_CHTVUPALData;
	const SiS_LVDSDataStruct  *SiS_CHTVOPALData;
	const SiS_LVDSDataStruct  *SiS_CHTVUPALMData;
	const SiS_LVDSDataStruct  *SiS_CHTVOPALMData;
	const SiS_LVDSDataStruct  *SiS_CHTVUPALNData;
	const SiS_LVDSDataStruct  *SiS_CHTVOPALNData;
	const SiS_LVDSDataStruct  *SiS_CHTVSOPALData;
	const SiS_LVDSDesStruct  *SiS_PanelType00_1;
	const SiS_LVDSDesStruct  *SiS_PanelType01_1;
	const SiS_LVDSDesStruct  *SiS_PanelType02_1;
	const SiS_LVDSDesStruct  *SiS_PanelType03_1;
	const SiS_LVDSDesStruct  *SiS_PanelType04_1;
	const SiS_LVDSDesStruct  *SiS_PanelType05_1;
	const SiS_LVDSDesStruct  *SiS_PanelType06_1;
	const SiS_LVDSDesStruct  *SiS_PanelType07_1;
	const SiS_LVDSDesStruct  *SiS_PanelType08_1;
	const SiS_LVDSDesStruct  *SiS_PanelType09_1;
	const SiS_LVDSDesStruct  *SiS_PanelType0a_1;
	const SiS_LVDSDesStruct  *SiS_PanelType0b_1;
	const SiS_LVDSDesStruct  *SiS_PanelType0c_1;
	const SiS_LVDSDesStruct  *SiS_PanelType0d_1;
	const SiS_LVDSDesStruct  *SiS_PanelType0e_1;
	const SiS_LVDSDesStruct  *SiS_PanelType0f_1;
	const SiS_LVDSDesStruct  *SiS_PanelTypeNS_1;
	const SiS_LVDSDesStruct  *SiS_PanelType00_2;
	const SiS_LVDSDesStruct  *SiS_PanelType01_2;
	const SiS_LVDSDesStruct  *SiS_PanelType02_2;
	const SiS_LVDSDesStruct  *SiS_PanelType03_2;
	const SiS_LVDSDesStruct  *SiS_PanelType04_2;
	const SiS_LVDSDesStruct  *SiS_PanelType05_2;
	const SiS_LVDSDesStruct  *SiS_PanelType06_2;
	const SiS_LVDSDesStruct  *SiS_PanelType07_2;
	const SiS_LVDSDesStruct  *SiS_PanelType08_2;
	const SiS_LVDSDesStruct  *SiS_PanelType09_2;
	const SiS_LVDSDesStruct  *SiS_PanelType0a_2;
	const SiS_LVDSDesStruct  *SiS_PanelType0b_2;
	const SiS_LVDSDesStruct  *SiS_PanelType0c_2;
	const SiS_LVDSDesStruct  *SiS_PanelType0d_2;
	const SiS_LVDSDesStruct  *SiS_PanelType0e_2;
	const SiS_LVDSDesStruct  *SiS_PanelType0f_2;
	const SiS_LVDSDesStruct  *SiS_PanelTypeNS_2;

	const SiS_LVDSDesStruct  *LVDS1024x768Des_1;
	const SiS_LVDSDesStruct  *LVDS1280x1024Des_1;
	const SiS_LVDSDesStruct  *LVDS1400x1050Des_1;
	const SiS_LVDSDesStruct  *LVDS1600x1200Des_1;
	const SiS_LVDSDesStruct  *LVDS1024x768Des_2;
	const SiS_LVDSDesStruct  *LVDS1280x1024Des_2;
	const SiS_LVDSDesStruct  *LVDS1400x1050Des_2;
	const SiS_LVDSDesStruct  *LVDS1600x1200Des_2;

	const SiS_LVDSDesStruct  *SiS_CHTVUNTSCDesData;
	const SiS_LVDSDesStruct  *SiS_CHTVONTSCDesData;
	const SiS_LVDSDesStruct  *SiS_CHTVUPALDesData;
	const SiS_LVDSDesStruct  *SiS_CHTVOPALDesData;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1800x600_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x768_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x1024_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11400x1050_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x768_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x600_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11152x768_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11600x1200_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1800x600_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x768_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x1024_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11400x1050_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x768_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x600_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11152x768_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11600x1200_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1800x600_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x768_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x1024_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11400x1050_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x768_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x600_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11152x768_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11600x1200_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1800x600_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x768_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x1024_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11400x1050_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11280x768_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11024x600_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11152x768_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT11600x1200_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1XXXxXXX_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1XXXxXXX_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1640x480_1;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1640x480_1_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1640x480_2;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1640x480_2_H;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1640x480_3;
	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1640x480_3_H;
	const SiS_LVDSCRT1DataStruct  *SiS_CHTVCRT1UNTSC;
	const SiS_LVDSCRT1DataStruct  *SiS_CHTVCRT1ONTSC;
	const SiS_LVDSCRT1DataStruct  *SiS_CHTVCRT1UPAL;
	const SiS_LVDSCRT1DataStruct  *SiS_CHTVCRT1OPAL;
	const SiS_LVDSCRT1DataStruct  *SiS_CHTVCRT1SOPAL;

	const SiS_LVDSCRT1DataStruct  *SiS_LVDSCRT1320x480_1;

	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT1800x600_1;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11024x768_1;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11280x1024_1;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11400x1050_1;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11600x1200_1;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT1800x600_1_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11024x768_1_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11280x1024_1_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11400x1050_1_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11600x1200_1_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT1800x600_2;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11024x768_2;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11280x1024_2;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11400x1050_2;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11600x1200_2;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT1800x600_2_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11024x768_2_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11280x1024_2_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11400x1050_2_H;
	const SiS_LCDACRT1DataStruct  *SiS_LCDACRT11600x1200_2_H;

	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1024x768_1;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1280x1024_1;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1400x1050_1;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1600x1200_1;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1024x768_2;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1280x1024_2;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1400x1050_2;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1600x1200_2;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1024x768_3;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1280x1024_3;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1400x1050_3;
	const SiS_Part2PortTblStruct *SiS_CRT2Part2_1600x1200_3;

	const SiS_CHTVRegDataStruct *SiS_CHTVReg_UNTSC;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_ONTSC;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_UPAL;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_OPAL;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_UPALM;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_OPALM;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_UPALN;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_OPALN;
	const SiS_CHTVRegDataStruct *SiS_CHTVReg_SOPAL;
	const UCHAR *SiS_CHTVVCLKUNTSC;
	const UCHAR *SiS_CHTVVCLKONTSC;
	const UCHAR *SiS_CHTVVCLKUPAL;
	const UCHAR *SiS_CHTVVCLKOPAL;
	const UCHAR *SiS_CHTVVCLKUPALM;
	const UCHAR *SiS_CHTVVCLKOPALM;
	const UCHAR *SiS_CHTVVCLKUPALN;
	const UCHAR *SiS_CHTVVCLKOPALN;
	const UCHAR *SiS_CHTVVCLKSOPAL;

	USHORT  PanelXRes;
	USHORT  PanelYRes;
	
	BOOLEAN UseCustomMode;
	BOOLEAN CRT1UsesCustomMode;
	USHORT  CHDisplay;
	USHORT  CHSyncStart;
	USHORT  CHSyncEnd;
	USHORT  CHTotal;
	USHORT  CHBlankStart;
	USHORT  CHBlankEnd;
	USHORT  CVDisplay;
	USHORT  CVSyncStart;
	USHORT  CVSyncEnd;
	USHORT  CVTotal;
	USHORT  CVBlankStart;
	USHORT  CVBlankEnd;
	ULONG   CDClock;
	ULONG   CFlags;   
	UCHAR   CCRT1CRTC[17];
	UCHAR   CSR2B;
	UCHAR   CSR2C;
	USHORT  CSRClock;
	USHORT  CSRClock_CRT1;
	USHORT  CModeFlag;
	USHORT  CModeFlag_CRT1;
	USHORT  CInfoFlag;

	BOOLEAN SiS_CHPALM;
	BOOLEAN SiS_CHPALN;
	
	BOOLEAN Backup;
	UCHAR Backup_Mode;
	UCHAR Backup_14;
	UCHAR Backup_15;
	UCHAR Backup_16;
	UCHAR Backup_17;
	UCHAR Backup_18;
	UCHAR Backup_19;
	UCHAR Backup_1a;
	UCHAR Backup_1b;
	UCHAR Backup_1c;
	UCHAR Backup_1d;
	
	int     UsePanelScaler;

	USHORT  CP_Vendor, CP_Product;
	BOOLEAN CP_HaveCustomData;
	int     CP_PreferredX, CP_PreferredY;
	int	CP_MaxX, CP_MaxY, CP_MaxClock;
	int     CP_HDisplay[7], CP_VDisplay[7];	/* For Custom LCD panel dimensions */
    	int     CP_HTotal[7], CP_VTotal[7];
    	int     CP_HSyncStart[7], CP_VSyncStart[7];
    	int     CP_HSyncEnd[7], CP_VSyncEnd[7];
	int     CP_HBlankStart[7], CP_VBlankStart[7];
	int     CP_HBlankEnd[7], CP_VBlankEnd[7];
    	int     CP_Clock[7];
	BOOLEAN CP_DataValid[7];
	BOOLEAN CP_HSync_P[7], CP_VSync_P[7], CP_SyncValid[7];
} SiS_Private;

#endif



