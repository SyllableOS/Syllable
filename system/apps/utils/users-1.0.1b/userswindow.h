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
#ifndef WR_USERS_USERSWINDOW_H
#define WR_USERS_USERSWINDOW_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/listview.h>

#include "userlistview.h"

using namespace os;

class UsersWindow : public Window {
  public:
    enum {
      ID_OK = 1,
      ID_CANCEL,
      ID_ADD,
      ID_DELETE,
      ID_PROPERTIES,
      ID_SET_PASSWD,
      ID_CHANGE_PWD,
      ID_SEL_CHANGE,
      ID_UPDATE_USER,
      ID_UPDATE_PWD
    };

    UsersWindow( const Rect& cFrame );
    virtual ~UsersWindow();

    void OnOK( Message *pcMessage );
    void OnCancel( Message *pcMessage );

    void OnAdd( Message *pcMessage );
    void OnDelete( Message *pcMessage );
    void OnProperties( Message *pcMessage );
    void OnSetPasswd( Message *pcMessage );
    
    void OnChangePwd( Message *pcMessage );
    
    void OnSelChange( Message *pcMessage );
    void OnUpdateUser( Message *pcMessage );
    void OnUpdatePwd( Message *pcMessage );

    virtual void HandleMessage( Message *pcMessage );
    virtual bool OkToQuit();

  private:
    UserListViewRow *GetSelRow();
    void SetupMenus();

    class Content : public os::LayoutView {
      public:
        Content( const Rect& cFrame, const std::string& cTitle,
                 uint32 nResizeMask = CF_FOLLOW_ALL,
                 uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );

        virtual ~Content();
        
        Button *m_pcProperties, *m_pcSetPassword, *m_pcChangePassword;
        Button *m_pcAdd, *m_pcDelete;
        UserListView *m_pcUserList;
    };

    Content *m_pcContent;
    Menu *m_pcMenu;
};

#endif
