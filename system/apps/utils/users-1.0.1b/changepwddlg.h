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
#ifndef WR_USERS_CHANGEPWDDLG_H
#define WR_USERS_CHANGEPWDDLG_H

#include <util/message.h>
#include <gui/window.h>
#include <gui/textview.h>
#include <gui/layoutview.h>

#include "userlistview.h"

class ChangePasswordDlg : public Window {
  public:
    static void ShowChangePassword(Window*,const string& cLogin,
                                    Looper *pcNotify,
                                    Message *pcMessage );

    ChangePasswordDlg( const Rect& cFrame,
                       const string& cName,
                       const string& cTitle,
                       const string& cLogin,
                       Looper *pcNotify,
                       Message *pcMessage );

    virtual ~ChangePasswordDlg();
    
    void OnOK( Message *pcMessage );
    void OnCancel( Message *pcMessage );

    virtual void HandleMessage( Message *pcMessage );
    virtual bool OkToQuit( );

  private:
    const static int ID_OK = 1, ID_CANCEL = 2;
    
    TextView  *m_pcOldPwd, *m_pcNewPwd1, *m_pcNewPwd2;
    Looper    *m_pcNotify;
    Message   *m_pcMessage;
};

#endif

