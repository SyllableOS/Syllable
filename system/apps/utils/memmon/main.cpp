/*
 *  memmon - Memory usage monitor for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include <gui/window.h>
#include <gui/tableview.h>
#include <gui/stringview.h>
#include <gui/menu.h>

#include <util/application.h>
#include <util/message.h>
#include <util/exceptions.h>
#include <gui/exceptions.h>

using namespace os;

enum
{
  ID_QUIT,
  ID_SPEED_LOW,
  ID_SPEED_MEDIUM,
  ID_SPEED_HIGH,
  ID_SPEED_VERY_HIGH,
  ID_SPEED_FULL
};

class MultiMeter : public View
{
public:
    MultiMeter( const Rect& cFrame, const char* pzTitle,
		uint32 nResizeMask, uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~MultiMeter();

      // From View:
    virtual Point GetPreferredSize( bool bLargest ) const { return( (bLargest) ? Point( 100000, 100000 ) : Point( 5, 5 ) );}
    virtual void	Paint( const Rect& cUpdateRect );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );

    void	AddValue( float vTotMem, float vCacheSize, float vDirtyCache );
private:
    Menu* m_pcPopupMenu;
    float m_avTotUsedMem[ 1024 ];
    float m_avCacheSize[ 1024 ];
    float m_avDirtyCacheSize[ 1024 ];

};



class MonWindow : public Window
{
public:
    MonWindow( const Rect& cFrame );
    ~MonWindow();

      // From Window:
    bool	OkToQuit();

    virtual void TimerTick( int nID );
  
private:
    TableView*	m_pcTableView;
    StringView*	m_pcMemUsageStr;
    MultiMeter*	m_pcMemUsage;
    int		m_nUpdateCount;
};

class	MyApp : public Application
{
public:
  MyApp( const char* pzName );
  ~MyApp();

    // From Handler:
  virtual void	HandleMessage( Message* pcMsg );
private:
  MonWindow* m_pcWindow;
  bigtime_t  m_nDelay;
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyApp::MyApp( const char* pzName ) : Application( pzName )
{
  m_nDelay 	  = 1000000;
  m_pcWindow 	  = new MonWindow( Rect( 20, 20, 200, 100 ) );

  m_pcWindow->Show();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyApp::~MyApp()
{
  m_pcWindow->Quit();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MyApp::HandleMessage( Message* pcMsg )
{
    switch( pcMsg->GetCode() )
    {
	case ID_QUIT:
	{
      
	    Message cMsg( M_QUIT );
	    m_pcWindow->PostMessage( &cMsg );
	    return;
	}
	case ID_SPEED_LOW:		m_nDelay = 10000000;	break;
	case ID_SPEED_MEDIUM:		m_nDelay = 1000000;	break;
	case ID_SPEED_HIGH:		m_nDelay = 100000;	break;
	case ID_SPEED_VERY_HIGH:	m_nDelay = 10000;	break;
	case ID_SPEED_FULL:		m_nDelay = 10;		break;
	default:
	    Application::HandleMessage( pcMsg );
	    return;
    }
    m_pcWindow->Lock();
    m_pcWindow->AddTimer( m_pcWindow, 0, m_nDelay, false );
    m_pcWindow->Unlock();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MonWindow::MonWindow( const Rect& cFrame ) :
    Window( cFrame, "sysmon_wnd", "Memory usage", 0 )
{
    Lock();

    Rect	  cWndBounds = GetBounds();
    m_nUpdateCount = 0;

    m_pcTableView   = new TableView( cWndBounds, "_cpumon_table", "", 1, 2, CF_FOLLOW_ALL );
    m_pcMemUsage    = new MultiMeter( Rect( 0, 0, 1, 1 ), "Mem usage", WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    m_pcMemUsageStr = new StringView( Rect( 0, 0, 1, 1 ), "mem_usage_str", "100", ALIGN_CENTER, WID_WILL_DRAW );
  
    m_pcTableView->SetChild( m_pcMemUsageStr, 0, 0 );
    m_pcTableView->SetChild( m_pcMemUsage, 0, 1 );
    m_pcTableView->SetCellAlignment( 0, 0, ALIGN_CENTER, ALIGN_CENTER );

    AddChild( m_pcTableView );
    m_pcMemUsageStr->SetFgColor( 0, 0, 0 );
    char*	pzHome = getenv( "HOME" );

    if ( NULL != pzHome )
    {
	FILE*	hFile;
	char	zPath[ 256 ];
			
	strcpy( zPath, pzHome );
	strcat( zPath, "/Settings/sysmon.cfg" );

	hFile = fopen( zPath, "rb" );

	if ( NULL != hFile )
	{
	    Rect	cNewFrame;
				
	    fread( &cNewFrame, sizeof( cNewFrame ), 1, hFile );
	    fclose( hFile );

	    Lock();
	    SetFrame( cNewFrame );
	    Unlock();
	}
    }
  
    AddTimer( this, 0, 1000000, false );
    Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MonWindow::~MonWindow()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MonWindow::OkToQuit( void )
{
  char*	pzHome = getenv( "HOME" );

  if ( NULL != pzHome )
  {
    FILE*	hFile;
    char	zPath[ 256 ];
			
    strcpy( zPath, pzHome );
    strcat( zPath, "/config/sysmon.cfg" );

    hFile = fopen( zPath, "wb" );

    if ( NULL != hFile )
    {
      Rect cFrame = GetFrame();

      fwrite( &cFrame, sizeof( cFrame ), 1, hFile );
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

void MonWindow::TimerTick( int nID )
{
    system_info sInfo;

    get_system_info( &sInfo );
  
    char* pzPostfix;
    char* pzCachePF;
    char* pzDirtyPF;
    float	vFreeMem = float(sInfo.nFreePages * PAGE_SIZE);
    float	vCache   = float(sInfo.nBlockCacheSize);
    float	vDirty   = float(sInfo.nDirtyCacheSize);
    
    if ( vFreeMem > 1000000.0f ) {
	vFreeMem /= 1000000.0f;
	pzPostfix = "Mb";
    } else if ( vFreeMem > 1000.0f ) {
	vFreeMem /= 1000.0f;
	pzPostfix = "Kb";
    } else {
	pzPostfix = "b";
    }
    if ( vCache > 1000000.0f ) {
	vCache /= 1000000.0f;
	pzCachePF = "Mb";
    } else if ( vCache > 1000.0f ) {
	vCache /= 1000.0f;
	pzCachePF = "Kb";
    } else {
	pzCachePF = "b";
    }
    if ( vDirty > 1000000.0f ) {
	vDirty /= 1000000.0f;
	pzDirtyPF = "Mb";
    } else if ( vDirty > 1000.0f ) {
	vDirty /= 1000.0f;
	pzDirtyPF = "Kb";
    } else {
	pzDirtyPF = "b";
    }
    
    char zBuffer[256];
    sprintf( zBuffer, "%.2f%s / %.2f%s / %.2f%s", vFreeMem, pzPostfix, vCache, pzCachePF, vDirty, pzDirtyPF );

    m_pcMemUsageStr->SetString( zBuffer );
    if ( (m_nUpdateCount++ % 2) == 0 ) {
	float vFreePst  = 1.0f - float(sInfo.nFreePages) / float(sInfo.nMaxPages);
	float vCachePst = float(sInfo.nBlockCacheSize) / float(sInfo.nMaxPages * PAGE_SIZE);
	float vDirtyPst = float(sInfo.nDirtyCacheSize) / float(sInfo.nMaxPages * PAGE_SIZE);

	m_pcMemUsage->AddValue( vFreePst - vCachePst, vCachePst - vDirtyPst, vDirtyPst );
	m_pcTableView->Layout();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MultiMeter::MultiMeter(  const Rect& cFrame, const char* pzTitle, uint32 nResizeMask, uint32 nFlags  )
    : View( cFrame, pzTitle, nResizeMask, nFlags  )
{
  m_pcPopupMenu     = new Menu( Rect( 0, 0, 10, 10 ), "popup", ITEMS_IN_COLUMN );
  Menu* pcSpeedMenu = new Menu( Rect( 0, 0, 10, 10 ), "Speed", ITEMS_IN_COLUMN );
  
  m_pcPopupMenu->AddItem( new MenuItem( "Quit", new Message( M_QUIT ) ) );
  m_pcPopupMenu->AddItem( pcSpeedMenu );
  
  pcSpeedMenu->AddItem( new MenuItem( "Low",       new Message( ID_SPEED_LOW ) ) );
  pcSpeedMenu->AddItem( new MenuItem( "Medium",    new Message( ID_SPEED_MEDIUM ) ) );
  pcSpeedMenu->AddItem( new MenuItem( "High",      new Message( ID_SPEED_HIGH ) ) );
  pcSpeedMenu->AddItem( new MenuItem( "Very high", new Message( ID_SPEED_VERY_HIGH ) ) );
  pcSpeedMenu->AddItem( new MenuItem( "Full speed", new Message( ID_SPEED_FULL ) ) );

  m_pcPopupMenu->SetTargetForItems( Application::GetInstance() );
  pcSpeedMenu->SetTargetForItems( Application::GetInstance() );
  
  memset( m_avTotUsedMem, 0, sizeof( m_avTotUsedMem ) );
  memset( m_avCacheSize, 0, sizeof( m_avCacheSize ) );
  memset( m_avDirtyCacheSize, 0, sizeof( m_avDirtyCacheSize ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MultiMeter::~MultiMeter()
{
  delete m_pcPopupMenu;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MultiMeter::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();
    float x;
    int	  i;

    DrawFrame( cBounds, FRAME_RECESSED | FRAME_TRANSPARENT );
  
    cBounds.left += 2;
    cBounds.top += 2;
    cBounds.right -= 2;
    cBounds.bottom -= 2;
    
    SetFgColor( 0, 0, 0, 0 );

    Rect cDrawRect( cBounds & cUpdateRect );
	
    for ( i = int( cBounds.right - cDrawRect.right ), x = cDrawRect.right ; x >= cDrawRect.left ; ++i )
    {
	float y1 = m_avTotUsedMem[ i ]     * (cBounds.Height()+1.0f);
	float y2 = m_avCacheSize[ i ]      * (cBounds.Height()+1.0f);
	float y3 = m_avDirtyCacheSize[ i ] * (cBounds.Height()+1.0f);

	if ( y3 == 0 && m_avDirtyCacheSize[ i ] > 0.0f ) {
	    y3++;
	    y2--;
	}
	
	MovePenTo( x, cBounds.bottom );
	SetFgColor( 0, 0, 0, 0 );
	DrawLine( Point( x, cBounds.bottom - y1 ) );
	SetFgColor( 80, 140, 180, 0 );
	DrawLine( Point( x, cBounds.bottom -  y1 - y2 ) );
	SetFgColor( 255, 0, 0, 0 );
	DrawLine( Point( x, cBounds.bottom - y1 - y2 - y3 ) );
	SetFgColor( 255, 255, 255, 0 );
	DrawLine( Point( x, cBounds.top ) );
	x -= 1.0f;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MultiMeter::AddValue( float vTotMem, float vCacheSize, float vDirtyCache )
{
  for ( int i = 1023 ; i > 0 ; --i )	{
    m_avTotUsedMem[ i ]     = m_avTotUsedMem[ i - 1 ];
    m_avCacheSize[ i ]      = m_avCacheSize[ i - 1 ];
    m_avDirtyCacheSize[ i ] = m_avDirtyCacheSize[ i - 1 ];
  }

  m_avTotUsedMem[ 0 ]     = std::min( 1.0f, vTotMem );
  m_avCacheSize[ 0 ]      = std::min( 1.0f, vCacheSize );
  m_avDirtyCacheSize[ 0 ] = std::min( 1.0f, vDirtyCache );

  Rect cBounds = GetBounds();
  
  cBounds.left += 2;
  cBounds.top += 2;
  cBounds.right -= 3;
  cBounds.bottom -= 2;
  ScrollRect( cBounds + Point( 1, 0 ), cBounds );
  float y1 = m_avTotUsedMem[ 0 ]     * (cBounds.Height()+1.0f);
  float y2 = m_avCacheSize[ 0 ]      * (cBounds.Height()+1.0f);
  float y3 = m_avDirtyCacheSize[ 0 ] * (cBounds.Height()+1.0f);

  if ( y3 == 0 && m_avDirtyCacheSize[ 0 ] > 0.0f ) {
      y3++;
      y2--;
  }
  
  MovePenTo( cBounds.right, cBounds.bottom );
  SetFgColor( 0, 0, 0, 0 );
  DrawLine( Point( cBounds.right, cBounds.bottom - y1 ) );
  SetFgColor( 80, 140, 180, 0 );
  DrawLine( Point( cBounds.right, cBounds.bottom -  y1 - y2 ) );
  SetFgColor( 255, 0, 0, 0 );
  DrawLine( Point( cBounds.right, cBounds.bottom - y1 - y2 - y3 ) );
  
  SetFgColor( 255, 255, 255, 0 );
  DrawLine( Point( cBounds.right, cBounds.top ) );
  Sync();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MultiMeter::MouseDown( const Point& cPosition, uint32 nButtons )
{
    if ( nButtons != 2 ) {
	return;
    }
    MakeFocus( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MultiMeter::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    if ( nButtons != 2 ) {
	return;
    }
    m_pcPopupMenu->Open( ConvertToScreen( cPosition ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int main()
{
  Application* pcMyApp;
  
  try {
	  pcMyApp = new MyApp( "application/x-vnd.KHS-atheos_memory_monitor" );
 
	  pcMyApp->Run();
  } catch( errno_exception &e ) {
  	printf( "Unhandled exception: %s\n", e.what() );
  } catch( GeneralFailure &e ) {
  	printf( "Unhandled exception: %s\n", e.what() );
  } catch( ... ) {
  	printf( "Unhandled exception!\n" );
  }
  return( 0 );
}
