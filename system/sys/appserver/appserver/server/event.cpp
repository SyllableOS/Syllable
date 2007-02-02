/*
 *  The Syllable application server
 *  Copyright (C) 2006 - 2007 Syllable Team
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

SrvEvents::SrvEvents() : m_cLock( "event_list_lock" )
{
	/* Create root event */
	m_psRoot = new SrvEventNode_s( "", "", NULL );
	m_psRoot->AddRef();
}

/* Returns the next segment of the id string */
const char* SrvEvents::ParseEventStringForward( const char* pzString, uint& nLength )
{
	while( nLength > 0 )
	{
		nLength--;
		if( *pzString == '/' )
		{
			return( pzString + 1 );
		}
		pzString++;
	}
	return( pzString + 1 );
}

/* Returns a node of a given name */
SrvEventNode_s* SrvEvents::GetNode( const SrvEventNode_s* psParent, const os::String& cName )
{
	std::vector<SrvEventNode_s*>::const_iterator i = psParent->m_cChildren.begin();
	for( ; i != psParent->m_cChildren.end(); i++ )
		if( (*i)->m_zName == cName )
			return( (*i) );
	return( NULL );
}

/* Checks if a node is present and creates it if not */
SrvEventNode_s* SrvEvents::CheckNode( SrvEventNode_s* psParent, const os::String& cName )
{
	std::vector<SrvEventNode_s*>::const_iterator i = psParent->m_cChildren.begin();
	for( ; i != psParent->m_cChildren.end(); i++ )
		if( (*i)->m_zName == cName )
			return( (*i) );
	
	/* Create it */
	SrvEventNode_s* psEventNode = new SrvEventNode_s( ( ( psParent == m_psRoot ) ? os::String( "" ) : ( psParent->m_zID + "/" ) ) + cName, cName, psParent );
	
	psParent->m_cChildren.push_back( psEventNode );
	psParent->AddRef();
	//dbprintf( "Created node %s as child of %s (refcount %i) %x\n", cName.c_str(), psParent->m_zID.c_str(), psParent->m_nRefCount, (uint)psEventNode );
	return( psEventNode );
}

/* Returns the parent node for a given id string*/
SrvEventNode_s* SrvEvents::GetParentNode( const os::String& zID, os::String& zName, bool bCreateDirs )
{
	/* Parse ID string */
	const char* pzID = zID.c_str();
	uint nLeft = zID.Length();
	SrvEventNode_s* psParent = m_psRoot;
	os::String zEventName = zID;
	
	while( true )
	{
		const char* pzPrevID = pzID;
		pzID = ParseEventStringForward( pzID, nLeft );
		uint nCurrentLength = (uint)( pzID - pzPrevID - 1 );
		if( nLeft == 0 )
		{
			zName = os::String( pzPrevID, nCurrentLength );
			//dbprintf( "Name %s\n", zName.c_str() );			
			return( psParent );
		}
		//dbprintf( "Parent %s\n", os::String( pzPrevID, nCurrentLength ).c_str() );
		if( nCurrentLength == 0 )
			continue;
		if( bCreateDirs )
		{
			if( ( psParent = CheckNode( psParent, os::String( pzPrevID, nCurrentLength ) ) ) == NULL )
			{
				return( NULL );
			}
		}
		else
		{
			if( ( psParent = GetNode( psParent, os::String( pzPrevID, nCurrentLength ) ) ) == NULL )
			{
				return( NULL );
			}
		}
	}
	return( NULL );
}	

