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
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/requesters.h>

#include <util/message.h>
#include <util/messenger.h>

#include "changepwddlg.h"
#include "resources/UsersAndGroups.h"

using namespace os;
using namespace std;

ChangePasswordDlg::ChangePasswordDlg( const Rect& cFrame,
                                      const string& cName,
                                      const string& cTitle,
                                      const string& cLogin,
                                      Handler *pcNotify,
                                      Message *pcMessage )
  : Window( cFrame, cName, cTitle,
            WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_MODAL ),
    m_pcNotify( pcNotify ), m_pcMessage( pcMessage ) {
  
  VLayoutNode *pcRootlet = new VLayoutNode( "rootlet" ), *pcRoot;
  
  HLayoutNode *pcLine;

  string cCaption = MSG_CHANGEPASS_TEXT + " " + cLogin + ":";
  
  StringView *pSV = new StringView( Rect( 0, 0, 100, 20 ), "caption",
                                    cCaption.c_str(), ALIGN_LEFT );
                                  
  
  pcRootlet->AddChild( pSV, 0.0f );

  pcRootlet->SetHAlignment( ALIGN_LEFT );
  
  pcRoot = new VLayoutNode( "root", 1.0f, pcRootlet );
  
  int nTabOrder = 0;
  
  if( getuid() != 0 ) {
    ResizeBy( 0.0, 20.0 );

    // Root doesn't need to enter the old password!
    pcLine = new HLayoutNode( "old_pwd_line", 1.0f, pcRoot );
    pcLine->SetHAlignment( ALIGN_RIGHT );
    
    pcLine->AddChild( new StringView( Rect( 0, 0, 100, 20 ),
                                      "old_pwd_lbl", MSG_CHANGEPASS_OLDPASS,
                                      ALIGN_RIGHT ) );
    m_pcOldPwd = new TextView( Rect( 0, 0, 80, 22 ), "old_pwd", "" );
    m_pcOldPwd->SetPasswordMode( true );
    m_pcOldPwd->SetTabOrder( ++nTabOrder );

    pcLine->AddChild( m_pcOldPwd, 1.0f );
  }
  
  pcLine = new HLayoutNode( "new_pwd_1_line", 1.0f, pcRoot );
  pcLine->SetHAlignment( ALIGN_RIGHT );
  
  pcLine->AddChild( new StringView( Rect( 0, 0, 100, 20 ),
                                    "new_pwd_1_lbl", MSG_CHANGEPASS_NEWPASS,
                                    ALIGN_RIGHT) );
  m_pcNewPwd1 = new TextView( Rect( 0, 0, 80, 22 ), "new_pwd_1", "" );
  m_pcNewPwd1->SetPasswordMode( true );
  m_pcNewPwd1->SetTabOrder( ++nTabOrder );

  pcLine->AddChild( m_pcNewPwd1, 1.0f );
  
  pcLine = new HLayoutNode( "new_pwd_2_line", 1.0f, pcRoot );
  pcLine->SetHAlignment( ALIGN_RIGHT );
  
  pcLine->AddChild( new StringView( Rect( 0, 0, 100, 20 ),
                                    "new_pwd_2_lbl", MSG_CHANGEPASS_CONFIRMPASS,
                                    ALIGN_RIGHT) );
  m_pcNewPwd2 = new TextView( Rect( 0, 0, 80, 22 ), "new_pwd_2", "" );
  m_pcNewPwd2->SetPasswordMode( true );
  m_pcNewPwd2->SetTabOrder( ++nTabOrder );

  pcLine->AddChild( m_pcNewPwd2, 1.0f );


  pcLine = new HLayoutNode( "buttons", 2.0f, pcRoot );

  Button *pcButton;
  
  pcButton = new Button( Rect( 0, 0, 60, 20 ), "cancel", MSG_CHANGEPASS_BUTTON_CANCEL,
                         new Message( ID_CANCEL ) );
  pcButton->SetTabOrder( nTabOrder += 2 );
  pcLine->AddChild( pcButton );

  pcButton = new Button( Rect( 0, 0, 60, 20 ), "ok", MSG_CHANGEPASS_BUTTON_OK,
                         new Message( ID_OK ) );
  pcButton->SetTabOrder( --nTabOrder );
  pcLine->AddChild( pcButton );

  pcRootlet->SetBorders( Rect( 5, 5, 5, 5 ) );
  pcRootlet->SetBorders( Rect( 0, 0, 3, 0 ), "caption", NULL );

  pcRoot->SetBorders( Rect( 5, 1, 5, 2 ), "new_pwd_1_line", "new_pwd_2_line",
                      "buttons", "old_pwd_line", NULL );
  pcRoot->SetBorders( Rect( 5, 0, 5, 0 ), "new_pwd_1_lbl", "new_pwd_2_lbl",
                      "old_pwd_lbl", NULL );
  pcRoot->SetBorders( Rect( 0, 0, 0, 0 ), "new_pwd_1", "new_pwd_2", "old_pwd",
                      NULL );
  pcRoot->SetBorders( Rect( 5, 5, 2, 0 ), "ok", "cancel", NULL );

  pcRoot->SameWidth( "new_pwd_1_lbl", "new_pwd_2_lbl", "old_pwd_lbl", NULL );
  
  // Make the layout view
  LayoutView *pcContent = new LayoutView( GetBounds(), "layout", pcRootlet );
  
  AddChild( pcContent );

	SetSizeLimits( Point( 299, 109 ), Point( 4096, 4096 ) );

  if( getuid() != 0 ) {
    m_pcOldPwd->MakeFocus( true );
  } else {
    m_pcNewPwd1->MakeFocus( true );
  }
}

