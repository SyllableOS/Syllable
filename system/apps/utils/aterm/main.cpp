
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
 
#define DEBUG( level, fmt, args... ) do { if ( level < g_nDebugLevel ) printf( fmt, ## args ); } while(0)
 
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
#include <gui/guidefines.h>
#include <util/resources.h>

#include "tview.h"
#include "messages.h"
#include <iostream>

using namespace os;

void SetDesktop( int nDesktop );

class MyWindow:public Window
{
      public:
	MyWindow( Rect cFrame, const char *pzName, const char *pzTitle, int nFlags, uint32 nDesktopMask );

	bool OkToQuit();
	void HandleMessage( Message * pcMessage );

	void FrameMoved( const Point & cDelta );
};

class MyApp:public Application
{
      public:
	MyApp();

	bool OkToQuit();
	void HandleMessage( Message * pcMessage );
	thread_id Run();
};

TermSettings *g_pcSettings = NULL;
MyWindow *g_pcWindow = NULL;
TermView *g_pcTermView = NULL;
ScrollBar *g_pcScrollBar = NULL;

int g_nShowHelp = 0;
int g_nShowVersion = 0;
int g_nDebugLevel = 0;
uint32 g_nDesktop = os::Desktop::ACTIVE_DESKTOP;
bool g_bWindowBorder = true;
bool g_bFullScreen = false;
extern char **environ;

int g_nMasterPTY;

thread_id g_hShellThread = 0;
thread_id g_hUpdateThread = 0;
thread_id g_hReadThread = 0;
thread_id g_hMainThread = 0;

static volatile bool g_bWindowActive = true;


/**
 * Application constructor
 */
MyApp::MyApp():Application( "application/x-vnd.KHS-aterm" )
{
}

thread_id MyApp::Run()
{
	return( Looper::Run() );  /* Want it to run in a new thread */
}


/**
 * Called when M_QUIT message received; possibly from user pressing Alt+Q or system shutting down.
 *
 * Kills the shell thread, whose death will be caught by main thread.
 */
bool MyApp::OkToQuit()
{
	if( g_hShellThread > 0 ) kill( g_hShellThread, SIGKILL );
	return( false );
}


/**
 * Generic message handler
 *
 * Forwards REFRESH messages from settings window to terminal window.
 */
void MyApp::HandleMessage( Message * pcMsg )
{
	switch ( pcMsg->GetCode() )
	{
	case M_REFRESH:
		g_pcTermView->HandleMessage( pcMsg );
		break;

	default:
		Application::HandleMessage( pcMsg );
	}
}


/**
 * Terminal window constructor
 */
MyWindow::MyWindow( Rect cFrame, const char *pzName, const char *pzTitle, int nFlags, uint32 nDesktopMask ):Window( cFrame, pzName, pzTitle, nFlags, nDesktopMask )
{
	/* Set Dock icon */
	os::BitmapImage *pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	pcImage->Load( cRes.GetResourceStream( "icon48x48.png" ) );
	SetIcon(pcImage->LockBitmap());
	delete(pcImage);
}

/**
 * Generic message handler
 *
 * Receives SCROLL messages and calls TermView::ScrollBack().
 */
void MyWindow::HandleMessage( Message * pcMsg )
{
	switch ( pcMsg->GetCode() )
	{
	case M_SCROLL:
		g_pcTermView->ScrollBack( g_pcScrollBar->GetValue() );
		break;

	default:
		Window::HandleMessage( pcMsg );
	}
}

/**
 * Generic window closing handler
 *
 * When user closes the window, kill the shell. Its death will be caught by main thread.
 */
bool MyWindow::OkToQuit()
{
	if( g_hShellThread > 0 ) kill( g_hShellThread, SIGKILL );
	return( false );
}


/**
 * Generic window movement handler
 *
 * When terminal window is moved, changes are immediatly reflected to the settings structure.
 */
void MyWindow::FrameMoved( const Point & cDelta )
{
	g_pcSettings->SetPosition( IPoint( GetFrame().LeftTop(  ) ) );
}


