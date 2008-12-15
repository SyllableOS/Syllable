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

#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include <string>

#include <gui/image.h>
#include <gui/imageview.h>
#include <gui/stringview.h>
#include <gui/requesters.h>
#include <util/application.h>
#include <util/resources.h>
#include <storage/file.h>

#include <users_window.h>
#include <changepwddlg.h>
#include <user_propertiesdlg.h>
#include <group_propertiesdlg.h>
#include "resources/UsersAndGroups.h"

using namespace os;
using namespace std;

UsersWindow::UsersWindow( const os::Rect cFrame ) : Window( cFrame, "Users", MSG_MAINWND_TITLE )
{
	Rect cBounds = GetBounds();

	Rect cTabViewBounds = cBounds;
	cTabViewBounds.bottom -= 30;

	m_pcTabView = new TabView( cTabViewBounds, "users_tabview", CF_FOLLOW_ALL );

	/* Create Views for each tab */
	Rect cTabBounds = m_pcTabView->GetBounds();

	m_pcUsersView = new UsersView( cTabBounds );
	m_pcTabView->AppendTab( MSG_MAINWND_TAB_USERS, m_pcUsersView );

	m_pcGroupsView = new GroupsView( cTabBounds );
	m_pcTabView->AppendTab( MSG_MAINWND_TAB_GROUPS, m_pcGroupsView );

	AddChild( m_pcTabView );

	/* The Apply/Save/Cancel buttons live in a layoutview of their own */
	Rect cButtonFrame = cBounds;
	cButtonFrame.top = cButtonFrame.bottom - 25;

	m_pcLayoutView = new LayoutView( cButtonFrame, "users_window", NULL, CF_FOLLOW_LEFT | CF_FOLLOW_BOTTOM | CF_FOLLOW_RIGHT );

	VLayoutNode *pcNode = new VLayoutNode( "users_window_root" );
	pcNode->SetBorders( Rect( 5, 4, 5, 3 ) );
	pcNode->AddChild( new VLayoutSpacer( "users_window_v_spacer", 1.0f ) );

	HLayoutNode *pcButtons = new HLayoutNode( "users_window_buttons", 0.0f );
	pcButtons->AddChild( new HLayoutSpacer( "users_window_h_spacer", 200.0f ) );
	Button *pcOkButton = new Button( Rect(), "users_window_ok", MSG_MAINWND_BUTTON_OK, new Message( ID_WINDOW_OK ) );
	pcButtons->AddChild( pcOkButton );
	pcButtons->AddChild( new HLayoutSpacer( "users_window_h_spacer" ) );
	pcButtons->AddChild( new Button( Rect(), "users_window_cancel", MSG_MAINWND_BUTTON_CANCEL, new Message( ID_WINDOW_CANCEL ) ) );
	pcButtons->AddChild( new HLayoutSpacer( "users_window_h_spacer" ) );

	pcNode->AddChild( pcButtons );

	m_pcLayoutView->SetRoot( pcNode );
	AddChild( m_pcLayoutView );

	/* Focus the controls */
	SetDefaultButton( pcOkButton );

	/* Don't let the window get too small */ 
	SetSizeLimits( Point( 399, 449 ), Point( 4096, 4096 ) );

	/* Set an application icon */
	Resources cRes( get_image_id() );
	BitmapImage *pcAppIcon = new BitmapImage();
	pcAppIcon->Load( cRes.GetResourceStream( "icon48x48.png" ) );
	SetIcon( pcAppIcon->LockBitmap() );
	delete( pcAppIcon );
}

UsersWindow::~UsersWindow()
{
	RemoveChild( m_pcLayoutView );
	delete( m_pcLayoutView );

	RemoveChild( m_pcTabView );
	delete( m_pcTabView );
}

void UsersWindow::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case ID_WINDOW_OK:
		{
			/* Instruct each View to write changes.  Note the reverse order which avoids problematic dependencies.. */
			m_pcGroupsView->SaveChanges();
			m_pcUsersView->SaveChanges();

			/* Fall through */
		}

		case ID_WINDOW_CANCEL:
		{
			OkToQuit();
			break;
		}

		default:
			Window::HandleMessage( pcMessage );
	}
}

bool UsersWindow::OkToQuit()
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

