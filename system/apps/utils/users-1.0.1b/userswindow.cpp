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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <alloca.h>

#include <gui/window.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/menu.h>
#include <gui/button.h>
#include <gui/requesters.h>

#include <util/application.h>
#include <util/message.h>
#include <storage/filereference.h>

#include "main.h"
#include "userswindow.h"
#include "labelview.h"
#include "bitmapview.h"
#include "changepwddlg.h"
#include "propertiesdlg.h"

using namespace os;
using namespace ui;

UsersWindow::UsersWindow( const Rect& cFrame )
 : Window( cFrame, "main", "Users and Passwords" ) {
  SetupMenus();

  
  // Two frames: user management + password change
  Rect cClientArea = GetBounds();
  
  cClientArea.top = m_pcMenu->GetBounds().bottom + 1;

  m_pcContent = new Content( cClientArea, "content_layout" );

  m_pcContent->m_pcUserList->SetTarget( this, this );
  m_pcContent->m_pcUserList->SetSelChangeMsg( new Message( ID_SEL_CHANGE ) );

  AddChild( m_pcContent );
}

UsersWindow::~UsersWindow( ) {
}

void UsersWindow::HandleMessage( Message *pcMessage ) {
  if( pcMessage == NULL )
    return;

  switch( pcMessage->GetCode() ) {
    case ID_OK:
      OnOK( pcMessage );
      break;
    case ID_CANCEL:
      OnCancel( pcMessage );
      break;
    case ID_ADD:
      OnAdd( pcMessage );
      break;
    case ID_DELETE:
      OnDelete( pcMessage );
      break;
    case ID_PROPERTIES:
      OnProperties( pcMessage );
      break;
    case ID_CHANGE_PWD:
      OnChangePwd( pcMessage );
      break;
    case ID_SET_PASSWD:
      OnSetPasswd( pcMessage );
      break;
    case ID_SEL_CHANGE:
      OnSelChange( pcMessage );
      break;
    case ID_UPDATE_USER:
      OnUpdateUser( pcMessage );
      break;
    case ID_UPDATE_PWD:
      OnUpdatePwd( pcMessage );
      break;
    default:
      Window::HandleMessage( pcMessage );
      break;
  }
}

UserListViewRow *UsersWindow::GetSelRow( ) {
  UserListViewRow *pcRow;
  int nRow;
  
  // Locate the row for this user.
  if( (nRow = m_pcContent->m_pcUserList->GetFirstSelected()) > -1 )
    pcRow = dynamic_cast<UserListViewRow *>
    (m_pcContent->m_pcUserList->GetRow( nRow ));
  else
    return NULL;
  
  return pcRow;
}

/**
 * \par Description:
 * Called when the close button in the window frame is pressed.
 */
bool UsersWindow::OkToQuit( ) {
  Application::GetInstance()->PostMessage( M_QUIT );
  return true;
}

/**
 * \par Descriptio:
 * Handles an incoming message from the OK button.
 */
void UsersWindow::OnOK( Message *pcMessage ) {
  if( geteuid() != 0 ) {
    PostMessage( M_QUIT );
  }
  
  UserListViewRow *pcRow;
  int nRow;
  char tmp[] = "/etc/passwd.XXXXXX";
  int fd = mkstemp( tmp );

  if( fd < 0 ) {
    errno_alert( "Save Error", "creating a temporary password file" );
    return;
  }
  
  if( fchmod( fd, 0644 ) < 0 ) {
    errno_alert( "Save Error", "creating a temporary password file" );
    
    close( fd );
    return;
  }
  
  FILE *fp = fdopen( fd, "w" );

  if( fp == NULL ) {
    errno_alert( "Save Error", "opening a temporary password file" );
    // Close the descriptor?
    return;
  }
  
  
  nRow = m_pcContent->m_pcUserList->GetRowCount();
  
  for( int r = 0; r < nRow; ++r ) {
    UserListViewRow *pcRow = dynamic_cast<UserListViewRow *>
      (m_pcContent->m_pcUserList->GetRow( r ));

    if( pcRow != NULL )
      pcRow->Write( fp );
  }

  fclose( fp );

  // Now move it over the old passwd file.
  if( rename( tmp, "/etc/passwd" ) < 0 ) {
    errno_alert( "Save Error", "replacing the old password file" );

    unlink( tmp ); // Avoid polluting with files
  }

  PostMessage( M_QUIT );
}

/**
 * \par Description:
 * Handles a clicks from the Cancel button.
 */
