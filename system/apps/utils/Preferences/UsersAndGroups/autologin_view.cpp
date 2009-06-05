
#include <pwd.h>
#include <grp.h>
#include <unistd.h>


#include <gui/layoutview.h>
#include <util/message.h>
#include <util/settings.h>

#include "autologin_view.h"
#include "resources/UsersAndGroups.h"

using namespace os;

AutoLoginView::AutoLoginView( const Rect& cFrame )  : View( cFrame, "autologin_pane" )
{
	m_bModified = false;
	m_pcSettings = NULL;
	
	HLayoutNode* pcLayoutRoot = new HLayoutNode( "autologin_root" );
	VLayoutNode* pcLayoutMain = new VLayoutNode( "autologin_main" );
	
	pcLayoutMain->AddChild( new VLayoutSpacer( "v_spacer_1" ) );
	
	m_pcCheckbox = new CheckBox( Rect(), "autologin_checkbox", MSG_MAINWND_TAB_AUTOLOGIN_ENABLE, new Message( ID_AUTOLOGIN_ENABLE ) );
	m_pcCheckbox->EnableStatusChanged( true );
	pcLayoutMain->AddChild( m_pcCheckbox, 0.0f );
	
	pcLayoutMain->AddChild( new VLayoutSpacer( "v_spacer_2", 5.0f, 5.0f ) );
	
	m_pcUserList = new DropdownMenu( Rect(), "autologin_user" );
	m_pcUserList->SetReadOnly( true );
	m_pcUserList->SetSelectionMessage( new Message( ID_AUTOLOGIN_SEL_CHANGE ) );
	pcLayoutMain->AddChild( m_pcUserList, 0.0f );
	pcLayoutMain->SetHAlignment( ALIGN_CENTER );	/* This doesn't seem to work?! */
	pcLayoutMain->SetVAlignment( ALIGN_CENTER );
	
	pcLayoutMain->AddChild( new VLayoutSpacer( "v_spacer_3" ) );

	PopulateMenu();
	LoadSettings();
	
	pcLayoutRoot->AddChild( new HLayoutSpacer( "h_spacer_1", 10.0f ) );
	pcLayoutRoot->AddChild( pcLayoutMain );
	pcLayoutRoot->AddChild( new HLayoutSpacer( "h_spacer_2", 10.0f ) );
	
	LayoutView* pcLayoutView = new LayoutView( GetBounds(), "autologin_layoutview" );
	pcLayoutView->SetRoot( pcLayoutRoot );
	AddChild( pcLayoutView );
}

AutoLoginView::~AutoLoginView()
{
}

void AutoLoginView::AllAttached( void )
{
	m_pcCheckbox->SetTarget( this );
	m_pcUserList->SetTarget( this );
}

void AutoLoginView::PopulateMenu()
{
	/* Examine the user list and populate the menu with non-system users. */
	/* TODO: It would be nice to update this list automatically if the user creates or deletes a user in the other tab. */
	m_nDefaultUser = -1;	/* We save the first non-root user as the default setting */
	
	struct passwd *psPwd;
	while( ( psPwd = getpwent() ) != NULL )
	{
		/* Create a row to display users details */
		if( psPwd->pw_uid == 0 || psPwd->pw_uid >= 100 )
		{
			m_acUsers.push_back( psPwd->pw_name );
			String zTmp;
			zTmp.Format( "%s (%s)", psPwd->pw_gecos, psPwd->pw_name );
			m_pcUserList->AppendItem( zTmp );
			
			if( psPwd->pw_uid >= 100 && m_nDefaultUser == -1 )
			{
				m_nDefaultUser = m_acUsers.size() - 1;
			}
		}
	}
	endpwent();
}


void AutoLoginView::LoadSettings()
{
	if( m_pcUserList->GetItemCount() == 0 )
	{
		/* No valid users!? */
		m_pcCheckbox->SetEnable( false );
		m_pcUserList->SetEnable( false );
		return;
	}
	
	m_pcFile = NULL;
	m_pcSettings = NULL;
	try {
		m_pcFile = new File( "/system/config/autologin", O_RDWR | O_CREAT );	/* TODO: use correct permissions */
		m_pcSettings = new Settings( m_pcFile );
		m_pcSettings->Load();
	} catch( ... )
	{
		/* Failed to load the file! Disable everything to be safe. */
		if( m_pcFile ) delete( m_pcFile );
		if( m_pcSettings ) delete( m_pcSettings );
		m_pcSettings = NULL;
		m_pcCheckbox->SetEnable( false );
		m_pcUserList->SetEnable( false );
		return;
	}
	
	bool bIsAutoLoginEnabled = m_pcSettings->GetBool( "auto_login", false );
	String zUser = m_pcSettings->GetString( "auto_login_user", "" );
	bool bFound = false;
	
	if( bIsAutoLoginEnabled && zUser != "" )
	{
		m_pcCheckbox->SetValue( Variant( true ), false );
		
		for( int i = 0; i < m_acUsers.size(); i++ )
		{
			if( m_acUsers[i] == zUser )
			{
				m_pcUserList->SetSelection( i, false );
				bFound = true;
				break;
			}
		}
	}
	else
	{
		m_pcCheckbox->SetValue( Variant( false ), false );
		
		for( int i = 0; i < m_acUsers.size(); i++ )
		{
			if( m_acUsers[i] == zUser )
			{
				m_pcUserList->SetSelection( i, false );
				bFound = true;
				break;
			}
		}
	}
	
	/* If we didn't find the user in the list, select the first non-root user */
	if( !bFound && m_nDefaultUser != -1 )
	{
		m_pcUserList->SetSelection( m_nDefaultUser, false );
	}
	
	if( getuid() != 0 )
	{
		m_pcCheckbox->SetEnable( false );
		m_pcUserList->SetEnable( false );
		return;
	}
	
	m_pcCheckbox->SetEnable( true );
	m_pcUserList->SetEnable( m_pcCheckbox->GetValue().AsBool() );
}

status_t AutoLoginView::SaveChanges()
{
	/* Only write the file if necessary */
	if( !m_bModified ) return( 0 );
	
	if( m_pcSettings == NULL ) return( -1 );
	
	
	bool bIsAutoLoginEnabled = m_pcCheckbox->GetValue().AsBool();
	int nIndex = m_pcUserList->GetSelection();
	String zUser = ( nIndex >= 0 && nIndex < m_acUsers.size() ? m_acUsers[ nIndex ] : "" );
	
	if( bIsAutoLoginEnabled && zUser == "" )
	{
		printf( "Users & Groups prefs: trying to save invalid autologin settings!\n" );
		return( -1 );
	}
	
	if( bIsAutoLoginEnabled )
	{
		m_pcSettings->RemoveName( "auto_login" );
		m_pcSettings->SetBool( "auto_login", true );
		
		m_pcSettings->RemoveName( "auto_login_user" );
		m_pcSettings->SetString( "auto_login_user", zUser );
	}	
	else
	{
		m_pcSettings->RemoveName( "auto_login" );
		m_pcSettings->SetBool( "auto_login", false );
	}
	
	m_pcSettings->Save();
	m_pcFile->Flush();
}

void AutoLoginView::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case ID_AUTOLOGIN_ENABLE:
		{
			bool bState = m_pcCheckbox->GetValue().AsBool();

			m_pcUserList->SetEnable( bState );

			m_bModified = true;
			break;
		}
		
		case ID_AUTOLOGIN_SEL_CHANGE:
		{
			m_bModified = true;
			break;
		}
		
		default:
		{
			View::HandleMessage( pcMessage );
		}
		
	}
}

