#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>

#include <pwd.h>

#include <atheos/time.h>

#include <gui/window.h>
#include <gui/button.h>
#include <gui/bitmap.h>
#include <gui/sprite.h>
#include <gui/icon.h>
#include <gui/requesters.h>
#include <gui/directoryview.h>

#include <util/application.h>
#include <util/message.h>

#include <storage/fsnode.h>

#include "iconview.h"

Bitmap* load_jpeg( const char* pzPath );

using namespace os;

class BitmapView;

static Bitmap* g_pcBackDrop = NULL;

class DirWindow : public Window
{
public:
    DirWindow( const Rect& cFrame, const char* pzName, const char* pzPath );
    virtual void	HandleMessage( Message* pcMessage );
private:
    enum { ID_PATH_CHANGED = 1 };
    DirectoryView* m_pcDirView;
};

class DirIconWindow : public Window
{
public:
    DirIconWindow( const Rect& cFrame, const char* pzName, const char* pzPath, Bitmap* pcBackdrop );
    virtual void	HandleMessage( Message* pcMessage );
private:
    enum { ID_PATH_CHANGED = 1 };
    IconView* m_pcDirView;
};

class BitmapView : public View
{
public:
    BitmapView( const Rect& cFrame, Bitmap* pcBitmap );
    ~BitmapView();

    Icon*		FindIcon( const Point& cPos );
    void 		Erase( const Rect& cFrame );
  
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	Paint( const Rect& cUpdateRect );
private:
    Point		     m_cLastPos;
    Point		     m_cDragStartPos;
    Rect		     m_cSelRect;
    bigtime_t	     m_nHitTime;
    Bitmap*	     m_pcBitmap;
    std::vector<Icon*> m_cIcons;
    bool		     m_bCanDrag;
    bool		     m_bSelRectActive;
};


void Icon::Select( BitmapView* pcView, bool bSelected )
{
    if ( m_bSelected == bSelected ) {
	return;
    }
    m_bSelected = bSelected;

    pcView->Erase( GetFrame( pcView->GetFont() ) );
  
    Paint( pcView, Point(0,0), true, true );
}

BitmapView::BitmapView( const Rect& cFrame, Bitmap* pcBitmap ) :
    View( cFrame, "_bitmap_view", CF_FOLLOW_ALL )
{
    m_pcBitmap = pcBitmap;

    struct stat sStat;
  
    m_cIcons.push_back( new Icon( "Root", "/system/icons/root.icon", sStat ) );
//    m_cIcons.push_back( new Icon( "Root2", "/system/icons/root.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Emacs V19.34", "/system/icons/emacs.icon", sStat ) );
    m_cIcons.push_back( new Icon( "ABrowse", "/system/icons/ABrowse.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Icon Edit", "/system/icons/iconedit.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Terminal", "/system/icons/terminal.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Prefs", "/system/icons/prefs.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Pulse", "/system/icons/cpumon.icon", sStat ) );
    m_cIcons.push_back( new Icon( "CPU usage", "/system/icons/cpumon.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Memory usage", "/system/icons/memmon.icon", sStat ) );
  
    m_bCanDrag = false;
    m_bSelRectActive = false;
  
    Point cPos( 20, 20 );
  
    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
	m_cIcons[i]->m_cPosition.x = cPos.x + 16;
	m_cIcons[i]->m_cPosition.y = cPos.y;
    
	cPos.y += 50;
	if ( cPos.y > 500 ) {
	    cPos.y = 20;
	    cPos.x += 50;
	}
    }
    m_nHitTime = 0;
}

BitmapView::~BitmapView()
{
    delete m_pcBitmap;
}

void BitmapView::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();

  
    SetDrawingMode( DM_COPY );
  
    Font* pcFont = GetFont();
  
    Erase( cUpdateRect );

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cUpdateRect ) ) {
	    m_cIcons[i]->Paint( this, Point(0,0), true, true );
	}
    }
}

