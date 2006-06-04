/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	__F_UTIL_MESSENGER_H__
#define	__F_UTIL_MESSENGER_H__

#include <atheos/types.h>

// The messenger has some friends in the application server
class	SrvWindow;
class	SrvApplication;
class	SrvWidget;
class	SrvEvents;

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class	Message;
class	Looper;
class	Handler;

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	Messenger
{
public:
    Messenger();
    Messenger( const Handler* pcHandler, const Looper* pcLooper = NULL );
    Messenger( const Messenger& cMessenger );
    Messenger( port_id hPort );
    Messenger( const char *pzPort );
    ~Messenger();

    Handler*	GetTarget( Looper** ppcLooper ) const;
    bool		IsTargetLocal( void ) const;

    bool		IsValid( void ) const;
    status_t	LockTarget( void ) const;

    status_t	SendMessage( Message* pcMessage, Message* pcReply, bigtime_t nSendTimeOut = INFINITE_TIMEOUT,
			     bigtime_t nReplyTimeOut = INFINITE_TIMEOUT ) const;
    status_t	SendMessage( Message* pcMessage, Handler* pcReplyHandler = NULL,
			     bigtime_t nTimeOut = INFINITE_TIMEOUT ) const;
    status_t	SendMessage( int nCode, Message* pcReply = NULL ) const;
    status_t	SendMessage( int nCode, Handler* pcReplyHandler ) const;


    Messenger& operator =(const Messenger& cMessenger);
    bool       operator ==(const Messenger& cMessenger) const;

private:
    friend class Message;
    friend class ::SrvWidget;
    friend class ::SrvWindow;
    friend class ::SrvApplication;
    friend class ::SrvEvents;
    friend class NodeMonitor;
    friend class Event;
    
	Messenger( port_id hPort, int nHandlerID, proc_id hDestProc );

    port_id	m_hPort;
    int		m_hHandlerID;
    bool	m_bPreferredTarget;
    proc_id	m_hDestProc;
};

}

#endif	//	GUI_MESSENGER_HPP