void UsersWindow::OnCancel( Message *pcMessage ) {
  PostMessage( M_QUIT );
}

void UsersWindow::OnAdd( Message *pcMessage ) {
  struct passwd sPwd = { "new", "", 100, 100,
                         "New User", "/home/new", "/bin/bash" };

  Message *pcProto = new Message( ID_UPDATE_USER );

  UserProperties::ShowUserProperties( sPwd, this, pcProto );
}

void UsersWindow::OnDelete( Message *pcMessage ) {
  ListViewRow *pcRow;
  int nRow = m_pcContent->m_pcUserList->GetFirstSelected();
  
  if( nRow < 0 || (pcRow = m_pcContent->m_pcUserList->RemoveRow(nRow)) == NULL )
    return;

  delete pcRow;
}

/**
 * \par Description:
 * Handles clicks on the "Properties..." button.
 */
void UsersWindow::OnProperties( Message *pcMessage ) {
  UserListViewRow *pcRow;
  
  if( (pcRow = GetSelRow()) == NULL )
    return;

  struct passwd sPwd;

  sPwd.pw_name   = const_cast<char *>( pcRow->GetName().c_str() );
  sPwd.pw_passwd = "";
  sPwd.pw_uid    = pcRow->GetUid();
  sPwd.pw_gid    = pcRow->GetGid();
  sPwd.pw_gecos  = const_cast<char *>( pcRow->GetGecos().c_str() );
  sPwd.pw_dir    = const_cast<char *>( pcRow->GetHome().c_str() );
  sPwd.pw_shell  = const_cast<char *>( pcRow->GetShell().c_str() );
  
  // Create a sample message to be filled in and sent back by the
  // Properties dialog if the user submits changes.
  Message *pcProto = new Message( ID_UPDATE_USER );

  pcProto->AddPointer( "original_row", pcRow );
  pcProto->AddInt32( "original_uid", sPwd.pw_uid );
  pcProto->AddString( "original_name", sPwd.pw_name );
  
  UserProperties::ShowUserProperties( sPwd, this, pcProto );
}

/**
 * \par Description:
 * Handles clicks on the "Set Password..." button by displaying a
 * "Change Password" dialog.
 */
void UsersWindow::OnSetPasswd( Message *pcMessage ) {
  UserListViewRow *pcRow;
  
  if( (pcRow = GetSelRow()) == NULL )
    return;

  Message *pcNew = new Message( ID_UPDATE_PWD );

  pcNew->AddPointer( "original_row", pcRow );
  
  ChangePasswordDlg::ShowChangePassword( pcRow->GetName(), this, pcNew );
}

/**
 * \par Description:
 * Handles clicks on the "Change Password..." button.
 */
void UsersWindow::OnChangePwd( Message *pcMessage ) {
  UserListViewRow *pcRow;
  uid_t nUid;
  uint  nRows;

  nUid = getuid();
  nRows = m_pcContent->m_pcUserList->GetRowCount();
  
  // Locate the row for this user.
  for( uint r = 0; r < nRows; ++r ) {
    UserListViewRow *pcRow = dynamic_cast<UserListViewRow *>
      (m_pcContent->m_pcUserList->GetRow( r ));

    if( pcRow != NULL && pcRow->GetUid() == nUid ) {
      Message *pcNew = new Message( ID_UPDATE_PWD );

      pcNew->AddPointer( "original_row", pcRow );
      
      ChangePasswordDlg::ShowChangePassword( pcRow->GetName(), this, pcNew );
      break;
    }
  }
}

/**
 * \par Description:
 * Called when the list selection changes.
 */
void UsersWindow::OnSelChange( Message *pcMessage ) {
  UserListViewRow *pcRow;
  
  if( (pcRow = GetSelRow()) != NULL ) {
    if( getuid() == 0 || pcRow->GetUid() == getuid() )
      m_pcContent->m_pcSetPassword->SetEnable( true );
    else
      m_pcContent->m_pcSetPassword->SetEnable( false );

    if( getuid() == 0 ) {
      m_pcContent->m_pcAdd->SetEnable( true );
      m_pcContent->m_pcProperties->SetEnable( true );
      m_pcContent->m_pcDelete->SetEnable( true );
    } else {
      m_pcContent->m_pcAdd->SetEnable( false );
      m_pcContent->m_pcProperties->SetEnable( false );
      m_pcContent->m_pcDelete->SetEnable( false );
    }
  } else {
    if( getuid() == 0 )
      m_pcContent->m_pcAdd->SetEnable( true );
    else
      m_pcContent->m_pcAdd->SetEnable( false );
    
    m_pcContent->m_pcProperties->SetEnable( false );
    m_pcContent->m_pcDelete->SetEnable( false );
    m_pcContent->m_pcSetPassword->SetEnable( false );
  }
}

