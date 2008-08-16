/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#include <util/messenger.h>
#include <util/looper.h>
#include <util/message.h>

#include <atheos/kernel.h>

using namespace os;

/** Default constructor.
 * \par Description:
 *	Initialize the messenger to a known but invalid state.
 *	The messenger must be further initialized before it
 *	can be used to send messages.
 * \sa Looper, Message
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Messenger::Messenger()
{
	m_hPort = -1;
	m_hHandlerID = -1;
	m_hDestProc = -1;
	m_bPreferredTarget = false;
}

/** Construct a messenger targeting a looper and optionally a specific handler.
 * \par Description:
 *	A messenger can target a Looper or specific Handler inside a Looper.
 *	If \p pcHandler is NULL the messenger will not have a specific target.
 *	Messages will be sendt to \p pcLooper and it will be dispatched to the
 *	currently default Handler in that looper. If the looper don't have
 *	a default handler the message will be handled by the loopers own
 *	HandleMessage() member.
 * \par
 *	If \p pcHandler is non-NULL the message will always be sent directly
 *	to this handler independent of which handler is active. In this case
 *	the target looper will be the looper \p pcHandler is a member of and the
 *	\p pcLooper parameter should be NULL. This means that only a handler
 *	that is added to a looper already can be targeted by a messenger.
 * \param pcHandler
 *	If non-NULL this specify the handler that will receive messages
 *	sendt by this messenger. The handler must already be added to
 *	a Looper with os::Looper::AddHandler().
 * \param pcLooper
 *	If \p pcHandler is NULL this parameter must point to valid
 *	os::Looper object. The looper will then receive the messages
 *	sendt and it will be dispatched to the current default handler
 *	or handled by the looper itself if there is no default handler.
 * \sa os::Looper, os::Message
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Messenger::Messenger( const Handler * pcHandler, const Looper * pcLooper )
{
	if( pcLooper == NULL && pcHandler != NULL )
	{
		pcLooper = pcHandler->GetLooper();
	}
	if( pcLooper != NULL )
	{
		m_hDestProc = get_process_id( NULL );
		m_hPort = pcLooper->GetMsgPort();
		if( pcHandler != NULL )
		{
			m_hHandlerID = pcHandler->m_nToken;
			m_bPreferredTarget = true;
		}
		else
		{
			m_hHandlerID = -1;
			m_bPreferredTarget = false;
		}
	}
	else
	{
		m_hPort = -1;
		m_hDestProc = -1;
		m_hHandlerID = -1;
		m_bPreferredTarget = false;
	}
}

/** Copy contructor
 * \par Description:
 *	Copy contructor
 * \param cMessenger
 *	The messenger to copy.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Messenger::Messenger( const Messenger & cMessenger )
{
	m_hPort = cMessenger.m_hPort;
	m_hHandlerID = cMessenger.m_hHandlerID;
	m_hDestProc = cMessenger.m_hDestProc;
	m_bPreferredTarget = cMessenger.m_bPreferredTarget;
}

/** Construct a messenger from a loopers message port.
 * \par Description:
 *	This constructor is mostly intended for internal usage by the toolkit
 *	but are made available to applications. It will construct a messenger
 *	that send messages to a specific message port. This should primarily
 *	be a message port belonging to an os::Looper. This can be useful to
 *	establish communication between mostly unrelated processes since unlike
 *	os::Looper and os::Handler pointers the message port ID can be passed
 *	between processes without being invalidated.
 * \note
 *	It is not possible to target a specific os::Handler using this
 *	constructor. You should normally use the constructor taking
 *	a os::Looper or os::Handler pointer rather than this.
 * \param hPort
 *	The message port this messanger should send messages through. This
 *	will primarily come from os::Looper::GetMsgPort().
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
Messenger::Messenger( port_id hPort )
{
	m_hPort = hPort;
	m_hHandlerID = -1;
	m_hDestProc = -1;
	m_bPreferredTarget = false;
}

/** Construct a messenger for a named message port.
 * \par Description:
 *	This constructor is used to target named message ports (public message
 *	ports), and can be used for communication between processes.
 * \note
 *	Use IsValid() to check whether the message port was found.
 * \param pzPort
 *	The name of the message port this messenger should send messages through.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Messenger::Messenger( const char* pzPort )
{
	m_hPort = find_port( pzPort );
	m_hHandlerID = -1;
	m_hDestProc = -1;
	m_bPreferredTarget = false;
}

/** \internal
 *****************************************************************************/

Messenger::Messenger( port_id hPort, int nHandlerID, proc_id hDestProc )
{
	m_hPort = hPort;
	m_hHandlerID = nHandlerID;
	m_hDestProc = hDestProc;
	m_bPreferredTarget = nHandlerID != -1;
}

