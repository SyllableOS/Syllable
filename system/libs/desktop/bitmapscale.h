/*  libdesktop.so - the API for Desktop development
 *  Copyright (C) 2003 - 2004 Rick Caudill
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

#ifndef __F__DESKTOP_BITMAPSCALE_H
#define __F__DESKTOP_BITMAPSCALE_H

// standard headers
#include <sys/types.h>

//syllable specfic headers
#include <gui/bitmap.h>

using namespace os;

namespace desktop
{
	enum bitmapscale_filtertype
	{
    	filter_filter,
    	filter_box,
    	filter_triangle,
    	filter_bell,
    	filter_bspline,
    	filter_lanczos3,
    	filter_mitchell
	};

/**
* \ingroup desktop
* \par Description:
*  Scales a bitmap.
* \param srcbitmap - pointer to the soure Bitmap object.
* \param distbitmap - pointer to the destination Bitmap object
* \param filtertype
* \param filterwidth.
***************************************************************************/
	void Scale( Bitmap *srcbitmap, Bitmap *dstbitmap, bitmapscale_filtertype filtertype, float filterwidth=0.0f );

}  //end namespace desktop
#endif  //end of file




