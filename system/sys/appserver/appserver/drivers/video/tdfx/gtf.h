
/*
 * Calculate timing parameters of any given video mode using the VESA GTF.
 *
 * taken from the Linux Kernel 2.6.0:
 * linux/drivers/video/fbmon.c
 *
 * Copyright (C) 2002 James Simmons <jsimmons@users.sf.net>
 *
 * Generalized Timing Formula is derived from:
 *
 *      GTF Spreadsheet by Andy Morrish (1/5/97)
 *      available at http://www.vesa.org
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __GTF_H__
#define __GTF_H__


typedef struct
{
	uint32 pixclock, hsync_len, vsync_len;
	uint32 right_margin, left_margin, lower_margin, upper_margin;
} TimingParams;


#define FLYBACK        550
#define V_FRONTPORCH   1
#define H_OFFSET       40
#define H_SCALEFACTOR  20
#define H_BLANKSCALE   128
#define H_GRADIENT     600
#define C_VAL          30
#define M_VAL          300

#define PICOS2KHZ(a) (1000000000UL/(a))
#define KHZ2PICOS(a) (1000000000UL/(a))


struct __fb_timings
{
	uint32 dclk;
	uint32 hfreq;
	uint32 vfreq;
	uint32 hactive;
	uint32 vactive;
	uint32 hblank;
	uint32 vblank;
	uint32 htotal;
	uint32 vtotal;
};


void gtf_get_mode( int nWidth, int nHeight, int nRefresh, TimingParams * t );


#endif