/**
 * \par Description:
 * Handles update password messages from change password dialogs.
 */
void UsersWindow::OnUpdatePwd( Message *pcMessage ) {
  UserListViewRow *pcRow;

  // Look for the original row pointer to help locate the right row
  if( pcMessage->FindPointer( "original_row", (void **)&pcRow ) < 0 )
    return; // Invalid message!

  // Scan through the rows in the list looking for any that match the
  // current row (i.e. is it still a valid pointer?).  While scanning,
  // check if any other rows match user name/id.
  bool bExists = false;
  uint nRows = m_pcContent->m_pcUserList->GetRowCount();

  for( uint r = 0; r < nRows; ++r ) {
    UserListViewRow *pcCmp = dynamic_cast<UserListViewRow *>
      ( m_pcContent->m_pcUserList->GetRow( r ) );

    if( pcCmp == pcRow ) {
      bExists = true;
      break;
    }
  }

  if( !bExists ) {
    Alert *pcAlert = new Alert( "Concurrency Problem",
                                "The entry you are trying to change the "
                                "password for has been deleted.  You will "
                                "need to re-try.", 0, "OK", NULL );

    pcAlert->Go();

    pcMessage->SendReply( M_QUIT );
    return;
  }
  
  const char *pzPasswd, *pzOldPasswd;
  
  if( getuid() != 0 ) {
    if( pcMessage->FindString( "old_passwd", &pzOldPasswd ) < 0 ) {
      Alert *pcAlert = new Alert( "Program Error",
                                  "The dialog did not send through the old "
                                  "password for this user correctly.  You "
                                  "will need to re-try.", 0, "OK", NULL );

      pcAlert->Go();

      pcMessage->SendReply( M_QUIT );
      return;
    }
    
    if( pcRow->GetPwd().length() > 0 && 
        !compare_pwd_to_txt( pcRow->GetPwd().c_str(), pzOldPasswd ) ) {
      // Alert
      Alert *pcAlert = new Alert( "Incorrect Password",
                                  "You have not entered the correct "
                                  "old password.  Please check and "
                                  "re-try.", 0, "OK", NULL );
      pcAlert->Go();
      return;
    }
  }
  
  // Okay, can change to the new password supplied.
  if( pcMessage->FindString( "passwd", &pzPasswd ) < 0 ) {
    Alert *pcAlert = new Alert( "Program Error",
                                "The dialog did not send through the old "
                                "password for this user correctly.  You "
                                "will need to re-try.", 0, "OK", NULL );

    pcAlert->Go();
  } else {
    pcRow->SetPwdText( pzPasswd );
  }

  pcMessage->SendReply( M_QUIT );
}

/**
 * \par Description:
 * Called when a Properties dialog sends a message containing updates.
 * This is quite a lengthy method, as there are many checks to be done.
 */