/**
 * Opens a new terminal window
 *
 * This method heavily relies on global variables...
 */
void OpenWindow()
{
	Desktop cDesktop( g_nDesktop );
	IRect cWinRect = IRect( 0, 0, cDesktop.GetResolution().x + 200, cDesktop.GetResolution(  ).y + 200 );

	if( !g_bFullScreen )
	{
		/* Get window position from settings */
		cWinRect.left = g_pcSettings->GetPosition().x;
		cWinRect.top = g_pcSettings->GetPosition().y;
		cWinRect.right = cWinRect.left + 200;	/* temporary size */
		cWinRect.bottom = cWinRect.top + 200;
	}
	/* Instanciate window */
	g_pcWindow = new MyWindow( cWinRect, "_sterm_", "Syllable Terminal", g_bWindowBorder ? 0 : WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NO_TITLE | WND_NO_BORDER, (g_nDesktop == os::Desktop::ACTIVE_DESKTOP ? CURRENT_DESKTOP : 1 << g_nDesktop) );

	Rect cTermFrame = g_pcWindow->GetBounds();
	Rect cScrollBarFrame = cTermFrame;

	/* Instanciate ScrollBar */
	g_pcScrollBar = new ScrollBar( cScrollBarFrame, "TermScroll", new Message( M_SCROLL ), 0, 24, VERTICAL, CF_FOLLOW_TOP | CF_FOLLOW_BOTTOM | CF_FOLLOW_RIGHT );

	cScrollBarFrame.left = cScrollBarFrame.right - g_pcScrollBar->GetPreferredSize( false ).x;
	g_pcScrollBar->SetFrame( cScrollBarFrame );

	cTermFrame.right = cScrollBarFrame.left - 1;

	/* Instanciate TermView */
	g_pcTermView = new TermView( cTermFrame, g_pcSettings, "TermView", CF_FOLLOW_ALL, WID_WILL_DRAW );

	/* Add childs to the window */
	g_pcWindow->AddChild( g_pcTermView );
	g_pcWindow->AddChild( g_pcScrollBar );
	g_pcWindow->SetDefaultWheelView( g_pcScrollBar );

	/* Compute window size and position from settings */
	IPoint cGlypSize( g_pcTermView->GetGlyphSize() );

	cWinRect.right = cWinRect.left + ( int )( cGlypSize.x * g_pcSettings->GetSize().x + cScrollBarFrame.Width(  ) );
	cWinRect.bottom = cWinRect.top + ( int )( cGlypSize.y * g_pcSettings->GetSize().y );

	if( g_bFullScreen )
	{
		Desktop cDesktop;

		g_pcWindow->SetFrame( Rect( 0, 0, cDesktop.GetResolution().x, cDesktop.GetResolution(  ).y ) );
	}

	else
		g_pcWindow->SetFrame( cWinRect, true );

	//Declare resize alignment 
	if( !g_bFullScreen )
	{
		g_pcWindow->SetAlignment( cGlypSize, IPoint( int ( cScrollBarFrame.Width() ) % int ( cGlypSize.x ), 0 ) );

		if( g_pcWindow->GetFrame().right >= Desktop(  ).GetResolution(  ).x )
		{
			g_pcWindow->MoveTo( Desktop().GetResolution(  ).x / 2 - g_pcWindow->GetFrame(  ).Width(  ) * 0.5f, g_pcWindow->GetFrame(  ).top );
		}

		if( g_pcWindow->GetFrame().bottom >= Desktop(  ).GetResolution(  ).y )
		{
			g_pcWindow->MoveTo( g_pcWindow->GetFrame().left, Desktop(  ).GetResolution(  ).y / 2 - g_pcWindow->GetFrame(  ).Height(  ) * 0.5f );
		}
		g_pcWindow->MakeFocus( true );
	}

	else
	{
		g_pcWindow->MoveTo( 0, 0 );
	}
	g_pcWindow->Show();
}

