/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2001 Kurt Skauen
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

#ifndef __F_UTIL_MESSAGEFILTER_H__
#define __F_UTIL_MESSAGEFILTER_H__

#include <list>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Looper;
class Message;
class Handler;

enum MessageFilterResult {
	MF_DISPATCH_MESSAGE = 0,
	MF_DISCARD_MESSAGE = 1
};

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class MessageFilter
{
public:
	MessageFilter();
	MessageFilter( int32 nFilterCode, Handler* pcNewTarget );
    virtual ~MessageFilter();

    virtual MessageFilterResult Filter( Message* pcMessage, Handler** ppcHandler );

	int32		GetFilterCode() const;
	void		SetFilterCode( int32 nCode );

	void		SetTarget( Handler* pcTarget );
	Handler*	GetTarget() const;

private:
	friend class Looper;
	friend class Handler;
    class Private;
    Private* 				  m;

	// Used by Handler & Looper to manage filters:
	std::list<MessageFilter*>::iterator m_cIterator;	// To find the filter easily in the Handlers list of filters
	bool			m_bGlobalFilter;	// Added to Looper with AddCommonFilter
	Handler*		m_pcHandler;		// Handler (or Looper) that the filter is added to
};


} // end of namespace

#endif // __F_UTIL_MESSAGEFILTER_H__
