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

shared_info si;
SiS_Private SiS_Pr;
HW_DEVICE_EXTENSION sishw_ext = {
	NULL, FALSE, NULL,
	0, 0, 0, 0, FALSE
};

static BOOLEAN
sis_bridgeisslave(s)
{
   unsigned char P1_00;

   if(!(si.vbflags & VB_VIDEOBRIDGE)) return FALSE;

   inSISIDXREG(SISPART1,0x00,P1_00);
   if( ((si.sisvga_engine == SIS_300_VGA) && (P1_00 & 0xa0) == 0x20) ||
       ((si.sisvga_engine == SIS_315_VGA) && (P1_00 & 0x50) == 0x10) ) {
	   return TRUE;
   } else {
           return FALSE;
   }
}

static BOOLEAN
sisallowretracecrt1(s)
{
   u8 temp;

   inSISIDXREG(SISCR,0x17,temp);
   if(!(temp & 0x80)) return FALSE;

   inSISIDXREG(SISSR,0x1f,temp);
   if(temp & 0xc0) return FALSE;

   return TRUE;
}


static void
siswaitretracecrt1()
{
   int watchdog;

   if(!sisallowretracecrt1()) return;

   watchdog = 65536;
   while((!(inSISREG(SISINPSTAT) & 0x08)) && --watchdog);
   watchdog = 65536;
   while((inSISREG(SISINPSTAT) & 0x08) && --watchdog);
}

static PCI_Info_s sis_get_northbridge(int basechipid)
{
	PCI_Info_s dev;
	dev.nDeviceID = 0xffff;
	int nbridgenum, nbridgeidx, i, j;
	const unsigned short nbridgeids[] = {
		0x0540,	/* for SiS 540 VGA */
		0x0630,	/* for SiS 630/730 VGA */
		0x0730,
		0x0550,   /* for SiS 550 VGA */
		0x0650,   /* for SiS 650/651/740 VGA */
		0x0651,
		0x0740,
		0x0661,	/* for SiS 661/741/660/760 VGA */
		0x0741,
		0x0660,
		0x0760
	};

    switch(basechipid) {
	case SIS_540:	nbridgeidx = 0; nbridgenum = 1; break;
	case SIS_630:	nbridgeidx = 1; nbridgenum = 2; break;
	case SIS_550:   nbridgeidx = 3; nbridgenum = 1; break;
	case SIS_650:	nbridgeidx = 4; nbridgenum = 3; break;
	case SIS_660:	nbridgeidx = 7; nbridgenum = 4; break;
	default:	return dev;
	}
	for( j = 0; get_pci_info( &dev, j ) == 0; j++ )
	{
		for( i = 0; i < nbridgenum; i++ )
		{
			if( dev.nVendorID == 0x1039 && dev.nDeviceID == nbridgeids[nbridgeidx+i] )
				return( dev );
		}
	}
	return( dev );
}

static int sis_get_dram_size()
{
	uint8 reg;

	si.video_size = 0;

	switch( si.chip) {
	case SIS_300:
		inSISIDXREG(SISSR, 0x14, reg);
		si.video_size = ((reg & 0x3F) + 1) << 20;
		break;
	case SIS_540:
	case SIS_630:
	case SIS_730:
		if(!si.bridge.nDeviceID == 0xffff) return -1;
		reg = pci_gfx_read_config( si.fd, si.bridge.nBus, si.bridge.nDevice, si.bridge.nFunction, 0x63, 1 );
		si.video_size = 1 << (((reg & 0x70) >> 4) + 21);
		break;

	case SIS_315H:
	case SIS_315PRO:
	case SIS_315:
		inSISIDXREG(SISSR, 0x14, reg);
		si.video_size = (1 << ((reg & 0xf0) >> 4)) << 20;
		switch((reg >> 2) & 0x03) {
		case 0x01:
		case 0x03:
		   si.video_size <<= 1;
		   break;
		case 0x02:
		   si.video_size += (si.video_size/2);
		}
		break;
	case SIS_330:
		inSISIDXREG(SISSR, 0x14, reg);
		si.video_size = (1 << ((reg & 0xf0) >> 4)) << 20;
		if(reg & 0x0c) si.video_size <<= 1;
		break;
	case SIS_550:
	case SIS_650:
	case SIS_740:
		inSISIDXREG(SISSR, 0x14, reg);
		si.video_size = (((reg & 0x3f) + 1) << 2) << 20;
		break;
	case SIS_661:
	case SIS_741:
		inSISIDXREG(SISCR, 0x79, reg);
		si.video_size = (1 << ((reg & 0xf0) >> 4)) << 20;
		break;
	case SIS_660:
	case SIS_760:
		inSISIDXREG(SISCR, 0x79, reg);
		reg = (reg & 0xf0) >> 4;
		if(reg)	si.video_size = (1 << reg) << 20;
		inSISIDXREG(SISCR, 0x78, reg);
		reg &= 0x30;
		if(reg) {
		   if(reg == 0x10) si.video_size += (32 << 20);
		   else		   si.video_size += (64 << 20);
		}
		break;
	default:
		return -1;
	}
	return 0;
}

/* -------------- video bridge device detection --------------- */

static void sis_detect_VB_connect(s)
{
	uint8 cr32, temp;

	if(si.sisvga_engine == SIS_300_VGA) {
		inSISIDXREG(SISSR, 0x17, temp);
		if((temp & 0x0F) && (si.chip != SIS_300)) {
			/* PAL/NTSC is stored on SR16 on such machines */
			if(!(si.vbflags & (TV_PAL | TV_NTSC | TV_PALM | TV_PALN))) {
				inSISIDXREG(SISSR, 0x16, temp);
				if(temp & 0x20)
					si.vbflags |= TV_PAL;
				else
					si.vbflags |= TV_NTSC;
			}
		}
	}

	inSISIDXREG(SISCR, 0x32, cr32);

	si.vbflags &= ~(CRT2_TV | CRT2_LCD | CRT2_VGA);

	if(cr32 & SIS_VB_TV)   si.vbflags |= CRT2_TV;
	if(cr32 & SIS_VB_LCD)  si.vbflags |= CRT2_LCD;
	if(cr32 & SIS_VB_CRT2) si.vbflags |= CRT2_VGA;


	/* Detect/set TV plug & type */
	{
		if(cr32 & SIS_VB_YPBPR)     	 si.vbflags |= (TV_YPBPR|TV_YPBPR525I); /* default: 480i */
		else if(cr32 & SIS_VB_HIVISION)  si.vbflags |= TV_HIVISION;
		else if(cr32 & SIS_VB_SCART)     si.vbflags |= TV_SCART;
		else {
			if(cr32 & SIS_VB_SVIDEO)    si.vbflags |= TV_SVIDEO;
			if(cr32 & SIS_VB_COMPOSITE) si.vbflags |= TV_AVIDEO;
		}
	}

	if(!(si.vbflags & (TV_YPBPR | TV_HIVISION))) {
	    if(si.vbflags & TV_SCART) {
	       si.vbflags &= ~(TV_NTSC | TV_PALM | TV_PALN | TV_NTSCJ);
	       si.vbflags |= TV_PAL;
	    }
	    if(!(si.vbflags & (TV_PAL | TV_NTSC | TV_PALM | TV_PALN | TV_NTSCJ))) {
		if(si.sisvga_engine == SIS_300_VGA) {
			inSISIDXREG(SISSR, 0x38, temp);
			if(temp & 0x01) si.vbflags |= TV_PAL;
			else		si.vbflags |= TV_NTSC;
		} else if((si.chip <= SIS_315PRO) || (si.chip >= SIS_330)) {
			inSISIDXREG(SISSR, 0x38, temp);
			if(temp & 0x01) si.vbflags |= TV_PAL;
			else		si.vbflags |= TV_NTSC;
		} else {
			inSISIDXREG(SISCR, 0x79, temp);
			if(temp & 0x20)	si.vbflags |= TV_PAL;
			else		si.vbflags |= TV_NTSC;
		}
	    }
	}
}