void BitmapView::Erase( const Rect& cFrame )
{
    if ( m_pcBitmap != NULL ) {
	Rect cBmBounds = m_pcBitmap->GetBounds();
	int nWidth  = int(cBmBounds.Width()) + 1;
	int nHeight = int(cBmBounds.Height()) + 1;
	SetDrawingMode( DM_COPY );
	for ( int nDstY = int(cFrame.top) ; nDstY <= cFrame.bottom ; )
	{
	    int y = nDstY % nHeight;
	    int nCurHeight = min( int(cFrame.bottom) - nDstY + 1, nHeight - y );
    
	    for ( int nDstX = int(cFrame.left) ; nDstX <= cFrame.right ; )
	    {
		int x = nDstX % nWidth;
		int nCurWidth = min( int(cFrame.right) - nDstX + 1, nWidth - x );

		Rect cRect( 0, 0, nCurWidth - 1, nCurHeight - 1 );
		DrawBitmap( m_pcBitmap, cRect + Point( x, y ), cRect + Point( nDstX, nDstY ) );
		nDstX += nCurWidth;
	    }
	    nDstY += nCurHeight;
	}
    } else {
	FillRect( cFrame, Color32_s( 0x00, 0x60, 0x6b, 0 ) );
    }
}

Icon* BitmapView::FindIcon( const Point& cPos )
{
    Font* pcFont = GetFont();

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
	if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cPos ) ) {
	    return( m_cIcons[i] );
	}
    }
    return( NULL );
}

void BitmapView::MouseDown( const Point& cPosition, uint32 nButtons )
{
    MakeFocus( true );

    Icon* pcIcon = FindIcon( cPosition );

    if ( pcIcon != NULL )
    {
	if (  pcIcon->m_bSelected ) {
	    if ( m_nHitTime + 500000 >= get_system_time() ) {
		if ( pcIcon->GetName() == "Root" ) {
		    Window*   pcWindow = new DirWindow( Rect( 200, 150, 600, 400 ), "_dir_view", "/" );
		    pcWindow->MakeFocus();
		} else if ( pcIcon->GetName() == "Root2" ) {
		    Window*   pcWindow = new DirIconWindow( Rect( 20, 20, 359, 220 ), "_dir_view", "/", g_pcBackDrop );
		    pcWindow->MakeFocus();
		} else  if ( pcIcon->GetName() == "Terminal" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "aterm", "aterm", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "ABrowse" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execl( "/Applications/ABrowse/ABrowse", "ABrowse", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "Emacs V19.34" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "emacs", "emacs", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "Icon Edit" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "iconedit", "iconedit", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "Prefs" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "guiprefs", "guiprefs", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "Pulse" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "pulse", "pulse", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "CPU usage" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "cpu", "cpu", NULL );
			exit( 1 );
		    }
		} else  if ( pcIcon->GetName() == "Memory usage" ) {
		    pid_t nPid = fork();
		    if ( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execlp( "memmon", "memmon", NULL );
			exit( 1 );
		    }
		}
	    } else {
		m_bCanDrag = true;
	    }
	    m_nHitTime = get_system_time();
	    return;
	}
    }
    for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	m_cIcons[i]->Select( this, false );
    }
    if ( pcIcon != NULL ) {
	m_bCanDrag = true;
	pcIcon->Select( this, true );
    } else {
	m_bSelRectActive = true;
	m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
	SetDrawingMode( DM_INVERT );
	DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
    }
    Flush();
    m_cLastPos = cPosition;
    m_nHitTime = get_system_time();
}