void ReadPTY()
{
	char zBuf[4096];

	int nBytesRead;

	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGALRM, SIG_IGN );
	signal( SIGCHLD, SIG_IGN );
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	while( g_bWindowActive && NULL != g_pcWindow )
	{
		nBytesRead = read( g_nMasterPTY, zBuf, 4096 );

		if( nBytesRead < 0 )
		{
			snooze( 40000 );
		}

		if( nBytesRead > 0 )
		{
			if( g_pcWindow->Lock() == 0 )
			{
				g_pcTermView->Write( zBuf, nBytesRead );
				g_pcWindow->Unlock();
			}
		}
	}
}

void RefreshThread( void *pData )
{
	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGALRM, SIG_IGN );
	signal( SIGCHLD, SIG_IGN );
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	while( g_bWindowActive && NULL != g_pcWindow )
	{
		if( g_pcWindow->Lock() == 0 )
		{
			g_pcTermView->RefreshDisplay( false );
			g_pcWindow->Unlock();
		}
		snooze( 40000 );
	}
}

static struct option const long_opts[] = {
	{"debug", required_argument, NULL, 'd'},
	{"frame", required_argument, NULL, 'f'},
	{"fullscreen", no_argument, NULL, 'F'},
	{"ibeam_halo", no_argument, NULL, 'i'},
	{"noborder", no_argument, NULL, 'b'},
	{"help", no_argument, &g_nShowHelp, 'h'},
	{"version", no_argument, &g_nShowVersion, 'v'},
	{"screen", required_argument, NULL, 's'},
	{NULL, 0, NULL, 0}
};

/**
 * Display usual usage recommandations
 *
 */
static void usage( const char *pzName, bool bFull )
{
	printf( "Usage: %s [-bdfhisv] [command [args]]\n", pzName );
	if( bFull )
	{
		printf( "  -b --noborder       Hide the window border\n" );
		printf( "  -d --debug=level    Set amount of debug output to send to parent terminal\n" );
		printf( "  -f --frame=x,y,c,l  Set window position/size (left,top,cols,lines)\n" );
		printf( "  -F --fullscreen     Fullscreen the terminal\n" );
		printf( "  -h --help           Display this help and exit\n" );
		printf( "  -i --ibeam_halo     Put an white halo around the i-beam cursor\n" );
		printf( "  -s --screen=n       Open the terminal window on desktop n (between 1 and 32)\n" );
		printf( "  -v --version        Display version information and exit\n" );
	}
}


