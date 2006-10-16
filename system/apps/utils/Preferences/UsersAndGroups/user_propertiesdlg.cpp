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

#include <util/looper.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/layoutview.h>

#include <util/message.h>

#include <pwd.h>

#include "user_propertiesdlg.h"
#include "resources/UsersAndGroups.h"

using namespace os;

UserProperties::UserProperties( const Rect& cFrame, const string& cName,
                                const string& cTitle,
                                const struct passwd* psDetails,
                                Handler *pcHandler, Message *pcMessage )
: Window( cFrame, cName, cTitle ),
  m_pcNotify( pcHandler ), m_pcMessage( pcMessage ) {

  const Rect cRect( 0, 0, 1, 1 );
    
  char acUidBuf[16], acGidBuf[16];

  snprintf( acUidBuf, 16, "%d", psDetails->pw_uid );
  snprintf( acGidBuf, 16, "%d", psDetails->pw_gid );
  
  
  VLayoutNode *pcRoot = new VLayoutNode( "root" );

  HLayoutNode *pcLayout;

  struct {
    TextView **view;
    string     name;
    const char *value, *caption;
  } *psLine, asLines[] = {
    { &m_pcGecos, "gecos", psDetails->pw_gecos, MSG_NEWUSERWND_NAME.c_str() },
    { &m_pcName,  "name",  psDetails->pw_name,  MSG_NEWUSERWND_LOGIN.c_str() },
    { &m_pcUid,   "uid",   acUidBuf,          MSG_NEWUSERWND_USERID.c_str() },
    { &m_pcGid,   "gid",   acGidBuf,          MSG_NEWUSERWND_GROUPID.c_str() },
    { &m_pcHome,  "home",  psDetails->pw_dir,   MSG_NEWUSERWND_HOME.c_str() },
    { &m_pcShell, "shell", psDetails->pw_shell, MSG_NEWUSERWND_SHELL.c_str() },
    { NULL, "", NULL, NULL }
  };


  // Create the various rows of the table.
  
  for( psLine = asLines; psLine->view != NULL; ++psLine ) {
    pcLayout = new HLayoutNode( (psLine->name + "_line").c_str(),
                              1.0f, pcRoot );
    
    pcLayout->AddChild( new StringView( cRect,
                                        (psLine->name + "_lbl").c_str(),
                                        psLine->caption ) );

    (*psLine->view) = new TextView( cRect,
                                    psLine->name.c_str(),
                                    psLine->value );
    
    (*psLine->view)->SetTabOrder( NEXT_TAB_ORDER );
    (*psLine->view)->SetShortcutFromLabel( psLine->name );
    
    pcLayout->AddChild( *psLine->view );
  }

  pcLayout = new HLayoutNode( "buttons", 1.0f, pcRoot );

  Button *pcButton;
  

  pcButton = new Button( cRect, "cancel", MSG_NEWUSERWND_BUTTON_CANCEL, new Message( ID_CANCEL ) );
  pcButton->SetTabOrder(  );
  pcLayout->AddChild( pcButton );
  
  pcButton = new Button( cRect, "ok", MSG_NEWUSERWND_BUTTON_OK, new Message( ID_OK ) );
  pcButton->SetTabOrder(  );
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
  
  LayoutView *pcContent = new LayoutView( GetBounds(), "root_layout", pcRoot );

  AddChild( pcContent );

	SetSizeLimits( Point( 249, 199 ), Point( 4096, 4096 ) );

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

		// Send changes to main
		Messenger cMessenger( m_pcNotify );
		cMessenger.SendMessage( pcNew );

		// Close dialog
		PostMessage( M_QUIT );
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