UsersView::UsersView( const Rect cFrame ) : View( cFrame, "users_users_view" )
{
	m_bModified = false;

	VLayoutNode *pcUsersRoot = new VLayoutNode( "users_root" );

	m_pcUsersList = new ListView( Rect(), "users_list", ListView::F_NO_AUTO_SORT | ListView::F_RENDER_BORDER );
	m_pcUsersList->InsertColumn( MSG_MAINWND_TAB_USERS_NAME.c_str(), 120 );
	m_pcUsersList->InsertColumn( MSG_MAINWND_TAB_USERS_LOGIN.c_str(), 60 );
	m_pcUsersList->InsertColumn( MSG_MAINWND_TAB_USERS_ID.c_str(), 40 );
	m_pcUsersList->InsertColumn( MSG_MAINWND_TAB_USERS_GROUP.c_str(), 60 );
	m_pcUsersList->InsertColumn( MSG_MAINWND_TAB_USERS_HOME.c_str(), 160 );
	m_pcUsersList->SetSelChangeMsg( new Message( ID_USERS_SELECT ) );

	pcUsersRoot->AddChild( m_pcUsersList, 200.0f );
	pcUsersRoot->AddChild( new VLayoutSpacer( "" ) );

	HLayoutNode *pcUsersButtons = new HLayoutNode( "users_buttons", 0.0f, pcUsersRoot );

	m_pcAdd = new Button( Rect(), "add_button", MSG_MAINWND_TAB_USERS_BUTTON_ADD, new Message( ID_USERS_ADD ) );
	pcUsersButtons->AddChild( m_pcAdd, 1.0f );

	m_pcEdit = new Button( Rect(), "edit_button", MSG_MAINWND_TAB_USERS_BUTTON_EDIT, new Message( ID_USERS_EDIT ) );
	pcUsersButtons->AddChild( m_pcEdit, 1.0f );

	m_pcDelete = new Button( Rect(), "delete_button", MSG_MAINWND_TAB_USERS_BUTTON_DEL, new Message( ID_USERS_DELETE ) );
	pcUsersButtons->AddChild( m_pcDelete, 1.0f );

	m_pcPassword = new Button( Rect(), "set_password_button", MSG_MAINWND_TAB_USERS_BUTTON_PASS, new Message( ID_USERS_SET_PASSWD ) );
	pcUsersButtons->AddChild( m_pcPassword, 1.0f );

	m_pcAdd->SetEnable( getuid() == 0 );
	m_pcEdit->SetEnable( false );
	m_pcDelete->SetEnable( false );
	m_pcPassword->SetEnable( false );

	pcUsersRoot->AddChild( pcUsersButtons );

	/* Create an internal list of all passwd entries and add a row for each user */
	struct passwd *psEntry;
	while( ( psEntry = getpwent() ) != NULL )
	{
		struct passwd *psPwd = (struct passwd *)calloc( 1, sizeof( struct passwd ) );
		if( NULL == psPwd )
			break;

		/* Create a copy of the (static) passwd entry */
		{
			psPwd->pw_name = (char*)calloc( 1, strlen( psEntry->pw_name ) + 1 );
			strcpy( psPwd->pw_name, psEntry->pw_name );

			psPwd->pw_passwd = (char*)calloc( 1, strlen( psEntry->pw_passwd ) + 1 );
			strcpy( psPwd->pw_passwd, psEntry->pw_passwd );

			psPwd->pw_uid = psEntry->pw_uid;
			psPwd->pw_gid = psEntry->pw_gid;

			psPwd->pw_gecos = (char*)calloc( 1, strlen( psEntry->pw_gecos ) + 1 );
			strcpy( psPwd->pw_gecos, psEntry->pw_gecos );

			psPwd->pw_dir = (char*)calloc( 1, strlen( psEntry->pw_dir ) + 1 );
			strcpy( psPwd->pw_dir, psEntry->pw_dir );

			psPwd->pw_shell = (char*)calloc( 1, strlen( psEntry->pw_shell ) + 1 );
			strcpy( psPwd->pw_shell, psEntry->pw_shell );
		}

		/* Create a row to display users details */
		ListViewStringRow *pcRow = new ListViewStringRow();
		pcRow->AppendString( psPwd->pw_gecos );
		pcRow->AppendString( psPwd->pw_name );

		char nId[6];
		snprintf( nId, 6, "%u", psPwd->pw_uid );
		pcRow->AppendString( nId );
		snprintf( nId, 6, "%u", psPwd->pw_gid );
		pcRow->AppendString( nId );

		pcRow->AppendString( psPwd->pw_dir );

		/* We can retrieve the data when we need it via. the cookie */
		pcRow->SetCookie( Variant( (void*)psPwd ) );

		/* Put the row in the list */
		m_pcUsersList->InsertRow( pcRow );
	}
	endpwent();

	/* Clean up the layout */
	pcUsersRoot->SetBorders( Rect(5, 10, 5, 0), "add_button", "edit_button", "delete_button", "set_password_button", NULL );
	pcUsersRoot->SameWidth( "add_button", "edit_button", "delete_button", "set_password_button", NULL );
	pcUsersRoot->SetBorders( Rect(10, 10, 10, 10) );

	m_pcLayoutView = new LayoutView( GetBounds(), "users" );
	m_pcLayoutView->SetRoot( pcUsersRoot );
	AddChild( m_pcLayoutView );
}