void UsersWindow::OnUpdateUser( Message *pcMessage ) {
  UserListViewRow *pcRow;
  uint  nRow = uint(-1);
  uid_t nUid;
  gid_t nGid;
  char *pzName;
  Alert *pcAlert;
  
  // Look for the original data to help locate the right row
  if( pcMessage->FindPointer( "original_row", (void **)&pcRow ) < 0 )
    pcRow = NULL;

  if( pcMessage->FindInt32( "uid", (int32 *)&nUid ) < 0 )
    nUid = uint(-1L);
  
  if( pcMessage->FindString( "name", (const char **)&pzName ) < 0 )
    pzName = NULL;
  
  // Scan through the rows in the list looking for any that match the
  // current row (i.e. is it still a valid pointer?).  While scanning,
  // check if any other rows match user name/id.
  bool bExists = false, bName = false, bUid = false;
  uint nRows = m_pcContent->m_pcUserList->GetRowCount();

  for( uint r = 0; r < nRows; ++r ) {
    UserListViewRow *pcCmp = dynamic_cast<UserListViewRow *>
      ( m_pcContent->m_pcUserList->GetRow( r ) );

    if( pcCmp == pcRow ) {
      bExists = true;
      nRow = r;
      continue;
    }

    if( pzName != NULL && pcCmp->GetName() == pzName)
      bName = true;
    
    if( nUid != uint(-1) && pcCmp->GetUid() == nUid )
      bUid = true;
  }
  
  // If there is a duplicate name, check that we have changed to the
  // duplicate to avoid redundant warnings
  if( bName ) {
    const char *pzOrigName;
    
    if( pcMessage->FindString( "original_name", &pzOrigName ) == 0 &&
        strcmp( pzName, pzOrigName ) == 0 )
      bName = false;
  }

  // If there is a duplicate id, check like we did for name.
  if( bUid ) {
    int32 nOrigUid;

    if( pcMessage->FindInt32( "original_uid", (int32 *)&nOrigUid ) == 0 &&
        nOrigUid == nUid )
      bUid = false;
  }
  
  if( pcRow != NULL && bExists == false ) {
    pcAlert = new Alert( "Concurrency Problem",
                                "The entry being edited in this "
                                "properties window has been deleted.\n"
                                "Would you like to create a new user "
                                "based on these properties, discard "
                                "these properties, or cancel?", 0,
                                "Cancel", "Discard", "Create New", NULL );

    switch( pcAlert->Go() ) {
      case 0:
        return;
      case 1:
        pcMessage->SendReply( M_QUIT );
        return;
      default:
        break;
    }
    
    pcRow = NULL;
  }

  // Extract data from message
  struct passwd sPwd;

  sPwd.pw_name = pzName;
  sPwd.pw_uid = nUid;
  
  if( pcMessage->FindString( "passwd", (const char **)&sPwd.pw_passwd ) < 0 )
    sPwd.pw_passwd = NULL;

  if( pcMessage->FindInt32( "gid", (int32 *)&sPwd.pw_gid ) < 0 )
    sPwd.pw_gid = gid_t(-1);

  if( pcMessage->FindString( "gecos", (const char **)&sPwd.pw_gecos ) < 0 )
    sPwd.pw_gecos = NULL;

  if( pcMessage->FindString( "home", (const char **)&sPwd.pw_dir ) < 0 )
    sPwd.pw_dir = NULL;

  if( pcMessage->FindString( "shell", (const char **)&sPwd.pw_shell ) < 0 )
    sPwd.pw_shell = NULL;
  
  // At this point, we're either making a new user, or editing an old one.
  // Warn in case of duplicates
  if( bName || bUid ) {
    char acMsgBuf[256];
    
    if( bName && bUid )
      strcpy( acMsgBuf,
              "Neither the new user name nor the user ID is unique.\n"
              "Are you sure you wish to proceed?" );
    else
      sprintf( acMsgBuf, "The new user %s provided is not unique.\n"
               "Are you sure you wish to proceed?",
               (bName ? "name" : "id") );
    
    pcAlert = new Alert( "Duplicate Warning", acMsgBuf, 0, "Cancel", "OK",
                         NULL );
    
    switch( pcAlert->Go() ) {
      case 0:
        return;
      default:
        break;
    }
  }
  
  // If we get to here, and pcRow is NULL, then we should try to insert
  // a new row.  Otherwise, update the existing row with any new
  // characteristics.
  if( pcRow == NULL ) {
    vector<const char *> cErrors;
    
    if( sPwd.pw_name == NULL || strlen( sPwd.pw_name ) < 1 )
      cErrors.insert( cErrors.end(), "a login name" );

    if( sPwd.pw_uid == uid_t(-1) )
      cErrors.insert( cErrors.end(), "a user ID" );

    if( sPwd.pw_gid == gid_t(-1) )
      cErrors.insert( cErrors.end(), "a group ID" );

    if( cErrors.size() > 0 ) {
      char acMsgBuf[256],
           *pzStart = "You cannot create a new user until you provide";

      switch( cErrors.size() ) {
        case 1:
          snprintf( acMsgBuf, 256, "%s %s.",
                    pzStart, cErrors[0] );
          break;
        case 2:
          snprintf( acMsgBuf, 256, "%s %s or %s.",
                    pzStart, cErrors[0], cErrors[1] );
          break;
        default:
          snprintf( acMsgBuf, 256, "%s %s, %s or %s.",
                    pzStart, cErrors[0], cErrors[1], cErrors[2] );
          break;
      }

      pcAlert = new Alert( "Missing Information", acMsgBuf, 0,
                                  "OK", NULL );

      pcAlert->Go();
      
      return;
    }
    
    // Fix any NULL pointers
    if( sPwd.pw_passwd == NULL )
      sPwd.pw_passwd = "";

    if( sPwd.pw_gecos == NULL )
      sPwd.pw_gecos = "";

    if( sPwd.pw_dir == NULL )
      sPwd.pw_dir = "";
    
    if( sPwd.pw_shell == NULL )
      sPwd.pw_shell = "";
    
    // Create and insert the row
    pcRow = new UserListViewRow( sPwd );

    m_pcContent->m_pcUserList->InsertRow( pcRow );
  } else {
    if( sPwd.pw_name != NULL )
      pcRow->SetName( sPwd.pw_name );

    if( sPwd.pw_passwd != NULL )
      pcRow->SetPwd( sPwd.pw_passwd );
    
    if( sPwd.pw_uid != uid_t(-1) )
      pcRow->SetUid( uid_t( sPwd.pw_uid ) );

    if( sPwd.pw_gid != gid_t(-1) )
      pcRow->SetGid( gid_t( sPwd.pw_gid ) );

    if( sPwd.pw_gecos != NULL )
      pcRow->SetGecos( sPwd.pw_gecos );

    if( sPwd.pw_dir != NULL )
      pcRow->SetHome( sPwd.pw_dir );

    if( sPwd.pw_shell != NULL )
      pcRow->SetShell( sPwd.pw_shell );
    
    m_pcContent->m_pcUserList->InvalidateRow( nRow, 0 );
  }
  
  // As all went well, close the Properties window that sent the message.
  pcMessage->SendReply( M_QUIT );
}

