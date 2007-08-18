/*  Syllable FileBrowser
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
 

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <gui/popupmenu.h>
#include <gui/image.h>
#include <gui/imagebutton.h>
#include <gui/layoutview.h>
#include <util/message.h>
#include <stack>
#include <gui/icondirview.h>
#include "toolbar.h"

class MainWindow : public os::Window
{
public:
	MainWindow( os::String zPath );
	~MainWindow();
	void HandleMessage( os::Message* );
	void LoadWindow( bool bStyleOnly );
	void SaveWindow();
	void LoadDefault();
	void SaveDefault();
private:
	bool OkToQuit();
	os::ImageButton* m_pcBackButton;
	std::stack<os::String> m_cBackStack;
	os::IconDirectoryView* m_pcView;
	os::ToolBar* m_pcToolBar;
	os::PopupMenu* m_pcViewMenu;
};

#endif