UsersView::~UsersView()
{
	RemoveChild( m_pcLayoutView );
	delete( m_pcLayoutView );
}

void UsersView::AllAttached( void )
{
	View::AllAttached();
	m_pcUsersList->SetTarget( this );
	m_pcAdd->SetTarget( this );
	m_pcEdit->SetTarget( this );
	m_pcDelete->SetTarget( this );
	m_pcPassword->SetTarget( this );
}

void UsersView::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case ID_USERS_SELECT:
		{
			if( getuid() > 0 )
				break;

			int nSelected = m_pcUsersList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct passwd *)m_pcUsersList->GetRow( nSelected )->GetCookie().AsPointer();

			m_pcEdit->SetEnable( true );
			m_pcPassword->SetEnable( true );

			if( 0 == m_psSelected->pw_uid )
				m_pcDelete->SetEnable( false );
			else
				m_pcDelete->SetEnable( true );

			break;
		}

		case ID_USERS_ADD:
		{
			struct passwd *psPasswd = (struct passwd *)calloc( 1, sizeof( struct passwd ) );
			if( psPasswd == NULL )
				break;

			/* XXXKV: We could try to be a little smarter here and work
			   out the highest UID & correct GID */
			psPasswd->pw_uid = 100;
			psPasswd->pw_gid = 100;

			psPasswd->pw_name = "new";
			psPasswd->pw_gecos = "New User";
			psPasswd->pw_dir = "/home/new";
			psPasswd->pw_shell = "/bin/bash";

			Message *pcChangeMsg = new Message( ID_USERS_POST_ADD );
			pcChangeMsg->AddPointer( "data", (void*)psPasswd );

			DisplayProperties( MSG_NEWUSERWND_TITLE, psPasswd, pcChangeMsg );
			break;
		}

		case ID_USERS_POST_ADD:
		{
			struct passwd *psPasswd;
			if( pcMessage->FindPointer( "data", (void**)&psPasswd ) != EOK )
				break;

			int32 nUid, nGid;
			string cName, cGecos, cHome, cShell;

			if( pcMessage->FindInt32( "uid", &nUid ) != EOK )
				break;

			if( pcMessage->FindInt32( "gid", &nGid ) != EOK )
				break;

			if( pcMessage->FindString( "name", &cName ) != EOK )
				break;

			if( pcMessage->FindString( "gecos", &cGecos ) != EOK )
				break;

			if( pcMessage->FindString( "home", &cHome ) != EOK )
				break;

			if( pcMessage->FindString( "shell", &cShell ) != EOK )
				break;

			/* Allocate & store new details */
			psPasswd->pw_uid = nUid;
			psPasswd->pw_gid = nGid;

			psPasswd->pw_name = (char*)calloc( 1, cName.size() + 1 );
			strcpy( psPasswd->pw_name, cName.c_str() );
			
			psPasswd->pw_gecos = (char*)calloc( 1, cGecos.size() + 1 );
			strcpy( psPasswd->pw_gecos, cGecos.c_str() );

			psPasswd->pw_dir = (char*)calloc( 1, cHome.size() + 1 );
			strcpy( psPasswd->pw_dir, cHome.c_str() );

			psPasswd->pw_shell = (char*)calloc( 1, cShell.size() + 1 );
			strcpy( psPasswd->pw_shell, cShell.c_str() );

			/* Create a row to display users details */
			ListViewStringRow *pcRow = new ListViewStringRow();
			pcRow->AppendString( psPasswd->pw_gecos );
			pcRow->AppendString( psPasswd->pw_name );

			char nId[6];
			snprintf( nId, 6, "%u", psPasswd->pw_uid );
			pcRow->AppendString( nId );
			snprintf( nId, 6, "%u", psPasswd->pw_gid );
			pcRow->AppendString( nId );

			pcRow->AppendString( psPasswd->pw_dir );

			/* We can retrieve the data when we need it via. the cookie */
			pcRow->SetCookie( Variant( (void*)psPasswd ) );

			/* Put the row in the list */
			m_pcUsersList->InsertRow( pcRow );

			/* Remember to create a home directory for this user */
			m_vNewHomes.push_back( psPasswd->pw_name );

			m_bModified = true;

			break;
		}

		case ID_USERS_EDIT:
		{
			int nSelected = m_pcUsersList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct passwd *)m_pcUsersList->GetRow( nSelected )->GetCookie().AsPointer();

			Message *pcChangeMsg = new Message( ID_USERS_POST_EDIT );
			pcChangeMsg->AddInt32( "selection", nSelected );

			string cTitle;
			cTitle = MSG_EDITUSERWND_TITLE + " " + string( m_psSelected->pw_gecos );

			DisplayProperties( cTitle, m_psSelected, pcChangeMsg );
			break;
		}

		case ID_USERS_POST_EDIT:
		{
			int32 nSelected;
			if( pcMessage->FindInt32( "selection", &nSelected ) != EOK )
				break;

			struct passwd *psPasswd = (struct passwd *)m_pcUsersList->GetRow( nSelected )->GetCookie().AsPointer();

			int32 nUid, nGid;
			string cName, cGecos, cHome, cShell;

			if( pcMessage->FindInt32( "uid", &nUid ) != EOK )
				break;

			if( pcMessage->FindInt32( "gid", &nGid ) != EOK )
				break;

			if( pcMessage->FindString( "name", &cName ) != EOK )
				break;

			if( pcMessage->FindString( "gecos", &cGecos ) != EOK )
				break;

			if( pcMessage->FindString( "home", &cHome ) != EOK )
				break;

			if( pcMessage->FindString( "shell", &cShell ) != EOK )
				break;

			/* Free previous details */
			free( psPasswd->pw_name );
			free( psPasswd->pw_gecos );
			free( psPasswd->pw_dir );
			free( psPasswd->pw_shell );

			/* Allocate & store new details */
			psPasswd->pw_uid = nUid;
			psPasswd->pw_gid = nGid;

			psPasswd->pw_name = (char*)calloc( 1, cName.size() + 1 );
			strcpy( psPasswd->pw_name, cName.c_str() );
			
			psPasswd->pw_gecos = (char*)calloc( 1, cGecos.size() + 1 );
			strcpy( psPasswd->pw_gecos, cGecos.c_str() );

			psPasswd->pw_dir = (char*)calloc( 1, cHome.size() + 1 );
			strcpy( psPasswd->pw_dir, cHome.c_str() );

			psPasswd->pw_shell = (char*)calloc( 1, cShell.size() + 1 );
			strcpy( psPasswd->pw_shell, cShell.c_str() );

			m_bModified = true;

			/* Update displayed details */
			UpdateList( nSelected );

			break;
		}

		case ID_USERS_DELETE:
		{
			int nSelected = m_pcUsersList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct passwd *)m_pcUsersList->GetRow( nSelected )->GetCookie().AsPointer();

			/* Delete the row */
			delete( m_pcUsersList->RemoveRow( nSelected ) );

			/* Check for an entry in the "new homes" list and remove if required */
			if( m_vNewHomes.size() > 0 )
			{
				vector<string>::iterator i;
				for( i = m_vNewHomes.begin(); i != m_vNewHomes.end(); i++ )
					if( strcmp( (*i).c_str(), m_psSelected->pw_name ) == 0 )
					{
						i = m_vNewHomes.erase( i );
						break;
					}
			}

			/* Free the memory held by the passwd struct */
			free( m_psSelected->pw_name );
			free( m_psSelected->pw_passwd );
			free( m_psSelected->pw_gecos );
			free( m_psSelected->pw_dir );
			free( m_psSelected->pw_shell );
			free( m_psSelected );
			m_psSelected = NULL;

			m_bModified = true;

			break;
		}

		case ID_USERS_SET_PASSWD:
		{
			int nSelected = m_pcUsersList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct passwd *)m_pcUsersList->GetRow( nSelected )->GetCookie().AsPointer();

			Message *pcChangeMsg = new Message( ID_USERS_NEW_PASSWORD );
			pcChangeMsg->AddInt32( "selection", nSelected );

			ChangePasswordDlg *pcDlg;

			pcDlg = new ChangePasswordDlg( Rect( 0, 0, 299, 109 ), "change_password", MSG_CHANGEPASS_TITLE, m_psSelected->pw_name, this, pcChangeMsg );
			pcDlg->CenterInWindow( GetWindow() );
			pcDlg->Show();
			pcDlg->MakeFocus( true );

			break;
		}

		case ID_USERS_NEW_PASSWORD:
		{
			int32 nSelected;
			if( pcMessage->FindInt32( "selection", &nSelected ) != EOK )
				break;

			struct passwd *psPasswd = (struct passwd *)m_pcUsersList->GetRow( nSelected )->GetCookie().AsPointer();

			std::string cNew = "";
			if( pcMessage->FindString( "passwd", &cNew ) != EOK )
				break;

			/* Change the password */
			psPasswd->pw_passwd = crypt( cNew.c_str(), "$1$" );
			m_bModified = true;

			break;
		}

		default:
			View::HandleMessage( pcMessage );
	}
}

