/*
 * Syllable Driver for SiS chipsets
 *
 * Syllable specific code is
 * Copyright (c) 2003, Arno Klenke
 *
 * Other code is
 * Copyright (c) Thomas Winischhofer <thomas@winischhofer.net>:
 * Copyright (c) SiS (www.sis.com.tw)
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
 */


#include "init.h"
#include "init301.h"
#include <string.h>
#include <stdlib.h>

#define WATCHDOG_DELAY  500000 /* Watchdog counter for Vertical Restrace waiting */

#define IMAGE_MIN_WIDTH         32  /* Minimum and maximum source image sizes */
#define IMAGE_MIN_HEIGHT        24
#define IMAGE_MAX_WIDTH        720
#define IMAGE_MAX_HEIGHT       576
#define IMAGE_MAX_WIDTH_M650  1920
#define IMAGE_MAX_HEIGHT_M650 1080

#define OVERLAY_MIN_WIDTH       32  /* Minimum overlay sizes */
#define OVERLAY_MIN_HEIGHT      24

#define DISPMODE_SINGLE1 0x1  /* TW: CRT1 only */
#define DISPMODE_SINGLE2 0x2  /* TW: CRT2 only */

#define PIXEL_FMT_YV12 0x01
#define PIXEL_FMT_UYVY 0x02
#define PIXEL_FMT_YUY2 0x03
#define PIXEL_FMT_I420 0x04
#define PIXEL_FMT_RGB5 0x05
#define PIXEL_FMT_RGB6 0x06


/* TW: Note on "MIRROR":
 *     When using VESA on machines with an enabled video bridge, this means
 *     a real mirror. CRT1 and CRT2 have the exact same resolution and
 *     refresh rate. The same applies to modes which require the bridge to
 *     operate in slave mode.
 *     When not using VESA and the bridge is not in slave mode otherwise,
 *     CRT1 and CRT2 have the same resolution but possibly a different
 *     refresh rate.
 */

/****************************************************************************
 * Raw register access : These routines directly interact with the sis's
 *                       control aperature.  Must not be called until after
 *                       the board's pci memory has been mapped.
 ****************************************************************************/


static uint8 getvideoreg( uint8 reg)
{
    uint8 ret;
    inSISIDXREG(SISVID, reg, ret);
    return(ret);
}

static void setvideoreg( uint8 reg, uint8 data)
{
    outSISIDXREG(SISVID, reg, data);
}

static void setvideoregmask( uint8 reg, uint8 data, uint8 mask)
{
    uint8   old;

    inSISIDXREG(SISVID, reg, old);
    data = (data & mask) | (old & (~mask));
    outSISIDXREG(SISVID, reg, data);
}

static void setsrregmask( uint8 reg, uint8 data, uint8 mask)
{
    uint8   old;

    inSISIDXREG(SISSR, reg, old);
    data = (data & mask) | (old & (~mask));
    outSISIDXREG(SISSR, reg, data);
}


/* VBlank */
static uint8 vblank_active_CRT1( )
{
    return (inSISREG(SISINPSTAT) & 0x08);
}

static uint8 vblank_active_CRT2( )
{
    uint8 ret;
    if(si.vga_engine == SIS_315_VGA) {
       inSISIDXREG(SISPART1, Index_310_CRT2_FC_VR, ret);
    } else {
       inSISIDXREG(SISPART1, Index_CRT2_FC_VR, ret);
    }
    return((ret & 0x02) ^ 0x02);
}


BOOLEAN
SiSBridgeIsInSlaveMode()
{
    unsigned char usScratchP1_00;

    if(!(si.hasVB != HASVB_NONE)) return FALSE;

    inSISIDXREG(SISPART1,0x00,usScratchP1_00);
    if( ((si.vga_engine == SIS_300_VGA) && (usScratchP1_00 & 0xa0) == 0x20) ||
        ((si.vga_engine == SIS_315_VGA) && (usScratchP1_00 & 0x50) == 0x10) ) {
	   return TRUE;
    } else {
           return FALSE;
    }
}

