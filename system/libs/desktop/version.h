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
 
#ifndef __F_DESKTOP_VERSION_H
#define __F_DESKTOP_VERSION_H

namespace desktop
{
/**
* \ingroup desktop
* \par Description:
*	DESKTOP_VERSION is the current version of the desktop.
***************************************************************************/
	const float DESKTOP_VERSION = 0.5;
	
/**
* \ingroup desktop
* \par Description:
*	DESKTOP_API is the current version of libdesktop's API.
***************************************************************************/
	const float DESKTOP_API  = 0.1;

/**
* \ingroup desktop
* \par Description:
*	DESKTOP_PLUGIN is the current version of the desktop's plugin archictecture.
***************************************************************************/
	const float DESKTOP_PLUGIN = 0.1;
}

#endif