status_t UsersView::SaveChanges( void )
{
	/* Check if there is any work to be done first */
	if( m_bModified == false )
		return EINVAL;

	/* Write the details currently held to a temp file, then move it over the current /etc/passwd */

	char zTemp[] = { "/tmp/passwd.XXXXXX" };
	int nTemp;
	if( ( nTemp = mkstemp( zTemp ) ) < 0 )
		return EIO;
	fchmod( nTemp, 0644 );
	FILE *psTemp = fdopen( nTemp, "w" );

	int nRow = 0;
	struct passwd *psEntry;
	for( int nRow = 0; nRow < m_pcUsersList->GetRowCount(); nRow++ )
	{
		psEntry = (struct passwd *)m_pcUsersList->GetRow( nRow )->GetCookie().AsPointer();

		if( putpwent( psEntry, psTemp ) < 0 )
			return EIO;
	}
	endpwent();
	fclose( psTemp );

#ifndef TEST
	/* Copy the temporary passwd file over the real file, then remove the temporary file */
	if( rename( zTemp, "/etc/passwd" ) < 0 )
	{
		/* Couldn't move it, so remove it */
		unlink( zTemp );

		/* Warn the user */
		Alert *pcAlert = new Alert( "File error", "Failed to write new file to /etc/passwd.  Your password was not changed.", Alert::ALERT_WARNING,0, "Close", NULL );
		pcAlert->CenterInWindow( GetWindow() );
		pcAlert->Go( new Invoker() );

		return EIO;
	}

	/* Create new user(s) home directories */
	if( m_vNewHomes.size() > 0 )
	{
		vector<string>::iterator i;
		for( i = m_vNewHomes.begin(); i != m_vNewHomes.end(); i++ )
		{
			for( int nRow = 0; nRow < m_pcUsersList->GetRowCount(); nRow++ )
			{
				psEntry = (struct passwd *)m_pcUsersList->GetRow( nRow )->GetCookie().AsPointer();
				if( psEntry->pw_name == (*i) )
				{
					char anSys[PATH_MAX];
					snprintf( anSys, PATH_MAX, "mkhome %s %s", psEntry->pw_dir, psEntry->pw_name );
					/* XXXKV: Debug */
					//printf("%s\n", anSys );
					system( anSys );
				}
			}
		}
	}
#endif

	return EOK;
}

