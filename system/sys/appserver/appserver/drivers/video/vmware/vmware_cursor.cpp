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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <atheos/types.h>

#include "CursorInfo.h"
#include "svga_reg.h"
#include "vmware.h"


bool VMware::Fifo_DefineCursor(uint32 cursorId, CursorInfo *cursInfo)
{
	int i;

	if((cursInfo == NULL) || (cursInfo->andMask == NULL)
		|| (cursInfo->xorMask == NULL))
		return false;

	/* Length of AND mask in words(32 bits) */
	int andLength = SVGA_BITMAP_SIZE(cursInfo->cursWidth, cursInfo->cursHeight);

	/* Length of XOR mask in words(32 bits) */
	int xorLength = SVGA_BITMAP_SIZE(cursInfo->cursWidth, cursInfo->cursHeight);

	Fifo_WriteWord(SVGA_CMD_DEFINE_CURSOR);
	Fifo_WriteWord(cursorId);				// Cursor Id
	Fifo_WriteWord(cursInfo->hotSpotX);		// HotX
	Fifo_WriteWord(cursInfo->hotSpotY);		// HotY
	Fifo_WriteWord(cursInfo->cursWidth);	// Cursor width
	Fifo_WriteWord(cursInfo->cursHeight);	// Cursor height
	Fifo_WriteWord(cursInfo->andDepth);		// Depth of AND mask
	Fifo_WriteWord(cursInfo->xorDepth);		// Depth of XOR mask

	/* Write out the AND mask */
	for(i = 0; i < andLength; i++)
	{
		Fifo_WriteWord(cursInfo->andMask[i]);
	}

	/* Write out the XOR mask */
	for(i = 0; i < xorLength; i++)
	{
		Fifo_WriteWord(cursInfo->xorMask[i]);
	}

	/* Sync the FIFO before any reference to the cursor is used */
	FifoSync();

	return true;
}


bool VMware::Fifo_DefineAlphaCursor(uint32 cursorId, CursorInfo *cursInfo)
{
	int i;

	if((cursInfo == NULL) || (cursInfo->andMask == NULL))
		return false;

	/* Length of pixmap in words(32 bits) */
	int pixSize = SVGA_PIXMAP_SIZE(cursInfo->cursWidth, cursInfo->cursHeight, 32);

	Fifo_WriteWord(SVGA_CMD_DEFINE_ALPHA_CURSOR);
	Fifo_WriteWord(cursorId);				// Cursor Id
	Fifo_WriteWord(cursInfo->hotSpotX);	// HotX
	Fifo_WriteWord(cursInfo->hotSpotY);	// HotY
	Fifo_WriteWord(cursInfo->cursWidth);	// Cursor width
	Fifo_WriteWord(cursInfo->cursHeight);	// Cursor height

	for(i = 0; i < pixSize; i++)
	{
		Fifo_WriteWord(cursInfo->andMask[i]);
	}

	/* Sync the FIFO before any reference to the cursor is used */
	FifoSync();

	return true;
}
