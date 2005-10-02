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
#include <storage/registrar.h>

class CFRemote : public ColdFishPlugin
{
public:
	CFRemote()
	{
	}
	~CFRemote()
	{
		try
		{
			os::RegistrarManager* pcRegManager = os::RegistrarManager::Get();
		
			pcRegManager->UnregisterCall("media/Coldfish/Play");
			pcRegManager->UnregisterCall("media/Coldfish/Stop");
			pcRegManager->UnregisterCall("media/Coldfish/Pause");
			pcRegManager->UnregisterCall("media/Coldfish/Next");
			pcRegManager->UnregisterCall("media/Coldfish/Previous");
			pcRegManager->UnregisterCall("media/Coldfish/GetSong");
			pcRegManager->UnregisterCall("media/Coldfish/GetPlayState");
			pcRegManager->UnregisterCall("media/Coldfish/AddFile");
			pcRegManager->UnregisterCall("media/Coldfish/About");
			pcRegManager->Put();

		} catch( ... ) 
		{
		}
	}
	os::String GetIdentifier()
	{
		return( "Remote" );
	}
	status_t Initialize()
	{
		try
		{
			os::RegistrarManager* pcRegManager = os::RegistrarManager::Get();
		
			/* Register remote calls */
			pcRegManager->RegisterCall("media/Coldfish/Play","Play",GetApp(),CF_GUI_PLAY);
			pcRegManager->RegisterCall("media/Coldfish/Stop","Stop",GetApp(),CF_GUI_STOP);
			pcRegManager->RegisterCall("media/Coldfish/Pause","Pause",GetApp(),CF_GUI_PAUSE);
			pcRegManager->RegisterCall("media/Coldfish/Next","Next",GetApp(),CF_PLAY_NEXT);
			pcRegManager->RegisterCall("media/Coldfish/Previous","Previous",GetApp(),CF_PLAY_PREVIOUS);
			pcRegManager->RegisterCall( "media/Coldfish/GetSong", "Gets the current song",GetApp(), CF_GET_SONG);
			pcRegManager->RegisterCall( "media/Coldfish/GetPlayState", "Gets the current playstate",GetApp(), CF_GET_PLAYSTATE);
			pcRegManager->RegisterCall( "media/Coldfish/AddFile", "Calls the file requester to add a file to ColdFish",GetApp(), CF_GUI_ADD_FILE);			
			pcRegManager->RegisterCall( "media/Coldfish/About", "Calls the about box",GetApp(), CF_GUI_ABOUT);	
			pcRegManager->Put();
		} catch( ... )
		{
			return( -1 );
		}
		return( 0 );
	}
private:
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