static void 
SISResetVideo()
{
	register unsigned char val;
   
	inSISIDXREG(SISSR, 0x05, val);
	if(val != 0xa1) {
       /* unlock */
       outSISIDXREG(SISSR, 0x05, 0x86);
       inSISIDXREG(SISSR, 0x05, val);
       if( val != 0xa1 )
       	dbprintf("Video password could not unlock registers\n");
	}
	
    if (getvideoreg (Index_VI_Passwd) != 0xa1) {
        setvideoreg (Index_VI_Passwd, 0x86);
        if (getvideoreg (Index_VI_Passwd) != 0xa1)
            dbprintf("Video password could not unlock registers\n");
    }

    /* Initialize first overlay (CRT1) ------------------------------- */

    /* Write-enable video registers */
    setvideoregmask(Index_VI_Control_Misc2,         0x80, 0x81);

    /* Disable overlay */
    setvideoregmask(Index_VI_Control_Misc0,         0x00, 0x02);

    /* Disable bobEnable */
    setvideoregmask(Index_VI_Control_Misc1,         0x02, 0x02);

    /* Reset scale control and contrast */
    setvideoregmask(Index_VI_Scale_Control,         0x60, 0x60);
    setvideoregmask(Index_VI_Contrast_Enh_Ctrl,     0x04, 0x1F);
  
    setvideoreg(Index_VI_Disp_Y_Buf_Preset_Low,     0x00);
    setvideoreg(Index_VI_Disp_Y_Buf_Preset_Middle,  0x00);
    setvideoreg(Index_VI_UV_Buf_Preset_Low,         0x00);
    setvideoreg(Index_VI_UV_Buf_Preset_Middle,      0x00);
    setvideoreg(Index_VI_Disp_Y_UV_Buf_Preset_High, 0x00);
    setvideoreg(Index_VI_Play_Threshold_Low,        0x00);
    setvideoreg(Index_VI_Play_Threshold_High,       0x00);
    

    /* set default properties for overlay 1 (CRT1) -------------------------- */
    setvideoregmask (Index_VI_Control_Misc2,        0x00, 0x01);
    setvideoregmask (Index_VI_Contrast_Enh_Ctrl,    0x04, 0x07);
    setvideoreg (Index_VI_Brightness,               0x20);
    if (si.vga_engine == SIS_315_VGA) {
      	setvideoreg (Index_VI_Hue,          	   0x00);
       	setvideoreg (Index_VI_Saturation,            0x00);
    }
    
    setvideoregmask (Index_VI_Control_Misc2,        0x01, 0x01);
    	setvideoregmask (Index_VI_Contrast_Enh_Ctrl,    0x04, 0x07);
    	setvideoreg (Index_VI_Brightness,               0x20);
    	if (si.vga_engine == SIS_315_VGA) {
       		setvideoreg (Index_VI_Hue,              0x00);
       		setvideoreg (Index_VI_Saturation,       0x00);
    	}
}


/* TW: Set display mode (single CRT1/CRT2, mirror).
 *     MIRROR mode is only available on chipsets with two overlays.
 *     On the other chipsets, if only CRT1 or only CRT2 are used,
 *     the correct display CRT is chosen automatically. If both
 *     CRT1 and CRT2 are connected, the user can choose between CRT1 and
 *     CRT2 by using the option XvOnCRT2.
 */
static void
set_dispmode()
{
 	si.video_port.bridgeIsSlave = FALSE;

    if(SiSBridgeIsInSlaveMode()) si.video_port.bridgeIsSlave = TRUE;

    if( (si.disp_state & DISPMODE_MIRROR ) ||
        ((si.video_port.bridgeIsSlave) && (si.disp_state & DISPTYPE_DISP2)) )  {
	  si.video_port.displayMode = DISPMODE_SINGLE2;
    } else {
      if(si.disp_state & DISPTYPE_DISP1) {
      	si.video_port.displayMode = DISPMODE_SINGLE1;  /* TW: CRT1 only */
      } else {
        si.video_port.displayMode = DISPMODE_SINGLE2;  /* TW: CRT2 only */
      }
    }
}

static void
set_disptype_regs()
{

    /* TW:
     *     SR06[7:6]
     *	      Bit 7: Enable overlay 2 on CRT2
     *	      Bit 6: Enable overlay 1 on CRT2
     *     SR32[7:6]
     *        Bit 7: DCLK/TCLK overlay 2
     *               0=DCLK (overlay on CRT1)
     *               1=TCLK (overlay on CRT2)
     *        Bit 6: DCLK/TCLK overlay 1
     *               0=DCLK (overlay on CRT1)
     *               1=TCLK (overlay on CRT2)
     *
     * On chipsets with two overlays, we can freely select and also
     * have a mirror mode. However, we use overlay 1 for CRT1 and
     * overlay 2 for CRT2.
     * For chipsets with only one overlay, user must choose whether
     * to display the overlay on CRT1 or CRT2 by setting XvOnCRT2
     * to TRUE (CRT2) or FALSE (CRT1). The hardware does not
     * support any kind of "Mirror" mode on these chipsets.
     */
	register unsigned char val;
   
	inSISIDXREG(SISSR, 0x05, val);
	if(val != 0xa1) {
       /* unlock */
       outSISIDXREG(SISSR, 0x05, 0x86);
       inSISIDXREG(SISSR, 0x05, val);
	}
    

 	switch (si.video_port.displayMode)
	{
        case DISPMODE_SINGLE1:				/* TW: CRT1 only */
			setsrregmask (0x06, 0x00, 0xc0);
			setsrregmask (0x32, 0x00, 0xc0);
		break;
       	case DISPMODE_SINGLE2:  			/* TW: CRT2 only */
			setsrregmask (0x06, 0x40, 0xc0);
			setsrregmask (0x32, 0x40, 0xc0);
		break;
    	case DISPMODE_MIRROR:				/* TW: CRT1 + CRT2 */
    	default:					
			setsrregmask (0x06, 0x80, 0xc0);
			setsrregmask (0x32, 0x80, 0xc0);
		break;
	}
}

