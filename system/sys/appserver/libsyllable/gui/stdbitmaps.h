
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

namespace os
{
#if 0
}				/* Fool emacs autoindent */
#endif


enum
{
	BMID_ARROW_LEFT,
	BMID_ARROW_RIGHT,
	BMID_ARROW_UP,
	BMID_ARROW_DOWN,
	BMID_COUNT
};

Bitmap *get_std_bitmap( int nBitmap, int nColor, Rect * pcRect );



}				// end of namespace os