/* Register an event */
SrvEvent_s* SrvEvents::RegisterEvent( os::String zID, int64 nProcess, os::String zDescription, 
								int64 nTargetPort, int64 nToken, int64 nMessageCode )
{
	m_cLock.Lock();

	/* Get parent node */
	SrvEventNode_s* psParent = m_psRoot;
	os::String zEventName = zID;
	
	psParent = GetParentNode( zID, zEventName, true );
	
	/* Create node */
	SrvEventNode_s* psNode = CheckNode( psParent, zEventName );
	psNode->AddRef();
	
	/* Create new event */
	SrvEvent_s* psCall;
	psCall = new SrvEvent_s;
	psCall->m_psNode = psNode;
	psCall->m_nProcess = nProcess;
	psCall->m_nTargetPort = nTargetPort;
	psCall->m_nHandlerToken = nToken;
	psCall->m_nMessageCode = nMessageCode;
	psCall->m_zDescription = zDescription;
	
	psNode->m_cEvents.push_back( psCall );
	
	//dbprintf( "Event %s registered by %i:%i:%i (node refcount %i)\n", zID.c_str(), (int)nProcess, (int)nTargetPort, (int)nToken, psNode->m_nRefCount );
	
	
	
	/* Send notification to all monitors of this event and of the parent events */
	os::Message cReply;
	cReply.AddBool( "event_registered", true );
	cReply.AddString( "id", zID );
	cReply.AddInt64( "target", nTargetPort );
	cReply.AddInt64( "token", nToken );
	cReply.AddInt64( "message_code", nMessageCode );
	cReply.AddInt64( "process", nProcess );
			
	psParent = psNode;
	while( psParent != NULL )
	{
		std::vector<SrvEventMonitor_s*>::const_iterator i = psParent->m_cMonitors.begin();
		for( ; i != psParent->m_cMonitors.end(); i++ )
		{
			cReply.SetCode( (*i)->m_nMessageCode );
			os::Messenger cLink( (*i)->m_nTargetPort, (*i)->m_nHandlerToken, -1 );
					
			cLink.SendMessage( &cReply );
						
			//dbprintf( "Notification %s about registration sent to %i\n", zID.c_str(), (int)(*i)->m_nTargetPort );
		}
		psParent = psParent->m_psParent;	
	}
	m_cLock.Unlock();
	return( psCall );	
}

/* Unregister an event */
void SrvEvents::UnregisterEvent( os::String zID, int64 nProcess, int64 nTargetPort )
{
	/* Find parent node */
	m_cLock.Lock();
	
	SrvEventNode_s* psParent;
	os::String zEventName;
	
	psParent = GetParentNode( zID, zEventName );
	if( psParent == NULL )
	{
		dbprintf( "SrvEvents::UnregisterCall(): Call %s by %i not present\n", zID.c_str(), (int)nProcess );	
		m_cLock.Unlock();
		return;
	}
	
	/* Find node */
	SrvEventNode_s* psNode = GetNode( psParent, zEventName );
	if( psNode == NULL )
	{
		dbprintf( "SrvEvents::UnregisterCall(): Call %s by %i not present\n", zID.c_str(), (int)nProcess );	
		m_cLock.Unlock();
		return;
	}
	
	/* Find event */
	psNode->AddRef();
	std::vector<SrvEvent_s*>::iterator i = psNode->m_cEvents.begin();
	while( i != psNode->m_cEvents.end() )
	{
		SrvEvent_s* psEvent = (*i);
		if( psEvent->m_nProcess == nProcess
			&& ( nTargetPort == -1 || ( psEvent->m_nTargetPort == nTargetPort ) ) )
		{
			/* Send notifications to the monitors */
			os::Message cReply;
			cReply.AddBool( "event_unregistered", true );
			cReply.AddString( "id", zID );
			cReply.AddInt64( "target", psEvent->m_nTargetPort );
			cReply.AddInt64( "token", psEvent->m_nHandlerToken );
			cReply.AddInt64( "message_code", psEvent->m_nMessageCode );
			cReply.AddInt64( "process", psEvent->m_nProcess );
			
			psParent = psNode;
			while( psParent != NULL )
			{
				std::vector<SrvEventMonitor_s*>::iterator j = psParent->m_cMonitors.begin();
				for( ; j != psParent->m_cMonitors.end(); j++ )
				{
					cReply.SetCode( (*j)->m_nMessageCode );
					os::Messenger cLink( (*j)->m_nTargetPort, (*j)->m_nHandlerToken, -1 );
					
					cLink.SendMessage( &cReply );
					
					//dbprintf( "Notification %s about unregistration sent to %i\n", zID.c_str(), (int)(*j)->m_nTargetPort );
				}
				psParent = psParent->m_psParent;	
			}
			
			i = psNode->m_cEvents.erase( i );
			delete( psEvent );
			psNode->Release();			
			//dbprintf( "Event %s unregistered by %i\n", zID.c_str(), (int)nProcess );
		} else
			i++;
	}
	/* Release the node */
	psNode->Release();
			
	m_cLock.Unlock();
	
	//dbprintf( "SrvEvents::UnregisterCall(): Call %s by %i unregistered\n", zID.c_str(), (int)nProcess );	
}

