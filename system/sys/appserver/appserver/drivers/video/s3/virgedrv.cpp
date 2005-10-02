/*
----------------------
Be Sample Code License
----------------------

Copyright 1991-1999, Be Incorporated.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions, and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions, and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    
*/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include <appserver/pci_graphics.h>
#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <atheos/pci.h>
#include "virge.h"

#include <gui/bitmap.h>

// Values to set we need to set into the 28 previous registers to get the
// selected CRT_CONTROL settings.
static 	uchar     settings[28];

static sem_id	io_sem = -1;
static sem_id   ge_sem = -1;
static atomic_t io_lock, ge_lock;

clone_info sCardInfo;

// Create the benaphores (if possible)
static void init_locks()
{
    if (io_sem == -1)
    {
	atomic_set( &io_lock, 0 );
	io_sem = create_semaphore( "vga io sem", 0, 0 );
	atomic_set( &ge_lock, 0 );
	ge_sem = create_semaphore( "vga ge sem", 0, 0 );
    }
	
}

// Protect the access to the general io-registers by locking a benaphore.
void lock_io()
{
    int	old;

    old = atomic_inc_and_read( &io_lock );
    if (old >= 1) {
	lock_semaphore(io_sem);
    }
}

// Release the protection on the general io-registers
void unlock_io()
{
    if ( !atomic_dec_and_test( &io_lock ) ) {
	unlock_semaphore(io_sem);
    }
}	

// Protect the access to the memory or the graphic engine registers by
// locking a benaphore.
void lock_ge()
{
    int	old;

    old = atomic_inc_and_read( &ge_lock );
    if (old >= 1) {
	lock_semaphore(ge_sem);
    }
}

// Release the protection on the memory and the graphic engine registers.
void unlock_ge()
{
    if ( !atomic_dec_and_test( &ge_lock ) ) {
	unlock_semaphore( ge_sem );
    }
}	


// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// This table is used to set the vga gcr registers.
// The first value (16 bits) is the adress of the register in the 64K io-space,
// the second (only one significative byte) is the value we set.
// Two pairs are put on the same line, because those registers use indirect
// access (one register with many meanings, indexed by another register). The
// index always comes first.
static uint16 g_anGcrTable[] =
{
    GCR_INDEX, 0x00, GCR_DATA, 0x00,	// Set/Reset
    GCR_INDEX, 0x01, GCR_DATA, 0x00,	// Enable Set/Reset
    GCR_INDEX, 0x02, GCR_DATA, 0x00,	// Color Compare
    GCR_INDEX, 0x03, GCR_DATA, 0x00,	// Raster Operations/Rotate Counter
    GCR_INDEX, 0x04, GCR_DATA, 0x00,	// Read Plane Select
    GCR_INDEX, 0x05, GCR_DATA, 0x40,	// Graphics Controller
    GCR_INDEX, 0x06, GCR_DATA, 0x05,	// Memory Map Mode
    GCR_INDEX, 0x07, GCR_DATA, 0x0f,	// Color Don't care
    GCR_INDEX, 0x08, GCR_DATA, 0xff,	// Bit Mask
    0x000,     0x00			// End-of-table
};

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// This table is used to set the standard vga sequencer registers.
// The first value (16 bits) is the adress of the register in the 64K io-space,
// the second (only one significative byte) is the value we set.
// Two pairs are put on the same line, because those registers use indirect
// access (one register with many meanings, indexed by another register). The
// index always comes first.
static uint16 g_anSequencerTable[] =
{
    SEQ_INDEX, 0x00, SEQ_DATA, 0x03,  // Reset
    SEQ_INDEX, 0x01, SEQ_DATA, 0x01,  // Clocking mode
    SEQ_INDEX, 0x02, SEQ_DATA, 0x0f,  // Enable write plane
    SEQ_INDEX, 0x03, SEQ_DATA, 0x00,  // Char Font Select
    SEQ_INDEX, 0x04, SEQ_DATA, 0x06,  // Memory Mode
    0x000,     0x00                   // End-of-table
};

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// This table contains the settings for vga attribute registers.
// The first 16 values are useless, but at least completly safe.
// The first value is the index of the attribute register (as described in
// every databook), the second the value we set for the register.
static uint16 g_anAttributeTable[] =
{
    0x00,	0x00,
    0x01,	0x01,
    0x02,	0x02,
    0x03,	0x03,
    0x04,	0x04,
    0x05,	0x05,
    0x06,	0x14,
    0x07,	0x07,
    0x08,	0x38,
    0x09,	0x39,
    0x0a,	0x3a,
    0x0b,	0x3b,
    0x0c,	0x3c,
    0x0d,	0x3d,
    0x0e,	0x3e,
    0x0f,	0x3f,
    0x10,	0x41,
    0x11,	0x00,
    0x12,	0x0f,
    0x13,	0x00,
    0x14,	0x00,
    0xff,	0xff   // End-of-table
};

static uint16 g_anVirgeTable[] =
{
    SEQ_INDEX,  0x09, SEQ_DATA,  0x80,   // IO map disable
    SEQ_INDEX,  0x0a, SEQ_DATA,  0xe0,   //0x40,   // External bus request (was 0x80/0xc0)
    SEQ_INDEX,  0x0d, SEQ_DATA,  0x00,   // Extended sequencer 1
    SEQ_INDEX,  0x14, SEQ_DATA,  0x00,   // CLKSYN
    SEQ_INDEX,  0x18, SEQ_DATA,  0x00,   // RAMDAC/CLKSYN

    CRTC_INDEX, 0x11, CRTC_DATA, 0x0c,	// CRTC regs 0-7 unlocked, v_ret_end = 0x0c
    CRTC_INDEX, 0x31, CRTC_DATA, 0x0c,	//0x08,//0x8c,	// Memory config (0x08)
    CRTC_INDEX, 0x32, CRTC_DATA, 0x00,	// all interrupt disenabled
    CRTC_INDEX, 0x33, CRTC_DATA, 0x22,	// Backward Compatibility 2
    CRTC_INDEX, 0x34, CRTC_DATA, 0x00,	// Backward compatibility 3
    CRTC_INDEX, 0x35, CRTC_DATA, 0x00,	// CRT Register Lock

    CRTC_INDEX, 0x37, CRTC_DATA, 0xff,    // want 0x19, bios has 0xff : bit 1 is magic :-(
      // Trio32/64 databook says bit 1 is 0 for test, 1 for
      // normal operation.  Must be a documentation error?
  
    CRTC_INDEX, 0x66, CRTC_DATA, 0x01, 	// Extended Miscellaneous Control 1 (enable)
    CRTC_INDEX, 0x3a, CRTC_DATA, 0x15,	// Miscellaneous 1

    CRTC_INDEX, 0x42, CRTC_DATA, 0x00,	// Mode Control (non interlace)
    CRTC_INDEX, 0x43, CRTC_DATA, 0x00,	// Extended Mode
    CRTC_INDEX, 0x45, CRTC_DATA, 0x00,	// Hardware Graphics Cursor Mode

    CRTC_INDEX, 0x53, CRTC_DATA, 0x08,	// Extended Memory Cont 1
    CRTC_INDEX, 0x54, CRTC_DATA, 0x02,	// Extended Memory Cont 2
    CRTC_INDEX, 0x55, CRTC_DATA, 0x00,	// Extended DAC Control
    CRTC_INDEX, 0x56, CRTC_DATA, 0x00,	// External Sync Cont 1
    CRTC_INDEX, 0x5c, CRTC_DATA, 0x00, 	// General Out Port
    CRTC_INDEX, 0x61, CRTC_DATA, 0x00,	// Extended Memory Cont 4 pg 18-23
    CRTC_INDEX, 0x63, CRTC_DATA, 0x00,	// External Sync Delay Adjust High
    CRTC_INDEX, 0x65, CRTC_DATA, 0x00, 	// Extended Miscellaneous Control 0x24
    CRTC_INDEX, 0x6a, CRTC_DATA, 0x00,	// Extended System Control 4
    CRTC_INDEX, 0x6b, CRTC_DATA, 0x00,	// Extended BIOS flags 3
    CRTC_INDEX, 0x6c, CRTC_DATA, 0x00,	// Extended BIOS flags 4
    0x000,      0x00			// End-of-table
};



