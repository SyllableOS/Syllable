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

#ifndef	GUI_INVOKER_HPP
#define	GUI_INVOKER_HPP

#include <sys/types.h>
#include <util/messenger.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class	Message;
class	Handler;
class	Looper;

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Invoker
{
public:
    Invoker();
    Invoker( Message* pcMessage );
    Invoker( Message* pcMessage, const Handler* pcHandler, const Looper* pcLooper = NULL );
//  Invoker( Message* pcMessage, const Looper* pcLooper );
    Invoker( Message* pcMessage, const Messenger& cTarget );
    virtual	~Invoker();

    virtual status_t	SetMessage( Message* pcMessage );
    Message*		GetMessage() const;
    uint32		GetCode() const;

    virtual bool Invoked( Message* pcMessage );
    virtual void TargetChanged( const Messenger& cOldTarget );
    virtual void MessageChanged( const Message& cOldMsg );
    
    virtual status_t	SetTarget( const Handler* pcHandler, const Looper* pcLooper = NULL );
    virtual status_t	SetTarget( const Messenger& cMessenger );
    bool		IsTargetLocal() const;
    Handler*		GetTarget( Looper** ppcLooper = NULL ) const;
    Messenger		GetMessenger() const;
    virtual status_t	SetHandlerForReply( Handler* pcHandler );
    Handler*		GetHandlerForReply() const;
    virtual status_t	Invoke( Message* pcMessage = NULL );

private:
    Message*	m_pcMessage;
    Messenger	m_cMessenger;
    Handler*	m_pcReplyTo;
};

}

#endif	//	GUI_INVOKER_HPP
