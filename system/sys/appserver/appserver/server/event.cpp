/*
 *  The Syllable application server
 *  Copyright (C) 2006 - Syllable Team
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <util/messenger.h>
#include <atheos/kernel.h>
#include <appserver/protocol.h>
#include "event.h"

using namespace os;

/* Register an event */
SrvEvent_s* SrvEvents::RegisterEvent( os::String zID, int64 nProcess, os::String zDescription, 
								int64 nTargetPort, int64 nToken, int64 nMessageCode )
{
	/* Create new call object */
	SrvEvent_s* psCall;
	psCall = new SrvEvent_s;
	psCall->m_nProcess = nProcess;
	psCall->m_zID = zID;
	psCall->m_nTargetPort = nTargetPort;
	psCall->m_nHandlerToken = nToken;
	psCall->m_nMessageCode = nMessageCode;
	psCall->m_zDescription = zDescription;
	psCall->m_cMonitors.clear();
	
	m_cEvents.push_back( psCall );
	
	//dbprintf( "Event %s registered by %i:%i:%i\n", zID.c_str(), (int)nProcess, (int)nTargetPort, (int)nToken );
	
	/* Send notifications to the monitors */
	for( uint j = 0; j < m_cMonitors.size(); j++ )
	{
		if( m_cMonitors[j]->m_zID == zID.substr( 0, m_cMonitors[j]->m_zID.Length() ) )
		{
			//dbprintf( "Add monitor %i to the event\n", (int)m_cMonitors[j]->m_nTargetPort );
			psCall->m_cMonitors.push_back( m_cMonitors[j] );
			
			os::Message cReply( m_cMonitors[j]->m_nMessageCode );
			cReply.AddBool( "event_registered", true );
			cReply.AddString( "id", zID );
			cReply.AddInt64( "target", nTargetPort );
			cReply.AddInt64( "token", nToken );
			cReply.AddInt64( "message_code", nMessageCode );
			cReply.AddString( "description", zDescription );
			cReply.AddInt64( "process", nProcess );
			
			os::Messenger cLink( m_cMonitors[j]->m_nTargetPort, m_cMonitors[j]->m_nHandlerToken, -1 );
					
			cLink.SendMessage( &cReply );
						
			//dbprintf( "Notification %s about registration sent to %i\n", zID.c_str(), (int)m_cMonitors[j]->m_nTargetPort );
		}
	}		
	return( psCall );	
}

/* Unregister an event */
void SrvEvents::UnregisterEvent( os::String zID, int64 nProcess, int64 nTargetPort )
{
	/* Find call */
	for( uint i = 0; i < m_cEvents.size(); i++ )
	{
		if( m_cEvents[i]->m_zID == zID && m_cEvents[i]->m_nProcess == nProcess
			&& ( nTargetPort == -1 || ( m_cEvents[i]->m_nTargetPort == nTargetPort ) ) )
		{
			
			/* Send notifications and add monitors */
			os::Message cData;
			cData.AddBool( "event_unregistered", true );
			for( uint j = 0; j < m_cEvents[i]->m_cMonitors.size(); j++ )
			{
				os::Message cReply( m_cEvents[i]->m_cMonitors[j]->m_nMessageCode );
				cReply.AddBool( "event_unregistered", true );
				cReply.AddString( "id", zID );
				cReply.AddInt64( "target", m_cEvents[i]->m_nTargetPort );
				cReply.AddInt64( "token", m_cEvents[i]->m_nHandlerToken );
				cReply.AddInt64( "message_code", m_cEvents[i]->m_nMessageCode );
				cReply.AddInt64( "process", m_cEvents[i]->m_nProcess );
			
				os::Messenger cLink( m_cEvents[i]->m_cMonitors[j]->m_nTargetPort, m_cEvents[i]->m_cMonitors[j]->m_nHandlerToken, -1 );
					
				cLink.SendMessage( &cReply );
					
				//dbprintf( "Notification %s about unregistration sent to %i\n", zID.c_str(), (int)m_cEvents[i]->m_cMonitors[j]->m_nTargetPort );
			}
			SrvEvent_s *psEvent = m_cEvents[i];
			
			m_cEvents.erase( m_cEvents.begin() + i );
			
			delete( psEvent );
			
			//dbprintf( "Event %s unregistered by %i\n", zID.c_str(), (int)nProcess );
			
			return;
		}
	}
	
	dbprintf( "SrvEvents::UnregisterCall(): Call %s by %i not present\n", zID.c_str(), (int)nProcess );	
}

