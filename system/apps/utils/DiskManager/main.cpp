/*
 *  DiskManager - AtheOS disk partitioning tool.
 *  Copyright (C) 2000 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/layoutview.h>
#include <gui/tabview.h>
#include <gui/progressbar.h>
#include <gui/button.h>
#include <gui/requesters.h>
#include <gui/desktop.h>

#include "diskview.h"
#include "resources/DiskManager.h"

using namespace os;

class MainWindow : public Window
{
public:
    MainWindow( const std::string& cTitle );

    virtual bool	OkToQuit();
private:
    void SetupMenus();
    Menu*	m_pcMenu;
    View*	m_pcView;
};


class	PEditApp : public Application
{
public:
    PEditApp();
    ~PEditApp();
private:
    MainWindow* m_pcMainWindow;
};
/*
PartitionInfo::~PartitionInfo()
{
    delete m_psExtended;
}

*/
MainWindow::MainWindow( const std::string& cTitle ) : Window( Rect(0,0,300,200), "main_window", cTitle )
{
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

    SetupMenus();

    Rect cMainFrame = GetBounds();
    Rect cMenuFrame = m_pcMenu->GetFrame();

    cMainFrame.top = cMenuFrame.bottom + 1.0f;

    cMainFrame.Resize( 5, 5, -5, -5 );
  
    m_pcView = new DiskView( cMainFrame, "main_view" );

  
    AddChild( m_pcView );
    MakeFocus( true );
    m_pcView->MakeFocus( true );

    Point cMinSize = m_pcView->GetPreferredSize(false) + Point( 10.0f, cMenuFrame.bottom + 11.0f );
    Point cMaxSize = m_pcView->GetPreferredSize(true) + Point( 10.0f, cMenuFrame.bottom + 11.0f );;
    SetSizeLimits( cMinSize, cMaxSize );

    Point cSize = cMinSize;
    if ( cSize.x < 450.0f ) {
	cSize.x = 450.0f;
    }
    if ( cSize.y < 300.0f ) {
	cSize.y = 300.0f;
    }
    ResizeTo( cSize );
    
    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome ) {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
	struct stat sStat;
	Message	cArchive;
	
	strcpy( zPath, pzHome );
	strcat( zPath, "/config/DiskManager.cfg" );

	if ( stat( zPath, &sStat ) >= 0 ) {
	    hFile = fopen( zPath, "rb" );
	    if ( NULL != hFile ) {
		uint8* pBuffer = new uint8[sStat.st_size];
		fread( pBuffer, sStat.st_size, 1, hFile );
		fclose( hFile );
		cArchive.Unflatten( pBuffer );
		
		Rect cFrame;
		if ( cArchive.FindRect( "window_frame", &cFrame ) == 0 ) {
		    if ( cFrame.Width() < cMinSize.x ) {
			cFrame.right = cFrame.left + cMinSize.x;
		    } else if ( cFrame.Width() > cMaxSize.x ) {
			cFrame.right = cFrame.left + cMaxSize.x;
		    }
		    if ( cFrame.Height() < cMinSize.y ) {
			cFrame.bottom = cFrame.top + cMinSize.y;
		    } else if ( cFrame.Height() > cMaxSize.y ) {
			cFrame.bottom = cFrame.top + cMaxSize.y;
		    }
		    SetFrame( cFrame );
		}
	    }
	}
    }
    Rect cBounds = GetBounds();
    Desktop cDesktop;
    MoveTo( cDesktop.GetResolution().x / 2 - cBounds.Width() / 2, cDesktop.GetResolution().y / 2 - cBounds.Height() / 2 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MainWindow::SetupMenus()
{
  Rect cMenuFrame = GetBounds();
  Rect cMainFrame = cMenuFrame;
  cMenuFrame.bottom = 16;
  
  m_pcMenu = new Menu( cMenuFrame, "Menu", ITEMS_IN_ROW,
		       CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP, WID_FULL_UPDATE_ON_H_RESIZE );

  Menu*	pcItem1	= new Menu( Rect( 0, 0, 100, 20 ), MSG_MAINWND_MENU_PROGRAM, ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

  pcItem1->AddItem( MSG_MAINWND_MENU_PROGRAM_EXIT, new Message( M_QUIT ) );

  m_pcMenu->AddItem( pcItem1 );

  cMenuFrame.bottom = m_pcMenu->GetPreferredSize( false ).y - 1.0f;
  cMainFrame.top = cMenuFrame.bottom + 1;
  
  m_pcMenu->SetFrame( cMenuFrame );
  
  m_pcMenu->SetTargetForItems( this );
  
  AddChild( m_pcMenu );
}

bool MainWindow::OkToQuit()
{
    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome ) {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
	Message	cArchive;
	
	strcpy( zPath, pzHome );
	strcat( zPath, "/config/DiskManager.cfg" );

	hFile = fopen( zPath, "wb" );

	if ( NULL != hFile ) {
	    cArchive.AddRect( "window_frame", GetFrame() );

	    int nSize = cArchive.GetFlattenedSize();
	    uint8* pBuffer = new uint8[nSize];
	    cArchive.Flatten( pBuffer, nSize );
	    fwrite( pBuffer, nSize, 1, hFile );
	    fclose( hFile );
	}
    }
    
    Application::GetInstance()->PostMessage( M_QUIT );
    return( true );
}

PEditApp::PEditApp() : Application( "application/x-vnd.KHS.atheos-partitionedit" )
{
	SetCatalog("DiskManager.catalog");
    m_pcMainWindow = new MainWindow( MSG_MAINWND_TITLE );
    m_pcMainWindow->Show();
}

PEditApp::~PEditApp()
{
}

int main()
{
    PEditApp* pcApp = new PEditApp;
    return( pcApp->Run() );
}