int main( int argc, char **argv )
{
	char zPtyName[128];
	char zShellPath[PATH_MAX] = "/bin/bash";
	int i;
	char *apzDefaultShellArgv[3] = { zShellPath, "--login", NULL };
	char **apzShellArgv = apzDefaultShellArgv;
	int c;

	/* Instanciate Application */
	MyApp *pcMyApp;

	pcMyApp = new MyApp();

	/* load user's settings */
	g_pcSettings = new TermSettings();

	struct passwd *psPW = getpwuid( getuid() );

	if( psPW != NULL )
	{
		strcpy( zShellPath, psPW->pw_shell );
	}

	while( ( c = getopt_long( argc, argv, "hvFid:f:bs:", long_opts, ( int * )0 ) ) != EOF )
	{
		switch ( c )
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
			g_pcSettings->SetIBeam( true );
			break;
		case 'd':
			printf( "Debug level = %s\n", optarg );
			g_nDebugLevel = atol( optarg );
			break;
		case 'f':
			{
				int x, y, c, l;

				sscanf( optarg, "%d,%d,%d,%d", &x, &y, &c, &l );
				g_pcSettings->SetPosition( IPoint( x, y ) );
				g_pcSettings->SetSize( IPoint( c, l ) );
			}
			break;

		case 'F':
			{
				g_bFullScreen = true;
			}
			break;
		case 'b':
			g_bWindowBorder = false;
			break;
		case 'a':
			g_nDefaultAttribs = strtol( optarg, NULL, 0 );
			break;

		case 's':
			SetDesktop( atoi( optarg ) );
			break;

		default:
			usage( argv[0], false );
			exit( 1 );
			break;
		}
	}
	if( g_nShowVersion )
	{
		printf( "Syllable virtual terminal " VERSION "\n" );
		exit( 0 );
	}
	if( g_nShowHelp )
	{
		usage( argv[0], true );
		exit( 0 );
	}

	if( optind < argc )
	{
		// are the argv array NULL terminated?
		apzShellArgv = ( char ** )malloc( ( 1 + argc - optind ) * sizeof( char * ) );
		memcpy( apzShellArgv, argv + optind, ( argc - optind ) * sizeof( char * ) );
		apzShellArgv[argc - optind] = NULL;
	}

	for( i = 0; i < 10000; ++i )
	{
		struct stat sStat;

		sprintf( zPtyName, "/dev/pty/mst/pty%d", i );

		if( stat( zPtyName, &sStat ) == -1 )
		{
			g_nMasterPTY = open( zPtyName, O_RDWR | O_CREAT );
			if( g_nMasterPTY >= 0 )
			{
				sprintf( zPtyName, "/dev/pty/slv/pty%d", i );
				break;
			}
		}
	}


	g_hShellThread = fork();

	if( 0 == g_hShellThread )
	{
		int nSlavePTY;

		if( setsid() == -1 )
		{
			printf( "setsid() failed - %s\n", strerror( errno ) );
		}
		nSlavePTY = open( zPtyName, O_RDWR );
		tcsetpgrp( nSlavePTY, getpgrp() );

		termios sTerm;

		tcgetattr( nSlavePTY, &sTerm );
		sTerm.c_oflag |= ONLCR;
		tcsetattr( nSlavePTY, TCSANOW, &sTerm );

		dup2( nSlavePTY, 0 );
		dup2( nSlavePTY, 1 );
		dup2( nSlavePTY, 2 );

		struct winsize sWinSize;

		sWinSize.ws_col = 80;
		sWinSize.ws_row = 24;
		sWinSize.ws_xpixel = 80 * 8;
		sWinSize.ws_ypixel = 24 * 8;

		ioctl( nSlavePTY, TIOCSWINSZ, &sWinSize );

		for( i = 3; i < 256; ++i )
		{
			close( i );
		}
		if( g_nDebugLevel > 0 )
		{
			printf( "Execute command %s\n", apzShellArgv[0] );
		}
		setenv( "TERM", "xterm", true );
		execve( apzShellArgv[0], apzShellArgv, environ );
		printf( "Failed to execute %s\n", apzShellArgv[0] );
		sleep( 2 );	// give the user a chance to the the error
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
		
		pcMyApp->Run();

		OpenWindow();

		g_hMainThread = getpid();

		g_hUpdateThread = spawn_thread( "update", ( void * )RefreshThread, 5, 0, NULL );
		resume_thread( g_hUpdateThread );

		g_hReadThread = spawn_thread( "read", ( void * )ReadPTY, 10, 0, NULL );
		resume_thread( g_hReadThread );
		
		pid_t hPid = waitpid( g_hShellThread, NULL, 0 );
		g_hShellThread = 0;
		g_bWindowActive = false;
	
		/* Kill all threads */
		if( g_hReadThread > 0 ) kill( g_hReadThread, SIGKILL );
		if( g_hUpdateThread > 0 ) kill( g_hReadThread, SIGKILL );
		close( g_nMasterPTY );

		MyApp::GetInstance()->Quit();  /* this will close the window, kill the application thread */
	}
	return ( 0 );
}

void SetDesktop( int nDesktop )
{
	if( nDesktop > 0 && nDesktop <= 32 ) /* Passed a specific desktop number */
	{
		g_nDesktop = nDesktop - 1;  /* system counts from 0, user counts from 1 */
	}
	else printf( "aterm: invalid parameter '%i' for -s option; defaulting to current desktop.\n" );
}