#define get_pci(o, s) 	 pci_gfx_read_config(sCardInfo.nFd, sCardInfo.pcii.nBus, sCardInfo.pcii.nDevice, sCardInfo.pcii.nFunction, (o), (s))
#define set_pci(o, s, v) pci_gfx_write_config(sCardInfo.nFd, sCardInfo.pcii.nBus, sCardInfo.pcii.nDevice, sCardInfo.pcii.nFunction, (o), (s), (v))

//#define isa_inb(a)	read_isa_io (0, (char *)(a) - 0x00008000, 1)
//#define isa_outb(a, b)	write_isa_io (0, (char *)(a) - 0x00008000, 1, (b))

#define isa_inb(a)	inb_p( (a) - 0x00008000 )
#define isa_outb(a, b)	outb_p( (b), (a) - 0x00008000 )

void lock_io();
void unlock_io();
void lock_ge();
void unlock_ge();

#define v_inb(a)	*((volatile uint8_t *)(sCardInfo.base0 + (a)))
#define v_outb(a, b)	*((volatile uint8_t *)(sCardInfo.base0 + (a))) = (b)
#define v_inl(a)	*((volatile uint32_t *)(sCardInfo.base0 + (a)))
#define v_outl(a, l)	*((volatile uint32_t *)(sCardInfo.base0 + (a))) = (l)

static area_id	g_nFrameBufArea = -1;

// Wait for n*0x100 empty FIFO slots.



static void set_table( uint16* ptr )
{
    uint16 p1;
    uint16 p2;

    for(;;) {
	p1 = *ptr++;
	p2 = *ptr++;
	if (p1 == 0 && p2 == 0) {
	    return;
	}
	v_outb(p1, p2);
    }
}

static void set_attr_table( uint16* ptr )
{
    ushort p1;
    ushort p2;
    uchar	 v;
    uchar	 t;

// ATTR_REG is the only register used at the same time as index and data register.
// So we've to be careful to stay synchronize (the card switch between index
// and data at each write. Reading INPUT_STATUS_1 give us a simple way to stay well-
// synchronized.
    dprintf("set_attr_table() begins\n");
    t = v_inb(INPUT_STATUS_1);
// Save the initial index set in ATTR_REG
    v = v_inb(ATTR_REG);

    for ( ;; )
    {
	p1 = *ptr++;
	p2 = *ptr++;
	if (p1 == 0xff && p2 == 0xff) {
	    t = v_inb(INPUT_STATUS_1);
	      // Restore the initial index set in ATTR_REG
	    v_outb(ATTR_REG, v | 0x20);
	    dprintf("set_attr_table() ends\n");
	    return;
	}
	t = v_inb(INPUT_STATUS_1);
	v_outb(ATTR_REG, p1);
	v_outb(ATTR_REG, p2);
    }
}

static void wait_for_fifo(long value) {
    if ( (int)(v_inl(SUBSYS_STAT) & 0x1f00) < value) {
	  //do delay(40);
	while ( (int)(v_inl(SUBSYS_STAT) & 0x1f00) < value);
    }
}

void wait_for_blitter() {
    if ((v_inl(SUBSYS_STAT) & 0x2000) == 0) {
	while ((v_inl(SUBSYS_STAT) & 0x2000) == 0);
    }
}


// Convert a clock frenquency in Mhz into the trio clock generator format (see
// the trio32/64 databook for more information). Those clock generator provide
// Fpll = (1/2^R)*Fref*(M+2)/(N+2) with M described by 7 bits, N by 5 bits and
// R by 2 bits.
long trio_encrypt_clock(float freq)
{
    float    fr,fr0,fr2; //, orig = freq;
    long     m,n,m2=0,n2=0,r;

// Fpll*2^R has to be > 135.0 Mhz.
    r = 0;
    while (freq < 135.0) {
	freq *= 2.0;
	r++;
    }

// Look for the best choice in all the allowed (M,N) couples (see trio32/64
// databook for specific conditions).
    fr0 = freq*(1.0/TI_REF_FREQ);
    fr2 = -2.0;
    for (n=1;n<=31;n++) {
	  // Check all allowed values of N, calculate the best choice for M
	m = (int)(fr0*(float)(n+2)-1.5);
	fr = ((float)(m+2))/((float)(n+2));
	  // Keep the most accurate choice.
	if (((fr0-fr)*(fr0-fr) < (fr0-fr2)*(fr0-fr2)) && (m >= 1) && (m <= 127)) {
	    m2 = m;
	    n2 = n;
	    fr2 = fr;
	}
    }

// Return the 3 parameters packed into a 15 bits word.
    return ((r<<13)+(n2<<8)+m2);
}

// Send a 32 bits configuration word to the trio DAC to program the frequency
// of the two clock generators (see trio32/64 databook for more informations).
// Each clock settings is packed into 2 bytes, using each 7 bits.
void trio_set_clock(ulong setup)
{
    dprintf("trio_set_clock(0x%08lx)\n", setup);
// Program the memory clock frenquency
    v_outb(SEQ_INDEX, 0x15);
    v_outb(SEQ_DATA, 0x00);

    v_outb(SEQ_INDEX, 0x10);
    v_outb(SEQ_DATA, (setup>>24)&127);
    v_outb(SEQ_INDEX, 0x11);
    v_outb(SEQ_DATA, (setup>>16)&127);
	
    v_outb(SEQ_INDEX, 0x15);
    v_outb(SEQ_DATA, 0x01);

// Program main video clock frenquency
    v_outb(SEQ_INDEX, 0x15);
    v_outb(SEQ_DATA, 0x00);

    v_outb(SEQ_INDEX, 0x12);
    v_outb(SEQ_DATA, (setup>>8)&127);
    v_outb(SEQ_INDEX, 0x13);
    v_outb(SEQ_DATA, setup&127);
	
    v_outb(SEQ_INDEX, 0x15);
    v_outb(SEQ_DATA, 0x02);

// All done
    v_outb(SEQ_INDEX, 0x15);
    v_outb(SEQ_DATA, 0x80);

    dprintf("trio_set_clock(0x%08lx) finished\n", setup);
}

