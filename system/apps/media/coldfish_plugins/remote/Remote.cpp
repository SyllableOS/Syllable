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
			pcRegManager->RegisterCall("media/Coldfish/Play",
				"Play",
				GetApp(),
				CF_GUI_PLAY);
			pcRegManager->RegisterCall("media/Coldfish/Stop",
				"Stop",
				GetApp(),
				CF_GUI_STOP);
			pcRegManager->RegisterCall("media/Coldfish/Pause",
				"Pause",
				GetApp(),
				CF_GUI_PAUSE);
			pcRegManager->RegisterCall("media/Coldfish/Next",
				"Next",
				GetApp(),
				CF_PLAY_NEXT);
			pcRegManager->Put();
		} catch( ... )
		{
			return( -1 );
		}
		printf( "Remote Plugin initialized\n" );
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
