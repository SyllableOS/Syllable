// Mouse (C)opyright 2008 Jonas Jarvoll
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


#include "main.h"

using namespace os;
using namespace std;

int main( int argc, char* argv[] )
{
 	Mouse* pcBrowser = new Mouse( argc, argv );
	pcBrowser->Run();

	return( 0 );
}

AppWindow* Mouse :: m_AppWindow = NULL;

Mouse :: Mouse( int argc, char* argv[] ) : Application( "application/x-MousePreferences" )
{
	try {
		SetCatalog( "mouse.catalog" );
	} catch( ... ) {
	}

	m_AppWindow = new AppWindow( Rect( 100, 100, 400, 275 ) );
	m_AppWindow->Show();
	m_AppWindow->MakeFocus();
}