static void
SISSetPortDefaults()
{
    si.video_port.videoStatus = 0;
    si.video_port.brightness  = 0;
    si.video_port.contrast    = 4;
    si.video_port.hue         = 0;
    si.video_port.saturation  = 0;
    si.video_port.autopaintColorKey = TRUE;
}

void SISSetupImageVideo()
{
	si.video_port.videoStatus = 0;

    SISSetPortDefaults();
    
     /* TW: 300 series require double words for addresses and pitches,
     *     310/325 series accept word.
     */
    switch (si.vga_engine) {
    case SIS_315_VGA:
    	si.video_port.shiftValue = 1;
	break;
    case SIS_300_VGA:
    default:
    	si.video_port.shiftValue = 2;
	break;
    }
    
     /* Set displayMode according to VBFlags */
    set_dispmode();
    
    SISSetPortDefaults();

    /* Set SR(06, 32) registers according to DISPMODE */
    set_disptype_regs();

    SISResetVideo();
}

static void
calc_scale_factor(SISOverlayPtr pOverlay, int index, int iscrt2)
{
  uint32 I=0,mult=0;
  int flag=0;

  int dstW = pOverlay->dx2 - pOverlay->dx1;
  int dstH = pOverlay->dy2 - pOverlay->dy1;
  int srcW = pOverlay->srcW;
  int srcH = pOverlay->srcH;
  uint16 LCDheight = si.LCDheight;
  int srcPitch = pOverlay->origPitch;
  int origdstH = dstH;

  /* TW: Stretch image due to idiotic LCD "auto"-scaling on LVDS (and 630+301B) */
	if(si.disp_state & DISPTYPE_LCD) {
		if(si.video_port.bridgeIsSlave) {
			if(si.hasVB & HASVB_LVDS) {
				dstH = (dstH * LCDheight) / pOverlay->SCREENheight;
			} else if( (si.vga_engine == SIS_300_VGA) &&
		  		(si.hasVB & (HASVB_301|HASVB_302|HASVB_303)) ) {
				dstH = (dstH * LCDheight) / pOverlay->SCREENheight;
			}
		} else if(iscrt2) {
  			if (si.hasVB & HASVB_LVDS) {
   				dstH = (dstH * LCDheight) / pOverlay->SCREENheight;
				if (si.video_port.displayMode == DISPMODE_MIRROR) flag = 1;
    		} else if ( (si.vga_engine == SIS_300_VGA) &&
         	  (si.hasVB & (HASVB_301|HASVB_302|HASVB_303)) ) {
    			dstH = (dstH * LCDheight) / pOverlay->SCREENheight;
				if (si.video_port.displayMode == DISPMODE_MIRROR) flag = 1;
    		}
		}
	}
  
  if(dstW < OVERLAY_MIN_WIDTH) dstW = OVERLAY_MIN_WIDTH;
  if (dstW == srcW) {
        pOverlay->HUSF   = 0x00;
        pOverlay->IntBit = 0x05;
	pOverlay->wHPre  = 0;
  } else if (dstW > srcW) {
        dstW += 2;
        pOverlay->HUSF   = (srcW << 16) / dstW;
        pOverlay->IntBit = 0x04;
	pOverlay->wHPre  = 0;
  } else {
        int tmpW = dstW;

	/* TW: It seems, the hardware can't scale below factor .125 (=1/8) if the
	       pitch isn't a multiple of 256.
	       TODO: Test this on the 310/325 series!
	 */
	if((srcPitch % 256) || (srcPitch < 256)) {
	   if(((dstW * 1000) / srcW) < 125) dstW = tmpW = ((srcW * 125) / 1000) + 1;
	}

        I = 0;
        pOverlay->IntBit = 0x01;
        while (srcW >= tmpW) {
            tmpW <<= 1;
            I++;
        }
        pOverlay->wHPre = (uint8)(I - 1);
        dstW <<= (I - 1);
        if ((srcW % dstW))
            pOverlay->HUSF = ((srcW - dstW) << 16) / dstW;
        else
            pOverlay->HUSF = 0x00;
  }

  if(dstH < OVERLAY_MIN_HEIGHT) dstH = OVERLAY_MIN_HEIGHT;
  if (dstH == srcH) {
        pOverlay->VUSF   = 0x00;
        pOverlay->IntBit |= 0x0A;
  } else if (dstH > srcH) {
        dstH += 0x02;
        pOverlay->VUSF = (srcH << 16) / dstH;
        pOverlay->IntBit |= 0x08;
  } else {
        uint32 realI;

        I = realI = srcH / dstH;
        pOverlay->IntBit |= 0x02;

        if (I < 2) {
            pOverlay->VUSF = ((srcH - dstH) << 16) / dstH;
	    /* TW: Needed for LCD-scaling modes */
	    if ((flag) && (mult = (srcH / origdstH)) >= 2)
	    		pOverlay->pitch /= mult;
        } else {

            if (((srcPitch * I)>>2) > 0xFFF) {
                I = (0xFFF*2/srcPitch);
                pOverlay->VUSF = 0xFFFF;
            } else {
                dstH = I * dstH;
                if (srcH % dstH)
                    pOverlay->VUSF = ((srcH - dstH) << 16) / dstH;
                else
                    pOverlay->VUSF = 0x00;
            }
            /* set video frame buffer offset */
            pOverlay->pitch = (uint16)(srcPitch*I);
        }
   }
}

