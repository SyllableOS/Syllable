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
	NULL, NULL, FALSE, NULL, NULL,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	NULL, NULL, NULL, NULL,
	{0, 0, 0, 0},
	0
};


struct _sisbios_mode sisbios_mode[] = {
#define MODE_INDEX_NONE           0  /* TW: index for mode=none */
	{"none",         0xFF, 0x0000, 0x0000,    0,    0,  0, 0,   0,  0, MD_SIS300|MD_SIS315},  /* TW: for mode "none" */
	{"320x240x16",   0x56, 0x0000, 0x0000,  320,  240, 16, 1,  40, 15,           MD_SIS315},
	{"320x480x8",    0x5A, 0x0000, 0x0000,  320,  480,  8, 1,  40, 30,           MD_SIS315},  /* TW: FSTN */
	{"320x480x16",   0x5B, 0x0000, 0x0000,  320,  480, 16, 1,  40, 30,           MD_SIS315},  /* TW: FSTN */
	{"640x480x8",    0x2E, 0x0101, 0x0101,  640,  480,  8, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"640x480x16",   0x44, 0x0111, 0x0111,  640,  480, 16, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"640x480x24",   0x62, 0x013a, 0x0112,  640,  480, 32, 1,  80, 30, MD_SIS300|MD_SIS315},  /* TW: That's for people who mix up color- and fb depth */
	{"640x480x32",   0x62, 0x013a, 0x0112,  640,  480, 32, 1,  80, 30, MD_SIS300|MD_SIS315},
	{"720x480x8",    0x31, 0x0000, 0x0000,  720,  480,  8, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x480x16",   0x33, 0x0000, 0x0000,  720,  480, 16, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x480x24",   0x35, 0x0000, 0x0000,  720,  480, 32, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x480x32",   0x35, 0x0000, 0x0000,  720,  480, 32, 1,  90, 30, MD_SIS300|MD_SIS315},
	{"720x576x8",    0x32, 0x0000, 0x0000,  720,  576,  8, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"720x576x16",   0x34, 0x0000, 0x0000,  720,  576, 16, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"720x576x24",   0x36, 0x0000, 0x0000,  720,  576, 32, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"720x576x32",   0x36, 0x0000, 0x0000,  720,  576, 32, 1,  90, 36, MD_SIS300|MD_SIS315},
	{"800x480x8",    0x70, 0x0000, 0x0000,  800,  480,  8, 1, 100, 30, MD_SIS300|MD_SIS315},
	{"800x480x16",   0x7a, 0x0000, 0x0000,  800,  480, 16, 1, 100, 30, MD_SIS300|MD_SIS315},
	{"800x480x24",   0x76, 0x0000, 0x0000,  800,  480, 32, 1, 100, 30, MD_SIS300|MD_SIS315},
	{"800x480x32",   0x76, 0x0000, 0x0000,  800,  480, 32, 1, 100, 30, MD_SIS300|MD_SIS315},
#define DEFAULT_MODE              20 /* TW: index for 800x600x8 */
#define DEFAULT_LCDMODE           20 /* TW: index for 800x600x8 */
#define DEFAULT_TVMODE            20 /* TW: index for 800x600x8 */
	{"800x600x8",    0x30, 0x0103, 0x0103,  800,  600,  8, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"800x600x16",   0x47, 0x0114, 0x0114,  800,  600, 16, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"800x600x24",   0x63, 0x013b, 0x0115,  800,  600, 32, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"800x600x32",   0x63, 0x013b, 0x0115,  800,  600, 32, 2, 100, 37, MD_SIS300|MD_SIS315},
	{"1024x576x8",   0x71, 0x0000, 0x0000, 1024,  576,  8, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x576x16",  0x74, 0x0000, 0x0000, 1024,  576, 16, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x576x24",  0x77, 0x0000, 0x0000, 1024,  576, 32, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x576x32",  0x77, 0x0000, 0x0000, 1024,  576, 32, 1, 128, 36, MD_SIS300|MD_SIS315},
	{"1024x600x8",   0x20, 0x0000, 0x0000, 1024,  600,  8, 1, 128, 37, MD_SIS300          },  /* TW: 300 series only */
	{"1024x600x16",  0x21, 0x0000, 0x0000, 1024,  600, 16, 1, 128, 37, MD_SIS300          },
	{"1024x600x24",  0x22, 0x0000, 0x0000, 1024,  600, 32, 1, 128, 37, MD_SIS300          },
	{"1024x600x32",  0x22, 0x0000, 0x0000, 1024,  600, 32, 1, 128, 37, MD_SIS300          },
	{"1024x768x8",   0x38, 0x0105, 0x0105, 1024,  768,  8, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1024x768x16",  0x4A, 0x0117, 0x0117, 1024,  768, 16, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1024x768x24",  0x64, 0x013c, 0x0118, 1024,  768, 32, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1024x768x32",  0x64, 0x013c, 0x0118, 1024,  768, 32, 2, 128, 48, MD_SIS300|MD_SIS315},
	{"1152x768x8",   0x23, 0x0000, 0x0000, 1152,  768,  8, 1, 144, 48, MD_SIS300          },  /* TW: 300 series only */
	{"1152x768x16",  0x24, 0x0000, 0x0000, 1152,  768, 16, 1, 144, 48, MD_SIS300          },
	{"1152x768x24",  0x25, 0x0000, 0x0000, 1152,  768, 32, 1, 144, 48, MD_SIS300          },
	{"1152x768x32",  0x25, 0x0000, 0x0000, 1152,  768, 32, 1, 144, 48, MD_SIS300          },
	{"1280x720x8",   0x79, 0x0000, 0x0000, 1280,  720,  8, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x720x16",  0x75, 0x0000, 0x0000, 1280,  720, 16, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x720x24",  0x78, 0x0000, 0x0000, 1280,  720, 32, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x720x32",  0x78, 0x0000, 0x0000, 1280,  720, 32, 1, 160, 45, MD_SIS300|MD_SIS315},
	{"1280x768x8",   0x23, 0x0000, 0x0000, 1280,  768,  8, 1, 160, 48,           MD_SIS315},  /* TW: 310/325 series only */
	{"1280x768x16",  0x24, 0x0000, 0x0000, 1280,  768, 16, 1, 160, 48,           MD_SIS315},
	{"1280x768x24",  0x25, 0x0000, 0x0000, 1280,  768, 32, 1, 160, 48,           MD_SIS315},
	{"1280x768x32",  0x25, 0x0000, 0x0000, 1280,  768, 32, 1, 160, 48,           MD_SIS315},
#define MODEINDEX_1280x960 48
	{"1280x960x8",   0x7C, 0x0000, 0x0000, 1280,  960,  8, 1, 160, 60, MD_SIS300|MD_SIS315},  /* TW: Modenumbers being patched */
	{"1280x960x16",  0x7D, 0x0000, 0x0000, 1280,  960, 16, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x960x24",  0x7E, 0x0000, 0x0000, 1280,  960, 32, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x960x32",  0x7E, 0x0000, 0x0000, 1280,  960, 32, 1, 160, 60, MD_SIS300|MD_SIS315},
	{"1280x1024x8",  0x3A, 0x0107, 0x0107, 1280, 1024,  8, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1280x1024x16", 0x4D, 0x011a, 0x011a, 1280, 1024, 16, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1280x1024x24", 0x65, 0x013d, 0x011b, 1280, 1024, 32, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1280x1024x32", 0x65, 0x013d, 0x011b, 1280, 1024, 32, 2, 160, 64, MD_SIS300|MD_SIS315},
	{"1400x1050x8",  0x26, 0x0000, 0x0000, 1400, 1050,  8, 1, 175, 65,           MD_SIS315},  /* TW: 310/325 series only */
	{"1400x1050x16", 0x27, 0x0000, 0x0000, 1400, 1050, 16, 1, 175, 65,           MD_SIS315},
	{"1400x1050x24", 0x28, 0x0000, 0x0000, 1400, 1050, 32, 1, 175, 65,           MD_SIS315},
	{"1400x1050x32", 0x28, 0x0000, 0x0000, 1400, 1050, 32, 1, 175, 65,           MD_SIS315},
	{"1600x1200x8",  0x3C, 0x0130, 0x011c, 1600, 1200,  8, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1600x1200x16", 0x3D, 0x0131, 0x011e, 1600, 1200, 16, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1600x1200x24", 0x66, 0x013e, 0x011f, 1600, 1200, 32, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1600x1200x32", 0x66, 0x013e, 0x011f, 1600, 1200, 32, 1, 200, 75, MD_SIS300|MD_SIS315},
	{"1920x1440x8",  0x68, 0x013f, 0x0000, 1920, 1440,  8, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"1920x1440x16", 0x69, 0x0140, 0x0000, 1920, 1440, 16, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"1920x1440x24", 0x6B, 0x0141, 0x0000, 1920, 1440, 32, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"1920x1440x32", 0x6B, 0x0141, 0x0000, 1920, 1440, 32, 1, 240, 75, MD_SIS300|MD_SIS315},
	{"2048x1536x8",  0x6c, 0x0000, 0x0000, 2048, 1536,  8, 1, 256, 96,           MD_SIS315},  /* TW: 310/325 series only */
	{"2048x1536x16", 0x6d, 0x0000, 0x0000, 2048, 1536, 16, 1, 256, 96,           MD_SIS315},
	{"2048x1536x24", 0x6e, 0x0000, 0x0000, 2048, 1536, 32, 1, 256, 96,           MD_SIS315},
	{"2048x1536x32", 0x6e, 0x0000, 0x0000, 2048, 1536, 32, 1, 256, 96,           MD_SIS315},
	{"\0", 0x00, 0, 0, 0, 0, 0, 0, 0}
};

void sis_set_reg4( uint16 port, unsigned long data )
{
	outl( (uint32)( data & 0xffffffff ), port );
}

uint32 sis_get_reg3( uint16 port )
{
	uint32 data;
	data = inl( port );
	return data;
}

BOOLEAN
sis_query_config_space(PSIS_HW_DEVICE_INFO psishw_ext,
	unsigned long offset, unsigned long set, unsigned long *value)
{
	if (set == 0)
		*value = read_pci_config( si.pci_dev.nBus, si.pci_dev.nDevice, si.pci_dev.nFunction,
								offset, 4 );
	else
		write_pci_config( si.pci_dev.nBus, si.pci_dev.nDevice, si.pci_dev.nFunction,
								offset, 4, *value );

	return TRUE;
}

int sis_do_sense(int tempbl, int tempbh, int tempcl, int tempch)
{
    int temp,i;

    outSISIDXREG(SISPART4,0x11,tempbl);
    temp = tempbh | tempcl;
    setSISIDXREG(SISPART4,0x10,0xe0,temp);
    for(i=0; i<10; i++) SiS_LongWait(&SiS_Pr);
    tempch &= 0x7f;
    inSISIDXREG(SISPART4,0x03,temp);
    temp ^= 0x0e;
    temp &= tempch;
    return(temp);
}


void sis_sense_30x(void)
{
  uint8 backupP4_0d;
  uint8 testsvhs_tempbl, testsvhs_tempbh;
  uint8 testsvhs_tempcl, testsvhs_tempch;
  uint8 testcvbs_tempbl, testcvbs_tempbh;
  uint8 testcvbs_tempcl, testcvbs_tempch;
  int myflag, result;

  inSISIDXREG(SISPART4,0x0d,backupP4_0d);
  outSISIDXREG(SISPART4,0x0d,(backupP4_0d | 0x04));

  if(si.vga_engine == SIS_300_VGA) {

        testsvhs_tempbh = 0x00; testsvhs_tempbl = 0xb9;
	testcvbs_tempbh = 0x00; testcvbs_tempbl = 0xb3;
	if((sishw_ext.ujVBChipID != VB_CHIP_301) &&
	   (sishw_ext.ujVBChipID != VB_CHIP_302) ) {
	   testsvhs_tempbh = 0x01; testsvhs_tempbl = 0x6b;
	   testcvbs_tempbh = 0x01; testcvbs_tempbl = 0x74;
	}
	inSISIDXREG(SISPART4,0x01,myflag);
	if(myflag & 0x04) {
	   testsvhs_tempbh = 0x00; testsvhs_tempbl = 0xdd;
	   testcvbs_tempbh = 0x00; testcvbs_tempbl = 0xee;
	}
	testsvhs_tempch = 0x06;	testsvhs_tempcl = 0x04;
	testcvbs_tempch = 0x08; testcvbs_tempcl = 0x04;

  } else if((si.chip == SIS_315) ||
    	    (si.chip == SIS_315H) ||
	    (si.chip == SIS_315PRO)) {

        testsvhs_tempbh = 0x00; testsvhs_tempbl = 0xb9;
	testcvbs_tempbh = 0x00; testcvbs_tempbl = 0xb3;
	if((sishw_ext.ujVBChipID != VB_CHIP_301) &&
	   (sishw_ext.ujVBChipID != VB_CHIP_302) ) {
	      testsvhs_tempbh = 0x01; testsvhs_tempbl = 0x6b;
	      testcvbs_tempbh = 0x01; testcvbs_tempbl = 0x74;
	}
	inSISIDXREG(SISPART4,0x01,myflag);
	if(myflag & 0x04) {
	   testsvhs_tempbh = 0x00; testsvhs_tempbl = 0xdd;
	   testcvbs_tempbh = 0x00; testcvbs_tempbl = 0xee;
	}
	testsvhs_tempch = 0x06;	testsvhs_tempcl = 0x04;
	testcvbs_tempch = 0x08; testcvbs_tempcl = 0x04;

    } else {

        testsvhs_tempbh = 0x02; testsvhs_tempbl = 0x00;
	testcvbs_tempbh = 0x01; testcvbs_tempbl = 0x00;

	testsvhs_tempch = 0x04;	testsvhs_tempcl = 0x08;
	testcvbs_tempch = 0x08; testcvbs_tempcl = 0x08;

    }

    result = sis_do_sense(testsvhs_tempbl, testsvhs_tempbh,
                        testsvhs_tempcl, testsvhs_tempch);
    if(result) {
        dbprintf("Detected TV connected to SVHS output\n");
        /* TW: So we can be sure that there IS a SVHS output */
		si.TV_plug = TVPLUG_SVIDEO;
		orSISIDXREG(SISCR, 0x32, 0x02);
    }

    if(!result) {
        result = sis_do_sense(testcvbs_tempbl, testcvbs_tempbh,
	                    testcvbs_tempcl, testcvbs_tempch);
	if(result) {
		dbprintf("Detected TV connected to CVBS output\n");
	    /* TW: So we can be sure that there IS a CVBS output */
	    si.TV_plug = TVPLUG_COMPOSITE;
	    orSISIDXREG(SISCR, 0x32, 0x01);
	}
    }
    sis_do_sense(0, 0, 0, 0);

    outSISIDXREG(SISPART4,0x0d,backupP4_0d);
}

/* TW: Determine and detect attached TV's on Chrontel */
void sis_sense_ch(void)
{

   uint8 temp1;
   uint8 temp2;

   if(si.chip < SIS_315H) {

       SiS_Pr.SiS_IF_DEF_CH70xx = 1;		/* TW: Chrontel 7005 */
       temp1 = SiS_GetCH700x(&SiS_Pr, 0x25);
       if ((temp1 >= 50) && (temp1 <= 100)) {
	   /* TW: Read power status */
	   temp1 = SiS_GetCH700x(&SiS_Pr, 0x0e);
	   if((temp1 & 0x03) != 0x03) {
     	        /* TW: Power all outputs */
		SiS_SetCH70xxANDOR(&SiS_Pr, 0x030E,0xF8);
	   }
	   /* TW: Sense connected TV devices */
	   SiS_SetCH700x(&SiS_Pr, 0x0110);
	   SiS_SetCH700x(&SiS_Pr, 0x0010);
	   temp1 = SiS_GetCH700x(&SiS_Pr, 0x10);
	   if(!(temp1 & 0x08)) {
		dbprintf(" Chrontel: Detected TV connected to SVHS output\n");
		/* TW: So we can be sure that there IS a SVHS output */
		si.TV_plug = TVPLUG_SVIDEO;
		orSISIDXREG(SISCR, 0x32, 0x02);
	   } else if (!(temp1 & 0x02)) {
		dbprintf( "Chrontel: Detected TV connected to CVBS output\n");
		/* TW: So we can be sure that there IS a CVBS output */
		si.TV_plug = TVPLUG_COMPOSITE;
		orSISIDXREG(SISCR, 0x32, 0x01);
	   } else {
 		SiS_SetCH70xxANDOR(&SiS_Pr, 0x010E,0xF8);
	   }
       } else if(temp1 == 0) {
	  SiS_SetCH70xxANDOR(&SiS_Pr, 0x010E,0xF8);
       }

   } else {

	SiS_Pr.SiS_IF_DEF_CH70xx = 2;		/* TW: Chrontel 7019 */
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
	     dbprintf("Chrontel: Detected TV connected to CVBS output\n");
	     si.TV_plug = TVPLUG_COMPOSITE;
	     orSISIDXREG(SISCR, 0x32, 0x01);
             break;
	case 0x02:
	     dbprintf("Chrontel: Detected TV connected to SVHS output\n");
	     si.TV_plug = TVPLUG_SVIDEO;
	     orSISIDXREG(SISCR, 0x32, 0x02);
             break;
	case 0x04:
	     /* TW: This should not happen */
	     dbprintf("Chrontel: Detected TV connected to SCART output!?\n");
             break;
	}

   }
}


int sis_get_dram_size_300(void)
{
	uint8 pci_data, reg;
	
	if( si.chip == SIS_540 || si.chip == SIS_630 || si.chip == 730 ) {
		pci_data = read_pci_config( si.pci_dev.nBus, si.pci_dev.nDevice, si.pci_dev.nFunction,
								IND_BRI_DRAM_STATUS, 1 );
		pci_data = ( pci_data & BRI_DRAM_SIZE_MASK ) >> 4;
		si.mem_size = (unsigned int)(1 << (pci_data+21));
				
		reg = SIS_DATA_BUS_64 << 6;
		switch( pci_data ) {
			case BRI_DRAM_SIZE_2MB:
				reg |= SIS_DRAM_SIZE_2MB;
			break;
			case BRI_DRAM_SIZE_4MB:
				reg |= SIS_DRAM_SIZE_4MB;
			break;
			case BRI_DRAM_SIZE_8MB:
				reg |= SIS_DRAM_SIZE_8MB;
			break;
			case BRI_DRAM_SIZE_16MB:
				reg |= SIS_DRAM_SIZE_16MB;
			break;
			case BRI_DRAM_SIZE_32MB:
				reg |= SIS_DRAM_SIZE_32MB;
			break;
			case BRI_DRAM_SIZE_64MB:
				reg |= SIS_DRAM_SIZE_64MB;
			break;
		}
		outSISIDXREG( SISSR, IND_SIS_DRAM_SIZE, reg );
	} else {
		inSISIDXREG( SISSR, IND_SIS_DRAM_SIZE,reg );
		si.mem_size = ( (unsigned int) ( ( reg & SIS_DRAM_SIZE_MASK) + 1 ) << 20 );
	}
	return 0;
}

void sis_detect_VB_connect_300()
{
	uint8 sr16, sr17, cr32, temp;

	si.TV_plug = si.TV_type = 0;

	switch( si.hasVB ) {
		case HASVB_LVDS_CHRONTEL:
		case HASVB_CHRONTEL:
			sis_sense_ch();
		break;
		case HASVB_301:
		case HASVB_302:
			sis_sense_30x();
		break;
	}

	inSISIDXREG( SISSR, IND_SIS_SCRATCH_REG_17, sr17 );
	inSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR32, cr32 );

	if( (sr17 & 0x0F) && (si.chip != SIS_300) ) {
		if (sr17 & 0x08 )
			si.disp_state = DISPTYPE_CRT2;
		else if( sr17 & 0x02 )
			si.disp_state = DISPTYPE_LCD;
		else if( sr17 & 0x04 )
			si.disp_state = DISPTYPE_TV;
		else
			si.disp_state = 0;

		if( sr17 & 0x20 )
			si.TV_plug = TVPLUG_SVIDEO;
		else if( sr17 & 0x10 )
			si.TV_plug = TVPLUG_COMPOSITE;

		inSISIDXREG( SISSR, IND_SIS_SCRATCH_REG_16, sr16 );
		if( sr16 & 0x20 )
			si.TV_type = TVMODE_PAL;
		else
			si.TV_type = TVMODE_NTSC;

	} else {
		if (cr32 & SIS_VB_CRT2)
			si.disp_state = DISPTYPE_CRT2;
		else if( cr32 & SIS_VB_LCD )
			si.disp_state = DISPTYPE_LCD;
		else if( cr32 & SIS_VB_TV )
			si.disp_state = DISPTYPE_TV;
		else
			si.disp_state = 0;

		if( cr32 & SIS_VB_SVIDEO )
			si.TV_plug = TVPLUG_SVIDEO;
		else if( cr32 & SIS_VB_COMPOSITE )
			si.TV_plug = TVPLUG_COMPOSITE;
		else if( cr32 & SIS_VB_SCART )
			si.TV_plug = TVPLUG_SCART;

		if( si.TV_type == 0 ) {
			inSISIDXREG( SISSR, IND_SIS_POWER_ON_TRAP, temp );
			if( temp & 0x01 )
				si.TV_type = TVMODE_PAL;
			else
				si.TV_type = TVMODE_NTSC;
		}

	}
}

int sis_has_VB_300()
{
	uint8 vb_chipid;

	inSISIDXREG( SISPART4, 0x00, vb_chipid );
	switch( vb_chipid ) {
		case 0x01:
			si.hasVB = HASVB_301;
		break;
		case 0x02:
			si.hasVB = HASVB_302;
		break;
		case 0x03:
			si.hasVB = HASVB_303;
		break;
		default:
			si.hasVB = HASVB_NONE;
		return FALSE;
	}
	return TRUE;
}

void sis_get_VB_type_300()
{
	uint8 reg;

	if( si.chip != SIS_300 ) {
		if( !sis_has_VB_300() ) {
			inSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR37, reg );
			switch( ( reg & SIS_EXTERNAL_CHIP_MASK ) >> 1 ) {
				case SIS_EXTERNAL_CHIP_LVDS:
					si.hasVB = HASVB_LVDS;
				break;
				case SIS_EXTERNAL_CHIP_TRUMPION:
					si.hasVB = HASVB_TRUMPION;
				break;
				case SIS_EXTERNAL_CHIP_LVDS_CHRONTEL:
					si.hasVB = HASVB_LVDS_CHRONTEL;
				break;
				case SIS_EXTERNAL_CHIP_CHRONTEL:
					si.hasVB = HASVB_CHRONTEL;
				break;
			   default:
				break;
			}
		}
	} else {
		sis_has_VB_300();
	}
}


void sis_enable_queue_300()
{
	unsigned int tqueue_pos;
	uint8 tq_state;
	
	tqueue_pos = ( si.mem_size -
		       TURBO_QUEUE_AREA_SIZE ) / ( 64 * 1024 );
	inSISIDXREG( SISSR, IND_SIS_TURBOQUEUE_SET, tq_state );
	tq_state |= 0xf0;
	tq_state &= 0xfc;
	tq_state |= (uint8)( tqueue_pos >> 8 );
	outSISIDXREG( SISSR, IND_SIS_TURBOQUEUE_SET, tq_state );
	outSISIDXREG( SISSR, IND_SIS_TURBOQUEUE_ADR, (uint8)( tqueue_pos & 0xff ) );
	si.mem_size -= TURBO_QUEUE_AREA_SIZE;
}


int sis_get_dram_size_315(void)
{
	uint8  pci_data;
	uint8  reg = 0;

	if( si.chip == SIS_550 || si.chip == SIS_650 || si.chip == SIS_740 ||
		si.chip == SIS_660 || si.chip == 760 ) {
		inSISIDXREG( SISSR, IND_SIS_DRAM_SIZE, reg );
		switch( reg & SIS550_DRAM_SIZE_MASK ) {
			case SIS550_DRAM_SIZE_4MB:
				si.mem_size = 0x400000;   break;
			case SIS550_DRAM_SIZE_8MB:
				si.mem_size = 0x800000;   break;
			case SIS550_DRAM_SIZE_16MB:
				si.mem_size = 0x1000000;  break;
			case SIS550_DRAM_SIZE_24MB:
				si.mem_size = 0x1800000;  break;
			case SIS550_DRAM_SIZE_32MB:
				si.mem_size = 0x2000000;	break;
			case SIS550_DRAM_SIZE_64MB:
				si.mem_size = 0x4000000;	break;
			case SIS550_DRAM_SIZE_96MB:
				si.mem_size = 0x6000000;	break;
			case SIS550_DRAM_SIZE_128MB:
				si.mem_size = 0x8000000;	break;
			case SIS550_DRAM_SIZE_256MB:
				si.mem_size = 0x10000000;	break;
			default:
		    
				dbprintf( "Warning: Could not determine memory size, "
			       "now reading from PCI config\n");
			
				pci_data = read_pci_config( si.pci_dev.nBus, si.pci_dev.nDevice, si.pci_dev.nFunction,
								IND_BRI_DRAM_STATUS, 1 );
				pci_data = ( pci_data & BRI_DRAM_SIZE_MASK ) >> 4;
				si.mem_size = (unsigned int)( 1 << ( pci_data + 21 ) );
				/* TW: Initialize SR14=IND_SIS_DRAM_SIZE */
				inSISIDXREG( SISSR, IND_SIS_DRAM_SIZE, reg );
				reg &= 0xC0;
				switch( pci_data ) {
					case BRI_DRAM_SIZE_4MB:
						reg |= SIS550_DRAM_SIZE_4MB;  break;
					case BRI_DRAM_SIZE_8MB:
						reg |= SIS550_DRAM_SIZE_8MB;  break;
					case BRI_DRAM_SIZE_16MB:
						reg |= SIS550_DRAM_SIZE_16MB; break;
					case BRI_DRAM_SIZE_32MB:
						reg |= SIS550_DRAM_SIZE_32MB; break;
					case BRI_DRAM_SIZE_64MB:
						reg |= SIS550_DRAM_SIZE_64MB; break;
					default:
				   		dbprintf( "Unable to determine memory size, giving up.\n" );
					return -1;
				}
				outSISIDXREG( SISSR, IND_SIS_DRAM_SIZE, reg );
		}
		return 0;

	} else {	/* 315, 330 */
		inSISIDXREG( SISSR, IND_SIS_DRAM_SIZE, reg );
		switch( ( reg & SIS315_DRAM_SIZE_MASK ) >> 4 ) {
			case SIS315_DRAM_SIZE_2MB:
				si.mem_size = 0x200000;
			break;
			case SIS315_DRAM_SIZE_4MB:
				si.mem_size = 0x400000;
			break;
			case SIS315_DRAM_SIZE_8MB:
				si.mem_size = 0x800000;
			break;
			case SIS315_DRAM_SIZE_16MB:
				si.mem_size = 0x1000000;
			break;
			case SIS315_DRAM_SIZE_32MB:
				si.mem_size = 0x2000000;
			break;
			case SIS315_DRAM_SIZE_64MB:
				si.mem_size = 0x4000000;
			break;
			case SIS315_DRAM_SIZE_128MB:
				si.mem_size = 0x8000000;
			break;
			default:
				return -1;
		}
	}

	reg &= SIS315_DUAL_CHANNEL_MASK;
	reg >>= 2;
	switch( reg ) {
		case SIS315_SINGLE_CHANNEL_2_RANK:
			si.mem_size <<= 1;
		break;
		case SIS315_DUAL_CHANNEL_1_RANK:
			si.mem_size <<= 1;
		break;
		case SIS315_ASYM_DDR:		/* TW: DDR asymentric */
			si.mem_size += ( si.mem_size / 2 );
		break;
	}

	return 0;
}


void sis_detect_VB_connect_315()
{
	uint8 cr32, temp=0;

	si.TV_plug = si.TV_type = 0;

 	switch(si.hasVB) {
 		case HASVB_LVDS_CHRONTEL:
		case HASVB_CHRONTEL:
			sis_sense_ch();
		break;
		case HASVB_301:
		case HASVB_302:
			sis_sense_30x();
		break;
	}

	inSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR32, cr32 );

	
	if( cr32 & SIS_VB_CRT2 )
		si.disp_state = DISPTYPE_CRT2;
	else if( cr32 & SIS_VB_LCD )
		si.disp_state = DISPTYPE_LCD;
	else if( cr32 & SIS_VB_TV )
		si.disp_state = DISPTYPE_TV;
	else
		si.disp_state = 0;

	
	if( cr32 & SIS_VB_SVIDEO )
		si.TV_plug = TVPLUG_SVIDEO;
	else if( cr32 & SIS_VB_COMPOSITE )
		si.TV_plug = TVPLUG_COMPOSITE;
	else if( cr32 & SIS_VB_SCART )
		si.TV_plug = TVPLUG_SCART;

	if( si.TV_type == 0 ) {
	    /* TW: PAL/NTSC changed for 650 */
	    if( si.chip <= SIS_315PRO ) {
	    	inSISIDXREG( SISCR, 0x38, temp );
			if( temp & 0x10 )
				si.TV_type = TVMODE_PAL;
			else
				si.TV_type = TVMODE_NTSC;
	    } else {
	    	inSISIDXREG(SISCR, 0x79, temp);
			if( temp & 0x20 )
				si.TV_type = TVMODE_PAL;
			else
				si.TV_type = TVMODE_NTSC;
		}
	}
}

int sis_has_VB_315(void)
{
	uint8 vb_chipid;

	inSISIDXREG( SISPART4, 0x00, vb_chipid );
	switch (vb_chipid) {
		case 0x01:
			si.hasVB = HASVB_301;
		break;
		case 0x02:
			si.hasVB = HASVB_302;
		break;
		case 0x03:
			si.hasVB = HASVB_303;
		break;
		default:
			si.hasVB = HASVB_NONE;
		return FALSE;
	}
	return TRUE;
}

void sis_get_VB_type_315(void)
{
	uint8 reg;

	if( !sis_has_VB_315() ) {
		inSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR37, reg );
		/* TW: CR37 changed on 310/325 series */
		switch( (reg & SIS_EXTERNAL_CHIP_MASK) >> 1 ) {
			case SIS310_EXTERNAL_CHIP_LVDS:
				si.hasVB = HASVB_LVDS;
			break;
			case SIS310_EXTERNAL_CHIP_LVDS_CHRONTEL:
				si.hasVB = HASVB_LVDS_CHRONTEL;
			break;
			default:
			break;
		}
	}
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
	*cmdq_baseport = si.mem_size - COMMAND_QUEUE_AREA_SIZE;
	
	si.mem_size -= COMMAND_QUEUE_AREA_SIZE;
}

void sis_pre_setmode( uint8 rate_idx )
{
	uint8 cr30 = 0, cr31 = 0;

	inSISIDXREG( SISCR, 0x31, cr31 );
	cr31 &= ~0x60;

	switch( si.disp_state & DISPTYPE_DISP2 ) {
		case DISPTYPE_CRT2:
			cr30 = (SIS_VB_OUTPUT_CRT2 | SIS_SIMULTANEOUS_VIEW_ENABLE);
			cr31 |= SIS_DRIVER_MODE;
		break;
		case DISPTYPE_LCD:
			cr30  = (SIS_VB_OUTPUT_LCD | SIS_SIMULTANEOUS_VIEW_ENABLE);
			cr31 |= SIS_DRIVER_MODE;
		break;
		case DISPTYPE_TV:
			if( si.TV_type == TVMODE_HIVISION )
				cr30 = (SIS_VB_OUTPUT_HIVISION | SIS_SIMULTANEOUS_VIEW_ENABLE);
			else if( si.TV_plug == TVPLUG_SVIDEO )
				cr30 = (SIS_VB_OUTPUT_SVIDEO | SIS_SIMULTANEOUS_VIEW_ENABLE);
			else if( si.TV_plug == TVPLUG_COMPOSITE )
				cr30 = (SIS_VB_OUTPUT_COMPOSITE | SIS_SIMULTANEOUS_VIEW_ENABLE);
			else if( si.TV_plug == TVPLUG_SCART )
				cr30 = (SIS_VB_OUTPUT_SCART | SIS_SIMULTANEOUS_VIEW_ENABLE);
			cr31 |= SIS_DRIVER_MODE;

			if( si.TV_type == TVMODE_PAL )
				cr31 |= 0x01;
			else
				cr31 &= ~0x01;
		break;
		default:	/* CRT2 disable */
			cr30 = 0x00;
			cr31 |= (SIS_DRIVER_MODE | SIS_VB_OUTPUT_DISABLE);
	}

	outSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR30, cr30 );
	outSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR31, cr31 );
	outSISIDXREG( SISCR, IND_SIS_SCRATCH_REG_CR33, (rate_idx & 0x0F) );

	if( si.vga_engine == SIS_315_VGA )
		SiS310Idle
	else
		SiS300Idle


	SiS_Pr.SiS_UseOEM = -1;
}

