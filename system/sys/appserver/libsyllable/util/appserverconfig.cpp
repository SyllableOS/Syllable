
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#include <util/appserverconfig.h>
#include <util/application.h>
#include <util/messenger.h>

#include <appserver/protocol.h>

#include <stdexcept>

using namespace os;

AppserverConfig::AppserverConfig()
{
	if( Messenger( Application::GetInstance()->GetServerPort(  ) ).SendMessage( DR_GET_APPSERVER_CONFIG, &m_cConfig ) != 0 )
	{
		throw( std::runtime_error( "Failed to request configuration" ) );
	}
	m_bModified = false;
}

AppserverConfig::~AppserverConfig()
{
	if( m_bModified )
	{
		Commit();
	}
}

status_t AppserverConfig::Commit()
{
	if( m_bModified == false )
	{
		return ( 0 );
	}
	m_cConfig.SetCode( DR_SET_APPSERVER_CONFIG );
	if( Messenger( Application::GetInstance()->GetServerPort(  ) ).SendMessage( &m_cConfig ) != 0 )
	{
		return ( -1 );
	}
	m_bModified = false;
	return ( 0 );
}

status_t AppserverConfig::SetPopoupSelectedWindows( bool bPopup )
{
	m_cConfig.RemoveName( "popoup_sel_win" );
	m_cConfig.AddBool( "popoup_sel_win", bPopup );
	m_bModified = true;
	return ( 0 );
}

bool AppserverConfig::GetPopoupSelectedWindows() const
{
	bool bPopup = false;

	m_cConfig.FindBool( "popoup_sel_win", &bPopup );
	return ( bPopup );
}

status_t AppserverConfig::SetDoubleClickTime( bigtime_t nDelay )
{
	m_cConfig.RemoveName( "doubleclick_delay" );
	m_cConfig.AddInt64( "doubleclick_delay", nDelay );
	m_bModified = true;
	return ( 0 );
}

bigtime_t AppserverConfig::GetDoubleClickTime() const
{
	bigtime_t nDelay = 50000LL;

	m_cConfig.FindInt64( "doubleclick_delay", &nDelay );
	return ( nDelay );
}

status_t AppserverConfig::SetKeyDelay( bigtime_t nDelay )
{
	m_cConfig.RemoveName( "key_delay" );
	m_cConfig.AddInt64( "key_delay", nDelay );
	m_bModified = true;
	return ( 0 );
}

status_t AppserverConfig::SetKeyRepeat( bigtime_t nDelay )
{
	m_cConfig.RemoveName( "key_repeat" );
	m_cConfig.AddInt64( "key_repeat", nDelay );
	m_bModified = true;
	return ( 0 );
}

bigtime_t AppserverConfig::GetKeyDelay() const
{
	bigtime_t nDelay = 300000LL;

	m_cConfig.FindInt64( "key_delay", &nDelay );
	return ( nDelay );
}

bigtime_t AppserverConfig::GetKeyRepeat() const
{
	bigtime_t nDelay = 30000LL;

	m_cConfig.FindInt64( "key_repeat", &nDelay );
	return ( nDelay );
}
