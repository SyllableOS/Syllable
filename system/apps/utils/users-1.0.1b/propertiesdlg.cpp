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

#include <util/looper.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/layoutview.h>

#include <util/message.h>

#include <pwd.h>

#include "propertiesdlg.h"

using namespace os;

void UserProperties::ShowUserProperties( const struct passwd& sDetails,
                                         Looper *pcLooper,
                                         Message *pcMessage ) {
  string cTitle( "Properties for " );
  Rect   cFrame( 0, 0, 250, 200 );

  UserProperties *pcDlg;

  pcDlg = new UserProperties( cFrame + Point( 100, 100 ), "user_properties",
                              cTitle + sDetails.pw_gecos, sDetails,
                              pcLooper, pcMessage );

  pcDlg->Show();
  pcDlg->MakeFocus( true );
}

UserProperties::UserProperties( const Rect& cFrame, const string& cName,
                                const string& cTitle,
                                const struct passwd& sDetails,
                                Looper *pcLooper, Message *pcMessage )
: Window( cFrame, cName, cTitle ),
  m_pcNotify( pcLooper ), m_pcMessage( pcMessage ) {

  const Rect cRect( 0, 0, 1, 1 );
    
  char acUidBuf[16], acGidBuf[16];

  snprintf( acUidBuf, 16, "%d", sDetails.pw_uid );
  snprintf( acGidBuf, 16, "%d", sDetails.pw_gid );
  
  
  VLayoutNode *pcRoot = new VLayoutNode( "root" );

  HLayoutNode *pcLayout;

  struct {
    TextView **view;
    string     name;
    const char *value, *caption;
  } *psLine, asLines[] = {
    { &m_pcGecos, "gecos", sDetails.pw_gecos, "Name:" },
    { &m_pcName,  "name",  sDetails.pw_name,  "Login:" },
    { &m_pcUid,   "uid",   acUidBuf,          "User ID:" },
    { &m_pcGid,   "gid",   acGidBuf,          "Group ID:" },
    { &m_pcHome,  "home",  sDetails.pw_dir,   "Home:" },
    { &m_pcShell, "shell", sDetails.pw_shell, "Shell:" },
    { NULL, "", NULL, NULL }
  };


  // Create the various rows of the table.
  int nTabStop = 0;
  
  for( psLine = asLines; psLine->view != NULL; ++psLine ) {
    pcLayout = new HLayoutNode( (psLine->name + "_line").c_str(),
                              1.0f, pcRoot );
    
    pcLayout->AddChild( new StringView( cRect,
                                        (psLine->name + "_lbl").c_str(),
                                        psLine->caption ) );

    (*psLine->view) = new TextView( cRect,
                                    psLine->name.c_str(),
                                    psLine->value );
    
    (*psLine->view)->SetTabOrder( ++nTabStop );
    
    pcLayout->AddChild( *psLine->view );
  }

  pcLayout = new HLayoutNode( "buttons", 1.0f, pcRoot );

  Button *pcButton;
  

  pcButton = new Button( cRect, "cancel", "Cancel", new Message( ID_CANCEL ) );
  pcButton->SetTabOrder( nTabStop += 2 );
  pcLayout->AddChild( pcButton );
  
  pcButton = new Button( cRect, "ok", "OK", new Message( ID_OK ) );
  pcButton->SetTabOrder( --nTabStop );
  pcLayout->AddChild( pcButton );
  
  pcRoot->SetBorders( Rect( 5, 5, 5, 5 ) );
  pcRoot->SetBorders( Rect( 5, 0, 0, 5 ),
                      "gecos_lbl", "name_lbl", "uid_lbl",
                      "gid_lbl", "home_lbl", "shell_lbl", NULL );
  pcRoot->SetBorders( Rect( 5, 1, 2, 5 ),
                      "gecos", "name", "uid",
                      "gid", "home", "shell", NULL );
  pcRoot->SetBorders( Rect( 5, 2, 2, 5 ), "ok", "cancel", NULL );
  
  pcRoot->SameWidth( "gecos_lbl", "name_lbl", "uid_lbl",
                     "gid_lbl", "home_lbl", "shell_lbl", NULL );
  // pcRoot->SameWidth( "gecos", "name", "uid", "gid", "home", "shell", NULL );
  
  LayoutView *pcContent = new LayoutView( GetBounds(), "root_layout", pcRoot );

  AddChild( pcContent );

  m_pcGecos->MakeFocus( true );
}

UserProperties::~UserProperties( ) {
  if( m_pcMessage )
    delete m_pcMessage;
}

void UserProperties::HandleMessage( Message *pcMessage ) {
  switch( pcMessage->GetCode() ) {
    case ID_OK:
      if( m_pcNotify != NULL ) {
        // Sanity checks
        string gecos( m_pcGecos->GetBuffer()[0] ),
               name ( m_pcName->GetBuffer()[0] ),
               uid  ( m_pcUid->GetBuffer()[0] ),
               gid  ( m_pcGid->GetBuffer()[0] ),
               home ( m_pcHome->GetBuffer()[0] ),
               shell( m_pcShell->GetBuffer()[0] );
        
        // Send message to host that user details should be modified.
        Message *pcNew;
        
        if( m_pcMessage != NULL )
          pcNew = new Message( *m_pcMessage );
        else
          pcNew = new Message();
        
        pcNew->AddInt32( "uid", atoi( uid.c_str() ) );
        pcNew->AddString( "name", name );
        pcNew->AddString( "gecos", gecos );
        pcNew->AddInt32( "gid", atoi( gid.c_str() ) );
        pcNew->AddString( "home", home );
        pcNew->AddString( "shell", shell );
        
        m_pcNotify->PostMessage( pcNew, m_pcNotify, this );
      }
      break;
    case ID_CANCEL:
      PostMessage( M_QUIT );
      break;
    default:
      Window::HandleMessage( pcMessage );
      break;
  }
}

bool UserProperties::OkToQuit( ) {
  return true;
}
