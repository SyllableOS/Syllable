/*
 *  aterm - x-term like terminal emulator for AtheOS
 *  Copyright (C) 1999  Kurt Skauen
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <pwd.h>

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <atheos/threads.h>
#include <atheos/kernel.h>

#include <gui/window.h>
#include <gui/desktop.h>
#include <gui/scrollbar.h>
#include <util/application.h>
#include <util/message.h>
#include "tview.h"

enum
{
    ID_SCROLL,
    ID_REFRESH
};

int  g_nShowHelp = 0;
int  g_nShowVersion = 0;
int  g_nDebugLevel = 0;
bool g_bWindowBorder = true;
int  g_bIBeamHalo = false;

os::Rect g_cWinRect( 100, 200, 800, 500 );

extern char** environ;

int g_nMasterPTY;

thread_id g_hUpdateThread;

static volatile bool g_bRun = true;

class	MyWindow : public Window
{
public:
    MyWindow( Rect cFrame,
	      const char* pzName,
	      const char* pzTitle,
	      int nFlags );

    bool	OkToQuit();
    void	HandleMessage( Message* pcMessage );

private:
};


class	MyApp : public Application
{
public:
    MyApp();
    ~MyApp();


// From Looper
    bool	OkToQuit();


/*	void	DispatchMessage( Message* pcMessage, Handler* pcHandler );	*/

// From Application

private:
};

MyWindow*  g_pcWindow 	 = NULL;
TermView*  g_pcTermView = NULL;
ScrollBar* g_pcScrollBar   = NULL;


MyApp::MyApp() : Application( "application/x-vnd.KHS-aterm" )
{
}

MyApp::~MyApp()
{
}

bool	MyApp::OkToQuit()
{
/*
	dbprintf( "Quit main thread\n" );
	g_bQuit = true;
	WaitForThread( g_hUpdateThread );
	dbprintf( "Quit window\n" );
	g_pcWindow->Quit();
	dbprintf( "Quit main thread\n" );
*/
  return( true );
}



MyWindow::MyWindow( Rect cFrame, const char* pzName, const char* pzTitle, int nFlags )
    : Window( cFrame, pzName, pzTitle, nFlags )
{
}

void MyWindow::HandleMessage( Message* pcMsg )
{
    switch( pcMsg->GetCode() )
    {
	case ID_SCROLL:
	    g_pcTermView->ScrollBack( g_pcScrollBar->GetValue() );
	    break;
/*	    
	case 12345678:
	{
	    MessageQueue*	pcQueue = GetMessageQueue();

	    if ( NULL == pcQueue->FindMessage( 12345678, 0 ) ) { // Wait for the last event
		g_pcTermView->RefreshDisplay( true );
	    }
	    break;
	}
	*/	
    }
}


bool MyWindow::OkToQuit()
{
    g_bRun = false;
    close( g_nMasterPTY );
    return( true );
}


bool OpenWindow()
{
    MyApp*	pcMyApp;
    int		i;

    pcMyApp = new MyApp();
    pcMyApp->Unlock();
    
    g_pcWindow = new MyWindow( g_cWinRect, "_aterm_", "AtheOS Terminal",
			       g_bWindowBorder?0:WND_NO_CLOSE_BUT|WND_NO_ZOOM_BUT|WND_NO_DEPTH_BUT|WND_NO_TITLE|WND_NO_BORDER );
  
    Rect cTermFrame   = g_pcWindow->GetBounds();
    Rect cScrollBarFrame = cTermFrame;
	
//    cScrollBarFrame.left = cTermFrame.right + 1;
		

    g_pcScrollBar = new ScrollBar( cScrollBarFrame, "", new Message( ID_SCROLL ), 0, 24,
				   VERTICAL, CF_FOLLOW_TOP | CF_FOLLOW_BOTTOM | CF_FOLLOW_RIGHT );

    cScrollBarFrame.left = cScrollBarFrame.right - g_pcScrollBar->GetPreferredSize(false).x;
    g_pcScrollBar->SetFrame( cScrollBarFrame );
    
    cTermFrame.right = cScrollBarFrame.left - 1;
    
    g_pcTermView  = new TermView( cTermFrame, "", CF_FOLLOW_ALL, WID_WILL_DRAW );
    
    g_pcWindow->AddChild( g_pcTermView );
    g_pcWindow->AddChild( g_pcScrollBar );
    g_pcWindow->SetDefaultWheelView( g_pcScrollBar );

    IPoint cGlypSize( g_pcTermView->GetGlyphSize() );
    g_pcWindow->SetAlignment( cGlypSize, IPoint( int(cScrollBarFrame.Width()) % int(cGlypSize.x), 0 ) );
    g_pcWindow->ResizeTo( cGlypSize.x * 80 + cScrollBarFrame.Width(), cGlypSize.y * 24 );
    if ( g_pcWindow->GetFrame().right >= Desktop().GetResolution().x ) {
	g_pcWindow->MoveTo( Desktop().GetResolution().x / 2 - g_pcWindow->GetFrame().Width() * 0.5f, g_cWinRect.top );
    }
    if ( g_pcWindow->GetFrame().bottom >= Desktop().GetResolution().y ) {
	g_pcWindow->MoveTo( g_pcWindow->GetFrame().left, Desktop().GetResolution().y / 2 - g_pcWindow->GetFrame().Height() * 0.5f );
    }
    g_pcWindow->MakeFocus( true );
    g_pcWindow->Show();
}

