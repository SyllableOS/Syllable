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
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <crypt.h>

#include <gui/font.h>
#include <gui/view.h>
#include <gui/listview.h>
#include <gui/button.h>

#include <util/message.h>

#include "userlistview.h"

using namespace os;

UserListView::UserListView( const Rect& cFrame, const std::string& cTitle,
                            const char *pzFile )
  : ListView( cFrame, cTitle.c_str(), F_RENDER_BORDER ) {
  
  InsertColumn( "Name", 120 );
  InsertColumn( "Login", 60 );
  InsertColumn( "ID", 40 );
  InsertColumn( "Group", 60 );
  InsertColumn( "Home", 160 );

  if( pzFile ) {
    m_pzFile = new char[ strlen( pzFile ) + 1 ];
    strcpy( m_pzFile, pzFile );
  } else {
    m_pzFile = new char[1];
    *m_pzFile = 0;
  }

  FILE *fp = fopen( m_pzFile, "r" );

  if( fp ) {
    while( !feof( fp ) ) {
      UserListViewRow *pcRow = new UserListViewRow();

      if( pcRow->Read( fp ) ) {
        InsertRow( pcRow, false );
      } else {
        delete pcRow;
        break;
      }
    }

    fclose( fp );
  }
}

void UserListView::SelectionChanged( int nFirstRow, int nLastRow ) {
  ListView::SelectionChanged( nFirstRow, nLastRow );
}

UserListView::~UserListView( ) {
  if( m_pzFile )
    delete[] m_pzFile;
}


UserListViewRow::UserListViewRow( )
  : ListViewRow(), m_nUID( 0 ), m_nGID( 0 ) {
    
  strcpy( m_acUID, "0" );
  strcpy( m_acGID, "0" );
}

UserListViewRow::UserListViewRow( const struct passwd& sPwd )
  : ListViewRow(),
  m_cGecos( sPwd.pw_gecos ), m_cName( sPwd.pw_name ),
  m_cPwd( sPwd.pw_passwd ), m_cShell( sPwd.pw_shell ), m_cHome( sPwd.pw_dir ),
  m_nUID( sPwd.pw_uid ), m_nGID( sPwd.pw_gid ) {
    
  sprintf( m_acUID, "%u", m_nUID );
  sprintf( m_acGID, "%u", m_nGID );
}

UserListViewRow::~UserListViewRow( ) {
}

void UserListViewRow::AttachToView( View *pcView, int nColumn ) {
}

void UserListViewRow::SetRect( const Rect& cRect, int nColumn ) {
}

float UserListViewRow::GetWidth( View *pcView, int nColumn ) {
  switch( nColumn ) {
    case 0:
      return pcView->GetStringWidth( m_cGecos ) + 4.0f;
    case 1:
      return pcView->GetStringWidth( m_cName ) + 4.0f;
    case 2:
      return pcView->GetStringWidth( m_acUID ) + 4.0f;
    case 3:
      return pcView->GetStringWidth( m_acGID ) + 4.0f;
    case 4:
      return pcView->GetStringWidth( m_cHome ) + 4.0f;
    case 5:
      return pcView->GetStringWidth( m_cShell ) + 4.0f;
    default:
      break;
  }

  return 0.0f;
}

float UserListViewRow::GetHeight( View *pcView ) {
  font_height sMetrics;

  pcView->GetFontHeight( &sMetrics );

  return sMetrics.ascender + sMetrics.descender + sMetrics.line_gap
         + sMetrics.line_gap;
}