/* Locks an event */
void SrvEvents::GetEventInfo( os::Message* pcMessage )
{
	os::String zID;
	int64 nIndex;
	int64 nCounter = 0;
	Message cReply( 0 );
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "SrvEvents::GetEventInfo() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "index", &nIndex ) != 0 ) {
		dbprintf( "SrvEvents::GetEventInfo() called without index\n" );
		return;
	}
	
	/* Find event */
	for( uint i = 0; i < m_cEvents.size(); i++ )
	{
		if( m_cEvents[i]->m_zID == zID  )
		{
			if( nIndex == nCounter )
			{
				cReply.AddInt64( "target", m_cEvents[i]->m_nTargetPort );
				cReply.AddInt64( "token", m_cEvents[i]->m_nHandlerToken );
				cReply.AddInt64( "process", m_cEvents[i]->m_nProcess );
				cReply.AddInt64( "message_code", m_cEvents[i]->m_nMessageCode );
				cReply.AddString( "description", m_cEvents[i]->m_zDescription );
							
				pcMessage->SendReply( &cReply );
				return;
			}
			nCounter++;
		}
	}
	
//	dbprintf( "SrvEvents::GetEventInfo(): Event %s with index %i not present\n", zID.c_str(), (int)nIndex );
	
	pcMessage->SendReply( -ENOENT );
}


/* Add a monitor */
void SrvEvents::AddMonitor( os::Message* pcMessage )
{
	os::String zID;
	int64 nTargetPort;
	int64 nMessageCode;
	int64 nProcess;
	int64 nToken;
	SrvEventMonitor_s* psMon;
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "SrvEvents::AddMonitor() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "SrvEvents::AddMonitorn() called without process\n" );
		return;
	}
	if( pcMessage->FindInt64( "token", &nToken ) != 0 ) {
		dbprintf( "SrvEvents::AddMonitorn() called without handler token\n" );
		return;
	}
	if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
		dbprintf( "SrvEvents::AddMonitorn() called without targer port\n" );
		return;
	}
	if( pcMessage->FindInt64( "message_code", &nMessageCode ) != 0 ) {
		dbprintf( "SrvEvents::AddMonitorn() called without message code\n" );
		return;
	}
	
	/* Create new notification object */
	psMon = new SrvEventMonitor_s;
	psMon->m_nProcess = nProcess;
	psMon->m_zID = zID;
	psMon->m_nTargetPort = nTargetPort;
	psMon->m_nHandlerToken = nToken;
	psMon->m_nMessageCode = nMessageCode;
	
	m_cMonitors.push_back( psMon );
	
	//dbprintf( "Monitor %s registered by %i:%i\n", zID.c_str(), (int)nProcess, (int)nTargetPort );
	
	/* Send notifications about already present calls and add monitors */
	for( uint i = 0; i < m_cEvents.size(); i++ )
	{
		if( m_cEvents[i]->m_zID.substr( 0, zID.Length() ) == zID )
		{
			m_cEvents[i]->m_cMonitors.push_back( psMon );
			
			os::Message cReply( m_cEvents[i]->m_cLastMessage );
			cReply.SetCode( nMessageCode );
			cReply.AddBool( "event_registered", true );
			cReply.AddString( "id", m_cEvents[i]->m_zID );
			cReply.AddInt64( "target", m_cEvents[i]->m_nTargetPort );
			cReply.AddInt64( "token", m_cEvents[i]->m_nHandlerToken );
			cReply.AddInt64( "message_code", m_cEvents[i]->m_nMessageCode );
			cReply.AddString( "description", m_cEvents[i]->m_zDescription );
			os::Messenger cLink( nTargetPort, nToken, -1 );
			cLink.SendMessage( &cReply );
		}
	}
}

/* Remove a monitor */
void SrvEvents::RemoveMonitor( os::Message* pcMessage )
{
	os::String zID;
	int64 nProcess;
	int64 nTargetPort;
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "SrvEvents::RemoveMonitor() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "SrvEvents::RemoveMonitor() called without process\n" );
		return;
	}
	if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
		dbprintf( "SrvEvents::RemoveMonitor() called without target port\n" );
		return;
	}
	
	/* Find call */
	for( uint i = 0; i < m_cMonitors.size(); i++ )
	{
		if( m_cMonitors[i]->m_zID == zID && m_cMonitors[i]->m_nProcess == nProcess
			&& ( m_cMonitors[i]->m_nTargetPort == nTargetPort ) )
		{
			SrvEventMonitor_s* psMonitor = m_cMonitors[i];
			m_cMonitors.erase( m_cMonitors.begin() + i );
			
			//dbprintf( "Monitor %s unregistered by %i\n", zID.c_str(), (int)nProcess );
			
			/* Delete from the event lists */
			for( uint j = 0; j < m_cEvents.size(); j++ )
			{
				for( uint k = 0; k < m_cEvents[j]->m_cMonitors.size(); k++ )
				{
					if( m_cEvents[j]->m_cMonitors[k] == psMonitor )
					{
						//dbprintf( "Removed monitor from event %s\n", m_cEvents[j]->m_zID.c_str() );
						m_cEvents[j]->m_cMonitors.erase( m_cEvents[j]->m_cMonitors.begin() + k );
						break;
					}
				}
			}
			
			delete( psMonitor );
			return;
		}
	}
	
	dbprintf( "SrvEvents::RemoveMonitor(): Monitor %s by %i not present\n", zID.c_str(), (int)nProcess );
}

