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
 
#ifndef _DOCK_H_
#define _DOCK_H_

#include <atheos/areas.h>
#include <atheos/kernel.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/desktop.h>
#include <gui/bitmap.h>
#include <gui/imagebutton.h>
#include <gui/stringview.h>
#include <util/resources.h>
#include <util/application.h>
#include <util/message.h>
#include <util/locker.h>
#include <util/event.h>
#include <storage/nodemonitor.h>
#include "dockplugin.h"
#include "DockMenu.h"
#include <storage/registrar.h>

class DockWin;

#define ICON_SYLLABLE 0xffff

class DockIcon
{
public:
	DockIcon( int nId, os::String zTitle )
	{
		m_zTitle = zTitle;
		m_nId = nId;
		m_pcBitmap = new os::BitmapImage();
	}
	~DockIcon()
	{
		delete( m_pcBitmap );
	}
	int GetID() { return( m_nId ); }
	os::String GetTitle() { return( m_zTitle ); }
	void SetTitle( os::String zTitle ) { m_zTitle = zTitle; }
	os::BitmapImage* GetBitmap()
	{
		
		return( m_pcBitmap );
	}
	
private:
	int m_nId;
	os::String m_zTitle;
	os::BitmapImage* m_pcBitmap;
};

class DockView : public os::View
{
public:
	DockView( DockWin* pcWin, os::Rect cFrame );
	~DockView();
	virtual void Paint( const os::Rect& cUpdate );
	virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
	virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
	DockMenu* GetSyllableMenu() { return( m_pcSyllableMenu ); }
	void SetSyllableMenuClosed() { Invalidate( os::Rect( 0, 0, 30, 30 ) ); Flush(); m_bSyllableMenuOpened = false; }
	void InvalidateSyllableMenu() { m_bSyllableMenuInvalid = true; }
private:
	int m_nCurrentIcon;
	os::BitmapImage* m_pcCurrentIcon;
	os::Window* m_pcCurrentInfo;
	DockWin* m_pcWin;
	bool m_bSyllableMenuInvalid;
	bool m_bSyllableMenuOpened;
	DockMenu* m_pcSyllableMenu;
};

class DockWin : public os::Window
{
public:
	DockWin();
	~DockWin();
	os::Rect GetDockFrame();
	virtual void Quit( int nAction );
	virtual void HandleMessage( os::Message* pcMessage );
	virtual bool OkToQuit();
	void LoadSettings();
	void SaveSettings();
	void AddPlugin( os::String zPath );
	void UpdatePlugins();
	void DeletePlugin( os::DockPlugin* pcPlugin );
	void ActivateWindow( int32 nWindow );
	void UpdateWindows( os::Message* pcWindows, int32 nCount );
	void UpdateWindowArea();
	virtual void WindowsChanged();
	virtual void ScreenModeChanged( const os::IPoint& cNewRes, os::color_space eSpace );
	virtual void DesktopActivated( int nDesktop, bool bActive );
	std::vector<DockIcon*> GetIcons() { return( m_pcIcons ); }
	os::BitmapImage* GetAboutIcon() { return( m_pcAboutIcon ); }
	os::BitmapImage* GetLogoutIcon() { return( m_pcLogoutIcon ); }
	std::vector<os::DockPlugin*> GetPlugins() { return( m_pcPlugins ); }
	os::alignment GetPosition() { return( m_eAlign ); }
	void SetPosition( os::alignment eAlign );
private:
	os::alignment m_eAlign;
	DockView* m_pcView;
	os::Desktop* m_pcDesktop;
	os::BitmapImage* m_pcDefaultWindowIcon;
	os::BitmapImage* m_pcAboutIcon;
	os::BitmapImage* m_pcLogoutIcon;
	int32 m_nLastWindowCount;
	os::Message m_cLastWindows;
	DockIcon* m_pcSyllableIcon;
	std::vector<DockIcon*> m_pcIcons;
	std::vector<os::DockPlugin*> m_pcPlugins;
	std::vector<os::View*> m_pcPluginViews;
	os::RegistrarManager* m_pcManager;
	os::Event* m_pcGetPluginsEv;
	os::Event* m_pcSetPluginsEv;
	os::Event* m_pcGetPosEv;
	os::Event* m_pcSetPosEv;
};


class DockApp : public os::Application
{
public:
	DockApp( const char* pzMimeType );
	~DockApp();
	virtual bool OkToQuit();
	virtual void HandleMessage( os::Message* pcMessage );
private:
	DockWin* m_pcWindow;
};

#endif



















































