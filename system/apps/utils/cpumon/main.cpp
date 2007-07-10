/*
 *  cpu - CPU usage monitor that can handle multiple CPU's for AtheOS 
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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
#include <getopt.h>

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include <util/application.h>
#include <util/resources.h>
#include <util/message.h>
#include <gui/view.h>
#include <gui/window.h>
#include <gui/tableview.h>
#include <gui/stringview.h>
#include <gui/imagebutton.h>

#include "main.h"
#include "barview.h"
#include "graphview.h"
#include "w8378x_driver.h"

#include "resources/CPU-Monitor.h"

using namespace os;

static int  g_nShowHelp = 0;
static int  g_nShowVersion = 0;
static bool g_bBarView = false;
static bool g_bWindowRectSet = false;

static Rect g_cWinRect( 20, 20, 200, 100 );

static struct option const long_opts[] =
{
    {"frame", required_argument, NULL, 'f' },
    {"bar_view", no_argument, NULL, 'b' },
    {"graph_view", no_argument, NULL, 'g' },
    {"help", no_argument, &g_nShowHelp, 1},
    {"version", no_argument, &g_nShowVersion, 1},
    {NULL, 0, NULL, 0}
};

struct Config_s
{
    Rect  m_cFrame;
    Point m_cPos;
};


void GetLoad( int nCPUCount, float* pavLoads )
{
    static bigtime_t anLastIdle[MAX_CPU_COUNT];
    static bigtime_t anLastSys[MAX_CPU_COUNT];

    for ( int i = 0 ; i < nCPUCount ; ++i ) {
	bigtime_t	nCurIdleTime;
	bigtime_t	nCurSysTime;

	bigtime_t	nIdleTime;
	bigtime_t	nSysTime;
	
	static float	avLoad[8];

	nCurIdleTime	= get_idle_time( i );
	nCurSysTime 	= get_real_time();

	
	nIdleTime	= nCurIdleTime - anLastIdle[i];
	nSysTime	= nCurSysTime  - anLastSys[i];
	
	anLastIdle[i] = nCurIdleTime;
	anLastSys[i]  = nCurSysTime;

	if ( nIdleTime > nSysTime ) {
	    nIdleTime = nSysTime;
	}
  
	if ( nSysTime == 0 ) {
	    nSysTime = 1;
	}

	avLoad[i] += ( (1.0 - double(nIdleTime) / double(nSysTime)) - avLoad[i]) * 0.5f;
	pavLoads[i] = avLoad[i];
	
    }
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MonWindow::MonWindow( const Rect& cFrame ) :
    Window( cFrame, "syllable_cpu_mon", MSG_MAINWND_TITLE, 0 )
{
	// Set Icon
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );


    if ( g_bWindowRectSet ) {
	SetFrame( g_cWinRect );
    } else {
	char* pzHome = getenv( "HOME" );

	if ( NULL != pzHome ) {
	    FILE* hFile;
	    char zPath[ PATH_MAX ];
			
	    strcpy( zPath, pzHome );
	    strcat( zPath, "/Settings/CPU-Monitor.cfg" );

	    hFile = fopen( zPath, "r" );

	    if ( NULL != hFile ) {
		Config_s sConfig;

		sConfig.m_cFrame = g_cWinRect;
		sConfig.m_cPos   = g_cWinRect.LeftTop();
		
		fread( &sConfig, sizeof( sConfig ), 1, hFile );
		
		fclose( hFile );
		if ( g_bBarView ) {
		    MoveTo( sConfig.m_cPos );
		} else {
		    SetFrame( sConfig.m_cFrame );
		}
	    }
	}
    }
    
    system_info sInfo;

    get_system_info( &sInfo );
    m_nCPUCount = sInfo.nCPUCount;
  
    Rect cWndBounds = GetBounds();

    cWndBounds.top = 2;

    uint32 nFlags = GetFlags();
    

    if ( g_bBarView ) {
	m_pcLoadView = new BarView( cWndBounds, m_nCPUCount );
	AddChild( m_pcLoadView );
	Point cSize = m_pcLoadView->GetPreferredSize( false );
	ResizeTo( cSize );
	SetSizeLimits( cSize, cSize );
	nFlags |= WND_NOT_RESIZABLE;
    } else {
	m_pcLoadView = new GraphView( cWndBounds, m_nCPUCount );
	AddChild( m_pcLoadView );
	if( m_nCPUCount > 1 )
		ResizeBy( os::Point( 0, 80 * ( m_nCPUCount - 1 ) ) );
    }
    SetFlags( nFlags );
    

    Show();
  
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
    char* pzHome = getenv( "HOME" );

    if ( g_bWindowRectSet == false && NULL != pzHome ) {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
			
	strcpy( zPath, pzHome );
	strcat( zPath, "/Settings/CPU-Monitor.cfg" );

	hFile = fopen( zPath, "r+" );

	if ( NULL != hFile ) {
	    Config_s sConfig;

	    fread( &sConfig, sizeof( sConfig ), 1, hFile );
	    fseek( hFile, 0, SEEK_SET );

	    if ( g_bBarView ) {
		sConfig.m_cPos = GetFrame().LeftTop();
	    } else {
		sConfig.m_cFrame = GetFrame();
	    }
	    fwrite( &sConfig, sizeof( sConfig ), 1, hFile );
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

MyApp::MyApp() : Application( "application/x-vnd.KHS.syllable-cpu-monitor" )
{
	SetCatalog("CPU-Monitor.catalog");
    m_pcWindow = new MonWindow( Rect( 20, 20, 200, 100 ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyApp::~MyApp()
{
}

static void usage( const char* pzName, bool bFull )
{
    printf( "Usage: %s [-hvfgb]\n", pzName );
    if ( bFull )
    {
	printf( "  -h --help          display this help and exit\n" );
	printf( "  -v --version       display version information and exit\n" );
	printf( "  -f --frame=l,t,r,b set window position/size (left,top,right,bottom)\n" );
	printf( "  -g --graph_view    display load as graphs\n" );
	printf( "  -b --bar_view      display load as bars\n" );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int main( int argc, char ** argv )
{
    int c;

    int nNameLen = strlen( argv[0] );
    if ( nNameLen >= 5 && strcasecmp( argv[0] + nNameLen - 5, "pulse" ) == 0 ) {
	g_bBarView = true;
    }
    while( (c = getopt_long (argc, argv, "hvfgb", long_opts, (int *) 0)) != EOF )
    {
	switch( c )
	{
	    case 0:
		break;
	    case 'h':
		g_nShowHelp = true;
		break;
	    case 'v':
		g_nShowVersion = true;
		break;
	    case 'f':
		sscanf( optarg, "%f,%f,%f,%f",
			&g_cWinRect.left,&g_cWinRect.top,&g_cWinRect.right,&g_cWinRect.bottom );
		g_bWindowRectSet = true;
		break;
	    case 'g':
		g_bBarView = false;
		break;
	    case 'b':
		g_bBarView = true;
		break;
	    default:
		usage( argv[0], false );
		exit( 1 );
		break;
	}
    }
    if ( g_nShowVersion ) {
	printf( "Syllable CPU Monitor 1.0\n" );
	exit( 0 );
    }
    if ( g_nShowHelp ) {
	usage( argv[0], true );
	exit( 0 );
    }
    
    MyApp* pcApp = new MyApp;
    pcApp->Run();
}
