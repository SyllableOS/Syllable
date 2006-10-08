/*  Syllable Desktop
 *  Copyright (C) 2003 Arno Klenke
 *
 *  Andreas Benzler 2006 - some font functions and clean ups.
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
 

#ifndef MESSAGES_H
#define MESSAGES_H

enum
{
	M_CHANGE_DIR,
	M_APP_QUIT,

	M_SET_DESKTOP_FONT,
	M_GET_DESKTOP_FONT,
	M_SET_DESKTOP_FONT_COLOR,
	M_GET_DESKTOP_FONT_COLOR,
	M_SET_DESKTOP_FONT_SHADOW_COLOR,
	M_GET_DESKTOP_FONT_SHADOW_COLOR,
	M_SET_DESKTOP_FONT_SHADOW,
	M_GET_DESKTOP_FONT_SHADOW,
	
	M_GET_SINGLE_CLICK,
	M_SET_SINGLE_CLICK,

	M_GET_BACKGROUND,
	M_SET_BACKGROUND,

	M_REFRESH,
	M_RELAYOUT
};

#endif










