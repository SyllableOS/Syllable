#include "application.h"
#include "mainwindow.h"
#include "resources/Contact.h"

App::App( const os::String& cArgv ) : os::Application( "application/x-vnd.syllable-contact" )
{
	SetCatalog("Contact.catalog");

	// Register our Contact format and make ourselves the default handler for it
	try
	{
		RegistrarManager *pcRegistrar = RegistrarManager::Get();

		pcRegistrar->RegisterType( "application/x-contact", MSG_MIMETYPE_APPLICATION_X_CONTACT );
		pcRegistrar->RegisterTypeIcon( "application/x-contact", Path( "/system/icons/filetypes/application_x_contact.png" ) );
		pcRegistrar->RegisterAsTypeHandler( "application/x-contact" );

		pcRegistrar->Put();
	}
	catch( std::exception e )
	{
	}


	m_pcMainWindow = new MainWindow( cArgv );
	m_pcMainWindow->CenterInScreen();
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();
}

