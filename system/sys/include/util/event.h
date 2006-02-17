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

#ifndef	__F_UTIL_EVENT_H__
#define	__F_UTIL_EVENT_H__

#include <gui/guidefines.h>
#include <util/message.h>
#include <util/looper.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent



/** Events.
 * \ingroup util
 * \par Description:
 * The event class provides methods for interprocess communication.
 *
 * \author	Arno Klenke
 *****************************************************************************/


class Event
{
public:
	Event();
	~Event();
	
	static Event*	Register( String zID, String zDescription, Handler* pcTarget, int nMessageCode, uint32 nFlags = 0 );
	status_t		SetToRemote( String zID, int nIndex = 0 );
	void			Unset();
	bool			IsRemote();
	status_t		SetMonitorEnabled( bool bEnabled, Handler* pcTarget = NULL, int nMessageCode = -1 );
	bool			IsMonitorEnabled();
	status_t		PostEvent( Message* pcData, Handler* pcReplyHandler = NULL, int nReplyCode = M_REPLY );
	status_t		GetLastEventMessage( os::Message* cReply );
private:
	class Private;
	Private* m;
};


}

#endif	// __F_UTIL_EVENT_H__