void UsersView::DisplayProperties( const string cTitle, const struct passwd *psPasswd, Message *pcMessage )
{
	UserProperties *pcDlg;
	pcDlg = new UserProperties( Rect( 0, 0, 249, 199 ), "user_properties", cTitle, psPasswd, this, pcMessage );
	pcDlg->CenterInWindow( GetWindow() );
	pcDlg->Show();
	pcDlg->MakeFocus();
}

status_t UsersView::UpdateList( const int nSelected )
{
	if( nSelected < 0 || nSelected > m_pcUsersList->GetRowCount() )
		return EINVAL;

	ListViewStringRow *pcRow = static_cast<ListViewStringRow*>( m_pcUsersList->GetRow( nSelected ) );
	struct passwd *psPasswd = (struct passwd *)pcRow->GetCookie().AsPointer();

	/* Update displayed details */
	pcRow->SetString( 0, psPasswd->pw_gecos );
	pcRow->SetString( 1, psPasswd->pw_name );
	char nId[6];
	snprintf( nId, 6, "%u", psPasswd->pw_uid );
	pcRow->SetString( 2, nId );
	snprintf( nId, 6, "%u", psPasswd->pw_gid );
	pcRow->SetString( 3, nId );
	pcRow->SetString( 4, psPasswd->pw_dir );

	m_pcUsersList->Invalidate( true );

	return EOK;
}