/** Destructor
 * \par Description:
 *	Does nothing.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Messenger::~Messenger()
{
}

/** Get a pointer to the handler targeted by this handler.
 * \par Description:
 *	Return a pointer to the os::Handler targeted by this
 *	messenger. If no specific handler is targeted the targeted
 *	looper will be returned.
 * \par
 *	If the messenger is not fully initialized, it's target have been
 *	deleted or the target is in a remote process this member will
 *	return NULL.
 * \par Warning:
 *	Since each looper run their own thread and might terminate themself
 *	at any moment it is very important that you never try to call any
 *	member on any of the returned objects before the looper is properly
 *	locked. Unless you have some other mechanism that guarantee you that
 *	the looper will be terminated between the return of this member and
 *	your attempt to lock it you must use os::Looper::SafeLock() rather
 *	than os::Looper::Lock() and make sure you validate that the looper
 *	was successfully locked before any further actions are performed.
 * \param ppcLooper
 *	If non-NULL a pointer to the looper owning the target will be written
 *	here.
 * \return Pointer to the destined os::Handler (this might be a looper if
 *	no specific handler is targeted) or NULL if the messenger is invalid
 *	or targets a handled/looper in a remote process.
 * \sa IsTargetLocal(), LockTarget(), os::Looper::SafeLock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler *Messenger::GetTarget( Looper ** ppcLooper ) const
{
	Looper *pcLooper = Looper::_GetLooperForPort( m_hPort );
	Handler *pcHandler = NULL;

	if( pcLooper != NULL )
	{
		if( m_bPreferredTarget )
		{
			pcHandler = pcLooper->_FindHandler( m_hHandlerID );
		}
	}
	if( ppcLooper != NULL ) { *ppcLooper = pcLooper; }
	return ( pcHandler );
}

/** Check if the targeted handler/looper lives in the calling process.
 * \par Description:
 *	Returns true if the target belongs to this process and false if
 *	it lives in a remote process.
 * \sa GetTarget(), LockTarget()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Messenger::IsTargetLocal() const
{
	return ( get_process_id( NULL ) == m_hDestProc );
}

/** Check if the messenger is fully initialized.
 * \par Description:
 *	Return true if the messenger have been initialized with a
 *	valid target and false if valid target have never been
 *	specified.
 * \note
 *	This member only check if the messenger was properly constructed.
 *	It will not check if the target is still valid.
 * \return True if initialized with a valid target false otherwhice.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


bool Messenger::IsValid( void ) const
{
	return ( m_hPort != -1 );
}

/** Lock the destination looper.
 * \par Description:
 *	Lock the os::Looper object targeted by this messenger.
 * \par Note:
 *	This member only work when the target is local
 * \return If the target was successfully locked 0 is returned. On
 *	failure -1 is returned and a error code is written to the
 *	global variable \b errno.
 * \sa GetTarget(), IsTargetLocal(), os::Looper::Lock(), os::Looper::SafeLock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Messenger::LockTarget() const
{
	Looper *pcLooper = Looper::_GetLooperForPort( m_hPort );

	if( NULL != pcLooper )
	{
		return ( pcLooper->SafeLock() );
	}
	else
	{
		errno = EINVAL;
		return ( -1 );
	}
}

/** Deliver a message syncronously
 * \par Description:
 *	Send a message to the target and optionally wait for a reply.
 *	If the \p pcReply argument is non-NULL, this function will not return
 *	until we receive a reply from the target or the nReplyTimeOut occure.
 *	The messenger will create a temporary message port from which it receives
 *	the reply. If pcReply is NULL the member will return as soon as the pcMsg
 *	message is delivered.
 * \par Bugs:
 *	The timeout values is currently ignored, and is always interpreted
 *	as INFINITE_TIMEOUT
 * \param pcMsg
 *	The message to deliver. The messages will be copyed and the caller
 *	keep ownership over the object.
 * \param pcReply
 *	The message to fill out with the reply or NULL to skip waiting for a reply.
 * \param nSendTimeOut
 *	Time to wait for a free slot in the receiving port before failing with errno == ETIME
 * \param nReplyTimeOut
 *	Time to wait for reply before failing with errno == ETIME
 * \return On success 0 is returned. In the case of failure -1 is returned and
 *	a error code is written to the global variable \b errno.
 * \sa os::Message::SendReply(), os::Message
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Messenger::SendMessage( Message * pcMsg, Message * pcReply, bigtime_t nSendTimeOut, bigtime_t nReplyTimeOut ) const
{
	if( IsValid() == false )
	{
		errno = EINVAL;
		return ( -1 );
	}
	port_id hReplyPort = -1;

	if( pcReply != NULL )
	{
		hReplyPort = create_port( "tmp_reply_port", DEFAULT_PORT_SIZE );
		if( hReplyPort < 0 )
		{
			dbprintf( "Error: Messenger::SendMessage() failed to create temporary message port\n" );
		}
	}
	status_t nError = pcMsg->_Post( m_hPort, m_hHandlerID, hReplyPort, -1, get_process_id( NULL ) );

	if( nError < 0 )
	{
		if( hReplyPort >= 0 )
		{
			delete_port( hReplyPort );
		}
		return ( nError );
	}
	if( pcReply != NULL )
	{
		nError = pcReply->_ReadPort( hReplyPort, nReplyTimeOut );
		delete_port( hReplyPort );
	}
	if( nError < 0 )
	{
		dbprintf( "Error: Messenger::SendMessage:1() failed to send message\n" );
	}

	return ( nError );
}

/** Deliver message asyncronously.
 * \par Description:
 *	The message is delivered to the target, and this member returns imediatly.
 *	If pcReplyHandler is non-NULL the following reply will be directed to
 *	that handler in a asyncronous manner through the Handler's Looper thread.
 * \par Bugs:
 *	The timeout value is currently ignored, and is always interpreted
 *	as INFINITE_TIMEOUT
 * \param pcMsg
 *	The message to deliver. The messages will be copyed and the caller
 *	keep ownership over the object.
 * \param pcReplyHandler
 *	The handler that should handle the reply or NULL if no reply is required.
 * \param nTimeOut
 *	Time to wait for a free slot in the receiving port before failing with
 *	errno == ETIME
 * \return On success 0 is returned. In the case of failure -1 is returned and
 *	a error code is written to the global variable \b errno.
 * \sa os::Message::SendReply(), os::Message
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Messenger::SendMessage( Message * pcMsg, Handler * pcReplyHandler, bigtime_t nTimeOut ) const
{
	if( IsValid() == false )
	{
		errno = EINVAL;
		return ( -1 );
	}

	Looper *pcLooper = NULL;
	port_id hReplyPort = -1;
	int nReplyToken = -1;
	proc_id hReplyProc = -1;

	if( NULL != pcReplyHandler )
	{
		pcLooper = pcReplyHandler->GetLooper();
		if( NULL != pcLooper )
		{
			hReplyPort = pcLooper->GetMsgPort();
			nReplyToken = pcReplyHandler->m_nToken;
			hReplyProc = get_process_id( NULL );
		}
	}
	status_t nError = pcMsg->_Post( m_hPort, m_hHandlerID, hReplyPort, nReplyToken, hReplyProc );

	if( nError < 0 )
	{
		dbprintf( "Error: Messenger::SendMessage:2() failed to send message\n" );
	}

	return ( nError );
}

/** Short for SendMessage( &Message( nCode ), pcReply )
 * \par Description:
 *	Short for SendMessage( &Message( nCode ), pcReply )
 * \sa status_t SendMessage( Message*, Message*, bigtime_t, bigtime_t ) const
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Messenger::SendMessage( int nCode, Message * pcReply ) const
{
	Message cMsg( nCode );

	return ( SendMessage( &cMsg, pcReply ) );
}

/** Short for SendMessage( &Message( nCode ), pcReplyHandler )
 * \par Description:
 *	Short for SendMessage( &Message( nCode ), pcReplyHandler )
 * \sa status_t SendMessage( Message*, Handler*, bigtime_t ) const
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Messenger::SendMessage( int nCode, Handler * pcReplyHandler ) const
{
	Message cMsg( nCode );

	return ( SendMessage( &cMsg, pcReplyHandler, ~0 ) );
}

/** Copy the target from another messenger.
 * \par Description:
 *	Copy the target from another messenger.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Messenger & Messenger::operator =( const Messenger & cMessenger )
{
	m_hPort = cMessenger.m_hPort;
	m_hHandlerID = cMessenger.m_hHandlerID;
	m_hDestProc = cMessenger.m_hDestProc;
	m_bPreferredTarget = cMessenger.m_bPreferredTarget;

	return ( *this );
}

/** Compare two messengers. Returns true if they target the same handler.
 * \par Description:
 *	Compare two messengers. Returns true if they target the same handler.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Messenger::operator ==( const Messenger & cMessenger ) const
{
	return ( m_hPort == cMessenger.m_hPort && m_hHandlerID == cMessenger.m_hHandlerID && m_bPreferredTarget == cMessenger.m_bPreferredTarget );
}


