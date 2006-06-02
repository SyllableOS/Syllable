// sIDE		(C) 2001-2005 Arno Klenke
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundati on; either version 2 of the License, or
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
//
//
#include <atheos/threads.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/treeview.h>
#include <gui/radiobutton.h>
#include <gui/separator.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/layoutview.h>
#include <util/application.h>
#include <util/message.h>
#include <storage/path.h>
#include <storage/registrar.h>

#include "messages.h"
#include "project.h"
#include "GroupWindow.h"
#include "AddWindow.h"
#include "AddLibrary.h"
#include "ProjectPrefs.h"
#include "ProjectWindow.h"

class sIDEApp : public os::Application
{
public:
	sIDEApp( os::String zFileName, bool bLoad ) : os::Application( "application/x-vnd.sIDE" ) {
		/* Register filetypes */
		m_pcManager = NULL;
		try
		{
			m_pcManager = os::RegistrarManager::Get();
			m_pcManager->RegisterType( "application/x-side", "sIDE project" );
			m_pcManager->RegisterTypeExtension( "application/x-side", "side" );
			m_pcManager->RegisterAsTypeHandler( "application/x-side" );
			m_pcManager->RegisterTypeIconFromRes( "application/x-side", "application_side_project.png" );
		} catch( ... ) {
		}
		
		/* Select string catalogue */
		try {
			SetCatalog( "sIDE.catalog" );
		} catch( ... ) {
			printf( "Failed to load catalog file!" );
		}
					
		m_pcWin = new ProjectWindow( os::Rect( 50, 50, 380, 450 ), MSG_PWND_TITLE );
		m_pcWin->CenterInScreen();
		m_pcWin->Show();
		m_pcWin->MakeFocus();
	
		if( bLoad )
		{
			os::Message cMsg( os::M_LOAD_REQUESTED );
			cMsg.AddString( "file/path", zFileName );
			cMsg.AddPointer( "source", m_pcWin->m_pcLoadRequester );
			m_pcWin->PostMessage( &cMsg, m_pcWin );
		}
	}
	~sIDEApp() {
		if( m_pcManager )
			m_pcManager->Put();
	}
	bool OkToQuit()
	{
		m_pcWin->Close();
		return( true );
	}
private:
	os::RegistrarManager* m_pcManager;
	ProjectWindow* m_pcWin;
};


int main( int argc, char *argv[] )
{
	sIDEApp* pcApp;
	if ( argc > 1 )
	{
		pcApp = new sIDEApp( argv[1], true );
	}
	else
	{
		pcApp = new sIDEApp( "", false );
	}
	
	pcApp->Run();
	return( 0 );
}