static BOOLEAN sis_test_DDC1()
{
    unsigned short old;
    int count = 48;

    old = SiS_ReadDDC1Bit(&SiS_Pr);
    do {
	if(old != SiS_ReadDDC1Bit(&SiS_Pr)) break;
    } while(count--);
    return (count == -1) ? FALSE : TRUE;
}

static void sis_sense_crt1()
{
    BOOLEAN mustwait = FALSE;
    uint8  sr1F, cr17;
    uint8  cr63=0;
    uint16 temp = 0xffff;
    int i;

    inSISIDXREG(SISSR,0x1F,sr1F);
    orSISIDXREG(SISSR,0x1F,0x04);
    andSISIDXREG(SISSR,0x1F,0x3F);
    if(sr1F & 0xc0) mustwait = TRUE;

    if(si.sisvga_engine == SIS_315_VGA) {
       inSISIDXREG(SISCR,SiS_Pr.SiS_MyCR63,cr63);
       cr63 &= 0x40;
       andSISIDXREG(SISCR,SiS_Pr.SiS_MyCR63,0xBF);
    }

    inSISIDXREG(SISCR,0x17,cr17);
    cr17 &= 0x80;
    if(!cr17) {
       orSISIDXREG(SISCR,0x17,0x80);
       mustwait = TRUE;
       outSISIDXREG(SISSR, 0x00, 0x01);
       outSISIDXREG(SISSR, 0x00, 0x03);
    }

    if(mustwait) {
       for(i=0; i < 10; i++) siswaitretracecrt1();
    }

    if(si.chip >= SIS_330) {
       andSISIDXREG(SISCR,0x32,~0x20);
       if(si.chip >= SIS_340) {
          outSISIDXREG(SISCR, 0x57, 0x4a);
       } else {
          outSISIDXREG(SISCR, 0x57, 0x5f);
       }
       orSISIDXREG(SISCR, 0x53, 0x02);
       while((inSISREG(SISINPSTAT)) & 0x01)    break;
       while(!((inSISREG(SISINPSTAT)) & 0x01)) break;
       if((inSISREG(SISMISCW)) & 0x10) temp = 1;
       andSISIDXREG(SISCR, 0x53, 0xfd);
       andSISIDXREG(SISCR, 0x57, 0x00);
    }

    if(temp == 0xffff) {
       i = 3;
       do {
          temp = SiS_HandleDDC(&SiS_Pr, si.vbflags, si.sisvga_engine, 0, 0, NULL);
       } while(((temp == 0) || (temp == 0xffff)) && i--);

       if((temp == 0) || (temp == 0xffff)) {
          if(sis_test_DDC1()) temp = 1;
       }
    }

    if((temp) && (temp != 0xffff)) {
       orSISIDXREG(SISCR,0x32,0x20);
    }

    if(si.sisvga_engine == SIS_315_VGA) {
       setSISIDXREG(SISCR,SiS_Pr.SiS_MyCR63,0xBF,cr63);
    }

    setSISIDXREG(SISCR,0x17,0x7F,cr17);

    outSISIDXREG(SISSR,0x1F,sr1F);
}

/* Determine and detect attached devices on SiS30x */
static int SISDoSense(uint16 type, uint16 test)
{
    int temp, mytest, result, i, j;

    for(j = 0; j < 10; j++) {
       result = 0;
       for(i = 0; i < 3; i++) {
          mytest = test;
          outSISIDXREG(SISPART4,0x11,(type & 0x00ff));
          temp = (type >> 8) | (mytest & 0x00ff);
          setSISIDXREG(SISPART4,0x10,0xe0,temp);
          SiS_DDC2Delay(&SiS_Pr, 0x1500);
          mytest >>= 8;
          mytest &= 0x7f;
          inSISIDXREG(SISPART4,0x03,temp);
          temp ^= 0x0e;
          temp &= mytest;
          if(temp == mytest) result++;

	  outSISIDXREG(SISPART4,0x11,0x00);
	  andSISIDXREG(SISPART4,0x10,0xe0);
	  SiS_DDC2Delay(&SiS_Pr, 0x1000);

       }
       if((result == 0) || (result >= 2)) break;
    }
    return(result);
}

static void SiS_Sense30x()
{
    u8  backupP4_0d,backupP2_00,backupP2_4d,backupSR_1e,biosflag=0;
    u16 svhs=0, svhs_c=0;
    u16 cvbs=0, cvbs_c=0;
    u16 vga2=0, vga2_c=0;
    int myflag, result;
    char stdstr[] = "Detected";
    char tvstr[]  = "TV connected to";

    if(si.vbflags & VB_301) {
       svhs = 0x00b9; cvbs = 0x00b3; vga2 = 0x00d1;
       inSISIDXREG(SISPART4,0x01,myflag);
       if(myflag & 0x04) {
	  svhs = 0x00dd; cvbs = 0x00ee; vga2 = 0x00fd;
       }
    } else if(si.vbflags & (VB_301B | VB_302B)) {
       svhs = 0x016b; cvbs = 0x0174; vga2 = 0x0190;
    } else if(si.vbflags & (VB_301LV | VB_302LV)) {
       svhs = 0x0200; cvbs = 0x0100;
    } else if(si.vbflags & (VB_301C | VB_302ELV)) {
       svhs = 0x016b; cvbs = 0x0110; vga2 = 0x0190;
    } else return;

    vga2_c = 0x0e08; svhs_c = 0x0404; cvbs_c = 0x0804;
    if(si.vbflags & (VB_301LV|VB_302LV|VB_302ELV)) {
       svhs_c = 0x0408; cvbs_c = 0x0808;
    }
    biosflag = 2;

    if(si.chip == SIS_300) {
       inSISIDXREG(SISSR,0x3b,myflag);
       if(!(myflag & 0x01)) vga2 = vga2_c = 0;
    }

    inSISIDXREG(SISSR,0x1e,backupSR_1e);
    orSISIDXREG(SISSR,0x1e,0x20);

    inSISIDXREG(SISPART4,0x0d,backupP4_0d);
    if(si.vbflags & VB_301C) {
       setSISIDXREG(SISPART4,0x0d,~0x07,0x01);
    } else {
       orSISIDXREG(SISPART4,0x0d,0x04);
    }
    SiS_DDC2Delay(&SiS_Pr, 0x2000);

    inSISIDXREG(SISPART2,0x00,backupP2_00);
    outSISIDXREG(SISPART2,0x00,((backupP2_00 | 0x1c) & 0xfc));

    inSISIDXREG(SISPART2,0x4d,backupP2_4d);
    if(si.vbflags & (VB_301C|VB_301LV|VB_302LV|VB_302ELV)) {
       outSISIDXREG(SISPART2,0x4d,(backupP2_4d & ~0x10));
    }

    if(!(si.vbflags & VB_301C)) {
       SISDoSense(0, 0);
    }

    andSISIDXREG(SISCR, 0x32, ~0x14);

    if(vga2_c || vga2) {
       if(SISDoSense(vga2, vga2_c)) {
          if(biosflag & 0x01) {
	     dbprintf( "%s %s SCART output\n", stdstr, tvstr);
	     orSISIDXREG(SISCR, 0x32, 0x04);
	  } else {
	     dbprintf( "%s secondary VGA connection\n", stdstr);
	     orSISIDXREG(SISCR, 0x32, 0x10);
	  }
       }
    }

    andSISIDXREG(SISCR, 0x32, 0x3f);

    if(si.vbflags & VB_301C) {
       orSISIDXREG(SISPART4,0x0d,0x04);
    }

    if((si.sisvga_engine == SIS_315_VGA) &&
       (si.vbflags & (VB_301C|VB_301LV|VB_302LV|VB_302ELV))) {
       outSISIDXREG(SISPART2,0x4d,(backupP2_4d | 0x10));
       SiS_DDC2Delay(&SiS_Pr, 0x2000);
       if((result = SISDoSense(svhs, 0x0604))) {
          if((result = SISDoSense(cvbs, 0x0804))) {
	     dbprintf( "%s %s YPbPr component output\n", stdstr, tvstr);
	     orSISIDXREG(SISCR,0x32,0x80);
	  }
       }
       outSISIDXREG(SISPART2,0x4d,backupP2_4d);
    }

    andSISIDXREG(SISCR, 0x32, ~0x03);

    if(!(si.vbflags & TV_YPBPR)) {
       if((result = SISDoSense(svhs, svhs_c))) {
          dbprintf( "%s %s SVIDEO output\n", stdstr, tvstr);
          orSISIDXREG(SISCR, 0x32, 0x02);
       }
       if((biosflag & 0x02) || (!result)) {
          if(SISDoSense(cvbs, cvbs_c)) {
	     dbprintf( "%s %s COMPOSITE output\n", stdstr, tvstr);
	     orSISIDXREG(SISCR, 0x32, 0x01);
          }
       }
    }

    SISDoSense( 0, 0);

    outSISIDXREG(SISPART2,0x00,backupP2_00);
    outSISIDXREG(SISPART4,0x0d,backupP4_0d);
    outSISIDXREG(SISSR,0x1e,backupSR_1e);

    if(si.vbflags & VB_301C) {
       inSISIDXREG(SISPART2,0x00,biosflag);
       if(biosflag & 0x20) {
          for(myflag = 2; myflag > 0; myflag--) {
	     biosflag ^= 0x20;
	     outSISIDXREG(SISPART2,0x00,biosflag);
	  }
       }
    }

    outSISIDXREG(SISPART2,0x00,backupP2_00);
}