static void
set_line_buf_size(SISOverlayPtr pOverlay)
{
    uint8  preHIDF;
    uint32 I;
    uint32 line = pOverlay->srcW;

    if ( (pOverlay->pixelFormat == PIXEL_FMT_YV12) ||
         (pOverlay->pixelFormat == PIXEL_FMT_I420) )
    {
        preHIDF = pOverlay->wHPre & 0x07;
        switch (preHIDF)
        {
            case 3 :
                if ((line & 0xffffff00) == line)
                   I = (line >> 8);
                else
                   I = (line >> 8) + 1;
                pOverlay->lineBufSize = (uint8)(I * 32 - 1);
                break;
            case 4 :
                if ((line & 0xfffffe00) == line)
                   I = (line >> 9);
                else
                   I = (line >> 9) + 1;
                pOverlay->lineBufSize = (uint8)(I * 64 - 1);
                break;
            case 5 :
                if ((line & 0xfffffc00) == line)
                   I = (line >> 10);
                else
                   I = (line >> 10) + 1;
                pOverlay->lineBufSize = (uint8)(I * 128 - 1);
                break;
            case 6 :
                if ((line & 0xfffff800) == line)
                   I = (line >> 11);
                else
                   I = (line >> 11) + 1;
                pOverlay->lineBufSize = (uint8)(I * 256 - 1);
                break;
            default :
                if ((line & 0xffffff80) == line)
                   I = (line >> 7);
                else
                   I = (line >> 7) + 1;
                pOverlay->lineBufSize = (uint8)(I * 16 - 1);
                break;
        }
    } else { /* YUV2, UYVY */
        if ((line & 0xffffff8) == line)
            I = (line >> 3);
        else
            I = (line >> 3) + 1;
        pOverlay->lineBufSize = (uint8)(I - 1);
    }
}

static void
merge_line_buf(bool enable)
{
  if(enable) {
    switch (si.video_port.displayMode){
    case DISPMODE_SINGLE1:
		setvideoregmask(Index_VI_Control_Misc2, 0x10, 0x11);
		setvideoregmask(Index_VI_Control_Misc1, 0x00, 0x04);
      	break;
    case DISPMODE_SINGLE2:
    	setvideoregmask(Index_VI_Control_Misc2, 0x10, 0x11);
		setvideoregmask(Index_VI_Control_Misc1, 0x00, 0x04);
     	break;
    case DISPMODE_MIRROR:
    default:
     	setvideoregmask(Index_VI_Control_Misc2, 0x00, 0x11);
      	setvideoregmask(Index_VI_Control_Misc1, 0x04, 0x04);
      	break;
    }
  } else {
    switch (si.video_port.displayMode) {
    case DISPMODE_SINGLE1:
    	setvideoregmask(Index_VI_Control_Misc2, 0x00, 0x11);
    	setvideoregmask(Index_VI_Control_Misc1, 0x00, 0x04);
    	break;
    case DISPMODE_SINGLE2:
    	setvideoregmask(Index_VI_Control_Misc2, 0x00, 0x11);
 		setvideoregmask(Index_VI_Control_Misc1, 0x00, 0x04);
	break;
    case DISPMODE_MIRROR:
    default:
    	setvideoregmask(Index_VI_Control_Misc2, 0x00, 0x11);
    	setvideoregmask(Index_VI_Control_Misc1, 0x00, 0x04);
        break;
    }
  }
}

