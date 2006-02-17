/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <string.h>

#include <util/handler.h>
#include <util/looper.h>
#include <util/messagefilter.h>
#include <atheos/kernel.h>

using namespace os;

/** Construct a handler.
 * \par Description:
 *	Initialize the handler and give it a name.
 *	The name does not need to be unique unless you want to be
 *	able to look it up with os::Looper::FindHandler()
 * \param cName
 *	A name identifying the handler. This can be passed to os::Looper::FindHandler()
 *	on a looper to find the handler object when it have been added to the
 *	looper with os::Looper::AddHandler().
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler::Handler( const String & cName ):m_cName( cName )
{
	static int nLastToken = 1;

	m_nToken = nLastToken++;
	m_pcNextHandler = NULL;
	m_pcLooper = NULL;
}

/** Destructor
 * \par Description:
 *	Free all resources allocated by the handler.
 *	If the handler is member of a looper it will detach itself
 *	from the looper and delete all message filters associated
 *	with the handler.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler::~Handler()
{
	if( m_pcLooper != NULL )
	{
		if( m_pcLooper->RemoveHandler( this ) == false )
		{
			dbprintf( "Handler::~Handler() failed to remove handler %p from looper\n", this );
		}
	}
	for( MsgFilterList::iterator i = m_cFilterList.begin(); i != m_cFilterList.end(  ); ++i )
	{
		( *i )->m_pcHandler = NULL;
		delete( *i );
	}
	m_nToken = ~m_nToken;
}

/** Handle a message targeted at this handler.
 * \par Description:
 *	Overload this member to dispatch messages sendt to this handler.
 *	When a looper receives a message for one of it's handlers it will
 *	call the taget handlers HandleMessage() to allow the handler to
 *	dispatch the message.
 * \par
 *	The message passed in \p pcMessage is also available through
 *	os::Looper::GetCurrentMessage() and os::Looper::DetachCurrentMessage()
 *	until this member returns. This is normally not very usefull for
 *	HandleMessage() itself but it can be convinient for other members
 *	called from HandleMessage() in case they need data from the message
 *	that was not passed on from HandleMessage().
 * \par
 *	The looper will be locked when this member is called. The default
 *	implementation of this member will pass the message on to the
 *	next handler if one was set with SetNextHandler().
 * \note
 *	Never do any lenthy operations in any hook members that are called
 *	from the looper thread if the looper is involved with the GUI (for
 *	example if the looper is a os::Window). The looper will not be
 *	able to dispatch messages until the hook returns so spending a
 *	long time in this members will make the GUI feel unresponsive.
 * \param pcMessage
 *	The message that should be handled. This message will be deleted by
 *	the looper when HandleMessage() returns unless you detach it with
 *	os::Looper::DetachCurrentMessage(),
 * \sa os::Looper::DispatchMessage(), os::Looper::DetachCurrentMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Handler::HandleMessage( Message * pcMessage )
{
	if( m_pcNextHandler != NULL )
	{
		m_pcLooper->PostMessage( pcMessage, m_pcNextHandler );
	}
}

/** Get the handlers name.
 * \par Description:
 *	Get the name assigned to the handler in the constructor or with
 *	the SetName() member.
 * \return The current name of the handler.
 * \sa SetName(), os::Looper::FindHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String Handler::GetName() const
{
	return ( m_cName );
}


/** Get the handlers token.
 * \par Description:
 *	Get the token assigned to the handler.
 * \return The token of the handler.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

const uint32 Handler::GetToken()
{
	return ( m_nToken );
}

/** Get a pointer to the looper this handler belongs to.
 * \par Description:
 *	GetLooper() returns a pointer to the looper this handler have been
 *	added to with os::Looper::AddHandler(). If the handler has not been
 *	added to any looper NULL will be returned.
 * \par
 *	If this is called on a os::Looper object (os::Looper is a subclass of os::Handler)
 *	this member will always return a pointer to itself.
 * \return Pointer to the looper this handler is a member to or NULL if it have
 *	not yet been added to a looper.
 * \sa os::Looper::AddHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Looper *Handler::GetLooper() const
{
	return ( m_pcLooper );
}

/** Set a handler that should handle messages this handler is not interrested in.
 * \par Description:
 *	The default implementation of HandleMessage() will check if there is
 *	set an alternative handler through SetNextHandler() and if so the
 *	message will be forwarded to this handler.
 * \param pcNextHandler
 *	Pointer to the handler that should receive messages after we are
 *	done with them or NULL to disable the forwarding.
 * \sa GetNextHandler(), HandleMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Handler::SetNextHandler( Handler * pcNextHandler )
{
	m_pcNextHandler = pcNextHandler;
}

/** Get the next handler in a handler chain.
 * \par Description:
 *	Get the next handler in a handler chain (as set with SetNextHandler()).
 * \return Pointer to the next handler in the handler chain or NULL if no
 *	"next" handler is set.
 * \sa SetNextHandler(), HandleMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler *Handler::GetNextHandler() const
{
	return ( m_pcNextHandler );
}

/** Rename the handler.
 * \par Description:
 *	Rename the handler
 * \param cName
 *	The new handler name.
 * \sa GetName(), os::Looper::FindHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Handler::SetName( const String & cName )
{
	m_cName = cName;
}

/** Timer dispatch member.
 * \par Description:
 *	This member will be called by the looper thread when a timer targeting
 *	this handler expires.
 * \par
 *	When a timer created with os::Looper::AddTimer() expires the looper
 *	thread will lock the looper and call this member on the target for
 *	the expired timer. If more than one timers are created it is possible
 *	to distinguish them by the timer ID that is assigned to the timer
 *	with os::Looper::AddTimer() and that is passed to TimerTick()
 *	through the \p nID parameter.
 * \note
 *	Never do any lenthy operations in any hook members that are called
 *	from the looper thread if the looper is involved with the GUI (for
 *	example if the looper is a os::Window). The looper will not be
 *	able to dispatch messages until the hook returns so spending a
 *	long time in this members will make the GUI feel unresponsive.
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Handler::TimerTick( int nID )
{
}

void Handler::AddFilter( MessageFilter * pcFilter )
{
	if( pcFilter == NULL )
	{
		dbprintf( "Error: Handler::AddFilter() called with a NULL pointer\n" );
		return;
	}
	if( pcFilter->m_pcHandler != NULL )
	{
		if( pcFilter->m_bGlobalFilter )
		{
			static_cast < Looper * >( pcFilter->m_pcHandler )->RemoveCommonFilter( pcFilter );
		}
		else
		{
			pcFilter->m_pcHandler->RemoveFilter( pcFilter );
		}
	}
	pcFilter->m_cIterator = m_cFilterList.insert( m_cFilterList.end(), pcFilter );
	pcFilter->m_pcHandler = this;
	pcFilter->m_bGlobalFilter = false;
}

void Handler::RemoveFilter( MessageFilter * pcFilter )
{
	if( pcFilter->m_pcHandler != this )
	{
		dbprintf( "Error: Handler::RemoveFilter() attempt to remove filter not belonging to us\n" );
		return;
	}
	if( pcFilter->m_bGlobalFilter )
	{
		dbprintf( "Error: Handler::RemoveFilter() attempt to remove a common filter using RemoveFilter()\n" );
		return;
	}
	m_cFilterList.erase( pcFilter->m_cIterator );
	pcFilter->m_pcHandler = NULL;
}

const MsgFilterList &Handler::GetFilterList() const
{
	return ( m_cFilterList );
}

/** \internal */
void Handler::__HA_reserved1__()
{
}

/** \internal */
void Handler::__HA_reserved2__()
{
}

/** \internal */
void Handler::__HA_reserved3__()
{
}

/** \internal */
void Handler::__HA_reserved4__()
{
}

/** \internal */
void Handler::__HA_reserved5__()
{
}