//**************************************************************************
//  Clock generator global setting
//
//  For each DAC available, program the good memory and video clock frenquency
//  depending of the space (resolution and depth), the refresh rate selected
//  and the crt settings when CRT_CONTROL is available.
//**************************************************************************

static void write_pll()
{
    ulong setup;
    float clock;

      // This table is used to calculate the master video frenquency needed to get
      // a selected refresh rate. First line is 8 bits spaces, second 16 bits
      // spaces, third 32 bits spaces. Resolutions are columns 1 to 5 : 640x480,
      // 800x600, 1024x768, 1280x1024 and 1600x1200 (see GraphicsCard.h for space
      // definition).
      // 
      // See att498_ScreenClockStep for more informations and example of how
      // calculate those values.
    static float trio_ScreenClockStep[32] =
    {
	8.0, 8.0, 8.0, 8.0, 8.0,
	8.0, 8.0, 8.0, 8.0, 8.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	8.0, 8.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0,
    };
  
    dprintf("write_pll()\n");	
// Set the memory clock to the standard frequency, 60 Mhz.
    setup = trio_encrypt_clock(56.0);

// As the trio DAC uses CRT_CONTROL, the clock frequency depends of the
// horizntal crt counter increment multiplied by the horizontal and vertical
// sizes of the frame (as described by crt registers), multiplied by the
// selected refresh rate.

//  int pixel_per_unit = 64 / sCardInfo.scrnColors;
//  (uint32)(sCardInfo.lastCrtHT * sCardInfo.lastCrtVT * pixel_per_unit * (sCardInfo.scrnRate / 1000.0));
  
    clock = trio_ScreenClockStep[sCardInfo.scrnResNum] * sCardInfo.scrnRate *
	(float)sCardInfo.lastCrtHT * (float)sCardInfo.lastCrtVT;
    clock = sCardInfo.dot_clock * 1000;
	
// Packed definition of both clock settings (memory and video)
    dprintf("clocks: %f, %f\n", clock, clock * 1e-6);
    setup = (setup<<16) + trio_encrypt_clock( clock * 1e-6 );
    dprintf("about to trio_set_clock(0x%08lx)\n", setup);
    trio_set_clock(setup);
    dprintf("write_pll() finished\n");	
}

// Calculate the settings of the 28 registers to get the positions and sizes
// described in sCardInfo.crtPosH, sCardInfo.crtPosV, sCardInfo.crtSizeH and
// sCardInfo.crtSizeV (value from 0 to 100).