static void
set_format(SISOverlayPtr pOverlay)
{
    uint8 fmt;

    switch (pOverlay->pixelFormat){
    case PIXEL_FMT_YV12:
    case PIXEL_FMT_I420:
        fmt = 0x0c;
        break;
    case PIXEL_FMT_YUY2:
        fmt = 0x28; 
        break;
    case PIXEL_FMT_UYVY:
        fmt = 0x08;
        break;
    case PIXEL_FMT_RGB5:   /* D[5:4] : 00 RGB555, 01 RGB 565 */
        fmt = 0x00;
	break;
    case PIXEL_FMT_RGB6:
        fmt = 0x10;
	break;
    default:
        fmt = 0x00;
        break;
    }
    setvideoregmask(Index_VI_Control_Misc0, fmt, 0x7c);
}

static void
set_colorkey(uint32 colorkey)
{
    uint8 r, g, b;

    b = (uint8)(colorkey & 0xFF);
    g = (uint8)((colorkey>>8) & 0xFF);
    r = (uint8)((colorkey>>16) & 0xFF);

    /* Activate the colorkey mode */
    setvideoreg(Index_VI_Overlay_ColorKey_Blue_Min  ,(uint8)b);
    setvideoreg(Index_VI_Overlay_ColorKey_Green_Min ,(uint8)g);
    setvideoreg(Index_VI_Overlay_ColorKey_Red_Min   ,(uint8)r);

    setvideoreg(Index_VI_Overlay_ColorKey_Blue_Max  ,(uint8)b);
    setvideoreg(Index_VI_Overlay_ColorKey_Green_Max ,(uint8)g);
    setvideoreg(Index_VI_Overlay_ColorKey_Red_Max   ,(uint8)r);
}

static void
set_brightness(uint8 brightness)
{
    setvideoreg(Index_VI_Brightness, brightness);
}

static void
set_contrast(uint8 contrast)
{
    setvideoregmask(Index_VI_Contrast_Enh_Ctrl, contrast, 0x07);
}

/* 310/325 series only */
static void
set_saturation(char saturation)
{
    uint8 temp = 0;
    
    if(saturation < 0) {
    	temp |= 0x88;
	saturation = -saturation;
    }
    temp |= (saturation & 0x07);
    temp |= ((saturation & 0x07) << 4);
    
    setvideoreg(Index_VI_Saturation, temp);
}

/* 310/325 series only */
static void
set_hue(uint8 hue)
{
    setvideoreg(Index_VI_Hue, (hue & 0x08) ? (hue ^ 0x07) : hue);
}

