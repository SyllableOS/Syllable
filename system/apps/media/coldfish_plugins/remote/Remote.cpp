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
		SetColdFishPluginState(false);
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
		SetColdFishPluginState(true);
		return( 0 );
	}
private:
	status_t SetColdFishPluginState(bool bState)
	{
		/* Connect to the dock and get a list of the currently enabled plugins */
		std::vector<os::String> m_cEnabledPlugins;
	
		try
		{
			os::RegistrarCall_s sCall;
			os::Message cReply;
			os::Message cDummy;
			os::Message cPlugins;
			os::RegistrarManager* m_pcRegManager = os::RegistrarManager::Get();	
			
			/* Get enabled plugins */
			if( m_pcRegManager->QueryCall( "os/Dock/GetPlugins", 0, &sCall ) == 0 )
			{
		
				if( m_pcRegManager->InvokeCall( &sCall, &cDummy, &cReply ) == 0 )
				{
					int i = 0;
					os::String zEnabledPlugin;
					while( cReply.FindString( "plugin", &zEnabledPlugin, i ) == 0 )
					{
						if (zEnabledPlugin == "/system/drivers/dock/ColdFishRemote.so" && bState)
						{
							return 1;
						}
						else
						{
							m_cEnabledPlugins.push_back( zEnabledPlugin );
							i++;
						}
					}
				}
			}
		else
		{
			return -1;
		}
	
		if (bState)
		{
			m_cEnabledPlugins.push_back("/system/drivers/dock/ColdFishRemote.so");
		}
		else
		{
			for (uint i=0; i<m_cEnabledPlugins.size(); i++)
			{
				if (m_cEnabledPlugins[i] == "/system/drivers/dock/ColdFishRemote.so")
				{
					m_cEnabledPlugins.erase(m_cEnabledPlugins.begin() + i);
				}
			}
		}
	
		if( m_pcRegManager->QueryCall( "os/Dock/SetPlugins", 0, &sCall ) == 0 )
		{
			for( uint i = 0; i < m_cEnabledPlugins.size(); i++ )
				cPlugins.AddString( "plugin", m_cEnabledPlugins[i] );
			
			if (m_pcRegManager->InvokeCall( &sCall, &cPlugins, NULL ) == 0)
			{
			}		
		}
		m_pcRegManager->Put();
	}
	
	catch(...)
	{
	}
	return 0;	
}		
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


