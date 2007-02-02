/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2006 Syllable Team
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

#include <cassert>
#include <util/event.h>
#include <util/application.h>
#include <util/messenger.h>
#include <appserver/protocol.h>

using namespace os;

class Event::Private
{
public:
	Private()
	{
		m_zID = "";
		m_pcTarget = NULL;
		m_bIsRegistered = false;
		m_bIsRemote = false;
		m_bMonitorEnabled = false;
	}
	void* m_pID;
	String m_zID;
	Handler* m_pcTarget;
	Messenger m_cServerLink;
	bool m_bIsRegistered;
	bool m_bIsRemote;
	bool m_bMonitorEnabled;
	
	port_id m_nRemotePort;
	uint32 m_nRemoteHandler;
	int m_nRemoteMsgCode;
	proc_id m_nRemoteProcess;
	os::String m_zRemoteDescription;
};


/** Creates a new event.
 * \par Description:
 * The event constructor prepares everything to make it possible to register or call events.
 * The os::Application object has to be set up before you create a new os::Event object.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 */
Event::Event()
{
	assert( os::Application::GetInstance() != NULL );
	m = new Private();
	port_id hPort = os::Application::GetInstance()->GetServerPort();
	m->m_cServerLink = Messenger( hPort );
}


/** Deletes an event
 * \par Description:
 * The event destructor deletes a registered event.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 */
Event::~Event()
{
	if( m->m_bIsRegistered )
	{
		/* Unregister our call */
		assert( m->m_pcTarget != NULL );
		os::Message cMessage( EV_UNREGISTER );
	
		cMessage.AddString( "id", m->m_zID );
		cMessage.AddInt64( "process", get_process_id( NULL ) );
		cMessage.AddInt64( "target", m->m_pcTarget->GetLooper()->GetMsgPort() );
	
		m->m_cServerLink.SendMessage( &cMessage );
	}
	if( m->m_bMonitorEnabled )
	{
		/* Delete monitor */
		os::Message cMessage( EV_REMOVE_MONITOR );
	
		cMessage.AddString( "id", m->m_zID );
		cMessage.AddInt64( "process", get_process_id( NULL ) );
		cMessage.AddInt64( "target", m->m_pcTarget->GetLooper()->GetMsgPort() );
		
		m->m_cServerLink.SendMessage( &cMessage );
	
	}
	delete( m );
}




/** Registers an event.
 * \par Description:
 * Registers an event that can be used for interprocess communication. All events
 * are identified using an id string that has the form class/function/subfunction.
 * An example: class/Mail/CreateNewMail.
 * \param zID - ID string.
 * \param zDescription - Description of the call.
 * \param pcTarget - Handler that will receive the calls. It needs to be added to a handler.
 * \param nMessageCode - The code of the message that will be sent to the looper. 
 *                       The message can include additional data provided by the caller.
 * \param nFlags - Flags.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
Event* Event::Register( String zID, String zDescription, os::Handler* pcTarget, int nMessageCode, uint32 nFlags )
{
	Event* pcEvent = new Event();
	
	assert( pcTarget != NULL && pcTarget->GetLooper() != NULL );
		
	os::Message cMessage( EV_REGISTER );
	
	cMessage.AddString( "id", zID );
	cMessage.AddInt64( "process", get_process_id( NULL ) );
	cMessage.AddString( "description", zDescription );
	cMessage.AddInt64( "target", pcTarget->GetLooper()->GetMsgPort() );
	cMessage.AddInt64( "token", pcTarget->GetToken() );
	cMessage.AddInt64( "message_code", nMessageCode );

	pcEvent->m->m_cServerLink.SendMessage( &cMessage );
	pcEvent->m->m_zID = zID;
	pcEvent->m->m_pcTarget = pcTarget;
	pcEvent->m->m_bIsRegistered = true;
	
	return( pcEvent );
}



/** Sets the object to a specific event.
 * \par Description:
 * Sets the object to a specific remote event. If the index is set to -1 then only
 * the id string is saved. This allows you to add a monitor to an event that doesn’t
 * exist yet.
 * \par Note:
 * This method will fail if this object already monitors an event or is a registered
 * event.
 * \param zID - ID string.
 * \param nIndex - Index of the string.
 * \return 0 if successful. Otherwise the call might not exist. The object is then
 * in an invalid state.
 * \sa Unset()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Event::SetToRemote( String zID, int nIndex )
{
	if( m->m_bMonitorEnabled || m->m_bIsRegistered )
		return( -EINVAL );
		
	Unset();

	if( nIndex == -1 )
	{
		/* Just set ID string */
		m->m_zID = zID;
		return( 0 );
	}
	
	/* Ask the appserver for the event information */
	
	os::Message cReply;
	os::Message cMessage( EV_GET_INFO );
	
	cMessage.AddString( "id", zID );
	cMessage.AddInt64( "index", nIndex );
	
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	if( cReply.GetCode() != 0 ) 
	{
		m->m_zID = "";
		return( -ENOENT );
	}
	
	int64 nTargetPort;
	int64 nMessageCode;
	int64 nToken = -1;
	String zDesc;
	int64 nProcess;
	
	cReply.FindInt64( "target", &nTargetPort );
	cReply.FindInt64( "message_code", &nMessageCode );
	cReply.FindString( "description", &zDesc );
	cReply.FindInt64( "token", &nToken );
	cReply.FindInt64( "process", &nProcess );
	
	m->m_zID = zID;
	m->m_nRemotePort = nTargetPort;
	m->m_nRemoteHandler = nToken;
	m->m_nRemoteMsgCode = nMessageCode;
	m->m_nRemoteProcess = nProcess;
	m->m_zRemoteDescription = zDesc;
	
	//printf( "Event set to %i:%i:%i\n", nTargetPort, (int)nToken, nMessageCode );
	
	m->m_bIsRemote = true;
	
	return( 0 );
}