/* Send a notification */
void SrvEvents::PostEvent( SrvEvent_s* psEvent, os::Message* pcData )
{
	/* Save message */
	pcData->AddString( "id", psEvent->m_zID );
	psEvent->m_cLastMessage = *pcData;
		
	for( uint j = 0; j < psEvent->m_cMonitors.size(); j++ )
	{
		pcData->SetCode( psEvent->m_cMonitors[j]->m_nMessageCode );
	
		os::Messenger cLink( psEvent->m_cMonitors[j]->m_nTargetPort, psEvent->m_cMonitors[j]->m_nHandlerToken, -1 );
		cLink.SendMessage( pcData );
				
		//dbprintf( "Notification %s sent to %i\n", psEvent->m_zID.c_str(), (int)psEvent->m_cMonitors[j]->m_nTargetPort );
	}
}

/* Send a notification */
void SrvEvents::PostEvent( os::Message* pcMessage )
{
	os::String zID;
	int64 nProcess;
	int64 nTargetPort;
	os::Message cData;
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "SrvEvents::PostEvent() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "SrvEvents::PostEvent() called without process\n" );
		return;
	}
	if( pcMessage->FindMessage( "data", &cData ) != 0 ) {
		dbprintf( "SrvEvents::PostEvent() called without data\n" );
		return;
	}
	if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
		dbprintf( "SrvEvents::PostEvent() called without target port\n" );
		return;
	}
	
	cData.AddInt64( "source_process", nProcess );
	
	/* Find call */
	for( uint i = 0; i < m_cEvents.size(); i++ )
	{
		if( m_cEvents[i]->m_zID == zID && m_cEvents[i]->m_nProcess == nProcess
			&& ( nTargetPort == -1 || ( m_cEvents[i]->m_nTargetPort == nTargetPort ) ) )
		{
			PostEvent( m_cEvents[i], &cData );
			return;
		}
	}
	
}

/* Called when a process is killed. Check the database users and calls */
void SrvEvents::ProcessKilled( proc_id hProc )
{
	/* Delete events owned by the process */
again:
	
	for( uint i = 0; i < m_cEvents.size(); i++ )
	{
		if( m_cEvents[i]->m_nProcess == hProc  )
		{
			dbprintf( "Process %i forgot to delete call %s\n", (int)hProc, m_cEvents[i]->m_zID.c_str() );
			/* Send notifications */
			os::Message cData;
			cData.AddBool( "event_unregistered", true );
			cData.AddString( "id", m_cEvents[i]->m_zID );
			
			for( uint j = 0; j < m_cEvents[i]->m_cMonitors.size(); j++ )
			{

				if( m_cEvents[i]->m_cMonitors[j]->m_nProcess != hProc )
				{
					cData.SetCode( m_cEvents[i]->m_cMonitors[j]->m_nMessageCode );
			
					os::Messenger cLink( m_cEvents[i]->m_cMonitors[j]->m_nTargetPort, m_cEvents[i]->m_cMonitors[j]->m_nHandlerToken, -1 );
					
					cLink.SendMessage( &cData );
					
					dbprintf( "Notification %s about unregistration sent to %i\n", m_cEvents[i]->m_zID.c_str(), (int)m_cEvents[i]->m_cMonitors[j]->m_nTargetPort );
				}
			}		
			SrvEvent_s* psEvent = m_cEvents[i];
			m_cEvents.erase( m_cEvents.begin() + i );
			delete( psEvent );
			goto again; /* m_cEvents.size() has changed */
		}
	}
	/* Delete monitors owned by the process */
again_2:
	
	for( uint i = 0; i < m_cMonitors.size(); i++ )
	{
		if( m_cMonitors[i]->m_nProcess == hProc )
		{
			dbprintf( "Process %i forgot to delete monitor %s\n", (int)hProc, m_cMonitors[i]->m_zID.c_str() );

			SrvEventMonitor_s* psMonitor = m_cMonitors[i];

			/* Delete from the event lists */
			for( uint j = 0; j < m_cEvents.size(); j++ )
			{
				for( uint k = 0; k < m_cEvents[j]->m_cMonitors.size(); k++ )
				{
					if( m_cEvents[j]->m_cMonitors[k] == psMonitor )
					{
						m_cEvents[j]->m_cMonitors.erase( m_cEvents[j]->m_cMonitors.begin() + k );
						break;
					}
				}
			}
			
			m_cMonitors.erase( m_cMonitors.begin() + i );
			delete( psMonitor );
			goto again_2; /* m_cMonitors.size() has changed */
		}
	}
}

