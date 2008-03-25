#include <storage/registrar.h>
#include "application.h"
#include "mainwindow.h"

App::App( os::String zFileName, bool bLoad ) : os::Application( "application/x-vnd.syllable-InterfaceDesigner" )
{
	/* Register filetypes */
	os::RegistrarManager* pcManager = NULL;
	try
	{
		pcManager = os::RegistrarManager::Get();
		pcManager->RegisterType( "application/x-interface", "LayoutEditor Interface" );
		pcManager->RegisterTypeExtension( "application/x-interface", "if" );
		pcManager->RegisterAsTypeHandler( "application/x-interface" );
		pcManager->RegisterTypeIconFromRes( "application/x-interface", "application_interface.png" );		
		pcManager->Put();
	} catch( ... ) {
	}
	
	/* Create window */
	m_pcMainWindow = new MainWindow();
	m_pcMainWindow->CenterInScreen();
	m_pcMainWindow->MoveBy( -320, 0 );
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();

	/* Load file */	
	if( bLoad )
	{
		os::Message cMsg( os::M_LOAD_REQUESTED );
		cMsg.AddString( "file/path", zFileName );
		m_pcMainWindow->PostMessage( &cMsg, m_pcMainWindow );
	}
}

/* Static function to convert a string to code. Used to allow translated string */
os::String ConvertStringToCode( os::String zString )
{
	if( zString.substr( 0, 1 ) == "\\" )
	{
		return( zString.substr( 2, zString.Length() - 2 ) );
	}
	return( os::String( "\"" ) + zString + "\"" );
}


/* Static function that can look up a string in the loaded catalog file */
os::String GetString( os::String zString )
{
	if( zString.substr( 0, 1 ) == "\\" )
	{
		App* pcApp = static_cast<App*>( os::Application::GetInstance() );
		return( pcApp->m_pcMainWindow->GetString( zString ) );
	}
	return( zString );
}

/* Returns the current catalog */
os::CCatalog* GetCatalog()
{
	App* pcApp = static_cast<App*>( os::Application::GetInstance() );
	return( pcApp->m_pcMainWindow->GetCatalog() );
}















