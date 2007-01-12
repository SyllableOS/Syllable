/*
 *  Login Details Preferences for Syllable
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
#include <unistd.h>

#include <string>

#include <gui/image.h>
#include <gui/imageview.h>
#include <gui/stringview.h>
#include <gui/requesters.h>
#include <util/application.h>
#include <util/resources.h>
#include <storage/file.h>
#include <storage/path.h>

#include <login_window.h>

#include "resources/Login.h"

using namespace os;
using namespace std;

LoginWindow::LoginWindow( const os::Rect cFrame ) : Window( cFrame, "Login", MSG_MAINWND_TITLE )
{
	m_nUid = getuid();

	/* Look up the username of the real user that launched this process. */
	struct passwd *psPwd = getpwuid( m_nUid );
	if( psPwd != NULL )
			m_cLogin = psPwd->pw_name;

	Rect cBounds = GetBounds(), cChildFrame, cControlFrame;
	Point cSize;

	cChildFrame.left = cBounds.left + 5;
	cChildFrame.top = cBounds.top + 5;
	cChildFrame.right = cBounds.right - 5;

	Resources cRes( get_image_id() );

	/* The main part of the window contains three views */

	/* The top of the window contains some basic information, with an icon and some text */
	cChildFrame.bottom = cChildFrame.top + 80;

	m_pcInfoFrame = new FrameView( cChildFrame, "login_info_frame", MSG_MAINWND_INFORMATION, CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT );
	HLayoutNode *pcInfoLayoutNode = new HLayoutNode( "login_info_layout" );
	Rect cInnerBounds = m_pcInfoFrame->GetBounds();
	cInnerBounds.left += 5;
	cInnerBounds.top += 20;
	cInnerBounds.right -= 5;
	cInnerBounds.bottom -= 5;

	/* This icon doesn't do anything other than look pretty */
	BitmapImage *pcLockIcon = new BitmapImage();
	pcLockIcon->Load( cRes.GetResourceStream( "lock.png" ) );
	Point cIconSize = pcLockIcon->GetSize();
	m_pcLockIconView = new ImageView( Rect( 0, 0, cIconSize.x, cIconSize.y ), "login_info_icon", pcLockIcon );
	pcInfoLayoutNode->AddChild( m_pcLockIconView, 0.0f );

	const unsigned MSG_BUF_LEN = 256;
	/* Ugly Workaround to make localization work. Someone should have a look at it... */	
	char acMsgBuf[MSG_BUF_LEN] = "";
	snprintf( acMsgBuf, MSG_BUF_LEN, MSG_MAINWND_INFORMATION_NOPERM.c_str() );

	StringView *pcInfoText = new StringView( Rect(), "login_info_text", "" );
	pcInfoLayoutNode->AddChild( pcInfoText );

	LayoutView *pcInfoLayoutView = new LayoutView( cInnerBounds, "login_info" );
	pcInfoLayoutView->SetRoot( pcInfoLayoutNode );
	m_pcInfoFrame->AddChild( pcInfoLayoutView );
	AddChild( m_pcInfoFrame );

	/* The user can pick an icon to display during login */
	cChildFrame.top = cChildFrame.bottom + 10;
	cChildFrame.bottom = cChildFrame.top + 130;

	m_pcIconFrame = new FrameView( cChildFrame, "login_icon_frame", MSG_MAINWND_LOGINICON, CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT );

	cInnerBounds = m_pcIconFrame->GetBounds();
	cInnerBounds.left += 10;
	cInnerBounds.top += 20;
	cInnerBounds.right -= 10;
	cInnerBounds.bottom -= 5;

	cControlFrame = cInnerBounds;

	StringView *pcIconText = new StringView( Rect(), "login_icon_text", MSG_MAINWND_LOGINICON_TEXT );

	cSize = pcIconText->GetPreferredSize( false );
	cControlFrame.right = cControlFrame.left + cSize.x;
	cControlFrame.bottom = cControlFrame.top + cSize.y;
	pcIconText->SetFrame( cControlFrame );

	m_pcIconFrame->AddChild( pcIconText );

	/* Display the icon for the current user. */
	cControlFrame.left = cInnerBounds.right - 96;
	cControlFrame.right = cInnerBounds.right;
	cControlFrame.bottom = cInnerBounds.top + 96;

	/* Load users icon */
	BitmapImage *pcUserIcon = GetUserIcon();

	m_pcIconButton = new ImageButton( cControlFrame, "login_icon", "Login Icon", new Message( ID_CHANGE_ICON ), pcUserIcon, ImageButton::IB_TEXT_BOTTOM, true, false, false, CF_FOLLOW_RIGHT );
	m_pcIconFrame->AddChild( m_pcIconButton );

	AddChild( m_pcIconFrame );

	/* Requester to allow the user to pick a new icon for themselves */
	m_pcIconRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "/system/icons/", FileRequester::NODE_FILE, false );

	/* Change password */
	cChildFrame.top = cChildFrame.bottom + 10;
	cChildFrame.bottom = cChildFrame.top + 170;

	m_pcPasswordFrame = new FrameView( cChildFrame, "login_password_frame", MSG_MAINWND_CHANGEPASS, CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT );

	cInnerBounds = m_pcPasswordFrame->GetBounds();
	cInnerBounds.left += 10;
	cInnerBounds.top += 20;
	cInnerBounds.right -= 10;
	cInnerBounds.bottom -= 5;

	cControlFrame = cInnerBounds;

	/* Current password, if the user is not UID 0 (root) */
	if( m_nUid > 0 )
	{
		StringView *pcCurrentPasswordText = new StringView( Rect(), "login_password_current_text", MSG_MAINWND_CHANGEPASS_CURRENTPASS );

		cSize = pcCurrentPasswordText->GetPreferredSize( false );
		cControlFrame.top += 3;
		cControlFrame.right = cControlFrame.left + cSize.x;
		cControlFrame.bottom = cControlFrame.top + cSize.y;
		pcCurrentPasswordText->SetFrame( cControlFrame );

		m_pcPasswordFrame->AddChild( pcCurrentPasswordText );

		m_pcCurrentPassword = new TextView( Rect(), "current_password", "", CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
		m_pcCurrentPassword->SetPasswordMode();

		cControlFrame = cInnerBounds;
		cSize = m_pcCurrentPassword->GetPreferredSize( true );
		cControlFrame.left = 120;
		cControlFrame.bottom = cControlFrame.top + cSize.y;
		m_pcCurrentPassword->SetFrame( cControlFrame );

		m_pcPasswordFrame->AddChild( m_pcCurrentPassword, 100.0f );

		cControlFrame.top = cControlFrame.bottom + 20;
		cControlFrame.left = cInnerBounds.left;
	}
	else
		m_pcCurrentPassword = NULL;

	/* New password */
	StringView *pcNew1PasswordText = new StringView( Rect(), "login_password_new1_text", MSG_MAINWND_CHANGEPASS_NEWPASS );

	cSize = pcNew1PasswordText->GetPreferredSize( false );
	cControlFrame.top += 3;
	cControlFrame.right = cInnerBounds.left + cSize.x;
	cControlFrame.bottom = cControlFrame.top + cSize.y;
	pcNew1PasswordText->SetFrame( cControlFrame );

	m_pcPasswordFrame->AddChild( pcNew1PasswordText );

	m_pcNew1Password = new TextView( Rect(), "new_1_password", "", CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
	m_pcNew1Password->SetPasswordMode();

	cSize = m_pcNew1Password->GetPreferredSize( true );
	cControlFrame.left = 120;
	cControlFrame.right = cInnerBounds.right;
	cControlFrame.bottom = cControlFrame.top + cSize.y;
	m_pcNew1Password->SetFrame( cControlFrame );

	m_pcPasswordFrame->AddChild( m_pcNew1Password, 100.0f );

	cControlFrame.top = cControlFrame.bottom + 20;
	cControlFrame.left = cInnerBounds.left;

	/* Confirm new password */
	StringView *pcNew2PasswordText = new StringView( Rect(), "login_password_new2_text", MSG_MAINWND_CHANGEPASS_CONFIRMPASS );

	cSize = pcNew2PasswordText->GetPreferredSize( false );
	cControlFrame.top += 3;
	cControlFrame.right = cInnerBounds.left + cSize.x;
	cControlFrame.bottom = cControlFrame.top + cSize.y;
	pcNew2PasswordText->SetFrame( cControlFrame );

	m_pcPasswordFrame->AddChild( pcNew2PasswordText );

	m_pcNew2Password = new TextView( Rect(), "new_2_password", "", CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
	m_pcNew2Password->SetPasswordMode();

	cSize = m_pcNew1Password->GetPreferredSize( true );
	cControlFrame.left = 120;
	cControlFrame.right = cInnerBounds.right;
	cControlFrame.bottom = cControlFrame.top + cSize.y;
	m_pcNew2Password->SetFrame( cControlFrame );

	m_pcPasswordFrame->AddChild( m_pcNew2Password, 100.0f );

	AddChild( m_pcPasswordFrame );

	/* The Apply/Save/Cancel buttons live in a layoutview of their own */
	Rect cButtonFrame = cBounds;
	cButtonFrame.top = cButtonFrame.bottom - 30;

	m_pcButtonLayout = new LayoutView( cButtonFrame, "login_buttons", NULL, CF_FOLLOW_LEFT | CF_FOLLOW_BOTTOM | CF_FOLLOW_RIGHT );

	VLayoutNode *pcButtonsRoot = new VLayoutNode( "login_buttons_root" );
	pcButtonsRoot->SetBorders( Rect( 5, 4, 5, 4 ) );
	pcButtonsRoot->AddChild( new VLayoutSpacer( "login_buttons_v_spacer", 1.0f ) );

	HLayoutNode *pcButtons = new HLayoutNode( "login_buttons_buttons"/*, 0.0f*/ );
	pcButtons->AddChild( new HLayoutSpacer( "login_buttons_h_spacer", 200.0f ) );
	Button *pcOkButton = new Button( Rect(), "login_buttons_ok", MSG_MAINWND_BUTTON_OK, new Message( ID_WINDOW_OK ) );
	pcButtons->AddChild( pcOkButton );
	pcButtons->AddChild( new HLayoutSpacer( "login_buttons_h_spacer" ) );
	Button *pcCancelButton = new Button( Rect(), "login_buttons_cancel", MSG_MAINWND_BUTTON_CANCEL, new Message( ID_WINDOW_CANCEL ) );
	pcButtons->AddChild( pcCancelButton );
	pcButtons->AddChild( new HLayoutSpacer( "login_buttons_h_spacer" ) );

	pcButtonsRoot->AddChild( pcButtons );
	m_pcButtonLayout->SetRoot( pcButtonsRoot );
	AddChild( m_pcButtonLayout );

	/* The information text changes depending on the users euid/gid and some controls may be disabled.  If the
	   effective user ID is 0, its probably because we're running setuid so people can change their passwords. */
	if( geteuid() == 0 )
	{
		/* Create a short message */
		snprintf( acMsgBuf, MSG_BUF_LEN, MSG_MAINWND_INFORMATION_WHO.c_str(), m_cLogin.c_str() );
	}
	else
	{
		pcLockIcon->ApplyFilter( Image::F_GRAY );
		m_pcIconButton->SetEnable( false );

		m_pcCurrentPassword->SetEnable( false );
		m_pcNew1Password->SetEnable( false );
		m_pcNew2Password->SetEnable( false );

	}
	pcInfoText->SetString( acMsgBuf );

	/* Make it all keyboard friendly */
	int nTab = 0;
	m_pcIconButton->SetTabOrder( nTab++ );
	if( m_nUid > 0 )
		m_pcCurrentPassword->SetTabOrder( nTab++ );
	m_pcNew1Password->SetTabOrder( nTab++ );
	m_pcNew2Password->SetTabOrder( nTab++ );
	pcOkButton->SetTabOrder( nTab++ );
	pcCancelButton->SetTabOrder( nTab++ );

	/* Focus the controls */
	SetDefaultButton( pcOkButton );

	/* Don't let the window get too small */ 
	SetSizeLimits( Point( 399, 449 ), Point( 4096, 449 ) );

	/* Set an application icon */
	BitmapImage *pcAppIcon = new BitmapImage();
	pcAppIcon->Load( cRes.GetResourceStream( "icon48x48.png" ) );
	SetIcon( pcAppIcon->LockBitmap() );
	delete( pcAppIcon );
}

LoginWindow::~LoginWindow()
{
	os::Image* pcImage = m_pcLockIconView->GetImage();
	m_pcLockIconView->SetImage( NULL );
	delete( pcImage );
	
	RemoveChild( m_pcInfoFrame );
	delete( m_pcInfoFrame );

	RemoveChild( m_pcIconFrame );
	delete( m_pcIconFrame );

	RemoveChild( m_pcPasswordFrame );
	delete( m_pcPasswordFrame );

	RemoveChild( m_pcButtonLayout );
	delete( m_pcButtonLayout );
	

}

void LoginWindow::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case ID_CHANGE_ICON:
		{
			m_pcIconRequester->Show();
			break;
		}

		case M_LOAD_REQUESTED:
		{
			const char *pzFilename;
			if( pcMessage->FindString( "file/path", &pzFilename ) == EOK )
			{
				m_cIcon = pzFilename;

				/* Display selected icon */
				BitmapImage *pcIcon = new BitmapImage();
				try
				{
					File cUserIcon( m_cIcon );
					pcIcon->Load( &cUserIcon );
					m_pcIconButton->SetImage( pcIcon );
				}
				catch(...)
				{
					/* Bail out */
					OkToQuit();
				}
			}

			break;
		}

		case ID_WINDOW_OK:
		{
			std::string cCurrentPassword;
			if( m_nUid > 0 )
				cCurrentPassword = m_pcCurrentPassword->GetBuffer()[0];

			if( m_nUid == 0 || cCurrentPassword != "" )
			{
				if( m_nUid > 0 )
					if( Validate( cCurrentPassword ) != EOK )
						break;

				/* Ensure the password entered match & are not empty */
				std::string cNew1, cNew2;

				cNew1 = m_pcNew1Password->GetBuffer()[0];
				cNew2 = m_pcNew2Password->GetBuffer()[0];

				if( cNew1 != "" && cNew2 != "" ) 
					if( cNew1 == cNew2 )
					{
						if( SavePassword( cNew1 ) != EOK )
						{
							/* Warn the user */
							Alert *pcAlert = new Alert( MSG_ALERTWND_FILEERROR, MSG_ALERTWND_FILEERROR_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_FILEERROR_OK.c_str(), NULL );
							pcAlert->CenterInWindow( this );
							pcAlert->Go();
						}
					}
					else
					{
						/* Password mismatch */
						Alert *pcAlert = new Alert( MSG_ALERTWND_NOMATCH, MSG_ALERTWND_NOMATCH_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_NOMATCH_OK.c_str(), NULL );
						pcAlert->CenterInWindow( this );
						pcAlert->Go( new Invoker() );

						break;
					}
			}

			/* Copy new icon if the user has changed it */
			if( m_cIcon != "" )
				SetUserIcon();

			/* Fall thru */
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

bool LoginWindow::OkToQuit()
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

status_t LoginWindow::Validate( const std::string cOld )
{
	if( m_nUid > 0 )
	{
		/* If the user is not root, compare the users password to the password they have suppplied */
		struct passwd *psEntry = getpwuid( m_nUid );

		if( strcmp( psEntry->pw_passwd, crypt( cOld.c_str(), "$1$" ) ) != 0 )
		{
			/* Password does not match */
			Alert *pcAlert = new Alert( MSG_ALERTWND_INCORRECTPASS, MSG_ALERTWND_INCORRECTPASS_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_INCORRECTPASS_OK.c_str(), NULL );
			pcAlert->CenterInWindow( this );
			pcAlert->Go( new Invoker() );

			return EINVAL;
		}
	}

	/* The user can change their password.   */
	return EOK;
}

status_t LoginWindow::SavePassword( const std::string cNew )
{
	/* Read the passwd file and re-write it to a temp file.  If we get to this user, change their password
	   before writing it to the temp file. */

	char zTemp[] = { "/tmp/passwd.XXXXXX" };
	int nTemp;
	if( ( nTemp = mkstemp( zTemp ) ) < 0 )
		return EIO;
	fchmod( nTemp, 0644 );
	FILE *psTemp = fdopen( nTemp, "w" );

	struct passwd *psEntry;
	while( ( psEntry = getpwent() ) != NULL )
	{
		if( psEntry->pw_uid == m_nUid )
			psEntry->pw_passwd = crypt( cNew.c_str(), "$1$" );

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
		return EIO;
	}
#else
	printf( "passwd writen to %s\n", zTemp );
#endif

	return EOK;
}

BitmapImage * LoginWindow::GetUserIcon( void )
{
	BitmapImage *pcIcon = new BitmapImage();
	String cPath = String( "/system/icons/users/" ) + m_cLogin + String( ".png" );

	/* If the file exists, load that, otherwise load the default icon */
	try
	{
		File cUserIcon( cPath );
		pcIcon->Load( &cUserIcon );
	}
	catch( std::exception &e )
	{
		Resources cRes( get_image_id() );
		pcIcon->Load( cRes.GetResourceStream( "default_user.png" ) );
	}
	return pcIcon;
}

status_t LoginWindow::SetUserIcon()
{
	String cPath = String( "/system/icons/users/" ) + m_cLogin + String( ".png" );

	try
	{
		File cIn( m_cIcon );
		File cOut( cPath, O_WRONLY | O_CREAT );

		/* Copy the file to the users icon */
		char pBuffer[4096];
		ssize_t nSize;
		while( nSize = cIn.Read( pBuffer, 4096 ) )
			cOut.Write( pBuffer, nSize );
	}
	catch( ... )
	{
		return EIO;
	}
	
	return EOK;
}

