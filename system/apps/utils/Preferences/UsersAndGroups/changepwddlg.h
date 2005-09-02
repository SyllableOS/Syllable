/*
 *  Users and Groups Preferences for Syllable
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 2005 Kristian Van Der Vliet
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

#include <string>

class ChangePasswordDlg : public os::Window {
  public:
    ChangePasswordDlg( const os::Rect& cFrame,
                       const std::string& cName,
                       const std::string& cTitle,
                       const std::string& cLogin,
                       os::Handler *pcNotify,
                       os::Message *pcMessage );

    virtual ~ChangePasswordDlg();
    
    void OnOK( os::Message *pcMessage );
    void OnCancel( os::Message *pcMessage );

    virtual void HandleMessage( os::Message *pcMessage );
    virtual bool OkToQuit( );

  private:
    const static int ID_OK = 1, ID_CANCEL = 2;
    
    os::TextView  *m_pcOldPwd, *m_pcNewPwd1, *m_pcNewPwd2;
    os::Handler    *m_pcNotify;
    os::Message   *m_pcMessage;
};

#endif

