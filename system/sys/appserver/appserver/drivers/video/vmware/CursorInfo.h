/*
 *  Syllable Driver for VMware virtual video card
 *
 *  Based on the the xfree86 VMware driver.
 *
 *  Syllable specific code is
 *  Copyright (C) 2004, James Hayhurst (james_hayhurst@hotmail.com)
 *
 *  Other code is
 *  Copyright (C) 1998-2001 VMware, Inc.
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
 *
 */


#ifndef __CURSOR_INFO_H__
#define __CURSOR_INFO_H__


typedef struct CursorInfo
{
	int hotSpotX;
	int hotSpotY;
	int cursWidth;
	int cursHeight;

	int andDepth;		/* BPP of andMask */
	int andSize;		/* Size in words(32 bits) of andMask */
	uint32 *andMask;	/* Pointer to AND mask */

	int xorDepth;		/* BPP of xorMask */
	int xorSize;		/* Size in words(32 bits) of xorMask */
	uint32 *xorMask;	/* Pointer to XOR mask */
} CursorInfoInfo;


#endif