GroupsView::GroupsView( const Rect cFrame ) : View( cFrame, "users_groups_view" )
{
	m_bModified = false;

	VLayoutNode *pcGroupsRoot = new VLayoutNode( "groups_root" );

	m_pcGroupsList = new ListView( Rect(), "groups_list", ListView::F_NO_AUTO_SORT | ListView::F_RENDER_BORDER );
	m_pcGroupsList->InsertColumn( MSG_MAINWND_TAB_GROUPS_NAME.c_str(), 250 );
	m_pcGroupsList->InsertColumn( MSG_MAINWND_TAB_GROUPS_ID.c_str(), 40 );
	m_pcGroupsList->SetSelChangeMsg( new Message( ID_GROUPS_SELECT ) );

	pcGroupsRoot->AddChild( m_pcGroupsList, 200.0f );
	pcGroupsRoot->AddChild( new VLayoutSpacer( "" ) );

	HLayoutNode *pcGroupsButtons = new HLayoutNode( "groups_buttons", 0.0f, pcGroupsRoot );

	m_pcAdd = new Button( Rect(), "add_button", MSG_MAINWND_TAB_GROUPS_BUTTON_ADD, new Message( ID_GROUPS_ADD ) );
	pcGroupsButtons->AddChild( m_pcAdd, 1.0f );

	m_pcEdit = new Button( Rect(), "edit_button", MSG_MAINWND_TAB_GROUPS_BUTTON_EDIT, new Message( ID_GROUPS_EDIT ) );
	pcGroupsButtons->AddChild( m_pcEdit, 1.0f );

	m_pcDelete = new Button( Rect(), "delete_button", MSG_MAINWND_TAB_GROUPS_BUTTON_DEL, new Message( ID_GROUPS_DELETE ) );
	pcGroupsButtons->AddChild( m_pcDelete, 1.0f );

	m_pcAdd->SetEnable( getuid() == 0 );
	m_pcEdit->SetEnable( false );
	m_pcDelete->SetEnable( false );

	pcGroupsRoot->AddChild( pcGroupsButtons );

	/* Create an internal list of all groups entries and add a row for each group */
	struct group *psEntry;
	while( ( psEntry = getgrent() ) != NULL )
	{
		struct group *psGrp = (struct group *)calloc( 1, sizeof( struct group ) );
		if( NULL == psGrp )
			break;

		/* Create a copy of the (static) group entry */
		{
			psGrp->gr_name = (char*)calloc( 1, strlen( psEntry->gr_name ) + 1 );
			strcpy( psGrp->gr_name, psEntry->gr_name );

			psGrp->gr_passwd = (char*)calloc( 1, strlen( psEntry->gr_passwd ) + 1 );
			strcpy( psGrp->gr_passwd, psEntry->gr_passwd );

			psGrp->gr_gid = psEntry->gr_gid;

			psGrp->gr_mem = (char**)calloc( 1, sizeof( psEntry->gr_mem ) );
			int n = 0;
			while( psEntry->gr_mem[n] != NULL )
			{
				psGrp->gr_mem[n] = (char*)calloc( 1, strlen( psEntry->gr_mem[n] ) + 1 );
				strcpy( psGrp->gr_mem[n], psEntry->gr_mem[n] );
				n++;
			}
		}

		/* Create a row to display group details */
		ListViewStringRow *pcRow = new ListViewStringRow();
		pcRow->AppendString( psGrp->gr_name );

		char nId[6];
		snprintf( nId, 6, "%u", psGrp->gr_gid );
		pcRow->AppendString( nId );

		/* We can retrieve the data when we need it via. the cookie */
		pcRow->SetCookie( Variant( (void*)psGrp ) );

		/* Put the row in the list */
		m_pcGroupsList->InsertRow( pcRow );
	}
	endgrent();

	/* Clean up the layout */
	pcGroupsRoot->SetBorders( Rect(5, 10, 5, 0), "add_button", "edit_button", "delete_button", NULL );
	pcGroupsRoot->SameWidth( "add_button", "edit_button", "delete_button", NULL );
	pcGroupsRoot->SetBorders( Rect(10, 10, 10, 10) );

	m_pcLayoutView = new LayoutView( GetBounds(), "groups" );
	m_pcLayoutView->SetRoot( pcGroupsRoot );
	AddChild( m_pcLayoutView );
}

