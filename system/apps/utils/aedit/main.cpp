// AEdit -:-  (C)opyright 2000-2001 Kristian Van Der Vliet
//	  			(C)opyright 2006 Jonas Jarvoll
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
#include "settings.h"
#include "resources/aedit.h"

#include <storage/registrar.h>

#include <iostream>

using namespace std;
using namespace os;

int main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	AEditApp* pcAEditApp=new AEditApp(argc, argv);
	pcAEditApp->Run();

	return(0);
}

AEditWindow* AEditApp::pcMainWindow = NULL;

AEditApp::AEditApp(int argc, char* argv[]) : Application("application/x-aedit")
{
	int i;

	try {
		SetCatalog( "aedit.catalog" );
	} catch( errno_exception &e ) {
		cout << e.what() << endl;
	}

	// Set default filehandler
	try
	{
		RegistrarManager* regman= os::RegistrarManager::Get();
		
		/* Register playlist type */
		regman->RegisterType( "text/plain", MSG_MIMETYPE_TEXT_PLAIN );
		regman->RegisterTypeIconFromRes( "text/plain", "text_plain.png" );
		regman->RegisterTypeExtension( "text/plain", "txt" );
		regman->RegisterTypeExtension( "text/plain", "text" );
		regman->RegisterTypeExtension( "text/plain", "unx" );
		regman->RegisterTypeExtension( "text/plain", "asc" );
		regman->RegisterTypeExtension( "text/plain", "ascii" );
		regman->RegisterTypeExtension( "text/plain", "log" );
		regman->RegisterTypeExtension( "text/plain", "utf8" );
		regman->RegisterTypeExtension( "text/plain", "map" );
		regman->RegisterTypeExtension( "text/plain", "utxt" );
		regman->RegisterTypeExtension( "text/plain", "ans" );
		regman->RegisterTypeExtension( "text/plain", "dos" );
		regman->RegisterTypeExtension( "text/plain", "ini" );
		regman->RegisterAsTypeHandler( "text/plain" );
		
		regman->Put();
	}
	catch(...)
	{
	}

	pcMainWindow = new AEditWindow( Rect( 100, 125, 700, 500 ) );

	// If we do not have any arguments open empty buffer
	if(argc<2)
		pcMainWindow->CreateNewBuffer();
	else	
		for(i=1;i<argc;i++)
			pcMainWindow->Load(argv[i]);

	pcMainWindow->Show();
	pcMainWindow->MakeFocus();
}


