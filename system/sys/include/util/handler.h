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

#ifndef	__F_GUI_HANDLER_H__
#define	__F_GUI_HANDLER_H__

#include <atheos/types.h>
#include <util/string.h>
#include <list>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Message;
class Looper;
class MessageFilter;

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

typedef std::list<MessageFilter*> MsgFilterList;

class Handler
{
public:
    Handler( const String& cName );
    virtual ~Handler();
    virtual void TimerTick( int nID );
    virtual void HandleMessage( Message* pcMessage );
private:
    virtual void __HA_reserved1__();
    virtual void __HA_reserved2__();
    virtual void __HA_reserved3__();
    virtual void __HA_reserved4__();
    virtual void __HA_reserved5__();
public:
    
    String	 GetName() const;
    void	 SetName( const String& cName );
    Looper*	 GetLooper() const;
    void	 SetNextHandler( Handler* pcNextHandler );
    Handler*	 GetNextHandler() const;

    void	AddFilter( MessageFilter* pcFilter );
    void	RemoveFilter( MessageFilter* pcFilter );
    
    const MsgFilterList& GetFilterList() const;
    
	const uint32 GetToken();
private:
    friend class Looper;
    friend class Message;
    friend class Messenger;
    friend class NodeMonitor;
    friend class Window;
    
    String	  m_cName;
    Handler*	  m_pcNextHandler;	// Next handler for unknown messages.
    Looper*	  m_pcLooper;
    uint32	  m_nToken;
    MsgFilterList m_cFilterList;
    uint32	__res[6];
};

}

#endif	//	GUI_HANDLER_HPP

