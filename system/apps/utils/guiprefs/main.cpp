#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>

#include <gui/window.h>
#include <gui/menu.h>

#include <util/application.h>
#include <util/message.h>

#include <atheos/time.h>
#include <atheos/kernel.h>

using namespace os;

#include "fontpanel.h"
#include "screenpanel.h"
#include "windowpanel.h"
#include "keyboardpanel.h"

#include <gui/tabview.h>


class MyWindow : public os::Window
{
public:
    MyWindow( const Rect& cFrame );
    ~MyWindow();

    virtual void	HandleMessage( Message* pcMessage );
      // From Window:
    virtual bool	OkToQuit();
private:
    void SetupMenus();
    Menu* m_pcMenu;
    TabView* m_pcView;
};


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MyWindow::SetupMenus()
{
  Rect cMenuFrame = GetBounds();
  Rect cMainFrame = cMenuFrame;
  cMenuFrame.bottom = 16;
  
  m_pcMenu = new Menu( cMenuFrame, "Menu", ITEMS_IN_ROW,
		       CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP, WID_FULL_UPDATE_ON_H_RESIZE );

  Menu*	pcItem1	= new Menu( Rect( 0, 0, 100, 20 ), "File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

  pcItem1->AddItem( "Quit", new Message( M_QUIT ) );

  m_pcMenu->AddItem( pcItem1 );

  cMenuFrame.bottom = m_pcMenu->GetPreferredSize( false ).y - 1;
  cMainFrame.top = cMenuFrame.bottom + 1;
  
  m_pcMenu->SetFrame( cMenuFrame );
  
  m_pcMenu->SetTargetForItems( this );
  
  AddChild( m_pcMenu );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyWindow::MyWindow( const Rect& cFrame ) :
    Window( cFrame, "prefs_wnd", "Preferences", 0 )
{
    SetupMenus();

    Rect cMainFrame = GetBounds();
    Rect cMenuFrame = m_pcMenu->GetFrame();

    cMainFrame.top = cMenuFrame.bottom + 1.0f;

    cMainFrame.Resize( 5, 5, -5, -5 );
  
    m_pcView = new TabView( cMainFrame, "main_tab", CF_FOLLOW_ALL, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    View* pcFontPanel = new FontPanel( cMainFrame );
    View* pcScreenPanel = new ScreenPanel( cMainFrame );
    View* pcWindowPanel = new WindowPanel( cMainFrame );
    View* pcKeyboardPanel = new KeyboardPanel( cMainFrame );

    m_pcView->AppendTab( "Keyboard", pcKeyboardPanel );
    m_pcView->AppendTab( "Default fonts", pcFontPanel );
    m_pcView->AppendTab( "Screen Mode", pcScreenPanel );
    m_pcView->AppendTab( "Apearance", pcWindowPanel );
  
    AddChild( m_pcView );
    MakeFocus( true );
    pcFontPanel->MakeFocus( true );
    Point cMinSize = m_pcView->GetPreferredSize(false) + Point( 10.0f, cMenuFrame.bottom + 11.0f );
    Point cMaxSize = m_pcView->GetPreferredSize(true);
    ResizeTo( cMinSize );
    SetSizeLimits( cMinSize, cMaxSize );

    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome )
    {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
	struct stat sStat;
	Message	cArchive;
	
	strcpy( zPath, pzHome );
	strcat( zPath, "/config/guiprefs.cfg" );

	if ( stat( zPath, &sStat ) >= 0 ) {
	    hFile = fopen( zPath, "rb" );
	    if ( NULL != hFile ) {
		uint8* pBuffer = new uint8[sStat.st_size];
		fread( pBuffer, sStat.st_size, 1, hFile );
		fclose( hFile );
		cArchive.Unflatten( pBuffer );
		
		Rect cFrame;
		if ( cArchive.FindRect( "window_frame", &cFrame ) == 0 ) {
		    SetFrame( cFrame );
		}
	    }
	}
    }
  
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyWindow::~MyWindow()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MyWindow::HandleMessage( Message* pcMessage )
{
  View*	pcSource;

  if ( pcMessage->FindPointer( "source", (void**) &pcSource ) != 0 ) {
    Window::HandleMessage( pcMessage );
    return;
  }
  
  switch( pcMessage->GetCode() )
  {
    case MID_ABOUT:      printf( "About...\n" );	break;
    case MID_HELP:       printf( "Help...\n" );		break;
    case MID_LOAD:       printf( "Load...\n" );		break;
    case MID_INSERT:     printf( "Insert...\n" );	break;
    case MID_SAVE:       printf( "Save\n" );		break;
                 		
    case MID_SAVE_AS:    printf( "Save as...\n" );	break;
    case MID_QUIT:       printf( "Quit...\n" );		break;
                 		
    case MID_DUP_SEL:    printf( "Duplicate\n" );	break;
    case MID_DELETE_SEL: printf( "Delete\n" );		break;
                 		
    case MID_RESTORE:    printf( "Restore...\n" );	break;
    case MID_SNAPSHOT:   printf( "Snapshot\n" );	break;
    case MID_RESET:      printf( "Reset...\n" );	break;
    default:
      Window::HandleMessage( pcMessage );
  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MyWindow::OkToQuit()
{
    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome )
    {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
	Message	cArchive;
	
	strcpy( zPath, pzHome );
	strcat( zPath, "/config/guiprefs.cfg" );

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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int main()
{
  Application* pcApp = new Application( "application/x-vnd.KHS-font_prefs" );
  MyWindow* pcWindow = new MyWindow( Rect( 0, 0, 350 - 1, 300 - 1 ) + Point( 100, 50 ) );
  pcWindow->Show();
  pcApp->Run();
}
