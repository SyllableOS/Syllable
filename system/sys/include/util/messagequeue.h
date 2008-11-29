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

#ifndef	GUI_MESSAGEQUEUE_HPP
#define	GUI_MESSAGEQUEUE_HPP

#include <atheos/types.h>
#include <util/locker.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class	Message;

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	MessageQueue
{
public:
    MessageQueue();
    virtual ~MessageQueue();

    void	AddMessage( Message* pcMsg );
    Message*	NextMessage();
    bool	RemoveMessage( Message* pcMsg );	// NOTE : The message will be deleted if found!
    Message*	FindMessage( int nIndex ) const;
    Message*	FindMessage( int nCode, int nIndex ) const;
    int		GetMessageCount( void ) const;
    bool	IsEmpty( void ) const;

    bool	Lock( void );
    void	Unlock( void );
private:
//  sem_id	m_hSemaphore;
    Locker	m_cMutex;
    Message*	m_pcFirstMessage;
    Message*	m_pcLastMessage;
    int		m_nMsgCount;
};

}
#endif	//	GUI_MESSAGEQUEUE_HPP