/* Locks an event */
void SrvEvents::GetEventInfo( os::Message* pcMessage )
{
	os::String zID;
	os::String zName;
	int64 nIndex;
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
	
	m_cLock.Lock();
	
	SrvEventNode_s* psParent;
	os::String zEventName;
	
	psParent = GetParentNode( zID, zEventName );
	if( psParent == NULL )
	{
		//dbprintf( "SrvEvents::GetEventInfo(): Event %s with index %i not present\n", zID.c_str(), (int)nIndex );
		m_cLock.Unlock();
		pcMessage->SendReply( -ENOENT );
		return;
	}
	
	/* Find node */
	SrvEventNode_s* psNode = GetNode( psParent, zEventName );
	if( psNode == NULL )
	{
		//dbprintf( "SrvEvents::GetEventInfo(): Event %s with index %i not present\n", zID.c_str(), (int)nIndex );
		m_cLock.Unlock();
		pcMessage->SendReply( -ENOENT );
		return;
	}
	
	/* Send event info if the index is valid */
	if( nIndex >= psNode->m_cEvents.size() )
	{
		//dbprintf( "SrvEvents::GetEventInfo(): Event %s with index %i not present\n", zID.c_str(), (int)nIndex );
		m_cLock.Unlock();
		pcMessage->SendReply( -ENOENT );
		return;
	}
	
	cReply.AddInt64( "target", psNode->m_cEvents[nIndex]->m_nTargetPort );
	cReply.AddInt64( "token", psNode->m_cEvents[nIndex]->m_nHandlerToken );
	cReply.AddInt64( "process", psNode->m_cEvents[nIndex]->m_nProcess );
	cReply.AddInt64( "message_code", psNode->m_cEvents[nIndex]->m_nMessageCode );
	cReply.AddString( "description", psNode->m_cEvents[nIndex]->m_zDescription );
							
	pcMessage->SendReply( &cReply );
	m_cLock.Unlock();
}

