/*  ColdFish Remote Plugin
 *  Copyright (C) 2004 Arno Klenke
 *	Copyright (C) 2004 Terry Glass (taglass75@bellsouth.net)
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
 
/* This plugins registers coldfish in the registrar server */

#include "../../coldfish/CFPlugin.h"
#include <util/event.h>

class CFRemote : public ColdFishPlugin
{
public:
	CFRemote()
	{
	}
	~CFRemote()
	{
		printf("Delete!\n");
		delete( pcPlayEvent );
		delete( pcStopEvent );
		delete( pcPauseEvent );
		delete( pcNextEvent );
		delete( pcPrevEvent );
		delete( pcGetSongEvent );
		delete( pcGetPlayStateEvent );
		delete( pcAddFileEvent );
		delete( pcAboutEvent );
	}
	os::String GetIdentifier()
	{
		return( "Remote" );
	}
	status_t Initialize()
	{
		try
		{
			//os::RegistrarManager* pcRegManager = os::RegistrarManager::Get();
			/* Register remote calls */
			pcPlayEvent = os::Event::Register( "media/Coldfish/Play","Play",GetApp(),CF_GUI_PLAY);
			pcStopEvent = os::Event::Register( "media/Coldfish/Stop","Stop",GetApp(),CF_GUI_STOP);
			pcPauseEvent = os::Event::Register( "media/Coldfish/Pause","Pause",GetApp(),CF_GUI_PAUSE);
			pcNextEvent = os::Event::Register( "media/Coldfish/Next","Next",GetApp(),CF_PLAY_NEXT);
			pcPrevEvent = os::Event::Register( "media/Coldfish/Previous","Previous",GetApp(),CF_PLAY_PREVIOUS);
			pcGetSongEvent = os::Event::Register( "media/Coldfish/GetSong", "Gets the current song",GetApp(), CF_GET_SONG);
			pcGetPlayStateEvent = os::Event::Register( "media/Coldfish/GetPlayState", "Gets the current playstate",GetApp(), CF_GET_PLAYSTATE);
			pcAddFileEvent = os::Event::Register( "media/Coldfish/AddFile", "Calls the file requester to add a file to ColdFish",GetApp(), CF_GUI_ADD_FILE);			
			pcAboutEvent = os::Event::Register( "media/Coldfish/About", "Calls the about box",GetApp(), CF_GUI_ABOUT);	
		} catch( ... )
		{
			return( -1 );
		}
		return( 0 );
	}
private:
	os::Event* pcPlayEvent;
	os::Event* pcStopEvent;
	os::Event* pcPauseEvent;
	os::Event* pcNextEvent;
	os::Event* pcPrevEvent;
	os::Event* pcGetSongEvent;
	os::Event* pcGetPlayStateEvent;
	os::Event* pcAddFileEvent;
	os::Event* pcAboutEvent;
};

class CFRemoteEntry : public ColdFishPluginEntry
{
public:
	uint GetPluginCount() { return( 1 ); }
	ColdFishPlugin* GetPlugin( uint nIndex ) { return( new CFRemote() ); }
};

extern "C"
{
	ColdFishPluginEntry* init_coldfish_plugin()
	{
		return( new CFRemoteEntry() );
	}
}


