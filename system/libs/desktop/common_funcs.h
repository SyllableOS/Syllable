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

#ifndef __F_DESKTOP_COMMON_FUNCS_H
#define __F_DESKTOP_COMMON_FUNCS_H

// standard headers
#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unistd.h>

// libsyllable headers
#include <gui/rect.h>
#include <gui/point.h>
#include <storage/directory.h>
#include <gui/desktop.h>
#include <util/string.h>

// libdesktop specfic heaers
#include <desktop/version.h>

using namespace os;  // i am probably going to change this

namespace desktop
{

/**
* \ingroup desktop
* \par Description:
*  Returns the current version of Syllable as a String.
***************************************************************************/
	String SyllableInfo(void);

/**
* \ingroup desktop
* \par Description:
*	Returns the current version of the desktop as a String.
***************************************************************************/
	String DesktopVersion(void);

/**
* \ingroup desktop
* \par Description:
*  Launches files in a specific directory.
* \param zPath - the path of the directory
***************************************************************************/
	void LaunchFiles(const std::string& zPath);

/**
* \ingroup desktop
* \par Description:
*	Gets the current screen resolution of Syllable.
***************************************************************************/
	IPoint ScreenRes(void);

/**
* \ingroup desktop
* \par Description:
*  Checks to see if a certain file exsists or not.
*  Returns true if the file exsists and returns false if the 
*	file does not exsist.
* \param zPath - the path of the file to test
***************************************************************************/
	bool CheckFileExists(const std::string& zPath );


/**
* \ingroup desktop
* \par Description:
*  CRect is a Rect class that centers the window on the screen
***************************************************************************/
class CRect : public Rect
{
public:
		CRect(float, float);
private:
		CRect();
};

}  //end of namespace desktop

#endif  //end of file


