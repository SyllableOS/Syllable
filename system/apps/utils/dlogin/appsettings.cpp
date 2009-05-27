#include "appsettings.h"

#include <storage/file.h>
#include <storage/memfile.h>

using namespace os;


AppSettings::AppSettings()
{
	/* Load the autologin file then close it */
	try {
		File* pcAutoLoginFile = new File( "/system/config/autologin", O_RDONLY );
		Settings* pcAutoLoginSettings = new Settings( pcAutoLoginFile );
		pcAutoLoginSettings->Load();
		
		m_bAutoLogin = pcAutoLoginSettings->GetBool( "auto_login", false );
		m_zAutoLoginUser = pcAutoLoginSettings->GetString( "auto_login_user", "" );
		
		delete( pcAutoLoginSettings );
	} catch( ... ) {
		m_bAutoLogin = false;
		m_zAutoLoginUser = "";
	}
	
	/* Load the settings file */
	try {
		m_pcSettingsFile = new File( "/system/config/login", O_RDWR | O_CREAT );
		m_pcSettings = new Settings( m_pcSettingsFile );
		m_pcSettings->Load();
	} catch( ... ) {
		dbprintf( "Could not access settings file!\n" );
		if( m_pcSettings ) delete( m_pcSettings );
		else if( m_pcSettingsFile ) delete( m_pcSettingsFile );
		
		m_pcSettingsFile = NULL;
		m_pcSettings = NULL;
	}
}

AppSettings::~AppSettings()
{
}

status_t AppSettings::Save()
{
	if( m_pcSettings == NULL ) return( -1 );
	status_t nResult = m_pcSettings->Save();
	m_pcSettingsFile->Flush();
	return( nResult );
}

bool AppSettings::IsAutoLoginEnabled()
{
	return( m_bAutoLogin );
}

String AppSettings::GetAutoLoginUser()
{
	return( m_zAutoLoginUser );
}

String AppSettings::GetActiveUser()
{
	if( m_pcSettings == NULL ) return( "" );
	
	return( m_pcSettings->GetString( "active_user", "" ) );
}

status_t AppSettings::SetActiveUser( const String& zUser )
{
	if( m_pcSettings == NULL ) return( -1 );

	return( m_pcSettings->SetString( "active_user", zUser ) );
}

String AppSettings::FindKeymapForUser( const String& zUserName )
{
	if( m_pcSettings == NULL ) return( "" );

	/* Per-user most-recently-used keymaps are stored in a message 'keymaps'.
	   We treat the 'keymaps' message as a map from username to keymap, using the member name as the index.
	 */
	Message cMsg;
	if( m_pcSettings->FindMessage( "keymaps", &cMsg ) != 0 ) { return( "" ); }
	
	String zKeymap;
	if( cMsg.FindString( zUserName.c_str(), &zKeymap ) != 0 ) { return( "" ); }
	
	return( zKeymap );
}


status_t AppSettings::SetKeymapForUser( const String& zUser, const String& zKeymap )
{
	if( m_pcSettings == NULL ) return( -1 );

	Message cMsg;
	m_pcSettings->FindMessage( "keymaps", &cMsg );
	String zOldKeymap;

	int nResult;
	/* Only save the config file if the keymap has actually changed */
	if( cMsg.FindString( zUser.c_str(), &zOldKeymap ) != 0 || zOldKeymap != zKeymap )
	{
		cMsg.RemoveName( zUser.c_str() );
		nResult = cMsg.AddString( zUser.c_str(), zKeymap );
		if( nResult != 0 ) return( nResult );
		
		m_pcSettings->RemoveName( "keymaps" );
		nResult = m_pcSettings->AddMessage( "keymaps", &cMsg );
	}

	return( nResult );
}


