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
 
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <gui/popupmenu.h>
#include <util/message.h>
#include <gui/icondirview.h>
#include <storage/registrar.h>
#include <util/event.h>


class Desktop : public os::Window
{
public:
	Desktop();
	~Desktop();
	void HandleMessage( os::Message* );
	bool OkToQuit();
	void ScreenModeChanged( const os::IPoint& cRes, os::color_space eSpace );
	void DesktopActivated( int nDesktop, bool bActive );
	
	os::String GetBackground() { return( m_zBackground ); }

	bool GetDesktopFontShadow() { return( m_bFontShadows ); }
	void SetDesktopFontShadow( bool bDesktopFontShadow ); 

	void SetBackground( os::String zBackground );

	bool GetSingleClick() { return( m_bSingleClick ); }
	void SetSingleClick( bool bSingleClick );

private:
	void LaunchFiles();
	void SaveSettings();
	void LoadBackground();
	os::IconDirectoryView* m_pcView;
	os::String m_zBackground;
	os::BitmapImage* m_pcBackground;
	bool m_bSingleClick;
	bool m_bFontShadows;
	
	os::Event* m_pcGetSingleClickEv;
	os::Event* m_pcSetSingleClickEv;
	os::Event* m_pcGetFontShadowEv;
	os::Event* m_pcSetFontShadowEv;
	os::Event* m_pcGetBackgroundEv;
	os::Event* m_pcSetBackgroundEv;
	os::Event* m_pcRefreshEv;
};

#endif