/**
 * \par Description:
 */
void UsersWindow::SetupMenus() {
  Rect cMenuFrame = GetBounds();

  cMenuFrame.bottom = 17;

  m_pcMenu = new Menu( cMenuFrame, "Menu", ITEMS_IN_ROW,
                       CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP,
                       WID_FULL_UPDATE_ON_H_RESIZE );

  Menu *pcFile = new Menu( Rect( 0, 0, 100, 20 ), "File", ITEMS_IN_COLUMN,
                           CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

  pcFile->AddItem( "Quit", new Message( M_QUIT ) );

  m_pcMenu->AddItem( pcFile );

  cMenuFrame.bottom = m_pcMenu->GetPreferredSize( false ).y - 1;
  
  m_pcMenu->SetFrame( cMenuFrame );
  m_pcMenu->SetTargetForItems( this );

  AddChild( m_pcMenu );
}


UsersWindow::Content::Content( const Rect& cFrame,
                               const std::string& cTitle,
                               uint32 nResizeMask,
                               uint32 nFlags )
  : LayoutView( cFrame, cTitle, NULL, nResizeMask, nFlags ) {
  
  VLayoutNode  *pcRoot = new VLayoutNode( "root" );
  
  FrameView *pcPwd = new FrameView( Rect( 0, 0, 20, 20 ),
                                    "password_frame", "Change Password" );
  FrameView *pcMgmt = new FrameView( Rect( 0, 0, 20, 20 ),
                                     "management_frame", "User Management" );

  pcRoot->AddChild( pcMgmt, 1.0f );
  pcRoot->AddChild( pcPwd, 0.0f );
  
  HLayoutNode *pcButtons = new HLayoutNode( "buttons_frame", 1.0f, pcRoot );

  pcButtons->AddChild( new Button( Rect( 0, 0, 100, 20 ), "cancel_button",
                                   "Cancel",
                                   new Message( UsersWindow::ID_CANCEL ) ) );

  pcButtons->AddChild( new Button( Rect( 0, 0, 100, 20 ), "ok_button",
                                   "OK",
                                   new Message( UsersWindow::ID_OK ) ) );

  // Build the user list frame
  VLayoutNode *pcMgmtRoot = new VLayoutNode( "management_root" );
  
  m_pcUserList = new UserListView( Rect( 0, 0, 1, 1 ), "user_list" );

  pcMgmtRoot->AddChild( m_pcUserList );
  
  HLayoutNode *pcMgmtBtns = new HLayoutNode( "management_buttons", 0.0f,
                                             pcMgmtRoot );
  
  pcMgmtBtns->AddChild( m_pcAdd = new Button( Rect( 0, 0, 100, 20 ),
                                    "add_button",
                                    "Add User..",
                                    new Message( UsersWindow::ID_ADD ) ),
                        1.0f );
  
  pcMgmtBtns->AddChild( m_pcProperties = new Button( Rect( 0, 0, 100, 20 ),
                                    "properties_button",
                                    "Properties...",
                                    new Message( UsersWindow::ID_PROPERTIES ) ),
                        1.0f );

  pcMgmtBtns->AddChild( m_pcDelete = new Button( Rect( 0, 0, 100, 20 ),
                                    "delete_button",
                                    "Delete User",
                                    new Message( UsersWindow::ID_DELETE ) ),
                        1.0f );

  pcMgmtBtns->AddChild( m_pcSetPassword = new Button( Rect( 0, 0, 100, 20 ),
                                    "set_password_button",
                                    "Set Password...",
                                    new Message( UsersWindow::ID_SET_PASSWD ) ),
                        1.0f );

  m_pcAdd->SetEnable( getuid() == 0 );
  m_pcProperties->SetEnable( false );
  m_pcDelete->SetEnable( false );
  m_pcSetPassword->SetEnable( false );

  pcMgmtRoot->SetBorders( Rect(5, 10, 5, 0), "add_button", "properties_button",
                          "delete_button", "set_password_button", NULL );
  pcMgmtRoot->SameWidth( "add_button", "properties_button", "delete_button", 
                         "set_password_button", NULL );
  pcMgmtRoot->SetBorders( Rect(10, 10, 10, 10) );

  pcMgmt->SetRoot( pcMgmtRoot );
  
  // Build the change password frame
  VLayoutNode *pcPwdRoot = new VLayoutNode( "password_root" );
  
  HLayoutNode *pcPwdTop  = new HLayoutNode( "password_top",
                                             1.0f, pcPwdRoot );
  
  pcPwdTop->SetVAlignment( ALIGN_TOP );
  
  // Left hand side is an icon
  Bitmap *pcPwdIcon = LoadPNG( "share/keys.png" );
  BitmapView *pcPwdIconView
    = new BitmapView( (pcPwdIcon?pcPwdIcon->GetBounds():Rect( 0, 0, 32, 32 )),
                      "password_icon", pcPwdIcon );
  
  pcPwdTop->AddChild( pcPwdIconView, 0.0f );

  // Right hand side is explanatory text, and optionally a button.
  const unsigned MSG_BUF_LEN = 256;
  char acMsgBuf[MSG_BUF_LEN] = "Your system administrator has not "
                               "given you permission to change your password.";

  LabelView *pcPwdText = new LabelView( Rect( 0, 0, 120, 60 ),
                                       "password_text", NULL );
  
  pcPwdTop->AddChild( pcPwdText );
  
  // Create a 'Change Password' button.
  m_pcChangePassword = new Button( Rect( 0, 0, 100, 20 ), "password_button",
                                   "Change Password...",
                                   new Message( UsersWindow::ID_CHANGE_PWD ) );
  pcPwdRoot->AddChild( m_pcChangePassword );
      
  // If the effective user ID is 0, its probably because we're running
  // setuid so people can change their passwords.
  if( geteuid() == 0 ) {
    // Look up the username of the real user that launched this process.
    struct passwd *pwd = getpwuid( getuid() );

    if( pwd == NULL ) {
      snprintf( acMsgBuf, MSG_BUF_LEN,
                "You are logged in as an unregistered user.  Please ask your "
                "system administrator to create a profile for your user ID "
                "(%d)", getuid() );
      m_pcChangePassword->SetEnable( false );
    } else {
      // Create a short message to go in the password frame.
      snprintf( acMsgBuf, MSG_BUF_LEN,
                "You are currently logged in as %s.", pwd->pw_name );
    }
  } else {
    m_pcChangePassword->SetEnable( false );
    // m_pcSetPassword->SetEnable( false );
  }
  
  pcPwdText->SetText( acMsgBuf );

  pcPwdRoot->SetBorders( Rect( 10, 10, 10, 10 ) );
  pcPwdRoot->SetBorders( Rect( 5, 0, 10, 0 ), "password_icon", NULL );
  // Finished building the Change Password frame

  pcPwd->SetRoot( pcPwdRoot );
  pcRoot->SetBorders( Rect( 5, 0,  5,  0 ), "ok_button", "cancel_button",
                      NULL );
  pcRoot->SameWidth( "ok_button", "cancel_button", NULL );
  
  pcRoot->SetBorders( Rect( 10, 10, 10, 10 ),
                      "management_frame", "password_frame", "buttons_frame",
                      NULL );
  
  SetRoot( pcRoot );
  InvalidateLayout();
}

UsersWindow::Content::~Content( ) {
}