static void prepare_crt (long pixel_per_unit)
{
    int       i,par1,par2,par3;
    uchar     Double;
    ulong     Eset[2];
    ulong     Hset[7],Vset[6];

    dprintf("prepare_crt(%ld)\n", pixel_per_unit);

// Calculate the value of the 7 basic horizontal crt parameters depending of
// sCardInfo.crtPosH and sCardInfo.crtSizeH. Those 7 parameters are (in the same order as refered
// in crt_864_640x480 for example) :
//   - 0 : Horizontal total number of clock character per line.
//   - 1 : Horizontal number of clock character in the display part of a line.
//   - 2 : Start of horizontal blanking.
//   - 3 : End of horizontal blanking.
//   - 4 : Start of horizontal synchro position.
//   - 5 : End of horizontal synchro position.
//   - 6 : fifo filling optimization parameter
    for (;;) {
	Hset[1] = sCardInfo.scrnWidth / pixel_per_unit;
	Hset[0] = (Hset[1]*(700-sCardInfo.crtSizeH)+250)/500;
	par1 = (3*Hset[1]+100)/200;
	if (par1 < 2) par1 = 2;
	Hset[2] = Hset[1]+par1;
	Hset[3] = Hset[0]-par1;
	i = Hset[3]-Hset[2]-127;
	if (i>0) {
	    Hset[3] -= i/2;
	    Hset[2] += (i+1)/2;
	}

	par3 = (Hset[1]+5)/10;
	par2 = Hset[0]-Hset[1]-2*par1-par3;
	if (par2 < 0) {
	    par3 += par2; 
	    par2 = 0;
	}
	Hset[4] = Hset[2]+(par2*(100-sCardInfo.crtPosH)+50)/100;
	Hset[5] = Hset[4]+par3;
	if (par3>63) {
	    Hset[5] -= (par3-63)/2;
	    Hset[4] += (par3-62)/2;
	}

	Hset[6] = Hset[0] - 5;
		
	if (Hset[6] > 505) {
	    sCardInfo.crtSizeH++;
	    continue;
	}
	break;
    }
	
// Calculate the value of the 6 basic vertical crt parameters depending of
// sCardInfo.crtPosV and sCardInfo.crtSizeV. Those 6 parameters are (in the same order as refered
// in crt_864_640x480 for example) :
//   - 0 : Vertical total number of counted lines per frame
//   - 1 : Start of vertical blanking (line number).
//   - 2 : End of vertical blanking (line number).
//   - 3 : Vertical number of lines in the display part of a frame.
//   - 4 : Start of vertical synchro position (line number).
//   - 5 : End of vertical synchro position (line number).
    Vset[3] = sCardInfo.scrnHeight;
    Vset[0] = (Vset[3]*108+50)/100;
    par1 = (Vset[3]*5+250)/500;
    if (par1 < 2) par1 = 1;
    Vset[4] = Vset[3]+par1;
    Vset[5] = Vset[0]-par1;
    par3 = (Vset[3]+80)/160;
    if (par3 < 2) par3 = 2;
    par2 = Vset[0]-Vset[3]-2*par1-par3;
    if (par2 < 0) {
	par3 += par2; 
	par2 = 0;
    }
    Vset[1] = Vset[4]+((100-sCardInfo.crtPosV)*par2+50)/100;
    Vset[2] = Vset[1]+par3;

      // Calculate extended parameters (rowByte and base adress of the frameBuffer
    Eset[0] = ((sCardInfo.scrnPosH*sCardInfo.scrnColors)/8+sCardInfo.scrnPosV*sCardInfo.scrnRowByte)/4;
    Eset[1] = sCardInfo.scrnRowByte/8;
	
      // Memorize the last setting for horizontal and vertical total for use to set
      // the clock frequency (see write_pll for more informations).
    sCardInfo.lastCrtHT = Hset[0];
    sCardInfo.lastCrtVT = Vset[0];
    if (sCardInfo.lastCrtVT < 300) {
	Double = 0x80;
	sCardInfo.lastCrtVT *= 2;
    }
    else
	Double = 0x00;

    sCardInfo.dot_clock = (uint32)(sCardInfo.lastCrtHT * sCardInfo.lastCrtVT * pixel_per_unit * (sCardInfo.scrnRate / 1000.0));
      // 5 of those 15 parameters have to be adjust by a specific offset before to be
      // encoded in the appropriate registers.
    Hset[0] -= 5;
    Hset[1] -= 1;
    Vset[0] -= 2;
    Vset[3] -= 1;
    Vset[4] -= 1;

    dprintf("Htotal: %ld, Hdisp: %ld, Hstart-blank: %ld, Hend-blank: %ld, Hstart-sync: %ld, Hend-sync: %ld\n",
	    Hset[0], Hset[1], Hset[2], Hset[3], Hset[4], Hset[5]);
      // 2 of those registers are encoded in a bit complicated way (see databook for
      // more information)
    Hset[3] = (Hset[3] & 0x03f); //(Hset[3]&63)+((Hset[3]-Hset[2])&64);
    Hset[5] = (Hset[5] & 0x01f); //(Hset[5]&31)+((Hset[5]-Hset[4])&32);
    dprintf("Htotal: %ld, Hdisp: %ld, Hstart-blank: %ld, Hend-blank: %ld, Hstart-sync: %ld, Hend-sync: %ld\n",
	    Hset[0], Hset[1], Hset[2], Hset[3], Hset[4], Hset[5]);

      // Those 2 parameters need to be trunc respectively to 4 and 8 bits.
    Vset[2] = (Vset[2]&15);
    Vset[5] = (Vset[5]&255);

      // Those 12 parameters are encoded through the 28 8-bits registers. That code
      // is doing the conversion, even inserting a few more significant bit depending
      // of the space setting.
    settings[0] = Hset[0]&255;
    settings[1] = Hset[6]&255;
    settings[2] = Hset[1]&255;
    settings[3] = Hset[2]&255;
    settings[4] = Hset[3]&31; // | 128;
    settings[5] = Hset[4]&255;
    settings[6] = Hset[5]&31 | ((Hset[3]&32)<<2);
    settings[7] = Vset[0]&255;
    settings[8] = ((Vset[0]&256)>>8) | ((Vset[3]&256)>>7) |
	((Vset[1]&256)>>6) | 0x10 |
	((Vset[4]&256)>>5) | ((Vset[0]&512)>>4) |
	((Vset[3]&512)>>3) | ((Vset[1]&512)>>2);
    settings[9] = Double | 0x40 | ((Vset[4]&512)>>4);
    settings[10] = Vset[1]&255;
    settings[11] = Vset[2]&15;
    settings[12] = Vset[3]&255;
    settings[13] = Vset[4]&255;
    settings[14] = Vset[5]&255;
    settings[15] = ((Hset[0]&256)>>8) | ((Hset[1]&256)>>7) |
	((Hset[2]&256)>>6) | ((Hset[3]&64)>>3) |
	((Hset[4]&256)>>4) | (Hset[5]&32) | ((Hset[6]&256)>>2);
    settings[16] = ((Vset[0]&1024)>>10) | ((Vset[3]&1024)>>9) |
	((Vset[4]&1024)>>8) | ((Vset[1]&1024)>>6);
    settings[17] = 0x00;
    settings[18] = 0x20;
    settings[19] = 0x00;
    settings[20] = (Eset[0]&0xff00)>>8;
    settings[21] = (Eset[0]&0xff);
    settings[22] = (Eset[1]&0xff);
    settings[23] = 0x60; //0x60
    settings[24] = 0xeb;
    settings[25] = 0xff;
    settings[26] = (Eset[1]&0x300)>>4;
    settings[27] = (Eset[0]&0xf0000)>>16;

    dprintf("prepare_crt(%ld) finished\n", pixel_per_unit);
}

void do_virge_settings()
{
    int      i;
    uchar    value[4];

      //  Complete specific color, width, mem and performance settings for virge.
    static uchar index_virge_settings[4] = {
	0x0b, 0x36, 0x58, 0x67
    };
  
    dprintf("do_virge_settings()\n");
    if (sCardInfo.scrnColors == 8) {
	value[0] = 0x00;
	value[3] = 0x00; //0x0c;
    } else {
	value[0] = 0x50; //0x50;
	value[3] = 0x50; //0x50;
    }

    if (sCardInfo.theMem == 2048) {
	value[1] = 0x8a; //0x8e; //0x8e;
	value[2] = 0x12; //0x12
    } else { // 4MB
	value[1] = 0x0a; //0x0e; // 0x0e
	value[2] = 0x13; // 0x13
    }
//    dprintf( "Set SEQ\n" );
    v_outb(SEQ_INDEX,index_virge_settings[0]);
    v_outb(SEQ_DATA,value[0]);
//    dprintf( "Set CRT\n" );
    for (i=1;i<4;i++) {
	v_outb(CRTC_INDEX,index_virge_settings[i]);
	v_outb(CRTC_DATA,value[i]);
    }
}

// Set the CRT_CONTROL by writing the preprocessed values in the 28 registers.
static void vid_select_crt()
{
    int      i;

      // This is the index of the 28 crt registers and extended registers (all access
      // through 3d5 index by 3d4) modified by CRT_CONTROL. After that, those
      // registers will just be refered as number 0 to 27.
    static uchar crt_index_864[] = {
	0x00, 0x3b, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x09, 0x10, 0x11, 0x12, 0x15, 0x16, 0x5d,
	0x5e,
	0x08, 0x0a, 0x0b, 0x0c, 0x0d, 0x13, 0x14, 0x17,
	0x18,
	0x51, 0x69
    };
  
//  volatile long  j;
    dprintf("vid_select_crt()\n");
    for (i=0;i<28;i++) {
	v_outb(CRTC_INDEX, crt_index_864[i]);
	  //for (j=0;j<6;j++) {;}
	v_outb(CRTC_DATA, settings[i]);
	  //for (j=0;j<6;j++) {;}
	dprintf("CR%02x = 0x%02x (%d)\n", crt_index_864[i], settings[i], settings[i]);
	  //snooze(1000000);
    }
    dprintf("vid_select_crt() finished\n");
}