void sis_post_setmode( int bpp, int xres )
{
	uint8 reg;
	BOOLEAN doit = TRUE;	

	/* TW: We can't switch off CRT1 on LVDS/Chrontel in 8bpp Modes */
	if ((si.hasVB == HASVB_LVDS) || (si.hasVB == HASVB_LVDS_CHRONTEL)) {
		if (bpp == 8) {
			doit = FALSE;
		}
	}

	/* TW: We can't switch off CRT1 on 630+301B in 8bpp Modes */
	if ( (sishw_ext.ujVBChipID == VB_CHIP_301B) && (si.vga_engine == SIS_300_VGA) &&
	     (si.disp_state & DISPTYPE_LCD) ) {
	        if (bpp == 8) {
				doit = FALSE;
	        }
	}

	/* TW: We can't switch off CRT1 if bridge is in slave mode */
	if(si.hasVB != HASVB_NONE) {
		inSISIDXREG( SISPART1, 0x00, reg );
		if( si.vga_engine == SIS_300_VGA ) {
			if( (reg & 0xa0) == 0x20 ) {
				doit = FALSE;
			}
		}
		if( si.vga_engine == SIS_315_VGA ) {
			if( (reg & 0x50) == 0x10 ) {
				doit = FALSE;
			}
		}
	}

	inSISIDXREG( SISCR, 0x17, reg );      
	reg |= 0x80;
	outSISIDXREG( SISCR, 0x17, reg );
	
	andSISIDXREG( SISSR, IND_SIS_RAMDAC_CONTROL, ~0x04 );

	if( (si.disp_state & DISPTYPE_TV) && (si.hasVB == HASVB_301) ) {

		inSISIDXREG( SISPART4, 0x01, reg );

		if( reg < 0xB0 ) {        	/* Set filter for SiS301 */

		switch( xres ) {
			case 320:
				si.filter_tb = (si.TV_type == TVMODE_NTSC) ? 4 : 12;
			break;
			case 640:
				si.filter_tb = (si.TV_type == TVMODE_NTSC) ? 5 : 13;
			break;
			case 720:
				si.filter_tb = (si.TV_type == TVMODE_NTSC) ? 6 : 14;
			break;
			case 800:
				si.filter_tb = (si.TV_type == TVMODE_NTSC) ? 7 : 15;
			break;
			default:
				si.filter = -1;
			break;
		}

		orSISIDXREG( SISPART1, si.CRT2_enable, 0x01 );

		if( si.TV_type == TVMODE_NTSC ) {

	        andSISIDXREG( SISPART2, 0x3a, 0x1f );

			if( si.TV_plug == TVPLUG_SVIDEO ) {

				andSISIDXREG(SISPART2, 0x30, 0xdf);

			} else if( si.TV_plug == TVPLUG_COMPOSITE ) {

				orSISIDXREG( SISPART2, 0x30, 0x20 );

				switch ( xres ) {
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
				case 800:
					outSISIDXREG(SISPART2, 0x35, 0xEB);
					outSISIDXREG(SISPART2, 0x36, 0x15);
					outSISIDXREG(SISPART2, 0x37, 0x25);
					outSISIDXREG(SISPART2, 0x38, 0xF6);
					break;
				}
			}

		} else if( si.TV_type == TVMODE_PAL ) {

			andSISIDXREG(SISPART2, 0x3A, 0x1F);

			if (si.TV_plug == TVPLUG_SVIDEO) {

			        andSISIDXREG(SISPART2, 0x30, 0xDF);

			} else if (si.TV_plug == TVPLUG_COMPOSITE) {

			        orSISIDXREG(SISPART2, 0x30, 0x20);

				switch (xres) {
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
				case 800:
					outSISIDXREG(SISPART2, 0x35, 0xFC);
					outSISIDXREG(SISPART2, 0x36, 0xFB);
					outSISIDXREG(SISPART2, 0x37, 0x14);
					outSISIDXREG(SISPART2, 0x38, 0x2A);
					break;
				}
			}
		}

		if ((si.filter >= 0) && (si.filter <=7)) {
			dbprintf("FilterTable[%d]-%d: %02x %02x %02x %02x\n", si.filter_tb, si.filter, 
				sis_TV_filter[si.filter_tb].filter[si.filter][0],
				sis_TV_filter[si.filter_tb].filter[si.filter][1],
				sis_TV_filter[si.filter_tb].filter[si.filter][2],
				sis_TV_filter[si.filter_tb].filter[si.filter][3]
			);
			outSISIDXREG(SISPART2, 0x35, (sis_TV_filter[si.filter_tb].filter[si.filter][0]));
			outSISIDXREG(SISPART2, 0x36, (sis_TV_filter[si.filter_tb].filter[si.filter][1]));
			outSISIDXREG(SISPART2, 0x37, (sis_TV_filter[si.filter_tb].filter[si.filter][2]));
			outSISIDXREG(SISPART2, 0x38, (sis_TV_filter[si.filter_tb].filter[si.filter][3]));
		}

	     }
	  
	}

}


