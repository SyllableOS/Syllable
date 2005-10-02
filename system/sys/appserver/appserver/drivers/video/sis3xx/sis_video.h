#ifndef _SIS_VIDEO_H
#define _SIS_VIDEO_H

/* video registers (300/630/730/315/550/650/740 only) */
#define  Index_VI_Passwd                        0x00

/* Video overlay horizontal start/end, unit=screen pixels */
#define  Index_VI_Win_Hor_Disp_Start_Low        0x01
#define  Index_VI_Win_Hor_Disp_End_Low          0x02
#define  Index_VI_Win_Hor_Over                  0x03 /* Overflow */

/* Video overlay vertical start/end, unit=screen pixels */
#define  Index_VI_Win_Ver_Disp_Start_Low        0x04
#define  Index_VI_Win_Ver_Disp_End_Low          0x05
#define  Index_VI_Win_Ver_Over                  0x06 /* Overflow */

/* Y Plane (4:2:0) or YUV (4:2:2) buffer start address, unit=word */
#define  Index_VI_Disp_Y_Buf_Start_Low          0x07
#define  Index_VI_Disp_Y_Buf_Start_Middle       0x08
#define  Index_VI_Disp_Y_Buf_Start_High         0x09

/* U Plane (4:2:0) buffer start address, unit=word */
#define  Index_VI_U_Buf_Start_Low               0x0A
#define  Index_VI_U_Buf_Start_Middle            0x0B
#define  Index_VI_U_Buf_Start_High              0x0C

/* V Plane (4:2:0) buffer start address, unit=word */
#define  Index_VI_V_Buf_Start_Low               0x0D
#define  Index_VI_V_Buf_Start_Middle            0x0E
#define  Index_VI_V_Buf_Start_High              0x0F

/* Pitch for Y, UV Planes, unit=word */
#define  Index_VI_Disp_Y_Buf_Pitch_Low          0x10
#define  Index_VI_Disp_UV_Buf_Pitch_Low         0x11
#define  Index_VI_Disp_Y_UV_Buf_Pitch_Middle    0x12

/* What is this ? */
#define  Index_VI_Disp_Y_Buf_Preset_Low         0x13
#define  Index_VI_Disp_Y_Buf_Preset_Middle      0x14

#define  Index_VI_UV_Buf_Preset_Low             0x15
#define  Index_VI_UV_Buf_Preset_Middle          0x16
#define  Index_VI_Disp_Y_UV_Buf_Preset_High     0x17

/* Scaling control registers */
#define  Index_VI_Hor_Post_Up_Scale_Low         0x18
#define  Index_VI_Hor_Post_Up_Scale_High        0x19
#define  Index_VI_Ver_Up_Scale_Low              0x1A
#define  Index_VI_Ver_Up_Scale_High             0x1B
#define  Index_VI_Scale_Control                 0x1C

/* Playback line buffer control */
#define  Index_VI_Play_Threshold_Low            0x1D
#define  Index_VI_Play_Threshold_High           0x1E
#define  Index_VI_Line_Buffer_Size              0x1F

/* Destination color key */
#define  Index_VI_Overlay_ColorKey_Red_Min      0x20
#define  Index_VI_Overlay_ColorKey_Green_Min    0x21
#define  Index_VI_Overlay_ColorKey_Blue_Min     0x22
#define  Index_VI_Overlay_ColorKey_Red_Max      0x23
#define  Index_VI_Overlay_ColorKey_Green_Max    0x24
#define  Index_VI_Overlay_ColorKey_Blue_Max     0x25

/* Source color key, YUV color space */
#define  Index_VI_Overlay_ChromaKey_Red_Y_Min   0x26
#define  Index_VI_Overlay_ChromaKey_Green_U_Min 0x27
#define  Index_VI_Overlay_ChromaKey_Blue_V_Min  0x28
#define  Index_VI_Overlay_ChromaKey_Red_Y_Max   0x29
#define  Index_VI_Overlay_ChromaKey_Green_U_Max 0x2A
#define  Index_VI_Overlay_ChromaKey_Blue_V_Max  0x2B