static void
set_overlay(SISOverlayPtr pOverlay, int index)
{

    uint16 pitch=0;
    uint8  h_over=0, v_over=0;
    uint16 top, bottom, left, right;
    uint16 screenX = si.width;
    uint16 screenY = si.height;
    uint8  data;
    uint32 watchdog;

    top = pOverlay->dy1;
    bottom = pOverlay->dy2;
    if (bottom > screenY) {
        bottom = screenY;
    }

    left = pOverlay->dx1;
    right = pOverlay->dx2;
    if (right > screenX) {
        right = screenX;
    }

    h_over = (((left>>8) & 0x0f) | ((right>>4) & 0xf0));
    v_over = (((top>>8) & 0x0f) | ((bottom>>4) & 0xf0));

    pitch = pOverlay->pitch >> si.video_port.shiftValue;

    /* set line buffer size */
    setvideoreg(Index_VI_Line_Buffer_Size, pOverlay->lineBufSize);

    /* set color key mode */
    setvideoregmask (Index_VI_Key_Overlay_OP, pOverlay->keyOP, 0x0f);

    /* TW: We don't have to wait for vertical retrace in all cases */
    if(si.video_port.mustwait) {
	watchdog = WATCHDOG_DELAY;
    	while (pOverlay->VBlankActiveFunc() && --watchdog);
	watchdog = WATCHDOG_DELAY;
	while ((!pOverlay->VBlankActiveFunc()) && --watchdog);
	if (!watchdog) dbprintf("Waiting for vertical retrace timed-out\n");
    }

    /* Unlock address registers */
    data = getvideoreg(Index_VI_Control_Misc1);
    setvideoreg (Index_VI_Control_Misc1, data | 0x20);
    /* TEST: Is this required? */
    setvideoreg (Index_VI_Control_Misc1, data | 0x20);
    /* TEST end */

    /* TEST: Is this required? */
    if (si.vga_engine == SIS_315_VGA)
    	setvideoreg (Index_VI_Control_Misc3, 0x00);
    /* TEST end */

    /* Set Y buf pitch */
    setvideoreg (Index_VI_Disp_Y_Buf_Pitch_Low, (uint8)(pitch));
    setvideoregmask (Index_VI_Disp_Y_UV_Buf_Pitch_Middle, (uint8)(pitch>>8), 0x0f);

    /* Set Y start address */
    setvideoreg (Index_VI_Disp_Y_Buf_Start_Low,    (uint8)(pOverlay->PSY));
    setvideoreg (Index_VI_Disp_Y_Buf_Start_Middle, (uint8)((pOverlay->PSY)>>8));
    setvideoreg (Index_VI_Disp_Y_Buf_Start_High,   (uint8)((pOverlay->PSY)>>16));

    /* set 310/325 series overflow bits for Y plane */
    if (si.vga_engine == SIS_315_VGA) {
        setvideoreg (Index_VI_Disp_Y_Buf_Pitch_High, (uint8)(pitch>>12));
    	setvideoreg (Index_VI_Y_Buf_Start_Over, ((uint8)((pOverlay->PSY)>>24) & 0x01));
    }

    /* Set U/V data if using plane formats */
    if ( (pOverlay->pixelFormat == PIXEL_FMT_YV12) ||
    	 (pOverlay->pixelFormat == PIXEL_FMT_I420) )  {

        uint32  PSU=0, PSV=0;

        PSU = pOverlay->PSU;
        PSV = pOverlay->PSV;

	/* Set U/V pitch */
	setvideoreg (Index_VI_Disp_UV_Buf_Pitch_Low, (uint8)(pitch >> 1));
    setvideoregmask (Index_VI_Disp_Y_UV_Buf_Pitch_Middle, (uint8)(pitch >> 5), 0xf0);

        /* set U/V start address */
        setvideoreg (Index_VI_U_Buf_Start_Low,   (uint8)PSU);
        setvideoreg (Index_VI_U_Buf_Start_Middle,(uint8)(PSU>>8));
        setvideoreg (Index_VI_U_Buf_Start_High,  (uint8)(PSU>>16));

        setvideoreg (Index_VI_V_Buf_Start_Low,   (uint8)PSV);
        setvideoreg (Index_VI_V_Buf_Start_Middle,(uint8)(PSV>>8));
        setvideoreg (Index_VI_V_Buf_Start_High,  (uint8)(PSV>>16));

	/* 310/325 series overflow bits */
	if (si.vga_engine == SIS_315_VGA) {
	   setvideoreg (Index_VI_Disp_UV_Buf_Pitch_High, (uint8)(pitch>>13));
	   setvideoreg (Index_VI_U_Buf_Start_Over, ((uint8)(PSU>>24) & 0x01));
	   setvideoreg (Index_VI_V_Buf_Start_Over, ((uint8)(PSV>>24) & 0x01));
	}
    }

    if (si.vga_engine == SIS_315_VGA) {
	/* Trigger register copy for 310 series */
	setvideoreg(Index_VI_Control_Misc3, 1 << index);
    }

    /* set scale factor */
    setvideoreg (Index_VI_Hor_Post_Up_Scale_Low, (uint8)(pOverlay->HUSF));
    setvideoreg (Index_VI_Hor_Post_Up_Scale_High,(uint8)((pOverlay->HUSF)>>8));
    setvideoreg (Index_VI_Ver_Up_Scale_Low,      (uint8)(pOverlay->VUSF));
    setvideoreg (Index_VI_Ver_Up_Scale_High,     (uint8)((pOverlay->VUSF)>>8));

    setvideoregmask (Index_VI_Scale_Control,     (pOverlay->IntBit << 3)
                                                      |(pOverlay->wHPre), 0x7f);

    /* set destination window position */
    setvideoreg(Index_VI_Win_Hor_Disp_Start_Low, (uint8)left);
    setvideoreg(Index_VI_Win_Hor_Disp_End_Low,   (uint8)right);
    setvideoreg(Index_VI_Win_Hor_Over,           (uint8)h_over);

    setvideoreg(Index_VI_Win_Ver_Disp_Start_Low, (uint8)top);
    setvideoreg(Index_VI_Win_Ver_Disp_End_Low,   (uint8)bottom);
    setvideoreg(Index_VI_Win_Ver_Over,           (uint8)v_over);

    setvideoregmask(Index_VI_Control_Misc1, pOverlay->bobEnable, 0x1a);

    /* Lock the address registers */
   // setvideoregmask(Index_VI_Control_Misc1, 0x00, 0x20);
}