char *sis_find_rom()
{
        uint32  segstart;
        unsigned char *rom_base;
        unsigned char *rom;
        int  stage;
        int  i;
        char sis_rom_sig[] = "Silicon Integrated Systems";
        char *sis_sig_300[4] = {
          "300", "540", "630", "730"
        };
        char *sis_sig_310[9] = {
          "315", "315", "315", "5315", "6325", "6325", "Xabre", "6330", "6330"
        };
	ushort sis_nums_300[4] = {
	  SIS_300, SIS_540, SIS_630, SIS_730
	};
	unsigned short sis_nums_310[9] = {
	  SIS_315PRO, SIS_315H, SIS_315, SIS_550, SIS_650, SIS_740, SIS_330, SIS_660, SIS_760
	};

        for(segstart=0x00000000; segstart<0x00030000; segstart+=0x00001000) {

                stage = 1;

                rom_base = (char *)si.rom + segstart;

                if ((*rom_base == 0x55) && (((*(rom_base + 1)) & 0xff) == 0xaa))
                   stage = 2;

                if (stage != 2) {
                   continue;
                }


		rom = rom_base + (unsigned short)(*(rom_base + 0x12) | (*(rom_base + 0x13) << 8));
                if(strncmp(sis_rom_sig, rom, strlen(sis_rom_sig)) == 0) {
                    stage = 3;
		}
                if(stage != 3) {
                    continue;
                }

		rom = rom_base + (unsigned short)(*(rom_base + 0x14) | (*(rom_base + 0x15) << 8));
                for(i = 0;(i < 4) && (stage != 4); i++) {
                    if(strncmp(sis_sig_300[i], rom, strlen(sis_sig_300[i])) == 0) {
                        if(sis_nums_300[i] == si.chip) {
			   stage = 4;
                           break;
			}
                    }
                }
		if(stage != 4) {
                   for(i = 0;(i < 9) && (stage != 4); i++) {
                      if(strncmp(sis_sig_310[i], rom, strlen(sis_sig_310[i])) == 0) {
		          if(sis_nums_310[i] == si.chip) {
                             stage = 4;
                             break;
			  }
                      }
                   }
		}

                if(stage != 4) {
                        continue;
                }

                return rom_base;
        }
        return NULL;
}