void UserListViewRow::Paint( const Rect& cFrame, View *pcView, uint nColumn,
                             bool bSelected, bool bHighlighted, bool bFocus ) {
  font_height sMetrics;

  pcView->GetFontHeight( &sMetrics );

  if( bHighlighted || bSelected ) {
    pcView->SetFgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
    pcView->SetBgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
  } else {
    pcView->SetFgColor( 255, 255, 255 );
    pcView->SetBgColor( 255, 255, 255 );
  }
  
  pcView->FillRect( cFrame );

  if( bHighlighted || bSelected ) {
    pcView->SetFgColor( get_default_color( COL_SEL_MENU_TEXT ) );
  } else {
    pcView->SetFgColor( 0, 0, 0 );
  }
  
  Point cPen( cFrame.left + 2.0f,
              cFrame.top + sMetrics.ascender + 0.5f *
               (cFrame.Height() - sMetrics.ascender - sMetrics.descender) );
  
  const char *pzRow = "";

  switch( nColumn ) {
    case 0:
      pzRow = m_cGecos.c_str();
      break;
    case 1:
      pzRow = m_cName.c_str();
      break;
    case 2:
      pzRow = m_acUID;
      cPen.x = cFrame.right - 2.0f - pcView->GetStringWidth( m_acUID );
      break;
    case 3:
      pzRow = m_acGID;
      cPen.x = cFrame.right - 2.0f - pcView->GetStringWidth( m_acGID );
      break;
    case 4:
      pzRow = m_cHome.c_str();
      break;
    case 5:
      pzRow = m_cShell.c_str();
      break;
    default:
      break;
  }
  
  pcView->DrawString( cPen, pzRow );
}

bool UserListViewRow::IsLessThan( const ListViewRow *pcOther, uint nColumn )
  const {
  const UserListViewRow *pcCmp = dynamic_cast<const UserListViewRow *>(pcOther);

  if( pcCmp ) {
    switch( nColumn ) {
      case 0:
        return m_cGecos < pcCmp->m_cGecos;
      case 1:
        return m_cName < pcCmp->m_cName;
      case 2:
        return m_nUID < pcCmp->m_nUID;
      case 3:
        return m_nGID < pcCmp->m_nGID;
      case 4:
        return m_cHome < pcCmp->m_cHome;
      case 5:
        return m_cShell < pcCmp->m_cShell;
      default:
        return false;
    }
  }

  return false;
}

bool UserListViewRow::Read( FILE *fp ) {
  struct passwd *psPwd;

  if( fp == NULL )
    return false;
  
  if( (psPwd = fgetpwent( fp )) != NULL ) {
    m_cName = psPwd->pw_name;
    m_cPwd = psPwd->pw_passwd;
    m_nUID = psPwd->pw_uid;
    sprintf( m_acUID, "%u", m_nUID );
    m_nGID = psPwd->pw_gid;
    sprintf( m_acGID, "%u", m_nGID );
    m_cGecos = psPwd->pw_gecos;
    m_cHome = psPwd->pw_dir;
    m_cShell = psPwd->pw_shell;
    
    return true;
  }

  return false;
}

bool UserListViewRow::Write( FILE *fp ) {
  struct passwd sPwd;

  if( fp == NULL )
    return false;
  
  sPwd.pw_name = (char *)m_cName.c_str();
  sPwd.pw_passwd = (char *)m_cPwd.c_str();
  sPwd.pw_uid = m_nUID;
  sPwd.pw_gid = m_nGID;
  sPwd.pw_gecos = (char *)m_cGecos.c_str();
  sPwd.pw_dir = (char *)m_cHome.c_str();
  sPwd.pw_shell = (char *)m_cShell.c_str();

  return putpwent( &sPwd, fp ) == 0;
}

void UserListViewRow::SetName( const string& cName ) {
  m_cName = cName;
}

void UserListViewRow::SetGecos( const string& cGecos ) {
  m_cGecos = cGecos;
}

void UserListViewRow::SetUid( unsigned nUid ) {
  m_nUID = nUid;
  snprintf( m_acUID, 16, "%d", nUid );
}

void UserListViewRow::SetGid( unsigned nGid ) {
  m_nGID = nGid;
  snprintf( m_acGID, 16, "%d", nGid );
}

void UserListViewRow::SetHome( const string& cHome ) {
  m_cHome = cHome;
}

void UserListViewRow::SetShell( const string& cShell ) {
  m_cShell = cShell;
}

void UserListViewRow::SetPwd( const string& cPwd ) {
  m_cPwd = cPwd;
}

void UserListViewRow::SetPwdText( const string& cPwd ) {
  m_cPwd = crypt( cPwd.c_str(), "$1$" );
}


