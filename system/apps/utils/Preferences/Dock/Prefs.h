/*  Syllable Dock Preferences
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
 
#ifndef _PREFS_H_
#define _PREFS_H_

#include <atheos/image.h>
#include <gui/window.h>
#include <gui/desktop.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/stringview.h>
#include <gui/image.h>
#include <gui/requesters.h>
#include <gui/dropdownmenu.h>
#include <util/application.h>
#include <util/resources.h>
#include <util/event.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include <storage/file.h>
#include <vector>
#include <appserver/dockplugin.h>

struct DockPluginFile
{
	os::String zPath;
	os::String zName;
};

class PrefsDockWin : public os::Window
{
public:
	PrefsDockWin( os::Rect cFrame );
	~PrefsDockWin();
	bool CheckPlugin( os::String zPath, os::String* pzName );
	void UpdatePluginsList();
	virtual bool OkToQuit();
	virtual void HandleMessage( os::Message* pcMessage );
private:
	os::LayoutView* m_pcRoot;
	os::VLayoutNode* m_pcVLayout;
	os::FrameView* m_pcPosition;
	os::HLayoutNode* m_pcHPos;
	os::DropdownMenu* m_pcPos;
	
	os::FrameView* m_pcPlugins;
	os::VLayoutNode* m_pcVPlugins;
	os::ListView* m_pcPluginsList;
	os::Button* m_pcEnable;
	os::StringView* m_pcPluginNote;
	os::HLayoutNode* m_pcHButtons;
	os::Button* m_pcApply;
	os::Button* m_pcUndo;
	os::Button* m_pcDefault;
	std::vector<DockPluginFile> m_cPluginFiles;
	std::vector<os::String> m_cEnabledPlugins;
	std::vector<os::String> m_cDefaultEnabledPlugins;
	std::vector<os::String> m_cSavedEnabledPlugins;
};

class PrefsDockApp : public os::Application
{
public:
	PrefsDockApp();
	~PrefsDockApp();
	virtual bool OkToQuit();
};

#endif





