//  AView (C)opyright 2005 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdlib.h>
#include <gui/rect.h>
#include <util/application.h>
#include <util/settings.h>
#include <storage/registrar.h>

#include <iostream>

#include "appwindow.h"
#include "resources/AView.h"

using namespace os;

class AViewApplication : public Application
{
	public:
		AViewApplication( char **ppzArgv, int nArgc );

	private:
		AViewWindow *m_pcWindow;
};

AViewApplication::AViewApplication( char **ppzArgv, int nArgc ) : Application( "application/x-VND.syllable-aview" )
{
	// Set locale catalog
	SetCatalog("AView.catalog");

	// Register some common image formats and make ourselves the default handler for them
	try
	{
		RegistrarManager *pcRegistrar = RegistrarManager::Get();

		pcRegistrar->RegisterType( "image/png", MSG_MIMETYPE_IMAGE_PNG );
		pcRegistrar->RegisterTypeExtension( "image/png", "png" );
		pcRegistrar->RegisterTypeIcon( "image/png", Path( "/system/icons/filetypes/image_png.png" ) );
		pcRegistrar->RegisterAsTypeHandler( "image/png" );

		pcRegistrar->RegisterType( "image/jpeg", MSG_MIMETYPE_IMAGE_JPEG );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jpeg" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jpg" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jpe" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jif" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jfif" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jfi" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jff" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "pjpeg" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jls" );
		pcRegistrar->RegisterTypeExtension( "image/jpeg", "jmh" );
		pcRegistrar->RegisterTypeIcon( "image/jpeg", Path( "/system/icons/filetypes/image_jpeg.png" ) );
		pcRegistrar->RegisterAsTypeHandler( "image/jpeg" );

		pcRegistrar->RegisterType( "image/gif", MSG_MIMETYPE_IMAGE_GIF );
		pcRegistrar->RegisterTypeExtension( "image/gif", "gif" );
		pcRegistrar->RegisterTypeIcon( "image/jpeg", Path( "/system/icons/filetypes/image_gif.png" ) );
		pcRegistrar->RegisterAsTypeHandler( "image/gif" );

		pcRegistrar->Put();
	}
	catch( std::exception e )
	{
		std::cerr << e.what() << std::endl;
	}

	// Load application settings
	Settings cSettings;
	cSettings.Load();

	Rect cBounds = cSettings.GetRect( "window_position", Rect( 100,125,400,400 ) );
	bool bFitToImage = cSettings.GetBool( "fit_to_image", false );
	bool bFitToWindow = cSettings.GetBool( "fit_to_window", false );

	// Create the application window and load any initial image
	m_pcWindow = new AViewWindow( cBounds, bFitToImage, bFitToWindow, ppzArgv, nArgc );

	// We must call Show() first as Load() will call Show() on a child View
	m_pcWindow->Show();
	m_pcWindow->MakeFocus();

	// Load the first image is any were passed to us on in the arguments
	if( nArgc > 1 )
	{
		// See the appwindow HandleMessage method for an explanation
		int nIndex = 0;
		while( ( nIndex < nArgc ) && ( m_pcWindow->Load( nIndex++ ) == EIO ) )
			;

		// Disable previous navigation if this is the first image
		if( 0 == ( nIndex - 1 ) )
			m_pcWindow->DisableNav( NAV_PREV );

		// It may also be the last image
		if( nIndex == ( nArgc - 1 ) )
			m_pcWindow->DisableNav( NAV_NEXT );
	}
	else
	{
		// There is no image loaded, disable both the previous and next navigation
		m_pcWindow->DisableNav( NAV_PREV );
		m_pcWindow->DisableNav( NAV_NEXT );
	}
}

int main( int argc, char **argv )
{
	AViewApplication *pcApplication = new AViewApplication( argv, argc );
	pcApplication->Run();

	return EXIT_SUCCESS;
}

