/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#ifndef __F_APP_APPSERVERCONFIG_H__
#define __F_APP_APPSERVERCONFIG_H__

#include <atheos/types.h>
#include <util/message.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class AppserverConfig
{
public:
    AppserverConfig();
    ~AppserverConfig();

    status_t Commit();
    status_t SetPopoupSelectedWindows( bool bPopup );
    bool     GetPopoupSelectedWindows() const;

    status_t	SetDoubleClickTime( bigtime_t nDelay );
    bigtime_t	GetDoubleClickTime() const;

    status_t	SetKeyDelay( bigtime_t nDelay );
    status_t	SetKeyRepeat( bigtime_t nDelay );
    
    bigtime_t	GetKeyDelay() const;
    bigtime_t	GetKeyRepeat() const;
    
private:
    Message m_cConfig;
    bool    m_bModified;
};


    
    
} // end of namespace

#endif // __F_APP_APPSERVERCONFIG_H__
