/*  ColdFish Plugin Interface
 *  Copyright (C) 2004 Arno Klenke
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

#ifndef _COLDFISHPLUGIN_H_
#define _COLDFISHPLUGIN_H_

#include <util/looper.h>
#include <gui/view.h>
#include <gui/menu.h>
#include <vector>

#include "messages.h"

class CFApp;

/* ColdFish menu */
enum ColdFishMenu
{
	CF_MENU_APP,
	CF_MENU_PLAYLIST,
	CF_MENU_FILE,
	CF_MENU_VIEW
};

class ColdFishPlugin : public os::Handler
{
public:	
	ColdFishPlugin() : os::Handler( "CFPlugin" ) {};
	virtual ~ColdFishPlugin() {};

	/* Implemented by ColdFish */
	void SetApp( os::Looper* pcApp, os::Menu* pcMenuBar )
	{
		m_pcApp = pcApp;
		m_pcMenuBar = pcMenuBar;
	}
	
	os::Looper* GetApp()
	{
		return( m_pcApp );
	}
	os::Menu* GetMenu( ColdFishMenu eMenu )
	{
		return( m_pcMenuBar->GetSubMenuAt( eMenu ) );
	}
	
	std::vector<os::String>* GetPlaylist() 
	{
		return( &m_cPlaylist );
	}
	
	void RegisterAsVisualization()
	{
		/* Add an entry to the view menu */
		os::Message* pcMsg = new os::Message( CF_SET_VIS_PLUGIN );
		pcMsg->AddString( "name", GetIdentifier() );
		os::MenuItem* pcItem = new os::MenuItem( GetIdentifier(), pcMsg );
		pcItem->SetTarget( GetApp() );
		GetMenu( CF_MENU_VIEW )->AddItem( pcItem );
	}

	
	/* Implemented by the plugin */
	virtual status_t Initialize() = 0;
	virtual os::String GetIdentifier() = 0;
	virtual void Activated() {}
	virtual void Deactivated() {}
	virtual void AudioData( os::View* pcTarget, int16 nData[2][512] ) {}
	virtual void FileChanged( os::String zPath ) {}
	virtual void NameChanged( os::String zName ) {}
	virtual void TrackChanged( int nNumber ) {}
	virtual void TimeChanged( uint64 nTime ) {}
	virtual void StateChanged( int nState ) {}
	virtual void PlaylistPositionChanged( uint nPosition ) {}
	virtual void PlaylistChanged() {}
private:
	os::Looper* m_pcApp;
	os::Menu* m_pcMenuBar;	
	std::vector<os::String> m_cPlaylist;
};

class ColdFishPluginEntry
{
public:
	virtual uint GetPluginCount() = 0;
	virtual ColdFishPlugin* GetPlugin( uint nIndex ) = 0;
};

#endif
