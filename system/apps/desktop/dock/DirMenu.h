/*  Syllable Dock
 *  Copyright (C) 2003 Arno Klenke
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
 
#ifndef _DIRMENU_H_
#define _DIRMENU_H_

#include <gui/menu.h>
#include <storage/path.h>
#include <storage/nodemonitor.h>

class DirMenu : public os::Menu
{
public:
	DirMenu( os::Handler* pcHandler, os::Path cPath, os::Rect cFrame, const char* pzName, os::MenuLayout_e eLayout );
	~DirMenu();
private:
	os::NodeMonitor* m_pcMonitor;
};

#endif

