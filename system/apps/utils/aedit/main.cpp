// AEdit -:-  (C)opyright 2000-2001 Kristian Van Der Vliet
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

#include <iostream>

using namespace std;

int main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	if(argc>1)
	{
		AEditApp* pcAEditApp=new AEditApp(argv[1],true);
		pcAEditApp->Run();
	}
	else
	{
		AEditApp* pcAEditApp=new AEditApp("",false);
		pcAEditApp->Run();
	}

	return(0);
}

AEditApp::AEditApp(char* pzFilename, bool bLoad) : Application("application/x-VND.Vanders-AEdit")
{
	try {
	SetCatalog( "aedit.catalog" );
	} catch( errno_exception &e ) {
		cout << e.what() << endl;
	}

	pcMainWindow = new AEditWindow(Rect(100,125,700,500));

	if(bLoad)
	{
		pcMainWindow->LoadOnStart(pzFilename);
	}

	pcMainWindow->Show();
	pcMainWindow->MakeFocus();
}