GroupsView::~GroupsView()
{
	RemoveChild( m_pcLayoutView );
	delete( m_pcLayoutView );
}

void GroupsView::AllAttached( void )
{
	View::AllAttached();
	m_pcGroupsList->SetTarget( this );
	m_pcAdd->SetTarget( this );
	m_pcEdit->SetTarget( this );
	m_pcDelete->SetTarget( this );
}

void GroupsView::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case ID_GROUPS_SELECT:
		{
			if( getuid() > 0 )
				break;

			int nSelected = m_pcGroupsList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct group *)m_pcGroupsList->GetRow( nSelected )->GetCookie().AsPointer();

			m_pcEdit->SetEnable( true );

			if( 0 == m_psSelected->gr_gid )
				m_pcDelete->SetEnable( false );
			else
				m_pcDelete->SetEnable( true );

			break;
		}

		case ID_GROUPS_ADD:
		{
			struct group *psGroup = (struct group *)calloc( 1, sizeof( struct group ) );
			if( psGroup == NULL )
				break;

			/* XXXKV: We could try to be a little smarter here and work
			   out the highest GID */
			psGroup->gr_name = "new";
			psGroup->gr_gid = 100;

			Message *pcChangeMsg = new Message( ID_GROUPS_POST_ADD );
			pcChangeMsg->AddPointer( "data", (void*)psGroup );

			DisplayProperties( MSG_NEWGROUPWND_TITLE, psGroup, pcChangeMsg );
			break;
		}

		case ID_GROUPS_POST_ADD:
		{
			struct group *psGroup;
			if( pcMessage->FindPointer( "data", (void**)&psGroup ) != EOK )
				break;

			int32 nGid;
			string cName;

			if( pcMessage->FindInt32( "gid", &nGid ) != EOK )
				break;

			if( pcMessage->FindString( "name", &cName ) != EOK )
				break;

			/* Allocate & store new details */
			psGroup->gr_name = (char*)calloc( 1, cName.size() + 1 );
			strcpy( psGroup->gr_name, cName.c_str() );

			psGroup->gr_passwd = "*";
			psGroup->gr_gid = nGid;
			psGroup->gr_mem = NULL;

			/* Create a row to display users details */
			ListViewStringRow *pcRow = new ListViewStringRow();
			pcRow->AppendString( psGroup->gr_name );

			char nId[6];
			snprintf( nId, 6, "%u", psGroup->gr_gid );
			pcRow->AppendString( nId );

			/* We can retrieve the data when we need it via. the cookie */
			pcRow->SetCookie( Variant( (void*)psGroup ) );

			/* Put the row in the list */
			m_pcGroupsList->InsertRow( pcRow );

			m_bModified = true;

			break;
		}

		case ID_GROUPS_EDIT:
		{
			int nSelected = m_pcGroupsList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct group *)m_pcGroupsList->GetRow( nSelected )->GetCookie().AsPointer();

			Message *pcChangeMsg = new Message( ID_GROUPS_POST_EDIT );
			pcChangeMsg->AddInt32( "selection", nSelected );

			string cTitle;
			cTitle = MSG_EDITGROUPWND_TITLE + " " + string( m_psSelected->gr_name );

			DisplayProperties( cTitle, m_psSelected, pcChangeMsg );
			break;
		}

		case ID_GROUPS_POST_EDIT:
		{
			int32 nSelected;
			if( pcMessage->FindInt32( "selection", &nSelected ) != EOK )
				break;

			struct group *psGroup = (struct group *)m_pcGroupsList->GetRow( nSelected )->GetCookie().AsPointer();

			int32 nGid;
			string cName;

			if( pcMessage->FindInt32( "gid", &nGid ) != EOK )
				break;

			if( pcMessage->FindString( "name", &cName ) != EOK )
				break;

			/* Free previous details.  Don't modify any group password or group member entries! */
			free( psGroup->gr_name );

			/* Allocate & store new details */
			psGroup->gr_gid = nGid;

			psGroup->gr_name = (char*)calloc( 1, cName.size() + 1 );
			strcpy( psGroup->gr_name, cName.c_str() );

			m_bModified = true;

			/* Update displayed details */
			UpdateList( nSelected );

			break;
		}

		case ID_GROUPS_DELETE:
		{
			int nSelected = m_pcGroupsList->GetFirstSelected();
			if( nSelected < 0 )
				break;

			m_psSelected = (struct group *)m_pcGroupsList->GetRow( nSelected )->GetCookie().AsPointer();

			/* Delete the row */
			delete( m_pcGroupsList->RemoveRow( nSelected ) );

			/* Free the memory held by the group struct */
			free( m_psSelected->gr_name );
			free( m_psSelected->gr_passwd );
			int n = 0;
			while( m_psSelected->gr_mem[n] != NULL )
			{
				free( m_psSelected->gr_mem[n] );
				n++;
			}
			free( m_psSelected );
			m_psSelected = NULL;

			m_bModified = true;

			break;
		}

		default:
			View::HandleMessage( pcMessage );
	}
}