/* Contrast enhancement and brightness control */
#define  Index_VI_Contrast_Factor               0x2C	/* obviously unused/undefined */
#define  Index_VI_Brightness                    0x2D
#define  Index_VI_Contrast_Enh_Ctrl             0x2E

#define  Index_VI_Key_Overlay_OP                0x2F

#define  Index_VI_Control_Misc0                 0x30
#define  Index_VI_Control_Misc1                 0x31
#define  Index_VI_Control_Misc2                 0x32

/* TW: Subpicture registers */
#define  Index_VI_SubPict_Buf_Start_Low		0x33
#define  Index_VI_SubPict_Buf_Start_Middle	0x34
#define  Index_VI_SubPict_Buf_Start_High	0x35

/* TW: What is this ? */
#define  Index_VI_SubPict_Buf_Preset_Low	0x36
#define  Index_VI_SubPict_Buf_Preset_Middle	0x37

/* TW: Subpicture pitch, unit=16 bytes */
#define  Index_VI_SubPict_Buf_Pitch		0x38

/* TW: Subpicture scaling control */
#define  Index_VI_SubPict_Hor_Scale_Low		0x39
#define  Index_VI_SubPict_Hor_Scale_High	0x3A
#define  Index_VI_SubPict_Vert_Scale_Low	0x3B
#define  Index_VI_SubPict_Vert_Scale_High	0x3C

#define  Index_VI_SubPict_Scale_Control		0x3D
/* (0x40 = enable/disable subpicture) */

/* TW: Subpicture line buffer control */
#define  Index_VI_SubPict_Threshold		0x3E

/* TW: What is this? */
#define  Index_VI_FIFO_Max			0x3F

/* TW: Subpicture palette; 16 colors, total 32 bytes address space */
#define  Index_VI_SubPict_Pal_Base_Low		0x40
#define  Index_VI_SubPict_Pal_Base_High		0x41

/* I wish I knew how to use these ... */
#define  Index_MPEG_Read_Ctrl0                  0x60	/* MPEG auto flip */
#define  Index_MPEG_Read_Ctrl1                  0x61	/* MPEG auto flip */
#define  Index_MPEG_Read_Ctrl2                  0x62	/* MPEG auto flip */
#define  Index_MPEG_Read_Ctrl3                  0x63	/* MPEG auto flip */

/* TW: MPEG AutoFlip scale */
#define  Index_MPEG_Ver_Up_Scale_Low            0x64
#define  Index_MPEG_Ver_Up_Scale_High           0x65

#define  Index_MPEG_Y_Buf_Preset_Low		0x66
#define  Index_MPEG_Y_Buf_Preset_Middle		0x67
#define  Index_MPEG_UV_Buf_Preset_Low		0x68
#define  Index_MPEG_UV_Buf_Preset_Middle	0x69
#define  Index_MPEG_Y_UV_Buf_Preset_High	0x6A

/* TW: The following registers only exist on the 310/325 series */

/* TW: Bit 16:24 of Y_U_V buf start address (?) */
#define  Index_VI_Y_Buf_Start_Over		0x6B
#define  Index_VI_U_Buf_Start_Over		0x6C
#define  Index_VI_V_Buf_Start_Over		0x6D

#define  Index_VI_Disp_Y_Buf_Pitch_High		0x6E
#define  Index_VI_Disp_UV_Buf_Pitch_High	0x6F

/* Hue and saturation */
#define	 Index_VI_Hue				0x70
#define  Index_VI_Saturation			0x71

#define  Index_VI_SubPict_Start_Over		0x72
#define  Index_VI_SubPict_Buf_Pitch_High	0x73

#define  Index_VI_Control_Misc3			0x74

/* 340 and later: */
/* DDA registers 0x75 - 0xb4 */
/* threshold high 0xb5, 0xb6 */
#define  Index_VI_Line_Buffer_Size_High		0xb7


