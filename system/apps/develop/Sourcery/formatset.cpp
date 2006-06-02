//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "formatset.h"

/****************************************
* Description: This class holds information about a filter
* Author: Anderas E-H and Rick Caudill
* Date: Fri Mar 19 15:36:19 2004
****************************************/
FormatSet::FormatSet()
{
	sBgColor = os::Color32_s(255,255,255);
	sFgColor = os::Color32_s(0,0,0);
	cName = std::string();
	cFilter = std::string();
	pcFormat = NULL;
	nTabSize = 4;
	bUseTab = true;
	bAutoIndent = true;
}

FormatSet::~FormatSet()
{
}