static void
close_overlay()
{
  uint32 watchdog;

  if ((si.video_port.displayMode == DISPMODE_SINGLE2) ||
      (si.video_port.displayMode == DISPMODE_MIRROR)) {
    if (si.video_port.displayMode == DISPMODE_SINGLE2) {
      	setvideoregmask (Index_VI_Control_Misc2, 0x00, 0x01);
     	watchdog = WATCHDOG_DELAY;
     	while(vblank_active_CRT1() && --watchdog);
     	watchdog = WATCHDOG_DELAY;
     	while((!vblank_active_CRT1()) && --watchdog);
     	setvideoregmask(Index_VI_Control_Misc0, 0x00, 0x02);
     	watchdog = WATCHDOG_DELAY;
     	while(vblank_active_CRT1() && --watchdog);
     	watchdog = WATCHDOG_DELAY;
     	while((!vblank_active_CRT1()) && --watchdog);
     }
  }
  if ((si.video_port.displayMode == DISPMODE_SINGLE1) ||
      (si.video_port.displayMode == DISPMODE_MIRROR)) {
     setvideoregmask (Index_VI_Control_Misc2, 0x00, 0x01);
     watchdog = WATCHDOG_DELAY;
     while(vblank_active_CRT1() && --watchdog);
     watchdog = WATCHDOG_DELAY;
     while((!vblank_active_CRT1()) && --watchdog);
     setvideoregmask(Index_VI_Control_Misc0, 0x00, 0x02);
     watchdog = WATCHDOG_DELAY;
     while(vblank_active_CRT1() && --watchdog);
     watchdog = WATCHDOG_DELAY;
     while((!vblank_active_CRT1()) && --watchdog);
  }
}

static void
SISDisplayVideo()
{
   short srcPitch = si.video_port.srcPitch;
   short height = si.video_port.height;
   SISOverlayRec overlay; 
   int srcOffsetX=0, srcOffsetY=0;
   int sx, sy;
   int index = 0, iscrt2 = 0;

   memset(&overlay, 0, sizeof(overlay));
   overlay.pixelFormat = si.video_port.id;
   overlay.pitch = overlay.origPitch = srcPitch;
   overlay.keyOP = 0x03;	/* DestKey mode */
   /* overlay.bobEnable = 0x02; */
   overlay.bobEnable = 0x00;    /* Disable BOB (whatever that is) */

   overlay.SCREENheight = si.height;
   
   overlay.dx1 = si.video_port.drw_x;
   overlay.dx2 = si.video_port.drw_x + si.video_port.drw_w;
   overlay.dy1 = si.video_port.drw_y;
   overlay.dy2 = si.video_port.drw_y + si.video_port.drw_h;

   if((overlay.dx1 > overlay.dx2) ||
   		(overlay.dy1 > overlay.dy2)) {
     dbprintf("Overlay has strange values!\n");
    }

   if((overlay.dx2 < 0) || (overlay.dy2 < 0)) {
    dbprintf("Overlay has strange values!\n");
   }

   if(overlay.dx1 < 0) {
     srcOffsetX = si.video_port.src_w * (-overlay.dx1) / si.video_port.drw_w;
     overlay.dx1 = 0;
   }
   if(overlay.dy1 < 0) {
     srcOffsetY = si.video_port.src_h * (-overlay.dy1) / si.video_port.drw_h;
     overlay.dy1 = 0;   
   }

   switch(si.video_port.id){
     case PIXEL_FMT_YV12:
       sx = (si.video_port.src_x + srcOffsetX) & ~7;
       sy = (si.video_port.src_y + srcOffsetY) & ~1;
       overlay.PSY = si.video_port.bufAddr + sx + sy*srcPitch;
       overlay.PSV = si.video_port.bufAddr + height*srcPitch + ((sx + sy*srcPitch/2) >> 1);
       overlay.PSU = si.video_port.bufAddr + height*srcPitch*5/4 + ((sx + sy*srcPitch/2) >> 1);

       overlay.PSY >>= si.video_port.shiftValue;
       overlay.PSV >>= si.video_port.shiftValue;
       overlay.PSU >>= si.video_port.shiftValue;
       break;
     case PIXEL_FMT_I420:
       sx = (si.video_port.src_x + srcOffsetX) & ~7;
       sy = (si.video_port.src_y + srcOffsetY) & ~1;
       overlay.PSY = si.video_port.bufAddr + sx + sy*srcPitch;
       overlay.PSV = si.video_port.bufAddr + height*srcPitch*5/4 + ((sx + sy*srcPitch/2) >> 1);
       overlay.PSU = si.video_port.bufAddr + height*srcPitch + ((sx + sy*srcPitch/2) >> 1);

       overlay.PSY >>= si.video_port.shiftValue;
       overlay.PSV >>= si.video_port.shiftValue;
       overlay.PSU >>= si.video_port.shiftValue;
       break;
     case PIXEL_FMT_YUY2:
     case PIXEL_FMT_UYVY:
     case PIXEL_FMT_RGB6:
     case PIXEL_FMT_RGB5:
     default:
       sx = (si.video_port.src_x + srcOffsetX) & ~1;
       sy = (si.video_port.src_y + srcOffsetY);
       overlay.PSY = (si.video_port.bufAddr + sx*2 + sy*srcPitch);

       overlay.PSY >>= si.video_port.shiftValue;
       break;      
   }

   /* FIXME: is it possible that srcW < 0 */
   overlay.srcW = si.video_port.src_w - (sx - si.video_port.src_x);
   overlay.srcH = si.video_port.src_h - (sy - si.video_port.src_y);

   if ( (si.video_port.oldx1 != overlay.dx1) ||
   	(si.video_port.oldx2 != overlay.dx2) ||
	(si.video_port.oldy1 != overlay.dy1) ||
	(si.video_port.oldy2 != overlay.dy2) ) {
	si.video_port.mustwait = 1;
	si.video_port.oldx1 = overlay.dx1; si.video_port.oldx2 = overlay.dx2;
	si.video_port.oldy1 = overlay.dy1; si.video_port.oldy2 = overlay.dy2;
   }

   /* TW: setup dispmode (MIRROR, SINGLEx) */
   set_dispmode();

   /* TW: set display mode SR06,32 (CRT1, CRT2 or mirror) */
   set_disptype_regs();

   /* set (not only calc) merge line buffer */
   merge_line_buf((overlay.srcW > 384));

   /* calculate (not set!) line buffer length */
   set_line_buf_size(&overlay);

   if (si.video_port.displayMode == DISPMODE_SINGLE2) {
     	  /* TW: On chips with only one overlay we
	   * use that only overlay for CRT2 */
          index = 0; iscrt2 = 1;
     overlay.VBlankActiveFunc = vblank_active_CRT2;
     /* overlay.GetScanLineFunc = get_scanline_CRT2; */
   } else {
     index = 0; iscrt2 = 0;
     overlay.VBlankActiveFunc = vblank_active_CRT1;
     /* overlay.GetScanLineFunc = get_scanline_CRT1; */
   }

   /* TW: Do the following in a loop for CRT1 and CRT2 ----------------- */

   /* calculate (not set!) scale factor */
   calc_scale_factor(&overlay, index, iscrt2);

   /* Select video1 (used for CRT1) or video2 (used for CRT2) */
   setvideoregmask(Index_VI_Control_Misc2, index, 0x01);

   /* set format */
   set_format(&overlay);

   /* set color key */
   set_colorkey(si.video_port.colorKey);

   /* set brightness, contrast, hue and saturation */
   set_brightness(si.video_port.brightness);
   set_contrast(si.video_port.contrast);
   if (si.vga_engine == SIS_315_VGA) {
   	set_hue(si.video_port.hue);
   	set_saturation(si.video_port.saturation);
   }

   /* set overlay */
   set_overlay(&overlay,index);

   /* enable overlay */
   setvideoregmask ( Index_VI_Control_Misc0, 0x02, 0x02);

   si.video_port.mustwait = 0;
}


