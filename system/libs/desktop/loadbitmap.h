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
  
#ifndef __F__DESKTOP_LOADBITMAP_H
#define __F__DESKTOP_LOADBITMAP_H

// standard headers
#include <stdio.h> 

// syllable specfic headers
#include <gui/bitmap.h>
#include <util/message.h>
#include <translation/translator.h>
#include <atheos/time.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/string.h>
#include <util/locker.h>
#include <util/resources.h>
#include <storage/file.h>
#include <gui/font.h>


using namespace os;

namespace desktop
{

/**
* \ingroup desktop
* \par Description:
*  Loads a bitmap from a file.
* \param zName - the name of the file.
* \return  The data from the as a bitmap.
***************************************************************************/
	Bitmap *LoadBitmapFromFile(const char *zName);

/**
* \ingroup desktop
* \par Description:
*  Loads a bitmap from a resource.
* \param zName - the name of the resource.
* \return  The data from the resource as a bitmap. 
***************************************************************************/
	Bitmap *LoadBitmapFromResource(const char *name);

/**
* \ingroup desktop
* \par Description:
*  Loads a bitmap from a StreamableIO.
* \param pcStream - the StreamableIO object.
* \return  The data from the StreamableIO object as a bitmap.
***************************************************************************/
	Bitmap *LoadBitmapFromStream(StreamableIO* pcStream);
} // end of namespace desktop
#endif //end of file