/* Sends a list of already registered events to the target */
void SrvEvents::SendEventList( const os::Messenger& cLink, const int nMessageCode, SrvEventNode_s* psNode )
{
	/* Send notifications about already present calls */
	std::vector<SrvEvent_s*>::const_iterator i = psNode->m_cEvents.begin();
	for( ; i != psNode->m_cEvents.end(); i++ )
	{
		os::Message cReply( (*i)->m_cLastMessage );
		cReply.SetCode( nMessageCode );
		cReply.AddBool( "event_registered", true );
		cReply.AddString( "id", psNode->m_zID );
		cReply.AddInt64( "target", (*i)->m_nTargetPort );
		cReply.AddInt64( "token", (*i)->m_nHandlerToken );
		cReply.AddInt64( "message_code", (*i)->m_nMessageCode );
		cLink.SendMessage( &cReply );
		
		//printf( "Send event %s to new monitor\n", psNode->m_zID.c_str() );
	}
	
	/* Recurse */
	std::vector<SrvEventNode_s*>::const_iterator j = psNode->m_cChildren.begin();
	for( ; j != psNode->m_cChildren.end(); j++ )
	{
		SendEventList( cLink, nMessageCode, (*j) );
	}
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
	
	m_cLock.Lock();	
	
	/* Get parent node */
	SrvEventNode_s* psParent = m_psRoot;
	os::String zEventName = zID;
	
	psParent = GetParentNode( zID, zEventName, true );
	
	/* Create node */
	SrvEventNode_s* psNode = CheckNode( psParent, zEventName );
	psNode->AddRef();
	
	/* Create new notification object */
	psMon = new SrvEventMonitor_s;
	psMon->m_nProcess = nProcess;
	psMon->m_zID = zID;
	psMon->m_nTargetPort = nTargetPort;
	psMon->m_nHandlerToken = nToken;
	psMon->m_nMessageCode = nMessageCode;
	
	psNode->m_cMonitors.push_back( psMon );
	
	//dbprintf( "Monitor %s registered by %i:%i (refcount %i)\n", zID.c_str(), (int)nProcess, (int)nTargetPort, psNode->m_nRefCount );
	
	os::Messenger cLink( nTargetPort, nToken, -1 );
					
	SendEventList( cLink, nMessageCode, psNode );
	
	m_cLock.Unlock();	
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
	
	m_cLock.Lock();
	SrvEventNode_s* psParent;
	os::String zEventName;
	
	/* Get parent node */	
	psParent = GetParentNode( zID, zEventName );
	if( psParent == NULL )
	{
		dbprintf( "SrvEvents::RemoveMonitor(): Monitor %s by %i not present\n", zID.c_str(), (int)nProcess );
		m_cLock.Unlock();
		return;
	}
	
	/* Find node */
	SrvEventNode_s* psNode = GetNode( psParent, zEventName );
	if( psNode == NULL )
	{
		dbprintf( "SrvEvents::RemoveMonitor(): Monitor %s by %i not present\n", zID.c_str(), (int)nProcess );
		m_cLock.Unlock();
		return;
	}
	
	/* Remove monitor */
	psNode->AddRef();
	std::vector<SrvEventMonitor_s*>::iterator i = psNode->m_cMonitors.begin();
	while( i != psNode->m_cMonitors.end() )
	{
		if( (*i)->m_nProcess == nProcess && ( (*i)->m_nTargetPort == nTargetPort ) )
		{
			SrvEventMonitor_s* psMonitor = (*i);
			i = psNode->m_cMonitors.erase( i );
			delete( psMonitor );
			psNode->Release();
		}
		else
			i++;
	}
	/* Release the node. */
	psNode->Release();
	m_cLock.Unlock();			
	//dbprintf( "SrvEvents::RemoveMonitor(): Monitor %s by %i removed\n", zID.c_str(), (int)nProcess );
}

/* Send a notification */
void SrvEvents::PostEvent( SrvEvent_s* psEvent, os::Message* pcData )
{
	/* Save message */
	pcData->AddString( "id", psEvent->m_psNode->m_zID );
	psEvent->m_cLastMessage = *pcData;
	
	
	m_cLock.Lock();
	SrvEventNode_s* psParent = psEvent->m_psNode;
	while( psParent != NULL )
	{
		std::vector<SrvEventMonitor_s*>::const_iterator i = psParent->m_cMonitors.begin();
		for( ; i != psParent->m_cMonitors.end(); i++ )
		{
			pcData->SetCode( (*i)->m_nMessageCode );
	
			os::Messenger cLink( (*i)->m_nTargetPort, (*i)->m_nHandlerToken, -1 );
			cLink.SendMessage( pcData );
				
			//dbprintf( "Notification %s sent to %i\n", psParent->m_zID.c_str(), (int)(*i)->m_nTargetPort );
		}
		psParent = psParent->m_psParent;
	}
	m_cLock.Unlock();
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
	
	m_cLock.Lock();
	
	SrvEventNode_s* psParent;
	os::String zEventName;

	/* Get parent node */	
	psParent = GetParentNode( zID, zEventName );
	if( psParent == NULL )
	{
		dbprintf( "SrvEvents::PostEvent(): Event %s by %i not present\n", zID.c_str(), (int)nProcess );
		m_cLock.Unlock();
		return;
	}
	
	/* Find node */
	SrvEventNode_s* psNode = GetNode( psParent, zEventName );
	if( psNode == NULL )
	{
		dbprintf( "SrvEvents::PostEvent(): Event %s by %i not present\n", zID.c_str(), (int)nProcess );
		m_cLock.Unlock();
		return;
	}
	
	/* Find event */
	std::vector<SrvEvent_s*>::const_iterator i = psNode->m_cEvents.begin();
	for( ; i != psNode->m_cEvents.end(); i++ )
	{
		if( (*i)->m_nProcess == nProcess
			&& ( nTargetPort == -1 || ( (*i)->m_nTargetPort == nTargetPort ) ) )
		{
			PostEvent( (*i), &cData );
			m_cLock.Unlock();
			return;
		}
	}
	m_cLock.Unlock();
	dbprintf( "SrvEvents::PostEvent(): Event %s by %i not present\n", zID.c_str(), (int)nProcess );
}