void SISStopVideo()
{
	close_overlay();
	si.video_port.mustwait = 1;
}

area_id SISStartVideo( short src_x, short src_y,
  short drw_x, short drw_y,
  short src_w, short src_h,
  short drw_w, short drw_h,
  int id, short width, short height, uint32 color_key )
{
	int totalSize=0;
	area_id hArea;
   si.video_port.drw_x = drw_x;
   si.video_port.drw_y = drw_y;
   si.video_port.drw_w = drw_w;
   si.video_port.drw_h = drw_h;
   si.video_port.src_x = src_x;
   si.video_port.src_y = src_y;
   si.video_port.src_w = src_w;
   si.video_port.src_h = src_h;
   si.video_port.id = id;
   si.video_port.height = height;
   
   switch(id){
     case PIXEL_FMT_YV12:
     case PIXEL_FMT_I420:
       si.video_port.srcPitch = (width + 7) & ~7;
       /* Size = width * height * 3 / 2 */
       totalSize = (si.video_port.srcPitch * height * 3) >> 1; /* Verified */
       break;
     case PIXEL_FMT_YUY2:
     case PIXEL_FMT_UYVY:
     case PIXEL_FMT_RGB6:
     case PIXEL_FMT_RGB5:
     default:
       si.video_port.srcPitch = ((width << 1) + 3) & ~3;	/* Verified */
       /* Size = width * 2 * height */
       totalSize = si.video_port.srcPitch * height;
   }
   
   /* Allocate memory ( only use the lower 16mb ) */
	if( si.mem_size > 1024 * 1024 * 16 ) {
		si.video_port.bufAddr = PAGE_ALIGN((1024 * 1024 * 16 - totalSize - PAGE_SIZE));
	} else {
		si.video_port.bufAddr = PAGE_ALIGN((si.mem_size - totalSize - PAGE_SIZE));
	}
   si.video_port.colorKey    = color_key & 0x00ffffff;
   //dbprintf("Video @ %x %x\n", si.video_port.bufAddr, ( si.pci_dev.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) + si.video_port.bufAddr  );
   
   SISDisplayVideo() ;
   
  
   hArea = create_area( "sis3xx_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
   remap_area( hArea, (void*)(( si.pci_dev.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) + si.video_port.bufAddr) );
   
   return( hArea );
}





















