int sis_init()
{
	uint8 reg;
	uint32 reg32;
	
	outb( 0x77, 0x80 );
	
	/* Check what chip we have ...*/
	switch( si.device_id ) {
		case 0x300: /* SIS 300 */
			si.chip = SIS_300;
			si.vga_engine = SIS_300_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_300;
			strcpy( si.name, "330" );
		break; 
		case 0x630: /* SIS 630/730 */
			sis_set_reg4( 0xCF8, 0x80000000 );
			reg32 = sis_get_reg3( 0xCFC );
			if( reg32 == 0x07301039 ) {
				si.chip = SIS_730;
				strcpy( si.name, "730" );
			}
			else {
				si.chip = SIS_630;
				strcpy( si.name, "630" );
			}
			si.vga_engine = SIS_300_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_300;
		break;
		case 0x5300: /* SIS 540 VGA */
			si.chip = SIS_540;
			si.vga_engine = SIS_300_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_300;
			strcpy( si.name, "540" );
		break;
		case 0x310: /* SIS 315H */
			si.chip = SIS_315H;
			si.vga_engine = SIS_315_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "315H" );
		break; 
		case 0x315: /* SIS 315 */
			si.chip = SIS_315H;
			si.vga_engine = SIS_315_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "315" );
		break; 
		case 0x325: /* SIS 315 PRO */
			si.chip = SIS_315H;
			si.vga_engine = SIS_315_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "315 PRO" );
		break; 
		case 0x6325: /* SIS 650/740 */
			si.chip = SIS_650;
			sis_set_reg4( 0xCF8, 0x80000000 );
			reg32 = sis_get_reg3( 0xCFC );
			if( reg32 == 0x07401039 ) {
				si.chip = SIS_740;
				strcpy( si.name, "740" );
			}
			else {
				si.chip = SIS_650;
				strcpy( si.name, "650" );
			}
			si.vga_engine = SIS_315_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
		break;
		case 0x330: /* SIS 330 ( Xabre ) */
			si.chip = SIS_330;
			si.vga_engine = SIS_315_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
			strcpy( si.name, "Xabre" );
		break;
		case 0x6330: /* SIS 660/760 */
			reg32 = sis_get_reg3( 0xCFC );
			if( reg32 == 0x07601039 ) {
				si.chip = SIS_760;
				strcpy( si.name, "760" );
			}
			else {
				si.chip = SIS_660;
				strcpy( si.name, "660" );
			}
			si.vga_engine = SIS_315_VGA;
			si.CRT2_enable = IND_SIS_CRT2_WRITE_ENABLE_315;
		break;
		default:
			return -1;
	}
	dbprintf( "SiS %s found\n", si.name );
	
	/* Initialize BIOS emulation */
	sishw_ext.jChipType = si.chip;
	sishw_ext.ulIOAddress = SiS_Pr.RelIO = si.io_base;
	sishw_ext.ulIOAddress += 0x30;
	SiS_Pr.SiS_Backup70xx = 0xff;
	SiS_Pr.SiS_CHOverScan = -1;
	SiS_Pr.SiS_ChSW = FALSE;
	SiSRegInit(&SiS_Pr, (USHORT)sishw_ext.ulIOAddress);
	
	/* Unlock hardware */
	outSISIDXREG( SISSR, IND_SIS_PASSWORD, SIS_PASSWORD );
	
	if( si.vga_engine == SIS_315_VGA ) {
		switch( si.chip ) {
			case SIS_315H:
			case SIS_315:
			case SIS_330:
				sishw_ext.bIntegratedMMEnabled = TRUE;
			break;
			case SIS_550:
			case SIS_650:
			case SIS_740:
			case SIS_660:
			case SIS_760:
				sishw_ext.bIntegratedMMEnabled = TRUE;
			break;
			default:
			break;
		}
	} else if( si.vga_engine == SIS_300_VGA ) {
		if( si.chip == SIS_300 ) {
			sishw_ext.bIntegratedMMEnabled = TRUE;
		} else {
			inSISIDXREG( SISSR, IND_SIS_SCRATCH_REG_1A, reg );
			if (reg & SIS_SCRATCH_REG_1A_MASK)
				sishw_ext.bIntegratedMMEnabled = TRUE;
			else
				sishw_ext.bIntegratedMMEnabled = FALSE;
		}
	}
	sishw_ext.pDevice = NULL;
	
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
	sishw_ext.pjCustomizedROMImage = NULL;
	sishw_ext.bSkipDramSizing = 0;
	sishw_ext.pQueryVGAConfigSpace = &sis_query_config_space;
	sishw_ext.pQueryNorthBridgeSpace = &sis_query_config_space;
	strcpy( sishw_ext.szVBIOSVer, "0.84" );

	/* TW: Mode numbers for 1280x960 are different for 300 and 310/325 series */
	if( si.vga_engine == SIS_300_VGA ) {
		sisbios_mode[MODEINDEX_1280x960].mode_no = 0x6e;
		sisbios_mode[MODEINDEX_1280x960+1].mode_no = 0x6f;
		sisbios_mode[MODEINDEX_1280x960+2].mode_no = 0x7b;
		sisbios_mode[MODEINDEX_1280x960+3].mode_no = 0x7b;
	}
	sishw_ext.pSR = (PSIS_DSReg)malloc( sizeof( SIS_DSReg ) * SR_BUFFER_SIZE );
	if( sishw_ext.pSR == NULL )
		return -1;
	sishw_ext.pSR[0].jIdx = sishw_ext.pSR[0].jVal = 0xFF;
	sishw_ext.pCR = (PSIS_DSReg)malloc( sizeof( SIS_DSReg ) * CR_BUFFER_SIZE );
	if( sishw_ext.pCR == NULL )
		return -1;
	sishw_ext.pCR[0].jIdx = sishw_ext.pCR[0].jVal = 0xFF;
	
	/* Calculate RAM size */
	if( si.vga_engine == SIS_300_VGA ) {
		if( sis_get_dram_size_300() ) {
			free( sishw_ext.pSR );
			free( sishw_ext.pCR );
			dbprintf( "Unable to determine DRAM size !\n" );
			return -1;
		}
	}
	if( si.vga_engine == SIS_315_VGA ) {
		if( sis_get_dram_size_315() ) {
			free( sishw_ext.pSR );
			free( sishw_ext.pCR );
			dbprintf( "Unable to determine DRAM size !\n" );
			return -1;
		}
	}
	dbprintf( "%i Mb RAM\n", (uint)( si.mem_size / 1024 / 1024 ) );
	
	/* Enable PCI_LINEAR_ADDRESSING and MMIO_ENABLE  */
	orSISIDXREG( SISSR, IND_SIS_PCI_ADDRESS_SET, ( SIS_PCI_ADDR_ENABLE | SIS_MEM_MAP_IO_ENABLE ) );

	/* Enable 2D accelerator engine */
	orSISIDXREG( SISSR, IND_SIS_MODULE_ENABLE, SIS_ENABLE_2D );
	
	sishw_ext.ulVideoMemorySize = si.mem_size;
	sishw_ext.pjVideoMemoryAddress = si.framebuffer;
	
	/* Initialize command queue */
	if( si.vga_engine == SIS_315_VGA ) {
		sis_enable_queue_315();
	}
	if( si.vga_engine == SIS_300_VGA ) {
		sis_enable_queue_300();
	}
	/* Initialize video bridge */
	if( si.vga_engine == SIS_315_VGA ) {
		sis_get_VB_type_315();
	}
	if( si.vga_engine == SIS_300_VGA ) {
		sis_get_VB_type_300();
	}
	
	sishw_ext.ujVBChipID = VB_CHIP_UNKNOWN;
	sishw_ext.usExternalChip = 0;

	switch( si.hasVB ) {
		case HASVB_301:
			inSISIDXREG( SISPART4, 0x01, reg );
			if( reg >= 0xE0 ) {
				sishw_ext.ujVBChipID = VB_CHIP_302LV;
				dbprintf("SiS302LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if( reg >= 0xD0 ) {
				sishw_ext.ujVBChipID = VB_CHIP_301LV;
				dbprintf("SiS301LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if( reg >= 0xB0 ) {
				sishw_ext.ujVBChipID = VB_CHIP_301B;
				dbprintf("SiS301B bridge detected (revision 0x%02x\n",reg);
			} else {
				sishw_ext.ujVBChipID = VB_CHIP_301;
				dbprintf("SiS301 bridge detected\n");
			}
			break;
		case HASVB_302:
			inSISIDXREG( SISPART4, 0x01, reg );
			if( reg >= 0xE0 ) {
				sishw_ext.ujVBChipID = VB_CHIP_302LV;
				dbprintf("SiS302LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if( reg >= 0xD0 ) {
				sishw_ext.ujVBChipID = VB_CHIP_301LV;
				dbprintf("SiS302LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if( reg >= 0xB0 ) {
				sishw_ext.ujVBChipID = VB_CHIP_302B;
				dbprintf("SiS302B bridge detected (revision 0x%02x)\n",reg);
			} else {
				sishw_ext.ujVBChipID = VB_CHIP_302;
				dbprintf("SiS302 bridge detected\n");
			}
			break;
		case HASVB_LVDS:
			sishw_ext.usExternalChip = 0x1;
			dbprintf("LVDS transmitter detected\n");
			break;
		case HASVB_TRUMPION:
			sishw_ext.usExternalChip = 0x2;
			dbprintf("Trumpion Zurac LVDS scaler detected\n");
			break;
		case HASVB_CHRONTEL:
			sishw_ext.usExternalChip = 0x4;
			dbprintf("Chrontel TV encoder detected\n");
			break;
		case HASVB_LVDS_CHRONTEL:
			sishw_ext.usExternalChip = 0x5;
			dbprintf("LVDS transmitter and Chrontel TV encoder detected\n");
			break;
		default:
			dbprintf("No or unknown bridge type detected\n");
			break;
	}

	if( si.hasVB != HASVB_NONE ) {
		if( si.vga_engine == SIS_300_VGA ) {
			sis_detect_VB_connect_300();
		}
		if( si.vga_engine == SIS_315_VGA ) {
			sis_detect_VB_connect_315();
		}
	}

	if( si.disp_state & DISPTYPE_DISP2 ) {
		si.disp_state |= (DISPMODE_MIRROR | DISPTYPE_CRT1);
	} else {
		si.disp_state = DISPMODE_SINGLE | DISPTYPE_CRT1;
	}
	
	if( si.disp_state & DISPTYPE_LCD ) {
		inSISIDXREG( SISCR, IND_SIS_LCD_PANEL, reg );
		if( si.vga_engine == SIS_300_VGA ) {
 			si.LCDheight = SiS300_LCD_Type[(reg & 0x0f)].LCDheight;
	    	si.LCDwidth = SiS300_LCD_Type[(reg & 0x0f)].LCDwidth;
			sishw_ext.ulCRT2LCDType = SiS300_LCD_Type[(reg & 0x0f)].LCDtype;
		} else {
			si.LCDheight = SiS310_LCD_Type[(reg & 0x0f)].LCDheight;
	    	si.LCDwidth = SiS310_LCD_Type[(reg & 0x0f)].LCDwidth;	
			sishw_ext.ulCRT2LCDType = SiS310_LCD_Type[(reg & 0x0f)].LCDtype;
		}
	}
	/* Save the current PanelDelayCompensation if the LCD is currently used */
	if( si.vga_engine == SIS_300_VGA) {
		if( (sishw_ext.usExternalChip == 0x01) ||   /* LVDS */
			(sishw_ext.usExternalChip == 0x05) ||   /* LVDS+Chrontel */
			(sishw_ext.ujVBChipID == VB_CHIP_301B) ) {
				int tmp;
				int pdc = 0;
				inSISIDXREG( SISCR, 0x30, tmp );
				if( tmp & 0x20 ) {
					/* Currently on LCD? If yes, read current pdc */	
					inSISIDXREG( SISPART1, 0x13, pdc );
					pdc &= 0x3c;
					sishw_ext.pdc = pdc;
					dbprintf("Detected LCD PanelDelayCompensation %d\n", pdc);
				}
		}
	}
	return 0;
}

















