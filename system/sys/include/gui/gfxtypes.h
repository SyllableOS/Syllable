/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	GRAPHICS_GFXTYPES_H
#define	GRAPHICS_GFXTYPES_H

#include <atheos/types.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Colour datatype
 * \ingroup gui
 * \par Description:
 *	Datatype for colour information, used by many parts of the OS.
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx), Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/

struct Color32_s
{
    Color32_s() {}
    Color32_s( int r, int g, int b, int a=255 ) {
	red = r; green = g; blue=b; alpha=a;
    }
    Color32_s( uint32 c ) {
    	red = (uint8)(c >> 16);
       	green = (uint8)(c >> 8);
       	blue = (uint8)(c);
       	alpha = (uint8)(c >> 24);
    }
    operator uint32() const {
    	return ( ( alpha << 24 ) | ( red << 16 ) | ( green << 8 ) | blue );
    }
    uint8 red;
    uint8 green;
    uint8 blue;
    uint8 alpha;
};

typedef Color32_s Color32;

} // end of namespace

#endif	/*	_GFX_TYPES_H	*/