/** Unsets the event.
 * \par Description:
 * This method removes all references to the event.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void Event::Unset()
{
	if( m->m_bIsRemote )
	{
		m->m_bIsRemote = false;
	}
}

/** Returns whether the object is set to a remote event.
 * \par Description:
 * This method will return true if the object was set to a remote event
 * using the SetToRemote() method.
 * \par Note:
 * If you passed -1 as the index to SetToRemote() then this method will return false.
 * \return true is set to a remote event.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

bool Event::IsRemote()
{
	return( m->m_bIsRemote );
}


/** Returns information about a remote event.
 * \par Description:
 * This method will return 0 if this event was successful. The provided variables
 * will then contain information about the event.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Event::GetRemoteInfo( proc_id* pnProcess, port_id* phPort, int* pnMessageCode, os::String* pzDesc )
{
	if( !IsRemote() )
		return( -EINVAL );
		
	if( pnProcess )
		*pnProcess = m->m_nRemoteProcess;
	if( phPort )
		*phPort = m->m_nRemotePort;
	if( pnMessageCode )
		*pnMessageCode = m->m_nRemoteMsgCode;
	if( pzDesc )
		*pzDesc = m->m_zRemoteDescription;
	
	return( 0 );
}

/** Returns a list of children.
 * \par Description:
 * This method will fill the provided list with the names of all children of an event node,
 * e.g. callling this for an event string called class/Mail might result in the
 * list containing entries like CreateNewMail.
 * \par Note:
 * You should call SetToRemote() if the index -1 before because not every event node might
 * contain its own events.
 * The provided list is not cleared by this method.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Event::GetRemoteChildren( std::vector<os::String>* pacList )
{
	if( pacList == NULL )
		return( -EINVAL );
	
	/* Ask the appserver for the children information */
	
	os::Message cReply;
	os::Message cMessage( EV_GET_CHILDREN );
	
	cMessage.AddString( "id", m->m_zID );
	
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	if( cReply.GetCode() != 0 ) 
	{
		return( -ENOENT );
	}
	
	int nIndex = 0;
	os::String zChild;
	while( cReply.FindString( "child", &zChild, nIndex ) == 0 )
	{
		pacList->push_back( zChild );
		nIndex++;
	}
	
	return( 0 );
}