/* Determine and detect attached TV's on Chrontel */
static void SiS_SenseCh()
{
    uint8 temp1, temp2;
    char stdstr[] = "Chrontel: Detected TV connected to";
    unsigned char test[3];
    int i;

    if(si.chip < SIS_315H) {

       SiS_Pr.SiS_IF_DEF_CH70xx = 1;		/* Chrontel 700x */
       SiS_SetChrontelGPIO(&SiS_Pr, 0x9c);	/* Set general purpose IO for Chrontel communication */
       SiS_DDC2Delay(&SiS_Pr, 1000);
       temp1 = SiS_GetCH700x(&SiS_Pr, 0x25);
       /* See Chrontel TB31 for explanation */
       temp2 = SiS_GetCH700x(&SiS_Pr, 0x0e);
       if(((temp2 & 0x07) == 0x01) || (temp2 & 0x04)) {
	  SiS_SetCH700x(&SiS_Pr, 0x0b0e);
	  SiS_DDC2Delay(&SiS_Pr, 300);
       }
       temp2 = SiS_GetCH700x(&SiS_Pr, 0x25);
       if(temp2 != temp1) temp1 = temp2;

       if((temp1 >= 0x22) && (temp1 <= 0x50)) {
	   /* Read power status */
	   temp1 = SiS_GetCH700x(&SiS_Pr, 0x0e);
	   if((temp1 & 0x03) != 0x03) {
     	        /* Power all outputs */
		SiS_SetCH700x(&SiS_Pr, 0x0B0E);
		SiS_DDC2Delay(&SiS_Pr, 300);
	   }
	   /* Sense connected TV devices */
	   for(i = 0; i < 3; i++) {
	       SiS_SetCH700x(&SiS_Pr, 0x0110);
	       SiS_DDC2Delay(&SiS_Pr, 0x96);
	       SiS_SetCH700x(&SiS_Pr, 0x0010);
	       SiS_DDC2Delay(&SiS_Pr, 0x96);
	       temp1 = SiS_GetCH700x(&SiS_Pr, 0x10);
	       if(!(temp1 & 0x08))       test[i] = 0x02;
	       else if(!(temp1 & 0x02))  test[i] = 0x01;
	       else                      test[i] = 0;
	       SiS_DDC2Delay(&SiS_Pr, 0x96);
	   }

	   if(test[0] == test[1])      temp1 = test[0];
	   else if(test[0] == test[2]) temp1 = test[0];
	   else if(test[1] == test[2]) temp1 = test[1];
	   else {
	   	dbprintf("TV detection unreliable - test results varied\n\n");
		temp1 = test[2];
	   }
	   if(temp1 == 0x02) {
		dbprintf( "%s SVIDEO output\n", stdstr);
		si.vbflags |= TV_SVIDEO;
		orSISIDXREG(SISCR, 0x32, 0x02);
		andSISIDXREG(SISCR, 0x32, ~0x05);
	   } else if (temp1 == 0x01) {
		dbprintf( "%s CVBS output\n", stdstr);
		si.vbflags |= TV_AVIDEO;
		orSISIDXREG(SISCR, 0x32, 0x01);
		andSISIDXREG(SISCR, 0x32, ~0x06);
	   } else {
 		SiS_SetCH70xxANDOR(&SiS_Pr, 0x010E,0xF8);
		andSISIDXREG(SISCR, 0x32, ~0x07);
	   }
       } else if(temp1 == 0) {
	  SiS_SetCH70xxANDOR(&SiS_Pr, 0x010E,0xF8);
	  andSISIDXREG(SISCR, 0x32, ~0x07);
       }
       /* Set general purpose IO for Chrontel communication */
       SiS_SetChrontelGPIO(&SiS_Pr, 0x00);

    } else {

	SiS_Pr.SiS_IF_DEF_CH70xx = 2;		/* Chrontel 7019 */
        temp1 = SiS_GetCH701x(&SiS_Pr, 0x49);
	SiS_SetCH701x(&SiS_Pr, 0x2049);
	SiS_DDC2Delay(&SiS_Pr, 0x96);
	temp2 = SiS_GetCH701x(&SiS_Pr, 0x20);
	temp2 |= 0x01;
	SiS_SetCH701x(&SiS_Pr, (temp2 << 8) | 0x20);
	SiS_DDC2Delay(&SiS_Pr, 0x96);
	temp2 ^= 0x01;
	SiS_SetCH701x(&SiS_Pr, (temp2 << 8) | 0x20);
	SiS_DDC2Delay(&SiS_Pr, 0x96);
	temp2 = SiS_GetCH701x(&SiS_Pr, 0x20);
	SiS_SetCH701x(&SiS_Pr, (temp1 << 8) | 0x49);
        temp1 = 0;
	if(temp2 & 0x02) temp1 |= 0x01;
	if(temp2 & 0x10) temp1 |= 0x01;
	if(temp2 & 0x04) temp1 |= 0x02;
	if( (temp1 & 0x01) && (temp1 & 0x02) ) temp1 = 0x04;
	switch(temp1) {
	case 0x01:
	     dbprintf( "%s CVBS output\n", stdstr);
	     si.vbflags |= TV_AVIDEO;
	     orSISIDXREG(SISCR, 0x32, 0x01);
	     andSISIDXREG(SISCR, 0x32, ~0x06);
             break;
	case 0x02:
	     dbprintf( "%s SVIDEO output\n", stdstr);
	     si.vbflags |= TV_SVIDEO;
	     orSISIDXREG(SISCR, 0x32, 0x02);
	     andSISIDXREG(SISCR, 0x32, ~0x05);
             break;
	case 0x04:
	     dbprintf( "%s SCART output\n", stdstr);
	     orSISIDXREG(SISCR, 0x32, 0x04);
	     andSISIDXREG(SISCR, 0x32, ~0x03);
             break;
	default:
	     andSISIDXREG(SISCR, 0x32, ~0x07);
	}
    }
}