/* Returns the last message sent by an event to the monitors */
void SrvEvents::GetLastEventMessage( os::Message* pcMessage )
{
	os::String zID;
	int64 nProcess;
	int64 nTargetPort;
	Message cReply( 0 );
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "SrvEvents::GetEventInfo() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "SrvEvents::AddMonitorn() called without process\n" );
		return;
	}
	if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
		dbprintf( "SrvEvents::PostEvent() called without target port\n" );
		return;
	}
	
	if( nProcess == -1 )
	{
		/* Return all last messages of events with the same id string */
		os::Message cReply;
		bool bFound = false;
		for( uint i = 0; i < m_cEvents.size(); i++ )
		{
			if( m_cEvents[i]->m_zID == zID )
			{
				cReply.AddMessage( "last_message", &m_cEvents[i]->m_cLastMessage );
				bFound = true;
			}
		}
		if( bFound )
			pcMessage->SendReply( &cReply );
		else
			pcMessage->SendReply( -ENOENT );
		return;
	}
	
	/* Find event */
	for( uint i = 0; i < m_cEvents.size(); i++ )
	{
		if( m_cEvents[i]->m_zID == zID && m_cEvents[i]->m_nProcess == nProcess
			&& ( nTargetPort == -1 || ( m_cEvents[i]->m_nTargetPort == nTargetPort ) ) )
		{
			pcMessage->SendReply( &m_cEvents[i]->m_cLastMessage );
			return;
		}
	}
	
	pcMessage->SendReply( -ENOENT );
}

void SrvEvents::DispatchMessage( Message * pcMessage )
{
	os::String zID;
	os::String zDescription;
	int64 nTargetPort;
	int64 nMessageCode;
	int64 nProcess;
	int64 nToken;
	
	
	switch( pcMessage->GetCode() )
	{
		case EV_REGISTER:
		{
			
			/* Unpack message */
			if( pcMessage->FindString( "id", &zID ) != 0 ) {
				dbprintf( "SrvEvents::RegisterEvent() called without id\n" );
				return;
			}
			if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
				dbprintf( "SrvEvents::RegisterEvent() called without process\n" );
				return;
			}
			if( pcMessage->FindString( "description", &zDescription ) != 0 ) {
				dbprintf( "SrvEvents::RegisterEvent() called without description\n" );
				return;
			}
			if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
				dbprintf( "SrvEvents::RegisterEvent() called without targer port\n" );
				return;
			}
			if( pcMessage->FindInt64( "token", &nToken ) != 0 ) {
				nToken = -1;
			}
			if( pcMessage->FindInt64( "message_code", &nMessageCode ) != 0 ) {
				dbprintf( "SrvEvents::RegisterEvent() called without message code\n" );
				return;
			}
	
			/* Register event */
			RegisterEvent( zID, nProcess, zDescription, nTargetPort, nToken, nMessageCode );
		}
		break;
		case EV_UNREGISTER:
		{
			/* Unpack message */
			if( pcMessage->FindString( "id", &zID ) != 0 ) {
				dbprintf( "SrvEvents::UnregisterEvent() called without id\n" );
				return;
			}
			if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
				dbprintf( "SrvEvents::RegisterEvent() called without process\n" );
				return;
			}
			if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
				nTargetPort = -1;
			}
			
			/* Unregister event */
			UnregisterEvent( zID, nProcess, nTargetPort );
		}
		break;
		case EV_GET_INFO:
			if( pcMessage->IsSourceWaiting() )
				GetEventInfo( pcMessage );
		break;
		case EV_ADD_MONITOR:
			AddMonitor( pcMessage );
		break;
		case EV_REMOVE_MONITOR:
			RemoveMonitor( pcMessage );
		break;
		case EV_GET_LAST_EVENT_MESSAGE:
			GetLastEventMessage( pcMessage );
		break;
		case EV_CALL:
			PostEvent( pcMessage );
		break;
		default:
			dbprintf( "Error: Unknown event message received\n" );
		break;
	}
}


