void ReadPTY()
{
    char zBuf[ 4096 ];

    int	nBytesRead;

    signal( SIGHUP, SIG_IGN );
    signal( SIGINT, SIG_IGN );
    signal( SIGQUIT, SIG_IGN );
    signal( SIGALRM, SIG_IGN );
    signal( SIGCHLD, SIG_IGN );
    signal( SIGTTIN, SIG_IGN );
    signal( SIGTTOU, SIG_IGN );
	
    while( g_bRun && NULL != g_pcWindow )
    {
	nBytesRead = read( g_nMasterPTY, zBuf, 4096 );

	if ( nBytesRead < 0 ) {
	    snooze( 40000 );
//	    dbprintf( "Failed to read from tty %d\n", nBytesRead );
	}
/*    
      if ( nBytesRead == 256 ) {
      SetTaskPri( g_hUpdateThread, 0 );
      } else {
      SetTaskPri( g_hUpdateThread, 0 );
      }
      */    
	if ( nBytesRead > 0 ) {
	    if ( g_pcWindow->Lock() == 0 ) {
		g_pcTermView->Write( zBuf, nBytesRead );
		g_pcWindow->Unlock();
	    }
	}
    }
}

int RefreshThread( void* pData )
{
    signal( SIGHUP, SIG_IGN );
    signal( SIGINT, SIG_IGN );
    signal( SIGQUIT, SIG_IGN );
    signal( SIGALRM, SIG_IGN );
    signal( SIGCHLD, SIG_IGN );
    signal( SIGTTIN, SIG_IGN );
    signal( SIGTTOU, SIG_IGN );
	
    while ( g_bRun && NULL != g_pcWindow )
    {
	if ( g_pcWindow->Lock() == 0 ) {
	    g_pcTermView->RefreshDisplay( false );
	    g_pcWindow->Unlock();
	}
	snooze( 40000 );
    }
}

static struct option const long_opts[] =
{
    {"debug", required_argument, NULL, 'd' },
    {"frame", required_argument, NULL, 'f' },
    {"def_attr", required_argument, NULL, 'a' },
    {"ibeam_halo", no_argument, &g_bIBeamHalo, 1 },
    {"noborder", no_argument, NULL, 'b' },
    {"help", no_argument, &g_nShowHelp, 1},
    {"version", no_argument, &g_nShowVersion, 1},
    {NULL, 0, NULL, 0}
};

static void usage( const char* pzName, bool bFull )
{
    printf( "Usage: %s [-hfbdv] [command [args]]\n", pzName );
    if ( bFull )
    {
	printf( "  -d --debug=level   set amount of debug output to send to parent terminal\n" );
	printf( "  -f --frame=l,t,r,b set window position/size (left,top,right,bottom)\n" );
	printf( "  -a --def_attr=attr set default display attribs (fg/bg color & bold/underline)\n" );
	printf( "  -i --ibeam_halo    put an white halo around the i-beam cursor\n" );
	printf( "  -b --noborder      hide the window border\n" );
	printf( "  -h --help          display this help and exit\n" );
	printf( "  -v --version       display version information and exit\n" );
    }
}
int main( int argc, char** argv )
{
    thread_id hShellThread;
    thread_id g_hReadThread;
    char	    zPtyName[128];
    char	    zShellPath[PATH_MAX] = "/bin/bash";
    int	    i;
    char*	    apzDefaultShellArgv[3] = { zShellPath, "--login", NULL };
    char**	    apzShellArgv = apzDefaultShellArgv;
    int	    c;
    const char* pzDefAttr;

    pzDefAttr = getenv( "ATERM_ATTR" );

    if ( pzDefAttr != NULL ) {
	g_nDefaultAttribs = strtol( pzDefAttr, NULL, 0 );
    }

    struct passwd* psPW = getpwuid( getuid() );

    if ( psPW != NULL ) {
	strcpy( zShellPath, psPW->pw_shell );
    }
    
    while( (c = getopt_long (argc, argv, "hvid:f:ba:", long_opts, (int *) 0)) != EOF )
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
	    case 'i':
		g_bIBeamHalo = true;
		break;
	    case 'd':
		printf( "Debug level = %s\n", optarg );
		g_nDebugLevel = atol( optarg );
		break;
	    case 'f':
		sscanf( optarg, "%f,%f,%f,%f",
			&g_cWinRect.left,&g_cWinRect.top,&g_cWinRect.right,&g_cWinRect.bottom );
		break;
	    case 'b':
		g_bWindowBorder = false;
		break;
	    case 'a':
		g_nDefaultAttribs = strtol( optarg, NULL, 0 );
		break;
	    default:
		usage( argv[0], false );
		exit( 1 );
		break;
	}
    }
    if ( g_nShowVersion ) {
	printf( "Syllable virtual terminal V0.1.1\n" );
	exit( 0 );
    }
    if ( g_nShowHelp ) {
	usage( argv[0], true );
	exit( 0 );
    }

    if ( optind < argc ) {
	  // are the argv array NULL terminated?
	apzShellArgv = (char**)malloc( (1+argc-optind)*sizeof(char*) );
	memcpy( apzShellArgv, argv+optind, (argc-optind)*sizeof(char*) );
	apzShellArgv[argc-optind] = NULL;
    }
