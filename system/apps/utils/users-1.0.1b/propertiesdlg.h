/*
 *  Users and Passwords Manager for AtheOS
 *  Copyright (C) 2002 William Rose
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef WR_USERS_PROPERTIESDLG_H
#define WR_USERS_PROPERTIESDLG_H

#include <util/looper.h>
#include <gui/window.h>
#include <gui/textview.h>

#include <pwd.h>

using namespace os;
using namespace std;

class UserProperties : public Window {
  public:
    UserProperties( const Rect& cFrame, const string& cName,
                    const string& cTitle, const struct passwd& sDetails,
                    Looper *pcLooper, Message *pcMessage );

    virtual ~UserProperties();

    static void ShowUserProperties( const struct passwd& sDetails,
                                    Looper *pcNotify, Message *pcMessage );

    virtual void HandleMessage( Message *pcMessage );
    virtual bool OkToQuit( );
    
  private:
    static const int ID_OK = 1,
                     ID_CANCEL = 2;
    
    TextView *m_pcGecos, *m_pcName, *m_pcUid, *m_pcGid, *m_pcHome, *m_pcShell;
    Looper   *m_pcNotify;
    Message  *m_pcMessage;
};

#endif