//**************************************************************************
//  Mode selection.
//
//  Do the configuration for a specific mode, without the glue.
//**************************************************************************
void DoSelectMode()
{
      // not sure this is required.
      //setup_dac();

      // streams fifo ctrl
    v_outl(0x8200, 0x0000c000);
      // disable streams
    v_outb(CRTC_INDEX, 0x67);
    v_outb(CRTC_DATA, v_inb(CRTC_DATA) & ~0x0c);

// Now, we've to preprocess the CRT_CONTROL (if available)
    prepare_crt( 64 / sCardInfo.scrnColors );
	
// After that, we now know the real size of the display and so we can set
// set the selected refresh rate.
    write_pll();

// After the clock setting, we can program the crt registers
    vid_select_crt();		

    do_virge_settings();

//    dprintf( "DoSelectMode() read MISC_OUT_R\n" );
    uchar t = v_inb(MISC_OUT_R);
    t &= 0x3f;	// clear top bits, we'll set them below
    if      (sCardInfo.scrnHeight < 400)
	t |= 0x40; /* +vsync -hsync : 350 lines */
    else if (sCardInfo.scrnHeight < 480)
	t |= 0x80; /* -vsync +hsync : 400 lines */
    else if (sCardInfo.scrnHeight < 768)
	t |= 0xC0; /* -vsync -hsync : 480 lines */
    else
	t |= 0x00; /* +vsync +hsync : other */
//    dprintf( "DoSelectMode() write MISC_OUT_W\n" );
    v_outb(MISC_OUT_W, t);
    snooze( 50000 );
}

//**************************************************************************
//  Check the amount of available memory
//
//  First, we're supposing that every card will have at least 1MB of memory.
//  After that, we test all the different interesting configuration one by
//  one. We can't just test memory from bottom to top because the mapping
//  can depend of the memory size set (chips can use from 16 bits to 64 bits
//  bus depending of the amount of available memory). So we've to set a
//  memory size, then test it to see if there isn't any wrapping.
//  Nowadays, 2MB is the only other size tested.
//
//  After that, this function determined the available spaces.
//**************************************************************************

static void vid_checkmem (void)
{
//  ulong      i, j;
    volatile uint8_t *scrn;

    dprintf("vid_checkmem()\n");
    SCREEN_OFF;

// test 4 MB configuration
    sCardInfo.theMem = 4096;
    sCardInfo.scrnColors = 8;
    sCardInfo.scrnWidth = 1024;
    sCardInfo.scrnHeight = 768;
    sCardInfo.scrnRowByte = 1024;
//  sCardInfo.scrnRes = vga1024x768;

    DoSelectMode();
    dprintf( "Mode selected\n" );
      // clear video memory
//  v_rect_8(0,0,1023,4095,0);
	
      // reset for later
    sCardInfo.scrnColors = 0;
//  sCardInfo.scrnRes = -1;
	
    scrn = (volatile uint8_t *)sCardInfo.scrnBase;
    scrn += 2 * 1024 * 1024;
    *scrn = 0xab;
    scrn -= 2 * 1024 * 1024;
    *scrn = 0xcd;
    scrn += 2 * 1024 * 1024;
    dprintf("byte at %p: 0x%02x\n", scrn, *scrn);
    if (*scrn == 0xab)
	sCardInfo.theMem = 4096;
    else {
	scrn -= 2 * 1024 * 1024;
	dprintf("byte at %p: 0x%02x\n", scrn, *scrn);
	if (*scrn == 0xcd) sCardInfo.theMem = 2048;
	else sCardInfo.theMem = 1024; // should never happen
    }

// The spaces available depend of the amount of video memory available...
// Not really, for now, as we always have enough for the 8bit modes.
/*    
    sCardInfo.available_spaces =
	B_8_BIT_640x480 | B_16_BIT_640x480 |
	B_8_BIT_800x600 | B_16_BIT_800x600 |
	B_8_BIT_1024x768 | B_16_BIT_1024x768 |
	B_8_BIT_1152x900 | // B_16_BIT_1152x900 |
	B_8_BIT_1280x1024; // can do B_16_BIT_1280x1024 if interlaced, but it takes 4MB
      // B_8_BIT_1600x1200;
    dprintf("sCardInfo.theMem: %d, spaces: 0x%08x\n", sCardInfo.theMem, sCardInfo.available_spaces);
*/    
}

//**************************************************************************
//  Space selection, on the fly...
//
//  This function is able to set any configuration available on the add-on
//  without restarting the machine. Until now, all cards works pretty well
//  with that. As Window's 95 is not really doing the same, we're still a
//  bit afraid to have a bad surprize some day... We'll see.
//  The configuration include space (depth and resolution), CRT_CONTROL and
//  refresh rate.
//**************************************************************************