static void sis_get_VB_type()
{
	char stdstr[]    = "Detected";
	char bridgestr[] = "video bridge";
	uint8 vb_chipid;
	uint8 reg;

	inSISIDXREG(SISPART4, 0x00, vb_chipid);
	switch(vb_chipid) {
	case 0x01:
		inSISIDXREG(SISPART4, 0x01, reg);
		if(reg < 0xb0) {
			si.vbflags |= VB_301;
			dbprintf( "%s SiS301 %s\n", stdstr, bridgestr);
		} else if(reg < 0xc0) {
			si.vbflags |= VB_301B;
			inSISIDXREG(SISPART4,0x23,reg);
			if(!(reg & 0x02)) {
			   si.vbflags |= VB_30xBDH;
			  dbprintf( "%s SiS301B-DH %s\n", stdstr, bridgestr);
			} else {
			  dbprintf( "%s SiS301B %s\n", stdstr, bridgestr);
			}
		} else if(reg < 0xd0) {
		 	si.vbflags |= VB_301C;
			dbprintf( "%s SiS301C %s\n", stdstr, bridgestr);
		} else if(reg < 0xe0) {
			si.vbflags |= VB_301LV;
			dbprintf( "%s SiS301LV %s\n", stdstr, bridgestr);
		} else if(reg <= 0xe1) {
			inSISIDXREG(SISPART4,0x39,reg);
			if(reg == 0xff) {
			   si.vbflags |= VB_302LV;
			   dbprintf( "%s SiS302LV %s\n", stdstr, bridgestr);
			} else {
			   si.vbflags |= VB_301C;
			   dbprintf( "%s SiS301C(P4) %s\n", stdstr, bridgestr);
#if 0
			   si.vbflags |= VB_302ELV;
			   dbprintf( "%s SiS302ELV %s\n", stdstr, bridgestr);
#endif
			}
		}
		break;
	case 0x02:
		si.vbflags |= VB_302B;
		dbprintf( "%s SiS302B %s\n", stdstr, bridgestr);
		break;
	}

	if((!(si.vbflags & VB_VIDEOBRIDGE)) && (si.chip != SIS_300)) {
		inSISIDXREG(SISCR, 0x37, reg);
		reg &= SIS_EXTERNAL_CHIP_MASK;
		reg >>= 1;
		if(si.sisvga_engine == SIS_300_VGA) {

			switch(reg) {
			   case SIS_EXTERNAL_CHIP_LVDS:
				si.vbflags |= VB_LVDS;
				break;
			   case SIS_EXTERNAL_CHIP_TRUMPION:
				si.vbflags |= VB_TRUMPION;
				break;
			   case SIS_EXTERNAL_CHIP_CHRONTEL:
				si.vbflags |= VB_CHRONTEL;
				break;
			   case SIS_EXTERNAL_CHIP_LVDS_CHRONTEL:
				si.vbflags |= (VB_LVDS | VB_CHRONTEL);
				break;
			}
			if(si.vbflags & VB_CHRONTEL) si.chronteltype = 1;

		} else if(si.chip < SIS_661) {
			switch (reg) {
			   case SIS310_EXTERNAL_CHIP_LVDS:
				si.vbflags |= VB_LVDS;
				break;
			   case SIS310_EXTERNAL_CHIP_LVDS_CHRONTEL:
				si.vbflags |= (VB_LVDS | VB_CHRONTEL);
				break;
			}
			if(si.vbflags & VB_CHRONTEL) si.chronteltype = 2;
		} else if(si.chip >= SIS_661) {
			inSISIDXREG(SISCR, 0x38, reg);
			reg >>= 5;
			switch(reg) {
			   case 0x02:
				si.vbflags |= VB_LVDS;
				break;
			   case 0x03:
				si.vbflags |= (VB_LVDS | VB_CHRONTEL);
				break;
			   case 0x04:
				si.vbflags |= (VB_LVDS | VB_CONEXANT);
				break;
			}
			if(si.vbflags & VB_CHRONTEL) si.chronteltype = 2;
		}
		if(si.vbflags & VB_LVDS) {
		   dbprintf( "%s LVDS transmitter\n", stdstr);
		}
		if(si.vbflags & VB_TRUMPION) {
		   dbprintf( "%s Trumpion Zurac LCD scaler\n", stdstr);
		}
		if(si.vbflags & VB_CHRONTEL) {
		  dbprintf( "%s Chrontel TV encoder\n", stdstr);
		}
		if(si.vbflags & VB_CONEXANT) {
		   dbprintf( "%s Conexant external device\n", stdstr);
		}
	}

	if(si.vbflags & VB_SISBRIDGE) {
		SiS_Sense30x();
	} else if(si.vbflags & VB_CHRONTEL) {
		SiS_SenseCh();
	}
}


void sis_enable_queue_300()
{
	unsigned int tqueue_pos;
	uint8 tq_state;
	
	tqueue_pos = ( si.video_size -
		       TURBO_QUEUE_AREA_SIZE ) / ( 64 * 1024 );
	inSISIDXREG( SISSR, IND_SIS_TURBOQUEUE_SET, tq_state );
	tq_state |= 0xf0;
	tq_state &= 0xfc;
	tq_state |= (uint8)( tqueue_pos >> 8 );
	outSISIDXREG( SISSR, IND_SIS_TURBOQUEUE_SET, tq_state );
	outSISIDXREG( SISSR, IND_SIS_TURBOQUEUE_ADR, (uint8)( tqueue_pos & 0xff ) );
	si.video_size -= TURBO_QUEUE_AREA_SIZE;
}


void sis_enable_queue_315()
{
	unsigned long *cmdq_baseport = 0;
	unsigned long *read_port = 0;
	unsigned long *write_port = 0;
	
	cmdq_baseport = (unsigned long *)(si.regs + MMIO_QUEUE_PHYBASE);
	write_port    = (unsigned long *)(si.regs + MMIO_QUEUE_WRITEPORT);
	read_port     = (unsigned long *)(si.regs + MMIO_QUEUE_READPORT);
	
	outSISIDXREG( SISSR, IND_SIS_CMDQUEUE_THRESHOLD, COMMAND_QUEUE_THRESHOLD );
	outSISIDXREG( SISSR, IND_SIS_CMDQUEUE_SET, SIS_CMD_QUEUE_RESET );
	
	*write_port = *read_port;
	
	/* TW: Set Auto_Correction bit */
	outSISIDXREG(SISSR, IND_SIS_CMDQUEUE_SET, SIS_CMD_QUEUE_SIZE_512k | SIS_MMIO_CMD_ENABLE | SIS_CMD_AUTO_CORR );
	*cmdq_baseport = si.video_size - COMMAND_QUEUE_AREA_SIZE;
	
	si.video_size -= COMMAND_QUEUE_AREA_SIZE;
}