/* Delete all objects (events, monitors) for one process */
void SrvEvents::DeleteObjectsForProcess( proc_id hProc, SrvEventNode_s* psNode )
{
	/* Recurse. We need a temporary list here because the node might get removed
		from the child list. We also increase the reference counter of our node
		here to avoid that we get deleted. */
	psNode->AddRef();
	std::vector<SrvEventNode_s*> cTempList;
	cTempList = psNode->m_cChildren;
	std::vector<SrvEventNode_s*>::const_iterator c = cTempList.begin();	
	for( ; c != cTempList.end(); c++ ) {
		DeleteObjectsForProcess( hProc, (*c) );
	}

	
	/* Delete monitors */
	std::vector<SrvEventMonitor_s*>::iterator j = psNode->m_cMonitors.begin();
	while( j != psNode->m_cMonitors.end() )
	{
		if( (*j)->m_nProcess == hProc )
		{
			dbprintf( "Process %i forgot to delete monitor %s\n", (int)hProc, psNode->m_zID.c_str() );
			SrvEventMonitor_s* psMonitor = (*j);
			j = psNode->m_cMonitors.erase( j );
			delete( psMonitor );
			psNode->Release();
		}
		else
			j++;
	}
	
	/* Delete events */
	std::vector<SrvEvent_s*>::iterator i = psNode->m_cEvents.begin();
	while( i != psNode->m_cEvents.end() )
	{
		if( (*i)->m_nProcess == hProc )
		{
			os::Message cData;
			cData.AddBool( "event_unregistered", true );
			cData.AddString( "id", psNode->m_zID );
			cData.AddInt64( "target", (*i)->m_nTargetPort );
			cData.AddInt64( "token", (*i)->m_nHandlerToken );
			cData.AddInt64( "message_code", (*i)->m_nMessageCode );
			cData.AddInt64( "process", hProc );
			
			SrvEventNode_s* psParent = psNode;
			while( psParent != NULL )
			{
				std::vector<SrvEventMonitor_s*>::const_iterator m = psParent->m_cMonitors.begin();
				for( ; m != psParent->m_cMonitors.end(); m++ )
				{
					if( (*m)->m_nProcess != hProc )
					{
						cData.SetCode( (*m)->m_nMessageCode );
						os::Messenger cLink( (*m)->m_nTargetPort, (*m)->m_nHandlerToken, -1 );
						dbprintf( "Notification %s about unregistration sent to %i\n", psNode->m_zID.c_str(), (int)(*m)->m_nTargetPort );	
						cLink.SendMessage( &cData );
					}
				}
				psParent = psParent->m_psParent;	
			}
			
			
			dbprintf( "Process %i forgot to delete event %s\n", (int)hProc, psNode->m_zID.c_str() );			
			SrvEvent_s* psEvent = (*i);
			i = psNode->m_cEvents.erase( i );
			delete( psEvent );
			psNode->Release();
		}
		else
			i++;
	}
	psNode->Release();
}