void vid_selectmode()
{
// As we are doing very very bad things, it's better to protect everything. So
// we need to get both io-register and graphic engine benaphores.
// NB : When both benaphores are taken, they must be taken always in the same
// order to avoid any stupid deadlock.
    lock_io();
    lock_ge();
	
// Blank the screen. Not doing that is criminal (you risk to definitively
// confuse your monitor, because in that case we're reprogramming most of the
// registers. So at time, we can get very bad intermediate state).
    dprintf( "vid_selectmode() Turn screen off\n" );
    SCREEN_OFF;
// Select the good mode...
    dprintf( "vid_selectmode() Select screen mode\n" );
    DoSelectMode();
	
// We need a buffer of 1024 bytes to store the cursor shape. This buffer has to
// be aligned on a 1024 bytes boundary. We put it just after the frame buffer.
// But if you want, you're free to put it at the beginning of the video memory,
// and then to offset the frame buffer by 1024 bytes.

//	sCardInfo.scrnBufBase = (uchar*)(((long)sCardInfo.scrnBase + sCardInfo.scrnRowByte * sCardInfo.scrnHeight + 1023) & 0xFFFFFC00);

// Unblank the screen now that all the very bad things are done.
//    dprintf( "vid_selectmode() Turn screen on\n" );

    
      // Turning on the screen to soon seems to make the card (and the
      // rest of the machine aswell) to lock up. Should probably wait
      // for the VBL or something like that, but I don't have the specs
      // handy so I'm not sure. This delay seems to make it work though.

    
    SCREEN_ON;

// Release the graphic engine benaphore and the benaphore of the io-registers.
    unlock_ge();
    unlock_io();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VirgeDriver::VirgeDriver( int nFd )
{
    /* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &sCardInfo.pcii ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	sCardInfo.nFd = nFd;
  
/*  
    m_pcMouseImage = new SrvBitmap( POINTER_WIDTH, POINTER_HEIGHT, CS_CMAP8 );
    m_pcMouseSprite = NULL;

    uint8* pRaster = m_pcMouseImage->m_pRaster;

    for ( int i = 0 ; i < POINTER_WIDTH * POINTER_HEIGHT ; ++i ) {
	uint8 anPalette[] = { 255, 0, 0, 63 };
	if ( g_anMouseImg[i] < 4 ) {
	    pRaster[i] = anPalette[ g_anMouseImg[i] ];
	} else {
	    pRaster[i] = 255;
	}
    }
    */    
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VirgeDriver::~VirgeDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

area_id VirgeDriver::Open( void )
{
    m_pFrameBuffer = NULL;
    g_nFrameBufArea = create_area( "virge_io",(void**) &m_pFrameBuffer, 1024 * 1024 * 64,
				   AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK );
    remap_area( g_nFrameBufArea, (void*) (sCardInfo.pcii.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) );

	
	
      // this rate MUST be initialized :-)
    sCardInfo.scrnRate = 60.1;
    sCardInfo.hotpt_v = sCardInfo.hotpt_h = -1000;
    dprintf("ViRGE found at pci index %d\n", index);

      // This is the pointer for memory mapped IO
    sCardInfo.base0 = (volatile uint8_t *)(m_pFrameBuffer) + 0x01000000;	// little endian area

      // This is the pointer to the beginning of the video memory, as mapped in
      // the add-on memory adress space. This add-on puts the frame buffer just at
      // the beginning of the video memory.
    sCardInfo.scrnBase = (uchar *)m_pFrameBuffer;

// Initialize the s3 chip in a reasonable configuration. Turn the chip on and
// unlock all the registers we need to access (and even more...). Do a few
// standard vga config. Then we're ready to identify the DAC.

      /*
	I don't know why we need to access this through the isa_io, but we seem to need to.
	If anybody knows a way around it, feel free to fix it.
	*/
    set_pci(0x04, 4, 0x02000003); // enable ISA IO, enable MemMapped IO
    isa_outb(VGA_ENABLE, 0x01); // pg 13-1
    dprintf("VGA_ENABLE: 0x%02x\n", (int)isa_inb(VGA_ENABLE));
    v_outb(VGA_ENABLE, 0x01); // pg 13-1
    dprintf("VGA_ENABLE: 0x%02x\n", (int)v_inb(VGA_ENABLE));
    isa_outb(MISC_OUT_W, 0x0f);
    dprintf("MISC_OUT_R: 0x%02x\n", (int)isa_inb(MISC_OUT_R));
    v_outb(MISC_OUT_W, 0x0f);
    dprintf("MISC_OUT_R: 0x%02x\n", (int)v_inb(MISC_OUT_R));


    uchar t = isa_inb(MISC_OUT_R); // pg 13-1
    t = v_inb(MISC_OUT_R); // pg 13-1
    isa_outb(DAC_ADR_MASK, 0xff);
    v_outb(DAC_ADR_MASK, 0xff);
    isa_outb(SEQ_INDEX, 0x08); // pg 13-2 -- unlock extended Sequencer regs
    isa_outb(SEQ_DATA,  0x06);
    v_outb(SEQ_INDEX, 0x08); // pg 13-2 -- unlock extended Sequencer regs
    v_outb(SEQ_DATA,  0x06);
    isa_outb(CRTC_INDEX, 0x38); // pg 13-2 -- unlock extended CRTC 2D->3F
    isa_outb(CRTC_DATA,  0x48);
    v_outb(CRTC_INDEX, 0x38); // pg 13-2 -- unlock extended CRTC 2D->3F
    v_outb(CRTC_DATA,  0x48);
    isa_outb(CRTC_INDEX, 0x39); // pg 13-2 -- unlock extended CRTC 40->FF
    isa_outb(CRTC_DATA,  0xa5);
    v_outb(CRTC_INDEX, 0x39); // pg 13-2 -- unlock extended CRTC 40->FF
    v_outb(CRTC_DATA,  0xa5);
    isa_outb(CRTC_INDEX, 0x40); // pg 13-2 -- enable access to enhanced programming regs
    t = isa_inb(CRTC_DATA);
    isa_outb(CRTC_DATA,  t | 0x01);
    v_outb(CRTC_INDEX, 0x40); // pg 13-2 -- enable access to enhanced programming regs
    t = v_inb(CRTC_DATA);
    v_outb(CRTC_DATA,  t | 0x01);
    isa_outb(CRTC_INDEX, 0x53);	// Extended memory control 1
    isa_outb(CRTC_DATA,  0x08);
    v_outb(CRTC_INDEX, 0x53);	// Extended memory control 1
    v_outb(CRTC_DATA,  0x08);

    dprintf("VGA_ENABLE: 0x%02x\n", (int)isa_inb(VGA_ENABLE));
    dprintf("VGA_ENABLE: 0x%02x\n", (int)v_inb(VGA_ENABLE));
    dprintf("MISC_OUT_R: 0x%02x\n", (int)isa_inb(MISC_OUT_R));
    dprintf("MISC_OUT_R: 0x%02x\n", (int)v_inb(MISC_OUT_R));
		

      //outb(VGA_ENABLE,1);
		
      // We've got the ViRGE they want, now lets put it in a REASONABLE mode.
    dprintf("Was PCI 0x04: 0x%08lx\n", get_pci(0x04, 4));
		
    set_pci(0x04, 4, 0x02000002); // disable ISA IO, enable MemMapped IO
//		set_pci(0x04, 4, 0x02000003); // disable ISA IO, enable MemMapped IO
		
    dprintf("Now PCI 0x04: 0x%08lx\n", get_pci(0x04, 4));
    dprintf("Was PCI 0x30: 0x%08lx\n", get_pci(0x30, 4));
    set_pci(0x30, 4, 0x00000000); // disable the BIOS
    dprintf("Now PCI 0x30: 0x%08lx\n", get_pci(0x30, 4));

    v_outb(CRTC_INDEX, 0x2d);
    dprintf("Chip ID Hi: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x2e);
    dprintf("Chip ID Lo: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x2f);
    dprintf("Chip Revis: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x59);
    dprintf("CR59: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x5a);
    dprintf("CR5A: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x31);
    dprintf("CR31: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x32);
    dprintf("CR32: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x35);
    dprintf("CR35: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x36);
    dprintf("CR36: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x37);
    dprintf("CR37: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x68);
    dprintf("CR68: 0x%02x\n", (int)v_inb(CRTC_DATA));
    v_outb(CRTC_INDEX, 0x6f);
    dprintf("CR6F: 0x%02x\n", (int)v_inb(CRTC_DATA));


// Set the graphic engine in enhanced mode
    ulong afc = v_inl(ADVFUNC_CNTL);
    dprintf("ADVFUNC_CNTL: 0x%08lx\n", afc);
    v_outl(ADVFUNC_CNTL, 0x00000013);
    v_outl(ADVFUNC_CNTL, 0x00000011);
    afc = v_inl(ADVFUNC_CNTL);
    dprintf("ADVFUNC_CNTL: 0x%08lx\n", afc);
    dprintf("outl(SUBSYS_CNTL, 0x007f)\n");
    v_outl(SUBSYS_CNTL, 0x0000007f);

    dprintf("set_table(g_anGcrTable)\n");
    set_table( g_anGcrTable );
    dprintf("set_table(g_anSequencerTable)\n");
    set_table( g_anSequencerTable );
    dprintf("set_attr_table(g_anAttributeTable)\n");
    set_attr_table(g_anAttributeTable);

    dprintf("set_table(g_anVirgeTable)\n");
    set_table(g_anVirgeTable);

// Arrived at that point, we now know we're able to drive that card. So we need
// to initialise what we need here, especially the benaphore protection that
// will allow concurent use of the add-on.
    init_locks();

// We finally need to test the size of the available memory, and we can test
// the available writing bandwidth, just for information.

    vid_checkmem();

    dbprintf( "Available VideMemory: %d\n", sCardInfo.theMem );
    
    float rf[] = { 60.0f, 75.0f, 85.0f };
    for( int i = 0; i < 3; i++ )
    {

    if ( sCardInfo.theMem >= 1024 ) {
	m_cModes.push_back( os::screen_mode( 640,  480,  640,  CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 800,  600,  800,  CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1024, 768,  1024, CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1152, 900,  1152, CS_CMAP8, rf[i] ) );
      
	m_cModes.push_back( os::screen_mode( 640,  480,  640*2,  CS_RGB16, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 800,  600,  800*2,  CS_RGB16, rf[i] ) );
    }
    if ( sCardInfo.theMem >= 2048 ) {
	m_cModes.push_back( os::screen_mode( 1280, 1024, 1280, CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1600, 1200, 1600, CS_CMAP8, rf[i] ) );

	m_cModes.push_back( os::screen_mode( 1024, 768,  1024*2, CS_RGB16, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1152, 900,  1152*2, CS_RGB16, rf[i] ) );
    }
    if ( sCardInfo.theMem >= 4096 ) {
	m_cModes.push_back( os::screen_mode( 1280, 1024, 1280*2, CS_RGB16, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1600, 1200, 1600*2, CS_RGB16, rf[i] ) );
    }
    }
    
    sCardInfo.scrnBufBase = (uchar*) m_pFrameBuffer + (sCardInfo.theMem * 1024) - 1024;
    dprintf("   scrnBase: 0x%p\nscrnBufBase: 0x%p\n", sCardInfo.scrnBase, sCardInfo.scrnBufBase);
// ...		v_show_cursor(false);
  
    dbprintf( "Gfx card initiated\n" );

    m_nFrameBufferSize = sCardInfo.theMem * 1024;
/*
    m_pcMouseSprite = new SrvSprite( IRect( 0, 0, POINTER_WIDTH - 1, POINTER_HEIGHT - 1 ),
				     IPoint( 0, 0 ), IPoint( 0, 0 ), g_pcScreenBitmap, m_pcMouseImage );
				     */
  
    return( g_nFrameBufArea );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void VirgeDriver::Close( void )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int VirgeDriver::SetScreenMode( os::screen_mode sMode )
{
    sCardInfo.crtPosH     = (int)sMode.m_vHPos;
    sCardInfo.crtSizeH    = (int)sMode.m_vHSize;
    sCardInfo.crtPosV     = (int)sMode.m_vVPos;
    sCardInfo.crtSizeV    = (int)sMode.m_vVSize;
    sCardInfo.scrnRate	  = sMode.m_vRefreshRate;
    sCardInfo.scrnWidth   = sMode.m_nWidth;
    sCardInfo.scrnHeight  = sMode.m_nHeight;
    sCardInfo.scrnRowByte = sMode.m_nWidth;
    sCardInfo.colorspace  = sMode.m_eColorSpace;
  
    switch( sMode.m_eColorSpace )
    {
	case CS_CMAP8:
	    sCardInfo.scrnColors = 8;
	    break;
	case CS_RGB16:
	    sCardInfo.scrnColors = 16;
	    sCardInfo.scrnRowByte <<= 1;
	    break;
	default:
	    dbprintf( "virge_set_creen_mode() invalide color dept %d\n", sCardInfo.scrnColors );
	    return( -1 );
    }

    if ( sMode.m_nWidth == 640 && sMode.m_nHeight == 480 ) {
	sCardInfo.scrnResNum = 0;
    } else if ( sMode.m_nWidth == 800 && sMode.m_nHeight == 600 ) {
	sCardInfo.scrnResNum = 1;
    } else if ( sMode.m_nWidth == 1024 && sMode.m_nHeight == 768 ) {
       sCardInfo.scrnResNum = 2;
    } else if ( sMode.m_nWidth == 1152 && sMode.m_nHeight == 900 ) {
	sCardInfo.scrnResNum = 2;
    } else if ( sMode.m_nWidth == 1280 && sMode.m_nHeight == 1024 ) {
	sCardInfo.scrnResNum = 3;
    } else if ( sMode.m_nWidth == 1600 && sMode.m_nHeight == 1200 ) {
	sCardInfo.scrnResNum = 4;
    } else  {
	dbprintf( "Invalid resolution %d-%d\n", sMode.m_nWidth, sMode.m_nHeight );
	return( -1 );
    }
    if ( sCardInfo.scrnColors == 16 ) {
	sCardInfo.scrnResNum += 5;
    } else if ( sCardInfo.scrnColors == 32 ) {
	sCardInfo.scrnResNum += 10;
    }
  
    sCardInfo.offscrnWidth  = sCardInfo.scrnWidth;
    sCardInfo.offscrnHeight = sCardInfo.scrnHeight;
    sCardInfo.scrnPosH = 0;
    sCardInfo.scrnPosV = 0;
    
    m_sCurrentMode = sMode;
    m_sCurrentMode.m_nBytesPerLine = sMode.m_nWidth * sCardInfo.scrnColors / 8;

    dprintf("Setting mode: %dx%dx%d@%f\n",
	    sCardInfo.scrnWidth, sCardInfo.scrnHeight, sCardInfo.scrnColors, sCardInfo.scrnRate);
  
    vid_selectmode();


//  SrvBitmap* pcBitmap = new SrvBitmap( nWidth, nHeight, eColorSpc, m_pFrameBuffer, sCardInfo.scrnRowByte );
      
//  pcBitmap->m_pcDriver  = this;
//  pcBitmap->m_bVideoMem = true;

//  m_pcMouse = new MousePtr( pcBitmap, this, 8, 8 );

    return( 0 );
}

int VirgeDriver::GetScreenModeCount()
{
    return( m_cModes.size() );
}

bool VirgeDriver::GetScreenModeDesc( int nIndex, os::screen_mode* psMode )
{
    if ( uint(nIndex) < m_cModes.size() ) {
	*psMode = m_cModes[nIndex];
	return( true );
    } else {
	return( false );
    }
}

os::screen_mode VirgeDriver::GetCurrentScreenMode()
{
    return( m_sCurrentMode );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VirgeDriver::DrawLine( SrvBitmap* psBitMap, const IRect& cClipRect,
			    const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode )
{
    uint32 nColor;
    switch( psBitMap->m_eColorSpc )
    {
	case CS_RGB32:
	    nColor = COL_TO_RGB32( sColor );
	    break;
	case CS_RGB16:
	    nColor = COL_TO_RGB16( sColor );
	    break;
	default:
	    return( false );
    }
    if ( psBitMap->m_bVideoMem && nMode == DM_COPY )
    {
	bool bXMajor;
	int dx, dy, tmp, Xdelta;
	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;
	  // Make the coordinates acceptable to the hardware clipper
	if ( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false ) {
	    return( false );
	}
  
	lock_ge();

	  // reset unit
	wait_for_fifo(0x400);
	v_outl(0xa900, 0x78000000);
	v_outl(0xa8d4, 0);
	v_outl(0xa8d8, 0);
	v_outl(0xa8e4, sCardInfo.scrnRowByte | (sCardInfo.scrnRowByte << 16));

	  // The virge blitter don't like drawing downwards
	if (y1 < y2) {
	    tmp = x1;
	    x1 = x2;
	    x2 = tmp;
	    tmp = y1;
	    y1 = y2;
	    y2 = tmp;
	}
	
	  // Calculate vector coordinates...
	dx = x1-x2;
	dy = y1-y2;

	  // process Xdelta
	if ( dy != 0 ) {
	    Xdelta = (-(dx << 20)) / dy;
	} else {
	    Xdelta = 0;
	}
	if ((dx > dy) || (-dx > dy)) {
	    bXMajor = true;
	} else {
	    bXMajor = false;
	}
	wait_for_fifo(0x700);
	v_outl(0xa970, Xdelta);
	
	  // Xstart
	tmp = (x1<<20);
	if (bXMajor) {
	    tmp += ( Xdelta / 2 );
	    if ( Xdelta < 0 )
		tmp +=((1<<20)-1);
	}
	v_outl(0xa974, tmp);

	  // Ystart
	v_outl(0xa978, y1);

	  // Xends
	v_outl(0xa96c, (x1<<16)|x2);

	  // color
	v_outl(0xa8f4, nColor);
	

	  // Setup the clipper
	v_outl(0xa8e0, ((cClipRect.top & 0x7ff)<<16) | (cClipRect.bottom & 0x7ff));
	v_outl(0xa8dc, ((cClipRect.left & 0x7ff)<<16) | (cClipRect.right & 0x7ff));
	wait_for_fifo(0x200);

	if ( psBitMap->m_eColorSpc == CS_CMAP8 ) {
	    v_outl(0xa900, 0x19e00121 | 0x02 ); // 0x02 == use clipper
	} else {
	    v_outl(0xa900, 0x19e00125 | 0x02 ); // 0x02 == use clipper
	}
	  // Ycount
	if (x1 >= x2) {
	    v_outl(0xa97c, dy+1);
	} else {
	    v_outl(0xa97c, (dy+1) | 0x80000000);
	}
	wait_for_blitter();

	unlock_ge();
    } else  {
	DisplayDriver::DrawLine( psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode );
    }
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VirgeDriver::FillRect( SrvBitmap* psBitMap, const IRect& cRect, const Color32_s& sColor, int nMode )
{
    uint32 nColor;
    
    if( nMode != DM_COPY ) {
		return( DisplayDriver::FillRect( psBitMap, cRect, sColor, nMode ) );
    }
    
    switch( psBitMap->m_eColorSpc )
    {
	case CS_RGB32:
	    nColor = COL_TO_RGB32( sColor );
	    break;
	case CS_RGB16:
	    nColor = COL_TO_RGB16( sColor );
	    break;
	default:
	    return( false );
    }
  
    if ( psBitMap->m_bVideoMem ) {
	lock_ge();

	wait_for_fifo(0x800);
    
	v_outl( 0xa500, 0x78000000 );
	v_outl( 0xa4d4, 0 );	// source base addr
	v_outl( 0xa4d8, 0 );	// dest base addr
	v_outl( 0xa4e4, sCardInfo.scrnRowByte | (sCardInfo.scrnRowByte << 16) );	// source/dest bytes per row
	v_outl( 0xa4f4, nColor );
	v_outl( 0xa504, (cRect.Width()<<16) | (cRect.Height()+1) );
	v_outl( 0xa50c, (cRect.left << 16) | cRect.top );

	if ( psBitMap->m_eColorSpc == CS_CMAP8 ) {
//      v_outl(0xa500, 0x16aa0120); (invertrect)
	    v_outl(0xa500, 0x17e00120);
	} else {
//      v_outl(0xa500, 0x16aa0124); (invertrect)
	    v_outl( 0xa500, 0x17e00124 );
	}
	wait_for_blitter();
	unlock_ge();
	return( true );
    } else {
	return( DisplayDriver::FillRect( psBitMap, cRect, sColor, nMode ) );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VirgeDriver::BltBitmap( SrvBitmap* dstbm, SrvBitmap* srcbm, IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha )
{
    if ( dstbm->m_bVideoMem && srcbm->m_bVideoMem && nMode == DM_COPY && cSrcRect.Size() == cDstRect.Size() ) {
    IPoint cDstPos = cDstRect.LeftTop();
	uint32 command;
	int x1 = cSrcRect.left;
	int y1 = cSrcRect.top;
	int x2 = cDstPos.x;
	int y2 = cDstPos.y;
	int width  = cSrcRect.Width()+1;
	int height = cSrcRect.Height()+1;
    
	  // Check degenerated blit (source == destination)
	if ( x1 == x2 && y1 == y2 ) {
	    return( false );
	}

	lock_ge();

	command = 0x07980020;

	if (x1 < x2) {
	    x1 += width - 1;
	    x2 += width - 1;
	    command &= ~0x02000000;
	}
	if (y1 < y2) {
	    y1 += height - 1;
	    y2 += height - 1;
	    command &= ~0x04000000;
	}
	  // 16 bit mode
	if ( sCardInfo.scrnColors == 16 ) {
	    command |= 0x00000004;
	}

	wait_for_fifo(0x700);

	v_outl(0xa500, 0x78000000); // turn off auto-execute
	v_outl(0xa4d4, 0);	// source base addr
	v_outl(0xa4d8, 0);	// dest base addr
	v_outl(0xa4e4, sCardInfo.scrnRowByte | (sCardInfo.scrnRowByte << 16));	// source/dest bytes per row
	v_outl(0xa504, ((width - 1)<<16)|(height));
	v_outl(0xa508, (x1<<16)|y1);
	v_outl(0xa50c, (x2<<16)|y2);
	v_outl(0xa500, command);

	wait_for_blitter();
	unlock_ge();
	return( true );
    } else {
	return( DisplayDriver::BltBitmap( dstbm, srcbm, cSrcRect, cDstRect, nMode, nAlpha ) );
    }
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void UpdateScreenMode()
{
    dbprintf( "Update screen mode\n" );

    vid_selectmode();
}

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf( "s3_virge attempts to initialize\n" );

    try {
	DisplayDriver* pcDriver = new VirgeDriver( nFd );
	return( pcDriver );
    }
    catch( std::exception&  cExc ) {
	return( NULL );
    }
}
