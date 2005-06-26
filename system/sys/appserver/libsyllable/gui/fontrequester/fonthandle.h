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

#ifndef FONT_HANDLE_H
#define FONT_HANDLE_H

#include <gui/font.h>

using namespace os;

/** \internal */
struct FontHandle {
    char	zFamilyName[FONT_FAMILY_LENGTH];
	char	zStyleName[FONT_STYLE_LENGTH];
    uint32	nFlags;
	float	vSize;
	float	vShear;
	float	vRotation;
};


/** \internal */
static FontHandle* GetDefaultFontHandle()
{
	FontHandle* pcHandle = new FontHandle;

	font_properties sProp;
	os::Font::GetDefaultFont(DEFAULT_FONT_FIXED,&sProp);

	strcpy(pcHandle->zFamilyName,sProp.m_cFamily.c_str());
	strcpy(pcHandle->zStyleName,sProp.m_cStyle.c_str());
	pcHandle->vSize = sProp.m_vSize;
	pcHandle->vShear = sProp.m_vShear;
	pcHandle->vRotation = sProp.m_vRotation;
	pcHandle->nFlags = sProp.m_nFlags;
	return pcHandle;
}


/** \internal */
static font_properties GetFontProperties(FontHandle* pcHandle)
{
	return (font_properties(pcHandle->zFamilyName,pcHandle->zStyleName,pcHandle->nFlags,pcHandle->vSize,pcHandle->vShear,pcHandle->vRotation));
}


/** \internal */
static FontHandle* GetNewFontHandle(FontHandle* pcHandle)
{
	FontHandle* pcReturn=new FontHandle();
	
	strcpy(pcReturn->zFamilyName,pcHandle->zFamilyName);
	strcpy(pcReturn->zStyleName,pcHandle->zStyleName);
	pcReturn->vSize = pcHandle->vSize;
	pcReturn->vShear = pcHandle->vShear;
	pcReturn->vRotation = pcHandle->vRotation;
	pcReturn->nFlags = pcHandle->nFlags;
	
	return pcReturn;
}

#endif  //FONT_HANDLE_H


