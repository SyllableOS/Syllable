/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001 Kurt Skauen
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
    MessageFilter( uint32 nFilterCode );
    virtual ~MessageFilter();

    virtual bool Filter( Message* pcMessage, Handler** ppcHandler ) = 0;

    Handler* GetHandler() const;
    Looper*  GetLooper() const;
    bool     FiltersAllCodes() const;
    uint32   GetFilterCode() const;
    
private:
    friend class Handler;
    friend class Looper;
    class Private;
    Private* 				  m;
    Handler* 				  m_pcHandler;
    std::list<MessageFilter*>::iterator m_cIterator;
    uint32   				  m_nFilterCode;
    bool     				  m_bFiltersAllCodes;
    bool				  m_bGlobalFilter;
};


} // end of namespace



#endif // __F_UTIL_MESSAGEFILTER_H__
