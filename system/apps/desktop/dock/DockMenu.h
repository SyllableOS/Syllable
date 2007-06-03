/*  Syllable Dock
 *  Copyright (C) 2005 Arno Klenke
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
 
#ifndef _DOCKMENU_H_
#define _DOCKMENU_H_

#include <gui/menu.h>
#include <storage/path.h>
#include <storage/nodemonitor.h>

namespace os
{
	class RegistrarManager;
};

class DockMenu : public os::Menu
{
public:
	DockMenu( os::Handler* pcHandler, os::Rect cFrame, const char* pzName, os::MenuLayout_e eLayout );
	~DockMenu();
	os::MenuItem* FindMenuItem( os::String zName );
	void AddEntry( os::RegistrarManager* pcManager, os::String zApplicationName, os::String zCategory, os::Path cPath,  bool bUseDirIcon, os::Path cDir );
private:
};

#endif
