/*
 *  rdloader - Utility to initialize a RAM disk from floppy disks.
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
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/progressbar.h>
#include <gui/button.h>
#include <gui/requesters.h>
#include <gui/desktop.h>

#include <zlib.h>

using namespace os;

#define DISK_SIZE (80*18*2*512)
#define HEADER_MAGIC 0xa105f100

#define min(a,b) ((a)<(b) ? (a) : (b))

enum
{
    ID_OK
};

class MainRequester : public LayoutView
{
public:
    MainRequester( const Rect& cRect, const std::string& cTitle );

    void SetTotalProgress( float vValue ) { m_pcTotalProgress->SetProgress( vValue ); }
    void SetDiskProgress( float vValue )  { m_pcDiskProgress->SetProgress( vValue ); }
    void SetDiskPath( const std::string& cString ) { m_pcDiskPathView->SetString( cString.c_str() ); InvalidateLayout(); }
    void SetDestPath( const std::string& cString ) { m_pcDestPathView->SetString( cString.c_str() ); InvalidateLayout(); }
    void SetDestName( const std::string& cString ) { m_pcDestNameView->SetString( cString.c_str() ); InvalidateLayout(); }
private:
    StringView*	 m_pcDiskPathView;
    ProgressBar* m_pcTotalProgress;
    ProgressBar* m_pcDiskProgress;
    StringView*	 m_pcDestPathView;
    StringView*	 m_pcDestNameView;
};

class MainWindow : public Window
{
public:
    MainWindow( const std::string& cTitle );
    void SetTotalProgress( float vValue ) { m_pcProgView->SetTotalProgress( vValue ); }
    void SetDiskProgress( float vValue )  { m_pcProgView->SetDiskProgress( vValue ); }
    void SetDiskPath( const std::string& cString ) { m_pcProgView->SetDiskPath( cString ); }
    void SetDestPath( const std::string& cString ) { m_pcProgView->SetDestPath( cString ); }
    void SetDestName( const std::string& cString ) { m_pcProgView->SetDestName( cString ); }

    virtual bool	OkToQuit() { return( true ); }
private:
    MainRequester* m_pcProgView;
};

class	MyApp : public Application
{
public:
    MyApp();
    ~MyApp();
private:
};

MainWindow* g_pcMainWindow;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyApp::MyApp() : Application( "application/x-vnd.KHS.rdloader" )
{
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

MainWindow::MainWindow( const std::string& cTitle ) : Window( Rect(0,0,300,200), "main_window", cTitle )
{
    Rect cBounds = GetBounds();
    Desktop cDesktop;
    MoveTo( cDesktop.GetResolution().x / 2 - cBounds.Width() / 2, cDesktop.GetResolution().y / 2 - cBounds.Height() / 2 );
    
    m_pcProgView = new MainRequester( cBounds, "main_view" );
    AddChild( m_pcProgView );
}

MainRequester::MainRequester( const Rect& cRect, const std::string& cTitle ) : LayoutView( cRect, cTitle )
{
    VLayoutNode* pcRoot = new VLayoutNode( "root" );
    m_pcDiskPathView  = new StringView( Rect(0,0,1,1), "disk_path", "", ALIGN_CENTER );
    m_pcTotalProgress = new ProgressBar( Rect(0,0,1,1), "total_progress" );
    m_pcDiskProgress  = new ProgressBar( Rect(0,0,1,1), "disk_progress" );
    m_pcDestPathView  = new StringView( Rect(0,0,1,1), "dest_path", "", ALIGN_CENTER );
    m_pcDestNameView  = new StringView( Rect(0,0,1,1), "dest_name", "", ALIGN_CENTER );

    LayoutNode* pcNode;
    pcNode = pcRoot->AddChild( m_pcDiskPathView );
    pcNode = pcRoot->AddChild( m_pcDestPathView );
    pcNode->ExtendMaxSize( Point( 10000.0f, 0.0f ) );
    pcNode = pcRoot->AddChild( m_pcDestNameView );
    pcNode->ExtendMaxSize( Point( 10000.0f, 0.0f ) );
    pcNode->ExtendMaxSize( Point( 10000.0f, 0.0f ) );
    pcRoot->AddChild( m_pcDiskProgress );
    pcRoot->AddChild( m_pcTotalProgress );

    pcRoot->SetBorders( Rect( 20.0f, 10.0f, 20.0f, 10.0f ), "disk_path", "total_progress", "disk_progress", "dest_path", "dest_name", NULL );
    
    SetRoot( pcRoot );
}

int do_read( gzFile hFile, void* pBuffer, int nSize )
{
    int nBytesRead = 0;
    while( nSize > 0 ) {
	int nCurLen = gzread( hFile, pBuffer, nSize );
	if ( nCurLen < 0 ) {
	    return( -1 );
	}
	nSize -= nCurLen;
	pBuffer = (void*)(((char*)pBuffer)+nCurLen);
	nBytesRead += nCurLen;
    }
    return( nBytesRead );
}

uint32 read_int32( gzFile hFile )
{
    uint32 nValue;
    do_read( hFile, &nValue, sizeof(nValue) );
    return( nValue );
}

uint8 read_int8( gzFile hFile )
{
    uint8 nValue;
    do_read( hFile, &nValue, sizeof(nValue) );
    return( nValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static int unpack_archive( char* pzPath, gzFile hSrcFile )
{
    int nBytesRead = 0;

    g_pcMainWindow->Lock();
    g_pcMainWindow->SetDestPath( pzPath );
    g_pcMainWindow->Unlock();
    
    while( true ) {
	char zFullPath[PATH_MAX];
	char zName[256];
	uint32 nMode  = read_int32( hSrcFile );
	uint32 nNameLen;

	if ( nMode == uint32(~0) ) {
	    break;
	}
	
	nNameLen = read_int8( hSrcFile );

	do_read( hSrcFile, zName, nNameLen );
	zName[nNameLen] = '\0';

	strcpy( zFullPath, pzPath );
	strcat( zFullPath, "/" );
	strcat( zFullPath, zName );
	
	g_pcMainWindow->Lock();
	g_pcMainWindow->SetDestName( zName );
	g_pcMainWindow->Unlock();
	
	if ( S_ISDIR( nMode ) ) {
	    mkdir( zFullPath, nMode );
	    
	    int nLen = unpack_archive( zFullPath, hSrcFile );
	    if ( nLen < 0 ) {
		return( nLen );
	    }
	} else if ( S_ISREG( nMode ) ) {
	    int32 nFileSize = read_int32( hSrcFile );

	    int nDest = open( zFullPath, O_WRONLY | O_CREAT, nMode );
	    while( nFileSize > 0 ) {
		char anBuffer[4096];
		int  nCurLen = do_read( hSrcFile, anBuffer, min( 4096, nFileSize ) );
		if ( nCurLen < 0 ) {
		    printf( "Error: failed to read file contents\n" );
		    break;
		}
		write( nDest, anBuffer, nCurLen );
		nFileSize -= nCurLen;
	    }
	    close( nDest );
	} else if ( S_ISLNK(nMode) ) {
	    uint32 nLinkSize = read_int32( hSrcFile );
	    char zLink[PATH_MAX];

	    do_read( hSrcFile, zLink, nLinkSize );
	    zLink[nLinkSize] = '\0';
	    symlink( zLink, zFullPath );
	}
    }
    return( nBytesRead );
}

int32 unpack_thread( void* pData )
{
    gzFile hStream = gzdopen( (int)pData, "r" );
    if ( hStream == NULL ) {
	printf( "Error: failed to open gzip stream\n" );
	close( (int)pData );
	return( 1 );
    }
    umask( 0 );
//    unpack_archive( "test", hStream );
    unpack_archive( "/boot", hStream );
    gzclose( hStream );
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int main( int argc, char ** argv )
{
    MyApp* pcApp = new MyApp;
    uint32 anHeader[128];
    int    anPipe[2];
    uint32 nNumDisks = 1;
    uint32 nImageSize = 0;
    uint32 nImageID = 0;

    int nBytesRead = 0;

    if ( open( "/bin/desktop", O_RDONLY ) >= 0 ) {
	printf( "Not on a RAM disk. Aborting to avoid overwriting system files\n" );
	exit( 1 );
    }

    g_pcMainWindow = new MainWindow( "AtheOS RAM Disk Loader V0.2" );
    g_pcMainWindow->Show();
    g_pcMainWindow->MakeFocus();
    
    pipe( anPipe );

    thread_id hThread = spawn_thread( "unarchive", unpack_thread, NORMAL_PRIORITY, 0, (void*)(anPipe[0]) );
    resume_thread( hThread );
    
    for ( uint32 i = 0 ; i < nNumDisks ; ++i ) {
	char zText[1024];
	char zPath[PATH_MAX];
	
	if ( i == 0 ) {
	    sprintf( zText, "Please insert data disk %ld in drive A:", i + 1 );
	} else {
	    sprintf( zText, "Please insert data disk %ld of %ld in drive A:", i + 1, nNumDisks + 1 );
	}
	Alert* pcAlert = new Alert( "Insert disk:", zText, 0, "Ok", NULL );

	pcAlert->Go();

	sprintf( zPath, "images/test.%02ld", i + 1 );

retry:	
//	int nDisk = open( zPath, O_RDONLY );
	int nDisk = open( "/dev/disk/bios/fda/raw", O_RDONLY );

	g_pcMainWindow->Lock();
	g_pcMainWindow->SetDiskPath( "/dev/disk/bios/fda/raw" );
	g_pcMainWindow->Unlock();
	
	if ( nDisk < 0 ) {
	    (new Alert( "Error:", "Failed to open floppy device!", 0, "Retry", NULL ))->Go();
	    goto retry;
	}
	
	if ( read( nDisk, anHeader, 512 ) != 512 ) {
	    (new Alert( "Error:", "Failed to read image header!", 0, "Retry", NULL ))->Go();
	    close( nDisk );
	    goto retry;
	}
	if ( anHeader[0] != HEADER_MAGIC ) {
	    (new Alert( "Error:", "Disk does not contain an AtheOS boot image!", 0, "Retry", NULL ))->Go();
	    close( nDisk );
	    goto retry;
	}
	if ( i == 0 ) {
	    nNumDisks = anHeader[1];
	    nImageSize = anHeader[3];
	    nImageID   = anHeader[4];
	}
	if ( anHeader[4] != nImageID ) {
	    (new Alert( "Error:", "Disk not from same boot image!", 0, "Retry", NULL ))->Go();
	    close( nDisk );
	    goto retry;
	    
	}
	
	if ( anHeader[2] != i ) {
	    char zMsg[1024];
	    sprintf( zMsg, "Wrong disk. This is disk #%ld please insert disk #%ld\n", anHeader[2] + 2, i + 2 );
	    (new Alert( "Error:", zMsg, 0, "Retry", NULL ))->Go();
	    close( nDisk );
	    goto retry;
	    
	}
	
	int nDiskSize = min(DISK_SIZE - 512, nImageSize - nBytesRead );
	int nBytesReadFromDisk = 0;

	while( nBytesReadFromDisk < nDiskSize ) {
	    char anBuffer[9216];
	    int  nCurSize = min(9216,(nDiskSize-nBytesReadFromDisk));

	    while ( read( nDisk, anBuffer, (nCurSize + 511) & ~511 ) != ((nCurSize + 511) & ~511) ) {
		(new Alert( "Error:", "Read error!", 0, "Retry", NULL ))->Go();
	    }
	    write( anPipe[1], anBuffer, nCurSize );
	    nBytesReadFromDisk += nCurSize;
	    nBytesRead += nCurSize;
	    g_pcMainWindow->Lock();
	    g_pcMainWindow->SetTotalProgress( float(nBytesRead) / float(nImageSize) );
	    g_pcMainWindow->SetDiskProgress( float(nBytesReadFromDisk) / float(nDiskSize) );
	    g_pcMainWindow->Unlock();
	}
	close( nDisk );
    }
    close( anPipe[1] );
    pcApp->PostMessage( M_QUIT );
    pcApp->Run();
    wait_for_thread( hThread );

    if ( fork() == 0 ) {
	execl( "/bin/aterm", "aterm", NULL );
    }
    return( 0 );
}