/* TW: Bits (and helpers) for Index_VI_Control_Misc0 */
#define  VI_Misc0_Enable_Overlay		0x02
#define  VI_Misc0_420_Plane_Enable		0x04 /* Select Plane or Packed mode */
#define  VI_Misc0_422_Enable			0x20 /* Select 422 or 411 mode */
#define  VI_Misc0_Fmt_YVU420P			0x0C /* YUV420 Planar (I420, YV12) */
#define  VI_Misc0_Fmt_YUYV			0x28 /* YUYV Packed (YUY2) */
#define  VI_Misc0_Fmt_UYVY			0x08 /* (UYVY) */

/* TW: Bits for Index_VI_Control_Misc1 */
/* #define  VI_Misc1_?                          0x01  */
#define  VI_Misc1_BOB_Enable			0x02
#define	 VI_Misc1_Line_Merge			0x04
#define  VI_Misc1_Field_Mode			0x08
/* #define  VI_Misc1_?                          0x10  */
#define  VI_Misc1_Non_Interleave                0x20 /* 300 series only? */
#define  VI_Misc1_Buf_Addr_Lock			0x20 /* 310 series only? */
/* #define  VI_Misc1_?                          0x40  */
/* #define  VI_Misc1_?                          0x80  */

/* TW: Bits for Index_VI_Control_Misc2 */
#define  VI_Misc2_Select_Video2			0x01
#define  VI_Misc2_Video2_On_Top			0x02
/* #define  VI_Misc2_?                          0x04  */
#define  VI_Misc2_Vertical_Interpol		0x08
#define  VI_Misc2_Dual_Line_Merge               0x10
#define  VI_Misc2_All_Line_Merge                0x20  /* 310 series only? */
#define  VI_Misc2_Auto_Flip_Enable		0x40  /* 300 series only? */
#define  VI_Misc2_Video_Reg_Write_Enable        0x80  /* 310 series only? */

/* TW: Bits for Index_VI_Control_Misc3 */
#define  VI_Misc3_Submit_Video_1		0x01  /* AKA "address ready" */
#define  VI_Misc3_Submit_Video_2		0x02  /* AKA "address ready" */
#define  VI_Misc3_Submit_SubPict		0x04  /* AKA "address ready" */

/* TW: Values for Index_VI_Key_Overlay_OP (0x2F) */
#define  VI_ROP_Never				0x00
#define  VI_ROP_DestKey				0x03
#define  VI_ROP_Always				0x0F


typedef struct {
    uint32       bufAddr;

    short drw_x, drw_y, drw_w, drw_h;
    short src_x, src_y, src_w, src_h;   
    int id;
    short srcPitch, height;
    
    char          brightness;
    unsigned char contrast;
    char 	  hue;
    char          saturation;

    //RegionRec    clip;
    uint32       colorKey;
    bool 	 autopaintColorKey;

    uint32       videoStatus;

    uint32       displayMode;
    bool	 bridgeIsSlave;


    bool         needToScale;      /* TW: Need to scale video */

    int          shiftValue;       /* 550/650 need word addr/pitch, 630 double word */

    short        oldx1, oldx2, oldy1, oldy2;
    int          mustwait;

    int          pitch;
    int          offset;

} SISPortPrivRec, *SISPortPrivPtr;  


typedef struct {
    int pixelFormat;

    uint16  pitch;
    uint16  origPitch;

    uint8   keyOP;
    uint16  HUSF;
    uint16  VUSF;
    uint8   IntBit;
    uint8   wHPre;

    uint16  srcW;
    uint16  srcH;

   // BoxRec  dstBox;
   uint32 dx1, dy1, dx2, dy2;

    uint32  PSY;
    uint32  PSV;
    uint32  PSU;
    uint8   bobEnable;

    uint8   contrastCtrl;
    uint8   contrastFactor;

    uint8   lineBufSize;

    uint8   (*VBlankActiveFunc)();


    uint16  SCREENheight;
    
} SISOverlayRec, *SISOverlayPtr;


#endif