/** Enables/Disables a monitor to an id string.
 * \par Description:
 * With this method you can enable or disable a monitor to an id string. Please
 * note that the monitor will cover all events with the specified id string, the index
 * does not matter. You need to call SetToRemote() before you can use this feature. 
 * The received message will contain a boolean field "event_registered", "event_unregistered"
 * or none if it is a notification message.
 * \par Note:
 * You can also add '*' to the end of the string to monitor events that start with this
 * string.
 * \param bEnabled - Enables/Disables the monitor.
 * \param pcTarget - Handler that should receive the monitor messages.
 * \param nMessageCode - The code of the message that will be sent to the looper. 
 *                       The message can include additional data provided by the caller.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Event::SetMonitorEnabled( bool bEnabled, os::Handler* pcTarget, int nMessageCode )
{
	assert( nMessageCode != -1 || !bEnabled );
	assert( m->m_zID != "" );
	
	if( bEnabled == m->m_bMonitorEnabled )
		return( -EINVAL );
	
	if( m->m_bMonitorEnabled )
	{
		/* Delete monitor */
		assert( m->m_pcTarget != NULL && pcTarget->GetLooper() != NULL );
		os::Message cMessage( EV_REMOVE_MONITOR );
	
		cMessage.AddString( "id", m->m_zID );
		cMessage.AddInt64( "process", get_process_id( NULL ) );
		cMessage.AddInt64( "target", m->m_pcTarget->GetLooper()->GetMsgPort() );
		
		m->m_cServerLink.SendMessage( &cMessage );
		m->m_bMonitorEnabled = false;
		return( 0 );
	}
	
	/* Create monitor */
	
	os::Message cReply;
	os::Message cMessage( EV_ADD_MONITOR );
	
	cMessage.AddString( "id", m->m_zID );
	cMessage.AddInt64( "process", get_process_id( NULL ) );
	cMessage.AddInt64( "target", pcTarget->GetLooper()->GetMsgPort() );
	cMessage.AddInt64( "token", pcTarget->GetToken() );
	cMessage.AddInt64( "message_code", nMessageCode );
	
	m->m_cServerLink.SendMessage( &cMessage );
	
	m->m_bMonitorEnabled = true;
	m->m_pcTarget = pcTarget;
	
	return( 0 );
}


/** Returns whether monitoring of the event is enabled.
 * \par Description:
 * This method will return true if the event is monitored.
 * \return true is set to a remote event.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

bool Event::IsMonitorEnabled()
{
	return( m->m_bMonitorEnabled );
}

/** Posts an event.
 * \par Description:
 * This method will post an event. If this object is set to an remote event using SetToRemote()
 * then the message will be sent to the application that has registered the call. If you have
 * registered the event using the Register() method then the message will be sent to all applications
 * that have set a monitor to this id string.
 * \param pcData - Data that will be submitted.
 * \param pcReplyHandler - Handler for the reply message. Can be NULL for remote calls and has
 * to be NULL if you have registered the event.
 * \param nReplyCode - The code for the reply.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Event::PostEvent( Message* pcData, Handler* pcReplyHandler, int nReplyCode )
{
	if( m->m_bIsRegistered )
	{
		/* Send message to the appserver */
		os::Message cMessage( EV_CALL );
		cMessage.AddString( "id", m->m_zID );
		cMessage.AddInt64( "process", get_process_id( NULL ) );
		cMessage.AddMessage( "data", pcData );
		cMessage.AddInt64( "target", m->m_pcTarget->GetLooper()->GetMsgPort() );
		m->m_cServerLink.SendMessage( &cMessage );
		return( 0 );
	} 
	else if( m->m_bIsRemote )
	{
		/* Send message to the remote application */
		os::Message cReply;
		os::Message cMessage( *pcData );

		os::Messenger cLink( m->m_nRemotePort, m->m_nRemoteHandler, -1 );
	
		cMessage.SetCode( m->m_nRemoteMsgCode );
		cMessage.AddInt64( "source_process", get_process_id( NULL ) );
		cMessage.AddInt32( "reply_code", nReplyCode );
	
		return( cLink.SendMessage( &cMessage, pcReplyHandler ) );
	}
	return( -EINVAL );
}



/** Returns the last message sent to the monitors.
 * \par Description:
 * This method will return the last message that has been sent by an event object to its monitors.
 * This can be used to create a "data storage" in the appserver.
 * \par Note:
 * If you have called SetToRemote() with index -1 then the supplied message will itself
 * contain messages with the code "last_message" and the content of every last message
 * of events with the same id string.
 * \param pcReply - The message content will be copied to this message.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Event::GetLastEventMessage( Message* pcReply )
{
	if( !m->m_bIsRemote && m->m_zID == "" )
		return( -EINVAL );
	assert( pcReply != NULL );
	
	os::Message cMessage( EV_GET_LAST_EVENT_MESSAGE );
	cMessage.AddString( "id", m->m_zID );
	if( m->m_bIsRemote ) {
		/* Get last message */
		cMessage.AddInt64( "process", m->m_nRemoteProcess );
		cMessage.AddInt64( "target", m->m_nRemotePort );
	} else
	{
		/* Get all last messages */
		cMessage.AddInt64( "process", -1 );
		cMessage.AddInt64( "target", -1 );
	}
		
	return( m->m_cServerLink.SendMessage( &cMessage, pcReply ) );

}















