void BitmapView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    m_bCanDrag = false;
  
    Font* pcFont = GetFont();
    if ( m_bSelRectActive ) {
	SetDrawingMode( DM_INVERT );
	DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
	m_bSelRectActive = false;

	if ( m_cSelRect.left > m_cSelRect.right ) {
	    float nTmp = m_cSelRect.left;
	    m_cSelRect.left = m_cSelRect.right;
	    m_cSelRect.right = nTmp;
	}
	if ( m_cSelRect.top > m_cSelRect.bottom ) {
	    float nTmp = m_cSelRect.top;
	    m_cSelRect.top = m_cSelRect.bottom;
	    m_cSelRect.bottom = nTmp;
	}
    
	for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	    m_cIcons[i]->Select( this, m_cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
	}
	Flush();
    } else if ( pcData != NULL && pcData->ReturnAddress() == Messenger( this ) ) {
	Point cHotSpot;
	pcData->FindPoint( "_hot_spot", &cHotSpot );


	for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	    Rect cFrame = m_cIcons[i]->GetFrame( pcFont );
	    Erase( cFrame );
	}
	Flush();
	for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	    if ( m_cIcons[i]->m_bSelected ) {
		m_cIcons[i]->m_cPosition += cPosition - m_cDragStartPos;
	    }
	    m_cIcons[i]->Paint( this, Point(0,0), true, true );
	}
	Flush();
    }
}

void BitmapView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    m_cLastPos = cNewPos;

    if ( (nButtons & 0x01) == 0 ) {
	return;
    }
  
    if ( m_bSelRectActive ) {
	SetDrawingMode( DM_INVERT );
	DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
	m_cSelRect.right = cNewPos.x;
	m_cSelRect.bottom = cNewPos.y;

	Rect cSelRect = m_cSelRect;
	if ( cSelRect.left > cSelRect.right ) {
	    float nTmp = cSelRect.left;
	    cSelRect.left = cSelRect.right;
	    cSelRect.right = nTmp;
	}
	if ( cSelRect.top > cSelRect.bottom ) {
	    float nTmp = cSelRect.top;
	    cSelRect.top = cSelRect.bottom;
	    cSelRect.bottom = nTmp;
	}
	Font* pcFont = GetFont();
	SetDrawingMode( DM_COPY );
	for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	    m_cIcons[i]->Select( this, cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
	}

	SetDrawingMode( DM_INVERT );
	DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
    
	Flush();
	return;
    }
  
    if ( m_bCanDrag )
    {
	Flush();
    
	Icon* pcSelIcon = NULL;

	Rect cSelFrame( 1000000, 1000000, -1000000, -1000000 );
    
	Font* pcFont = GetFont();
	Message cData(1234);
	for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
	    if ( m_cIcons[i]->m_bSelected ) {
		cData.AddString( "file/path", m_cIcons[i]->GetName().c_str() );
		cSelFrame |= m_cIcons[i]->GetFrame( pcFont );
		pcSelIcon = m_cIcons[i];
	    }
	}
	if ( pcSelIcon != NULL ) {
	    m_cDragStartPos = cNewPos; // + cSelFrame.LeftTop() - cNewPos;

	    if ( (cSelFrame.Width()+1.0f) * (cSelFrame.Height()+1.0f) < 12000 )
	    {
		Bitmap cDragBitmap( cSelFrame.Width()+1.0f, cSelFrame.Height()+1.0f, CS_RGB32, Bitmap::ACCEPT_VIEWS | Bitmap::SHARE_FRAMEBUFFER );

		View* pcView = new View( cSelFrame.Bounds(), "" );
		cDragBitmap.AddChild( pcView );

		pcView->SetFgColor( 255, 255, 255, 0 );
		pcView->FillRect( cSelFrame.Bounds() );


		for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
		    if ( m_cIcons[i]->m_bSelected ) {
			m_cIcons[i]->Paint( pcView, -cSelFrame.LeftTop(), true, false );
		    }
		}
		cDragBitmap.Sync();

		uint32* pRaster = (uint32*)cDragBitmap.LockRaster();

		for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y ) {
		    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x ) {
			if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0x00ffffff &&
			     (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0xff000000 ) {
			    pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0xb0000000;
			}
		    }
		}
		BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), &cDragBitmap );

	    } else {
		BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), cSelFrame.Bounds() );
	    }
	}
	m_bCanDrag = false;
    }
    Flush();
}