void sis_pre_setmode( uint8 rate_idx )
{
	uint8 cr30 = 0, cr31 = 0, cr33 = 0, cr35 = 0, cr38 = 0;
	int tvregnum = 0;

	si.currentvbflags &= (VB_VIDEOBRIDGE | VB_DISPTYPE_DISP2);

	inSISIDXREG(SISCR, 0x31, cr31);
	cr31 &= ~0x60;
	cr31 |= 0x04;

	cr33 = si.rate_idx & 0x0F;

	if(si.sisvga_engine == SIS_315_VGA) {
	   if(si.chip >= SIS_661) {
	      inSISIDXREG(SISCR, 0x38, cr38);
	      cr38 &= ~0x07;  /* Clear LCDA/DualEdge and YPbPr bits */
	   } else {
	      tvregnum = 0x38;
	      inSISIDXREG(SISCR, tvregnum, cr38);
	      cr38 &= ~0x3b;  /* Clear LCDA/DualEdge and YPbPr bits */
	   }
	}
	if(si.sisvga_engine == SIS_300_VGA) {
	   tvregnum = 0x35;
	   inSISIDXREG(SISCR, tvregnum, cr38);
	}

	SiS_SetEnableDstn(&SiS_Pr, FALSE);
	SiS_SetEnableFstn(&SiS_Pr, FALSE);

	switch(si.currentvbflags & VB_DISPTYPE_DISP2) {

	   case CRT2_TV:
	      cr38 &= ~0xc0;   /* Clear PAL-M / PAL-N bits */
	      if((si.vbflags & TV_YPBPR) && (si.vbflags & (VB_301C|VB_301LV|VB_302LV))) {
	         if(si.chip >= SIS_661) {
		    cr38 |= 0x04;
		    if(si.vbflags & TV_YPBPR525P)       cr35 |= 0x20;
		    else if(si.vbflags & TV_YPBPR750P)  cr35 |= 0x40;
		    else if(si.vbflags & TV_YPBPR1080I) cr35 |= 0x60;
		    cr30 |= SIS_SIMULTANEOUS_VIEW_ENABLE;
		    cr35 &= ~0x01;
		    si.currentvbflags |= (TV_YPBPR | (si.vbflags & TV_YPBPRALL));
		 } else if(si.sisvga_engine == SIS_315_VGA) {
		    cr30 |= (0x80 | SIS_SIMULTANEOUS_VIEW_ENABLE);
		    cr38 |= 0x08;
		    if(si.vbflags & TV_YPBPR525P)       cr38 |= 0x10;
		    else if(si.vbflags & TV_YPBPR750P)  cr38 |= 0x20;
		    else if(si.vbflags & TV_YPBPR1080I) cr38 |= 0x30;
		    cr31 &= ~0x01;
		    si.currentvbflags |= (TV_YPBPR | (si.vbflags & TV_YPBPRALL));
	         }
	      } else if((si.vbflags & TV_HIVISION) && (si.vbflags & (VB_301|VB_301B|VB_302B))) {
	         if(si.chip >= SIS_661) {
	            cr38 |= 0x04;
	            cr35 |= 0x60;
	         } else {
	            cr30 |= 0x80;
	         }
		 cr30 |= SIS_SIMULTANEOUS_VIEW_ENABLE;
	         cr31 |= 0x01;
	         cr35 |= 0x01;
		 si.currentvbflags |= TV_HIVISION;
	      } else if(si.vbflags & TV_SCART) {
		 cr30 = (SIS_VB_OUTPUT_SCART | SIS_SIMULTANEOUS_VIEW_ENABLE);
		 cr31 |= 0x01;
		 cr35 |= 0x01;
		 si.currentvbflags |= TV_SCART;
	      } else {
		 if(si.vbflags & TV_SVIDEO) {
		    cr30 = (SIS_VB_OUTPUT_SVIDEO | SIS_SIMULTANEOUS_VIEW_ENABLE);
		    si.currentvbflags |= TV_SVIDEO;
		 }
		 if(si.vbflags & TV_AVIDEO) {
		    cr30 = (SIS_VB_OUTPUT_COMPOSITE | SIS_SIMULTANEOUS_VIEW_ENABLE);
		    si.currentvbflags |= TV_AVIDEO;
		 }
	      }
	      cr31 |= SIS_DRIVER_MODE;

	      if(si.vbflags & (TV_AVIDEO|TV_SVIDEO)) {
		 if(si.vbflags & TV_PAL) {
		    cr31 |= 0x01; cr35 |= 0x01;
		    si.currentvbflags |= TV_PAL;
		    if(si.vbflags & TV_PALM) {
		       cr38 |= 0x40; cr35 |= 0x04;
		       si.currentvbflags |= TV_PALM;
		    } else if(si.vbflags & TV_PALN) {
		       cr38 |= 0x80; cr35 |= 0x08;
		       si.currentvbflags |= TV_PALN;
		    }
		 } else {
		    cr31 &= ~0x01; cr35 &= ~0x01;
		    si.currentvbflags |= TV_NTSC;
		    if(si.vbflags & TV_NTSCJ) {
		       cr38 |= 0x40; cr35 |= 0x02;
		       si.currentvbflags |= TV_NTSCJ;
		    }
		 }
	      }
	      break;

	   case CRT2_LCD:
	      cr30  = (SIS_VB_OUTPUT_LCD | SIS_SIMULTANEOUS_VIEW_ENABLE);
	      cr31 |= SIS_DRIVER_MODE;
	      SiS_SetEnableDstn(&SiS_Pr, 0);
	      SiS_SetEnableFstn(&SiS_Pr, 0);
	      break;

	   case CRT2_VGA:
	      cr30 = (SIS_VB_OUTPUT_CRT2 | SIS_SIMULTANEOUS_VIEW_ENABLE);
	      cr31 |= SIS_DRIVER_MODE;
	     
		 cr33 |= ((si.rate_idx & 0x0F) << 4);
	      break;

	   default:	/* disable CRT2 */
	      cr30 = 0x00;
	      cr31 |= (SIS_DRIVER_MODE | SIS_VB_OUTPUT_DISABLE);
	}

	outSISIDXREG(SISCR, 0x30, cr30);
	outSISIDXREG(SISCR, 0x33, cr33);

	if(si.chip >= SIS_661) {
	   cr31 &= ~0x01;                          /* Clear PAL flag (now in CR35) */
	   setSISIDXREG(SISCR, 0x35, ~0x10, cr35); /* Leave overscan bit alone */
	   cr38 &= 0x07;                           /* Use only LCDA and HiVision/YPbPr bits */
	   setSISIDXREG(SISCR, 0x38, 0xf8, cr38);
	} else if(si.chip != SIS_300) {
	   outSISIDXREG(SISCR, tvregnum, cr38);
	}
	outSISIDXREG(SISCR, 0x31, cr31);

	if( si.sisvga_engine == SIS_315_VGA )
		SiS310Idle
	else
		SiS300Idle
	
	SiS_Pr.SiS_UseOEM = -1;
}

static void
sis_fixup_SR11()
{
    u8  tmpreg;

    if(si.chip >= SIS_661) {
       inSISIDXREG(SISSR,0x11,tmpreg);
       if(tmpreg & 0x20) {
	  inSISIDXREG(SISSR,0x3e,tmpreg);
	  tmpreg = (tmpreg + 1) & 0xff;
	  outSISIDXREG(SISSR,0x3e,tmpreg);
	  inSISIDXREG(SISSR,0x11,tmpreg);
       }
       if(tmpreg & 0xf0) {
          andSISIDXREG(SISSR,0x11,0x0f);
       }
    }
}