/*  
    if ( argc > 1 ) {
    pzCommand = argv[1];
    }
    */
/*	setpgid( 0, 0 ); */
	
    for ( i = 0 ; i < 10000 ; ++i )
    {
	struct stat sStat;
		
	sprintf( zPtyName, "/dev/pty/mst/pty%d", i );

	if ( stat( zPtyName, &sStat ) == -1 )
	{
	    g_nMasterPTY = open( zPtyName, O_RDWR | O_CREAT );
	    if ( g_nMasterPTY >= 0 ) {
		sprintf( zPtyName, "/dev/pty/slv/pty%d", i );
		break;
	    }
	}
    }


    hShellThread = fork();

    if ( 0 == hShellThread )
    {
	int nSlavePTY;
		
	if ( setsid() == -1 ) {
	    printf( "setsid() failed - %s\n", strerror( errno ) );
	}
	nSlavePTY = open( zPtyName, O_RDWR );
	tcsetpgrp( nSlavePTY, getpgrp() );

	termios	sTerm;
		
	tcgetattr( nSlavePTY, &sTerm );
	sTerm.c_oflag |= ONLCR;
	tcsetattr( nSlavePTY, TCSANOW, &sTerm );
		
	dup2( nSlavePTY, 0 );
	dup2( nSlavePTY, 1 );
	dup2( nSlavePTY, 2 );

	struct winsize sWinSize;

	sWinSize.ws_col 	 = 80;
	sWinSize.ws_row 	 = 24;
	sWinSize.ws_xpixel = 80 * 8;
	sWinSize.ws_ypixel = 24 * 8;
		
	ioctl( nSlavePTY, TIOCSWINSZ, &sWinSize );

	for ( i = 3 ; i < 256 ; ++i ) {
	    close( i );
	}
	if ( g_nDebugLevel > 0 ) {
	    printf( "Execute command %s\n", apzShellArgv[0] );
	}
//    setenv( "TERM", "xterm-color", true );
	setenv( "TERM", "xterm", true );
	execve( apzShellArgv[0], apzShellArgv, environ );
	printf( "Failed to execute %s\n", apzShellArgv[0] );
	sleep( 2 ); // give the user a chance to the the error
	exit( 1 );
    }
    else
    {
	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGALRM, SIG_IGN );
	signal( SIGCHLD, SIG_DFL );
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	OpenWindow();

	g_hUpdateThread = spawn_thread( "update", (void*)RefreshThread, 5, 0, NULL );
	resume_thread( g_hUpdateThread );

	g_hReadThread = spawn_thread( "read", (void*)ReadPTY, 10, 0, NULL );
	resume_thread( g_hReadThread );
	
	
/* 	ReadPTY(); */

	int nStatus;
	
	while( g_bRun && NULL != g_pcWindow )
	{
	    pid_t hPid = waitpid( hShellThread, NULL, 0 );
	    if (  hPid == hShellThread ) {
		g_bRun = false;
		if ( g_pcWindow != NULL ) {
		    g_pcWindow->Quit();
		}
	    }
	    if ( hPid < 0  ) {
		break;
	    }
	}
/*	kill( g_hReadThread, SIGKILL ); */
/*
  kill( hShellThread, SIGKILL );
  kill( g_hReadThread, SIGKILL );
  */
    }
    return( 0 );
}