DirWindow::DirWindow( const Rect& cFrame, const char* pzName, const char* pzPath ) :
    Window( cFrame, pzName, "" )
{
    m_pcDirView = new DirectoryView( GetBounds(), pzPath, ListView::F_MULTI_SELECT, CF_FOLLOW_ALL );
    AddChild( m_pcDirView );
    m_pcDirView->SetDirChangeMsg( new Message( ID_PATH_CHANGED ) );
    m_pcDirView->MakeFocus();
    SetTitle( m_pcDirView->GetPath().c_str() );
    Show();
}

void DirWindow::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_PATH_CHANGED:
	    SetTitle( m_pcDirView->GetPath().c_str() );
	    break;
	default:
	    Window::HandleMessage( pcMessage );
	    break;
    }
}

DirIconWindow::DirIconWindow( const Rect& cFrame, const char* pzName, const char* pzPath, Bitmap* pcBitmap ) :
    Window( cFrame, pzName, "" )
{
    m_pcDirView = new IconView( GetBounds(), pzPath, pcBitmap );
    AddChild( m_pcDirView );
    m_pcDirView->SetDirChangeMsg( new Message( ID_PATH_CHANGED ) );
    m_pcDirView->MakeFocus();
    SetTitle( m_pcDirView->GetPath().c_str() );
    Show();
}

void DirIconWindow::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_PATH_CHANGED:
	    SetTitle( m_pcDirView->GetPath().c_str() );
	    break;
	default:
	    Window::HandleMessage( pcMessage );
	    break;
    }
}

bool get_login( std::string* pcName, std::string* pcPassword );

static void authorize( const char* pzLoginName )
{
    for (;;)
    {
	std::string cName;
	std::string cPassword;

	if ( pzLoginName != NULL || get_login( &cName, &cPassword ) )
	{
	    struct passwd* psPass;

	    if ( pzLoginName != NULL ) {
		psPass = getpwnam( pzLoginName );
	    } else {
		psPass = getpwnam( cName.c_str() );
	    }

	    if ( psPass != NULL ) {
		const char* pzPassWd = crypt(  cPassword.c_str(), "$1$" );
	
		if ( pzLoginName == NULL && pzPassWd == NULL ) {
		    perror( "crypt()" );
		    pzLoginName = NULL;
		    continue;
		}

		if ( pzLoginName != NULL || strcmp( pzPassWd, psPass->pw_passwd ) == 0 ) {
		    setgid( psPass->pw_gid );
		    setuid( psPass->pw_uid );
		    setenv( "HOME", psPass->pw_dir, true );
		    chdir( psPass->pw_dir );
		    break;
		} else {
		    Alert* pcAlert = new Alert( "Login failed:",  "Incorrect password", 0, "Sorry", NULL );
		    pcAlert->Go();
		}
	    } else {
		Alert* pcAlert = new Alert( "Login failed:",  "No such user", 0, "Sorry", NULL );
		pcAlert->Go();
	    }
	}
	pzLoginName = NULL;
    }
    
}

int main( int argc, char** argv )
{
    Application* pcApp = new Application( "application/x-vnd.KHS-desktop_manager" );

    if ( getuid() == 0 ) {
	const char* pzLoginName = NULL;

	if ( argc == 2 ) {
	    pzLoginName = argv[1];
	}
	pcApp->Unlock();
	authorize( pzLoginName );
	pcApp->Lock();
    }
    g_pcBackDrop = load_jpeg( "/system/backdrop.jpg" );

    Window* pcBitmapWindow = new Window( Rect( 0, 0, 1599, 1199 ), "_bitmap_window", "",
					 WND_NO_BORDER | WND_BACKMOST, ALL_DESKTOPS );
    pcBitmapWindow->AddChild( new BitmapView( Rect( 0, 0, 1599, 1199 ), g_pcBackDrop ) );
    pcBitmapWindow->Show();

    pcApp->Run();
  
    return( 0 );
}