/* Called when a process is killed. Check the database users and calls */
void SrvEvents::ProcessKilled( proc_id hProc )
{
	/* Delete events owned by the process */
	m_cLock.Lock();
	DeleteObjectsForProcess( hProc, m_psRoot );
	m_cLock.Unlock();
	
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
	
	m_cLock.Lock();
	
	SrvEventNode_s* psParent;
	os::String zEventName;
	
	/* Get parent node */
	psParent = GetParentNode( zID, zEventName );
	if( psParent == NULL )
	{
		dbprintf( "SrvEvents::GetLastEventMessage(): Event %s not present\n", zID.c_str() );
		m_cLock.Unlock();
		pcMessage->SendReply( -ENOENT );
		return;
	}
	
	/* Find node */
	SrvEventNode_s* psNode = GetNode( psParent, zEventName );
	if( psNode == NULL )
	{
		dbprintf( "SrvEvents::GetLastEventMessage(): Event %s not present\n", zID.c_str() );
		m_cLock.Unlock();
		pcMessage->SendReply( -ENOENT );
		return;
	}
	
	if( nProcess == -1 )
	{
		/* Return all last messages of events with the same id string */
		os::Message cReply;
		
		std::vector<SrvEvent_s*>::const_iterator i = psNode->m_cEvents.begin();
		for( ; i != psNode->m_cEvents.end(); i++ )
			cReply.AddMessage( "last_message", &(*i)->m_cLastMessage );
	
		pcMessage->SendReply( &cReply );
		m_cLock.Unlock();
		return;
	}
	
	/* Find event */
	std::vector<SrvEvent_s*>::const_iterator i = psNode->m_cEvents.begin();
	for( ; i != psNode->m_cEvents.end(); i++ )
		if( (*i)->m_nProcess == nProcess && ( nTargetPort == -1 || ( (*i)->m_nTargetPort == nTargetPort ) ) )
		{
			pcMessage->SendReply( &(*i)->m_cLastMessage );
			m_cLock.Unlock();
			return;
		}
	
	m_cLock.Unlock();
	pcMessage->SendReply( -ENOENT );
	dbprintf( "SrvEvents::GetLastEventMessage(): Event %s not present\n", zID.c_str() );
}

/* Returns a list of children of one event node */
void SrvEvents::GetChildren( os::Message* pcMessage )
{
	os::String zID;
	
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "SrvEvents::GetEventInfo() called without id\n" );
		return;
	}
	
	
	m_cLock.Lock();
	
	SrvEventNode_s* psParent;
	SrvEventNode_s* psNode;
	os::String zEventName;
	
	if( zID == "/" || zID == "" )
	{
		psNode = m_psRoot;
	}
	else
	{
	
		/* Get parent node */
		psParent = GetParentNode( zID, zEventName );
		if( psParent == NULL )
		{
			dbprintf( "SrvEvents::GetChildren(): Event %s not present\n", zID.c_str() );
			m_cLock.Unlock();
			pcMessage->SendReply( -ENOENT );
			return;
		}
	
		/* Find node */
		psNode = GetNode( psParent, zEventName );
		if( psNode == NULL )
		{
			dbprintf( "SrvEvents::GetChildren(): Event %s not present\n", zID.c_str() );
			m_cLock.Unlock();
			pcMessage->SendReply( -ENOENT );
			return;
		}
	}
	
	if( psNode->m_cChildren.empty() )
	{
		m_cLock.Unlock();
		pcMessage->SendReply( -ENOENT );
		return;
	}
	
	/* Add the names of all children */
	os::Message cReply( 0 );
	std::vector<SrvEventNode_s*>::const_iterator c = psNode->m_cChildren.begin();	
	for( ; c != psNode->m_cChildren.end(); c++ ) {
		cReply.AddString( "child", (*c)->m_zName );
	}
	
	pcMessage->SendReply( &cReply );
	
	m_cLock.Unlock();
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
		case EV_GET_CHILDREN:
			GetChildren( pcMessage );
		break;
		default:
			dbprintf( "Error: Unknown event message received\n" );
		break;
	}
}


























