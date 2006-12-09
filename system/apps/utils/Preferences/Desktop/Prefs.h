/*  Syllable Desktop Preferences
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

#include <gui/window.h>
#include <gui/desktop.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/stringview.h>
#include <gui/checkbox.h>
#include <gui/image.h>
#include <gui/requesters.h>
#include <util/application.h>
#include <util/resources.h>
#include <util/appserverconfig.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include <storage/file.h>
#include <storage/path.h>
#include <vector>

class PrefsDesktopWin : public os::Window
{
public:
	PrefsDesktopWin( os::Rect cFrame );
	~PrefsDesktopWin();
	void UpdateBackgroundList();
	virtual bool OkToQuit();
	virtual void HandleMessage( os::Message* pcMessage );
private:
	os::LayoutView* m_pcRoot;
	os::VLayoutNode* m_pcVLayout;
	os::VLayoutNode* m_pcVBackground;
	os::VLayoutNode* m_pcVWindows;
	os::FrameView* m_pcBackground;
	os::ListView* m_pcBackgroundList;
	os::FrameView* m_pcWindows;
	os::CheckBox* m_pcPopupWindows;
	os::CheckBox* m_pcSingleClick;
	os::CheckBox* m_pcFontShadow;
	os::HLayoutNode* m_pcHButtons;
	os::Button* m_pcApply;
	os::Button* m_pcUndo;
	os::Button* m_pcDefault;
	os::String m_zBackground;
	os::String m_zSavedBackground;
	bool m_bPopupWindowSave;
	bool m_bSingleClickSave;
	bool m_bFontShadowSave;
};

class PrefsDesktopApp : public os::Application
{
public:
	PrefsDesktopApp();
	~PrefsDesktopApp();
	virtual bool OkToQuit();
};

#endif