void sis_post_setmode()
{
	BOOLEAN crt1isoff = FALSE;
	BOOLEAN doit = TRUE;
	u8 reg;
	u8 reg1;

	outSISIDXREG(SISSR,0x05,0x86);

	sis_fixup_SR11();

	/* We can't switch off CRT1 if bridge is in slave mode */
	if(si.vbflags & VB_VIDEOBRIDGE) {
		if(sis_bridgeisslave()) doit = FALSE;
	} else si.crt1off = 0;

	if(si.sisvga_engine == SIS_300_VGA) {
	   if((si.crt1off) && (doit)) {
	        crt1isoff = TRUE;
		reg = 0x00;
	   } else {
	        crt1isoff = FALSE;
		reg = 0x80;
	   }
	   setSISIDXREG(SISCR, 0x17, 0x7f, reg);
	}
	if(si.sisvga_engine == SIS_315_VGA) {
	   if((si.crt1off) && (doit)) {
	        crt1isoff = TRUE;
		reg  = 0x40;
		reg1 = 0xc0;
	   } else {
	        crt1isoff = FALSE;
		reg  = 0x00;
		reg1 = 0x00;

	   }
	   setSISIDXREG(SISCR, SiS_Pr.SiS_MyCR63, ~0x40, reg);
	   setSISIDXREG(SISSR, 0x1f, ~0xc0, reg1);
	}

	if(crt1isoff) {
	   si.currentvbflags &= ~VB_DISPTYPE_CRT1;
	   si.currentvbflags |= VB_SINGLE_MODE;
	} else {
	   si.currentvbflags |= VB_DISPTYPE_CRT1;
	   if(si.currentvbflags & VB_DISPTYPE_CRT2) {
	  	si.currentvbflags |= VB_MIRROR_MODE;
	   } else {
	 	si.currentvbflags |= VB_SINGLE_MODE;
	   }
	}

        andSISIDXREG(SISSR, IND_SIS_RAMDAC_CONTROL, ~0x04);

	if(si.currentvbflags & CRT2_TV) {
	   if(si.vbflags & VB_SISBRIDGE) {
	      inSISIDXREG(SISPART2,0x1f,si.p2_1f);
	      inSISIDXREG(SISPART2,0x20,si.p2_20);
	      inSISIDXREG(SISPART2,0x2b,si.p2_2b);
	      inSISIDXREG(SISPART2,0x42,si.p2_42);
	      inSISIDXREG(SISPART2,0x43,si.p2_43);
	      inSISIDXREG(SISPART2,0x01,si.p2_01);
	      inSISIDXREG(SISPART2,0x02,si.p2_02);
	   } else if(si.vbflags & VB_CHRONTEL) {
	      if(si.chronteltype == 1) {
	         si.tvx = SiS_GetCH700x(&SiS_Pr, 0x0a);
	         si.tvx |= (((SiS_GetCH700x(&SiS_Pr, 0x08) & 0x02) >> 1) << 8);
	         si.tvy = SiS_GetCH700x(&SiS_Pr, 0x0b);
	         si.tvy |= ((SiS_GetCH700x(&SiS_Pr, 0x08) & 0x01) << 8);
 	      }
	   }
	}

	if((si.currentvbflags & CRT2_TV) && (si.vbflags & VB_301)) {  /* Set filter for SiS301 */

		unsigned char filter_tb = 0;

		switch (si.video_width) {
		   case 320:
			filter_tb = (si.vbflags & TV_NTSC) ? 4 : 12;
			break;
		   case 640:
			filter_tb = (si.vbflags & TV_NTSC) ? 5 : 13;
			break;
		   case 720:
			filter_tb = (si.vbflags & TV_NTSC) ? 6 : 14;
			break;
		   case 400:
		   case 800:
			filter_tb = (si.vbflags & TV_NTSC) ? 7 : 15;
			break;
		   default:
			break;
		}

		orSISIDXREG(SISPART1, si.CRT2_write_enable, 0x01);

		if(si.vbflags & TV_NTSC) {

		        andSISIDXREG(SISPART2, 0x3a, 0x1f);

			if (si.vbflags & TV_SVIDEO) {

			        andSISIDXREG(SISPART2, 0x30, 0xdf);

			} else if (si.vbflags & TV_AVIDEO) {

			        orSISIDXREG(SISPART2, 0x30, 0x20);

				switch (si.video_width) {
				case 640:
				        outSISIDXREG(SISPART2, 0x35, 0xEB);
					outSISIDXREG(SISPART2, 0x36, 0x04);
					outSISIDXREG(SISPART2, 0x37, 0x25);
					outSISIDXREG(SISPART2, 0x38, 0x18);
					break;
				case 720:
					outSISIDXREG(SISPART2, 0x35, 0xEE);
					outSISIDXREG(SISPART2, 0x36, 0x0C);
					outSISIDXREG(SISPART2, 0x37, 0x22);
					outSISIDXREG(SISPART2, 0x38, 0x08);
					break;
				case 400:
				case 800:
					outSISIDXREG(SISPART2, 0x35, 0xEB);
					outSISIDXREG(SISPART2, 0x36, 0x15);
					outSISIDXREG(SISPART2, 0x37, 0x25);
					outSISIDXREG(SISPART2, 0x38, 0xF6);
					break;
				}
			}

		} else if(si.vbflags & TV_PAL) {

			andSISIDXREG(SISPART2, 0x3A, 0x1F);

			if (si.vbflags & TV_SVIDEO) {

				andSISIDXREG(SISPART2, 0x30, 0xDF);

			} else if (si.vbflags & TV_AVIDEO) {

				orSISIDXREG(SISPART2, 0x30, 0x20);

				switch (si.video_width) {
				case 640:
					outSISIDXREG(SISPART2, 0x35, 0xF1);
					outSISIDXREG(SISPART2, 0x36, 0xF7);
					outSISIDXREG(SISPART2, 0x37, 0x1F);
					outSISIDXREG(SISPART2, 0x38, 0x32);
					break;
				case 720:
					outSISIDXREG(SISPART2, 0x35, 0xF3);
					outSISIDXREG(SISPART2, 0x36, 0x00);
					outSISIDXREG(SISPART2, 0x37, 0x1D);
					outSISIDXREG(SISPART2, 0x38, 0x20);
					break;
				case 400:
				case 800:
					outSISIDXREG(SISPART2, 0x35, 0xFC);
					outSISIDXREG(SISPART2, 0x36, 0xFB);
					outSISIDXREG(SISPART2, 0x37, 0x14);
					outSISIDXREG(SISPART2, 0x38, 0x2A);
					break;
				}
			}
		}
	}

}

#define readb(addr) (*(volatile unsigned char *) (addr))

char *sis_find_rom()
{
	USHORT pciid;
	u32    temp;
	SIS_IOTYPE1 *rom_base, *rom;
	int    romptr;
	UCHAR  *myrombase;
	
	if(!(myrombase = malloc(65536)))
		return NULL;
	
	for(temp = 0x000c0000; temp < 0x000f0000; temp += 0x00001000) {

		rom_base = (SIS_IOTYPE1 *)si.rom + temp - 0x000c0000;
		
		if((readb(rom_base) != 0x55) || (readb(rom_base + 1) != 0xaa)) {
			continue;
		}

		romptr = (readb(rom_base + 0x18) | (readb(rom_base + 0x19) << 8));
		if(romptr > (0x10000 - 8)) {
			continue;
		}

		rom = rom_base + romptr;

		if((readb(rom)     != 'P') || (readb(rom + 1) != 'C') ||
		   (readb(rom + 2) != 'I') || (readb(rom + 3) != 'R')) {
			continue;
		}

		pciid = readb(rom + 4) | (readb(rom + 5) << 8);
		if(pciid != 0x1039) {
			continue;
		}

		pciid = readb(rom + 6) | (readb(rom + 7) << 8);
		if(pciid == si.device_id) {
			memcpy(myrombase, rom_base, 65536);
			return myrombase;
		}

        }
	
		free(myrombase);
		return NULL;
}