status_t GroupsView::SaveChanges( void )
{
	/* Check if there is any work to be done first */
	if( m_bModified == false )
		return EINVAL;

	/* Write the details currently held to a temp file, then move it over the current /etc/group */

	char zTemp[] = { "/tmp/group.XXXXXX" };
	int nTemp;
	if( ( nTemp = mkstemp( zTemp ) ) < 0 )
		return EIO;
	fchmod( nTemp, 0644 );
	FILE *psTemp = fdopen( nTemp, "w" );

	int nRow = 0;
	struct group *psEntry;
	for( int nRow = 0; nRow < m_pcGroupsList->GetRowCount(); nRow++ )
	{
		psEntry = (struct group *)m_pcGroupsList->GetRow( nRow )->GetCookie().AsPointer();

		if( putgrent( psEntry, psTemp ) < 0 )
			return EIO;
	}
	endgrent();
	fclose( psTemp );

#ifndef TEST
	/* Copy the temporary group file over the real file, then remove the temporary file */
	if( rename( zTemp, "/etc/group" ) < 0 )
	{
		/* Couldn't move it, so remove it */
		unlink( zTemp );

		/* Warn the user */
		Alert *pcAlert = new Alert( MSG_ALERTWND_GRPWRTERR, MSG_ALERTWND_GRPWRTERR_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_GRPWRTERR_OK.c_str(), NULL );
		pcAlert->CenterInWindow( GetWindow() );
		pcAlert->Go( new Invoker() );

		return EIO;
	}
#endif

	return EOK;
}

void GroupsView::DisplayProperties( const string cTitle, const struct group *psGroup, Message *pcMessage )
{
	GroupProperties *pcDlg;
	pcDlg = new GroupProperties( Rect( 0, 0, 249, 99 ), "group_properties", cTitle, psGroup, this, pcMessage );
	pcDlg->CenterInWindow( GetWindow() );
	pcDlg->Show();
	pcDlg->MakeFocus();
}

status_t GroupsView::UpdateList( const int nSelected )
{
	if( nSelected < 0 || nSelected > m_pcGroupsList->GetRowCount() )
		return EINVAL;

	ListViewStringRow *pcRow = static_cast<ListViewStringRow*>( m_pcGroupsList->GetRow( nSelected ) );
	struct group *psGroup = (struct group *)pcRow->GetCookie().AsPointer();

	/* Update displayed details */
	pcRow->SetString( 0, psGroup->gr_name );
	char nId[6];
	snprintf( nId, 6, "%u", psGroup->gr_gid );
	pcRow->SetString( 1, nId );

	m_pcGroupsList->Invalidate( true );

	return EOK;
}