ChangePasswordDlg::~ChangePasswordDlg() {
  if( m_pcMessage )
    delete m_pcMessage;
}
    
void ChangePasswordDlg::HandleMessage( Message *pcMessage ) {
  if( pcMessage == NULL )
    return;

  switch( pcMessage->GetCode() ) {
    case ID_OK:
      OnOK( pcMessage );
      break;
    case ID_CANCEL:
      OnCancel( pcMessage );
      break;
    default:
      Window::HandleMessage( pcMessage );
      break;
  }
}

void ChangePasswordDlg::OnOK( Message *pcMessage ) {
  if( m_pcNotify == NULL )
    return;
  
  // Check the new password & confirmation match
  const char *pzNew1, *pzNew2;

  pzNew1 = m_pcNewPwd1->GetBuffer()[0].c_str();
  pzNew2 = m_pcNewPwd2->GetBuffer()[0].c_str();

  if( strcmp( pzNew1, pzNew2 ) != 0 ) {
    // Alert
    Alert *pcAlert = new Alert( MSG_ALERTWND_PASSDONTMATCH, MSG_ALERTWND_PASSDONTMATCH_TEXT, 0, MSG_ALERTWND_PASSDONTMATCH_OK.c_str(), NULL );
    pcAlert->Go();
    return;
  }

  Message *pcNew;

  if( m_pcMessage != NULL )
    pcNew = new Message( *m_pcMessage );
  else
    pcNew = new Message( );
  
  pcNew->AddString( "passwd", pzNew1 );
  
  // If not root, add the contents of the old password field to the message.
  if( getuid() != 0 )
    pcNew->AddString( "old_passwd", m_pcOldPwd->GetBuffer()[0] );
  
	// Send changes to main
	Messenger cMessenger( m_pcNotify );
	cMessenger.SendMessage( pcNew );

	// Close dialog
	PostMessage( M_QUIT );
}

void ChangePasswordDlg::OnCancel( Message *pcMessage ) {
  PostMessage( M_QUIT );
}

bool ChangePasswordDlg::OkToQuit( ) {
  return true;
}