int sis_init()
{
	uint8 reg;
	
	outb( 0x77, 0x80 );
	
	/* Check what chip we have ...*/
	switch( si.device_id ) {
		case 0x300: /* SIS 300 */
			si.chip = SIS_300;
			si.sisvga_engine = SIS_300_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_300;
			strcpy( si.name, "330" );
		break; 
		case 0x6300: /* SIS 630/730 */
			si.chip = SIS_630;
			strcpy( si.name, "630" );
			si.sisvga_engine = SIS_300_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_300;
		break;
		case 0x5300: /* SIS 540 VGA */
			si.chip = SIS_540;
			si.sisvga_engine = SIS_300_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_300;
			strcpy( si.name, "540" );
		break;
		case 0x310: /* SIS 315H */
			si.chip = SIS_315H;
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "315H" );
		break; 
		case 0x315: /* SIS 315 */
			si.chip = SIS_315H;
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "315" );
		break; 
		case 0x325: /* SIS 315 PRO */
			si.chip = SIS_315H;
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "315 PRO" );
		break;
		case 0x5315: /* SIS 550 VGA */
			si.chip = SIS_550;
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "550" );
		break;
		case 0x6325: /* SIS 650/740 */
			si.chip = SIS_650;
			strcpy( si.name, "650" );
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
		break;
		case 0x330: /* SIS 330 ( Xabre ) */
			si.chip = SIS_330;
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "Xabre" );
		break;
		case 0x6330: /* SIS 660/760 */	
			si.chip = SIS_660;		
			strcpy( si.name, "660" );
			si.sisvga_engine = SIS_315_VGA;
			si.CRT2_write_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
		break;
		default:
			return -1;
	}
	
	
	/* Set default values */
	si.crt2type = -1;
	si.detectedpdc  = 0xff;
	si.detectedpdca = 0xff;
	si.detectedlcda = 0xff;
	si.vbflags = 0;
	si.crt1off = 0;
	
	SiS_Pr.UsePanelScaler = -1;
	SiS_Pr.CenterScreen = -1;
	SiS_Pr.SiS_CustomT = CUT_NONE;
	SiS_Pr.LVDSHL = -1;

	SiS_Pr.SiS_Backup70xx = 0xff;
	SiS_Pr.SiS_CHOverScan = -1;
	SiS_Pr.SiS_ChSW = FALSE;
	SiS_Pr.SiS_UseLCDA = FALSE;
	SiS_Pr.HaveEMI = FALSE;
	SiS_Pr.HaveEMILCD = FALSE;
	SiS_Pr.OverruleEMI = FALSE;
	SiS_Pr.SiS_SensibleSR11 = FALSE;
	SiS_Pr.SiS_MyCR63 = 0x63;
	SiS_Pr.PDC  = -1;
	SiS_Pr.PDCA = -1;
	if(si.chip >= SIS_330) {
		SiS_Pr.SiS_MyCR63 = 0x53;
		if(si.chip >= SIS_661) {
			SiS_Pr.SiS_SensibleSR11 = TRUE;
		}
	}
	
	/* Use northbridge to find out the exact chip */
	si.bridge = sis_get_northbridge( si.chip );
	if( si.bridge.nDeviceID != 0xffff )
	{
		switch( si.bridge.nDeviceID )
		{
			case 0x0730:
				si.chip = SIS_730;
				strcpy( si.name, "730" );
			break;
			case 0x0651:
				strcpy( si.name, "651" );
			break;
			case 0x0740:
				si.chip = SIS_740;
				strcpy( si.name, "740" );
			break;
			case 0x0661:
				si.chip = SIS_661;
				strcpy( si.name, "661" );
			break;
			case 0x0741:
				si.chip = SIS_741;
				strcpy( si.name, "741" );
			break;
			case 0x0760:
				si.chip = SIS_760;
				strcpy( si.name, "760" );
			break;
		}
	}
	
	dbprintf( "SiS %s found\n", si.name );
	
	/* Initialize BIOS emulation */
	sishw_ext.jChipType = si.chip;
	sishw_ext.ulIOAddress = SiS_Pr.RelIO = si.io_base;
	sishw_ext.ulIOAddress += 0x30;
	
	
	SiSRegInit(&SiS_Pr, (USHORT)sishw_ext.ulIOAddress);
	
	/* Read subsystem ids */
	si.subsys_vendor_id = pci_gfx_read_config( si.fd, si.pci_dev.nBus, si.pci_dev.nDevice, si.pci_dev.nFunction, PCI_SUBSYSTEM_VENDOR_ID, 2 );
    si.subsys_device_id = pci_gfx_read_config( si.fd, si.pci_dev.nBus, si.pci_dev.nDevice, si.pci_dev.nFunction, PCI_SUBSYSTEM_ID, 2 );
	
	dbprintf( "Subsystem id 0x%x 0x%x\n", (uint)si.subsys_vendor_id, (uint)si.subsys_device_id );
	
	/* Find PCI systems for Chrontel/GPIO communication setup */
	if(si.chip == SIS_630) {
		int i = 0;
        	do {
			if(mychswtable[i].subsysVendor == si.subsys_vendor_id &&
			   mychswtable[i].subsysCard   == si.subsys_device_id) {
				SiS_Pr.SiS_ChSW = TRUE;
				dbprintf( "Identified [%s %s] "
					"requiring Chrontel/GPIO setup\n",
					mychswtable[i].vendorName,
					mychswtable[i].cardName);
				break;
			}
			i++;
		} while(mychswtable[i].subsysVendor != 0);
	}
	
	/* Unlock hardware */
	outSISIDXREG( SISSR, IND_SIS_PASSWORD, SIS_PASSWORD );
	
	sishw_ext.bIntegratedMMEnabled = TRUE;
	if( si.sisvga_engine == SIS_300_VGA ) {
		if( si.chip != SIS_300 ) {
			inSISIDXREG( SISSR,0x1a, reg );
			if (!( reg & 0x10 ))
				sishw_ext.bIntegratedMMEnabled = FALSE;
		}
	}

	/* Search for ROM */
	sishw_ext.pjVirtualRomBase = sis_find_rom();
	if( sishw_ext.pjVirtualRomBase ) {
		dbprintf( "Video ROM found and mapped to %p\n",
			sishw_ext.pjVirtualRomBase );
		sishw_ext.UseROM = TRUE;
	} else {
		dbprintf( "Video ROM not found\n" );
		return -1;
	}
	
	/* Find systems for special custom timing */
	if(SiS_Pr.SiS_CustomT == CUT_NONE) {
	   int j;
	   int i;
	   unsigned char *biosver = NULL;
           unsigned char *biosdate = NULL;
	   BOOLEAN footprint;
	   u32 chksum = 0;

	   if(sishw_ext.UseROM) {
	      biosver = sishw_ext.pjVirtualRomBase + 0x06;
	      biosdate = sishw_ext.pjVirtualRomBase + 0x2c;
              for(i=0; i<32768; i++) chksum += sishw_ext.pjVirtualRomBase[i];
	   }

	   i=0;
           do {
	      if( (mycustomttable[i].chipID == si.chip) &&
		  ((!strlen(mycustomttable[i].biosversion)) ||
		   (sishw_ext.UseROM &&
		   (!strncmp(mycustomttable[i].biosversion, biosver, strlen(mycustomttable[i].biosversion))))) &&
		  ((!strlen(mycustomttable[i].biosdate)) ||
		   (sishw_ext.UseROM &&
		   (!strncmp(mycustomttable[i].biosdate, biosdate, strlen(mycustomttable[i].biosdate))))) &&
		  ((!mycustomttable[i].bioschksum) ||
		   (sishw_ext.UseROM &&
		   (mycustomttable[i].bioschksum == chksum)))	&&
		  (mycustomttable[i].pcisubsysvendor == si.subsys_vendor_id) &&
		  (mycustomttable[i].pcisubsyscard == si.subsys_device_id) ) {
		 footprint = TRUE;
		 for(j = 0; j < 5; j++) {
		    if(mycustomttable[i].biosFootprintAddr[j]) {
		       if(sishw_ext.UseROM) {
	                  if(sishw_ext.pjVirtualRomBase[mycustomttable[i].biosFootprintAddr[j]] !=
				mycustomttable[i].biosFootprintData[j]) {
				footprint = FALSE;
			  }
		       } else footprint = FALSE;
		    }
		 }
		 if(footprint) {
		    SiS_Pr.SiS_CustomT = mycustomttable[i].SpecialID;
		    dbprintf( "[%s %s], special timing applies\n",
			mycustomttable[i].vendorName,
			mycustomttable[i].cardName);
		    dbprintf("[specialtiming parameter name: %s]\n",
			mycustomttable[i].optionName);
	            break;
                 }
	      }
              i++;
           } while(mycustomttable[i].chipID);
	}
	
	/* Calculate RAM size */
	if( sis_get_dram_size() ) {
		dbprintf( "Unable to determine DRAM size !\n" );
		return -1;
	}
	
	dbprintf( "%i Mb RAM\n", (uint)( si.video_size / 1024 / 1024 ) );
	
	/* Enable PCI_LINEAR_ADDRESSING and MMIO_ENABLE  */
	orSISIDXREG( SISSR, IND_SIS_PCI_ADDRESS_SET, ( SIS_PCI_ADDR_ENABLE | SIS_MEM_MAP_IO_ENABLE ) );

	/* Enable 2D accelerator engine */
	orSISIDXREG( SISSR, IND_SIS_MODULE_ENABLE, SIS_ENABLE_2D );
	
	sishw_ext.ulVideoMemorySize = si.video_size;
	sishw_ext.pjVideoMemoryAddress = si.framebuffer;
	
	
	si.newrom = SiSDetermineROMLayout661(&SiS_Pr, &sishw_ext);
	
	/* Initialize video bridge */
	sis_sense_crt1();
	sis_get_VB_type();
	if(si.vbflags & VB_VIDEOBRIDGE) {
		sis_detect_VB_connect();
	}
	
	si.currentvbflags = si.vbflags & (VB_VIDEOBRIDGE | TV_STANDARD);
	
	if(si.vbflags & VB_VIDEOBRIDGE) {
		      /* Chrontel 700x TV detection often unreliable, therefore use a
		       * different default order on such machines
		       */
		      if((si.sisvga_engine == SIS_300_VGA) && (si.vbflags & VB_CHRONTEL)) {
		         if(si.vbflags & CRT2_LCD)      si.currentvbflags |= CRT2_LCD;
		         else if(si.vbflags & CRT2_TV)  si.currentvbflags |= CRT2_TV;
		         else if(si.vbflags & CRT2_VGA) si.currentvbflags |= CRT2_VGA;
		      } else {
		         if(si.vbflags & CRT2_TV)       si.currentvbflags |= CRT2_TV;
		         else if(si.vbflags & CRT2_LCD) si.currentvbflags |= CRT2_LCD;
		         else if(si.vbflags & CRT2_VGA) si.currentvbflags |= CRT2_VGA;
		      }
		   }

		if(si.vbflags & CRT2_LCD) {
			int i;
		   inSISIDXREG(SISCR, 0x36, reg);
		   reg &= 0x0f;
		   if(si.sisvga_engine == SIS_300_VGA) {
		      si.CRT2LCDType = sis300paneltype[reg];
		   } else if(si.chip >= SIS_661) {
		      si.CRT2LCDType = sis661paneltype[reg];
		   } else {
		      si.CRT2LCDType = sis310paneltype[reg];
		   }
		   if(si.CRT2LCDType == LCD_UNKNOWN) {
		      /* For broken BIOSes: Assume 1024x768, RGB18 */
		      si.CRT2LCDType = LCD_1024x768;
		      setSISIDXREG(SISCR,0x36,0xf0,0x02);
		      setSISIDXREG(SISCR,0x37,0xee,0x01);
		      dbprintf(" Invalid panel ID (%02x), assuming 1024x768, RGB18\n", reg);
		   }
		   for(i = 0; i < SIS_LCD_NUMBER; i++) {
		      if(si.CRT2LCDType == sis_lcd_data[i].lcdtype) {
		         si.lcdxres = sis_lcd_data[i].xres;
			 si.lcdyres = sis_lcd_data[i].yres;
			 si.lcddefmodeidx = sis_lcd_data[i].default_mode_idx;
			 break;
		      }
		   }
		   if(SiS_Pr.SiS_CustomT == CUT_BARCO1366) {
			si.lcdxres = 1360; si.lcdyres = 1024;
		   } else if(SiS_Pr.SiS_CustomT == CUT_PANEL848) {
			si.lcdxres =  848; si.lcdyres =  480;
		   }
		   dbprintf( "Detected %dx%d flat panel\n",	si.lcdxres, si.lcdyres);
		}
        
        /* Save the current PanelDelayCompensation if the LCD is currently used */
		if(si.sisvga_engine == SIS_300_VGA) {
	           if(si.vbflags & (VB_LVDS | VB_30xBDH)) {
		       int tmp;
		       inSISIDXREG(SISCR,0x30,tmp);
		       if(tmp & 0x20) {
		          /* Currently on LCD? If yes, read current pdc */
		          inSISIDXREG(SISPART1,0x13,si.detectedpdc);
			  si.detectedpdc &= 0x3c;
			  if(SiS_Pr.PDC == -1) {
			     /* Let option override detection */
			     SiS_Pr.PDC = si.detectedpdc;
			  }
			  dbprintf( "Detected LCD PDC 0x%02x\n",
			         si.detectedpdc);
		       }
		       if((SiS_Pr.PDC != -1) && (SiS_Pr.PDC != si.detectedpdc)) {
		          dbprintf( "Using LCD PDC 0x%02x\n",SiS_Pr.PDC);
		       }
	           }
		}

		if(si.sisvga_engine == SIS_315_VGA) {

		   /* Try to find about LCDA */
		   if(si.vbflags & (VB_301C | VB_302B | VB_301LV | VB_302LV | VB_302ELV)) {
		      int tmp;
		      inSISIDXREG(SISPART1,0x13,tmp);
		      if(tmp & 0x04) {
		         SiS_Pr.SiS_UseLCDA = TRUE;
		         si.detectedlcda = 0x03;
		      }
	           }

		   /* Save PDC */
		   if(si.vbflags & (VB_301LV | VB_302LV | VB_302ELV)) {
		      int tmp;
		      inSISIDXREG(SISCR,0x30,tmp);
		      if((tmp & 0x20) || (si.detectedlcda != 0xff)) {
		         /* Currently on LCD? If yes, read current pdc */
			 u8 pdc;
			 inSISIDXREG(SISPART1,0x2D,pdc);
			 si.detectedpdc  = (pdc & 0x0f) << 1;
			 si.detectedpdca = (pdc & 0xf0) >> 3;
			 inSISIDXREG(SISPART1,0x35,pdc);
			 si.detectedpdc |= ((pdc >> 7) & 0x01);
			 inSISIDXREG(SISPART1,0x20,pdc);
			 si.detectedpdca |= ((pdc >> 6) & 0x01);
			 if(si.newrom) {
			    /* New ROM invalidates other PDC resp. */
			    if(si.detectedlcda != 0xff) {
			       si.detectedpdc = 0xff;
			    } else {
			       si.detectedpdca = 0xff;
			    }
			 }
			 if(SiS_Pr.PDC == -1) {
			    if(si.detectedpdc != 0xff) {
			       SiS_Pr.PDC = si.detectedpdc;
			    }
			 }
			 if(SiS_Pr.PDCA == -1) {
			    if(si.detectedpdca != 0xff) {
			       SiS_Pr.PDCA = si.detectedpdca;
			    }
			 }
			 if(si.detectedpdc != 0xff) {
			    dbprintf("Detected LCD PDC 0x%02x (for LCD=CRT2)\n",
			          si.detectedpdc);
			 }
			 if(si.detectedpdca != 0xff) {
			    dbprintf("Detected LCD PDC1 0x%02x (for LCD=CRT1)\n",
			          si.detectedpdca);
			 }
		      }

		      /* Save EMI */
		      if(si.vbflags & (VB_302LV | VB_302ELV)) {
		         inSISIDXREG(SISPART4,0x30,SiS_Pr.EMI_30);
			 inSISIDXREG(SISPART4,0x31,SiS_Pr.EMI_31);
			 inSISIDXREG(SISPART4,0x32,SiS_Pr.EMI_32);
			 inSISIDXREG(SISPART4,0x33,SiS_Pr.EMI_33);
			 SiS_Pr.HaveEMI = TRUE;
			 if((tmp & 0x20) || (si.detectedlcda != 0xff)) {
			  	SiS_Pr.HaveEMILCD = TRUE;
			 }
		      }
		   }

		   /* Let user override detected PDCs (all bridges) */
		   if(si.vbflags & (VB_301B | VB_301C | VB_301LV | VB_302LV | VB_302ELV)) {
		      if((SiS_Pr.PDC != -1) && (SiS_Pr.PDC != si.detectedpdc)) {
		         dbprintf("Using LCD PDC 0x%02x (for LCD=CRT2)\n",
				 SiS_Pr.PDC);
		      }
		      if((SiS_Pr.PDCA != -1) && (SiS_Pr.PDCA != si.detectedpdca)) {
		         dbprintf( "Using LCD PDC1 0x%02x (for LCD=CRT1)\n",
				 SiS_Pr.PDCA);
		      }
		   }

		}
	
	/* Initialize command queue */
	if( si.sisvga_engine == SIS_315_VGA ) {
		sis_enable_queue_315();
	}
	if( si.sisvga_engine == SIS_300_VGA ) {
		sis_enable_queue_300();
	}
	
	return 0;
}


















