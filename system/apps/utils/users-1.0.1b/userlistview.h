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
#ifndef WR_USERS_USERLISTVIEW_H
#define WR_USERS_USERLISTVIEW_H

#include <pwd.h>
#include <stdio.h>
#include <gui/listview.h>
#include <gui/button.h>
#include <string>

using namespace os;
using namespace std;

class UserListView : public ListView {
  public:
    UserListView( const Rect& cFrame, const string& cName,
                  const char *pzFile = "/etc/passwd" );

    virtual ~UserListView();

  private:
    virtual void SelectionChanged( int nFirstRow, int nLastRow );

    char *m_pzFile;
};

class UserListViewRow : public ListViewRow {
  public:
    UserListViewRow( );
    UserListViewRow( const struct passwd& sPwd );
    virtual ~UserListViewRow( );

    // Required overrides
    virtual void AttachToView( View *pcView, int nColumn );
    virtual void SetRect( const Rect& cRect, int nColumn );
    virtual float GetWidth( View *pcView, int nColumn );
    virtual float GetHeight( View *pcView );
    virtual void Paint( const Rect& cFrame, View *pcView, uint nColumn,
                        bool bSelected, bool bHighlighted, bool bHasFocus );
    virtual bool IsLessThan( const ListViewRow *pcOther, uint nColumn ) const;

    // Optional overrides
    // virtual bool HitTest( View *pcView, const Rect& cFrame, int nColumn,
    //                      Point cPos );
    bool Read( FILE *fp );
    bool Write( FILE *fp );

    const string& GetName( ) const;
    const string& GetGecos( ) const;
    const uid_t   GetUid( ) const;
    const gid_t   GetGid( ) const;
    const string& GetHome( ) const;
    const string& GetShell( ) const;
    const string& GetPwd( ) const;
    
    void SetName( const string& );
    void SetGecos( const string& );
    void SetUid( unsigned );
    void SetGid( unsigned );
    void SetHome( const string& );
    void SetShell( const string& );
    void SetPwd( const string& );
    void SetPwdText( const string& );

  private:
    string  m_cName, m_cGecos, m_cHome, m_cShell, m_cPwd;
    char    m_acUID[16], m_acGID[16];
    
    uid_t   m_nUID;
    gid_t   m_nGID;
};

inline const string& UserListViewRow::GetName( ) const {
  return m_cName;
}

inline const string& UserListViewRow::GetGecos( ) const {
  return m_cGecos;
}

inline const uid_t UserListViewRow::GetUid( ) const {
  return m_nUID;
}

inline const gid_t UserListViewRow::GetGid( ) const {
  return m_nGID;
}

inline const string& UserListViewRow::GetHome( ) const {
  return m_cHome;
}

inline const string& UserListViewRow::GetShell( ) const {
  return m_cShell;
}

inline const string& UserListViewRow::GetPwd( ) const {
  return m_cPwd;
}

#endif
