#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include <atheos/kernel.h>

#include <gui/window.h>
#include <gui/bitmap.h>
#include <util/message.h>

static int32 render_thread( void* pData );

#include "pdfview.h"

extern char* g_pzFileName;


PDFView::PDFView( const Rect& cFrame ) :
    View( cFrame, "bitmap_view", CF_FOLLOW_ALL )
{
  m_pcBitmap = new Bitmap( 850, 1100, CS_RGB32 );
  m_nPage = 0;
  m_hRenderThread = -1;
}

void PDFView::Paint( const Rect& cUpdateRect )
{
    Rect cRect = m_pcBitmap->GetBounds();
    DrawBitmap( m_pcBitmap, cRect, cRect );
}

int RunGS( char* pzSrc, const char* pzDst, int nPage, int nDPI, int* pnWriteFile, pid_t* pnPid  )
{
  pid_t nPid;
  int anReadPipes[2];
  int anWritePipes[2];
  
  pipe( anReadPipes );
  pipe( anWritePipes );
  
  nPid = fork();
  if ( nPid == 0 ) {
//    char zSrcFileCmd[256]
    char zResCmd[64];
    char zFirstPageCmd[64];
    char zLastPageCmd[64];

    sprintf( zFirstPageCmd, "-dFirstPage=%d\n", nPage );
    sprintf( zLastPageCmd, "-dLastPage=%d\n", nPage );
    sprintf( zResCmd, "-r%d\n", nDPI );

    char* argv[] = {
      "gs",
      "-q",
//      "-dCOLORSCREEN",
//      "-g1024x768",
//      "-dBATCH",
      "-dNOPAUSE",
//      "-dDITHERPPI=3",
//      "-dFirstPage=11",
//      "-dLastPage=11",
//      "-sDEVICE=png16m",
      "-dRedValues=256",
      "-dBlueValues=256",
      "-dGreenValues=256",
      "-sDEVICE=bitrgb",
//      "-sDEVICE=pcx24b",
      "-sOutputFile=-",
//      "-sOutputFile=opt.png1",
      zFirstPageCmd,
      zLastPageCmd,
      zResCmd,
//      "-sOutputFile=/tmp/tst.bin",
//      "/boot/docs/inteldoc/optimation/24281601.pdf",
//      "/boot/docs/inteldoc/241430_4.pdf",
//      "/boot/docs/inteldoc/multi_proc/24201605.pdf",
//      pzSrc,
      NULL
    };
    
//    printf( "Dup file1\n" );
    dup2( anWritePipes[0], 0 );
    fcntl( 0, F_SETFD, 0 );
    
    dup2( anReadPipes[1], 1 );
    fcntl( 1, F_SETFD, 0 );
    for ( int i = 3 ; i < 256 ; ++i ) {
      close( i );
    }
    execvp( "gs", argv );
  } else {
    *pnPid = nPid;
//    int nStatus;
//    waitpid( nPid, &nStatus, 0 );
  }
  close( anWritePipes[0] );
  close( anReadPipes[1] );
  *pnWriteFile = anWritePipes[1];
  return( anReadPipes[0] );
//  return( 0 );
}

void PDFView::ReadBitmap()
{
  int nSrcWidth  = 850 * 2;
  int nSrcHeight = 1100 * 2;

  uint8  anBuffer[nSrcWidth*2*3];
  uint8* pRowBuffer1 = anBuffer;
  uint8* pRowBuffer2 = anBuffer + nSrcWidth * 3;
  static pid_t	 nGSPid = -1;

restart:  
  uint8* pRaster = m_pcBitmap->LockRaster();
  int nPage = m_nPage;
  
  static int nWritePipe;
  static int nFile;
  static FILE* pWritePipe;
  
  if ( nGSPid == -1 ) {
    printf( "Start interpreter\n" );
    nFile = RunGS( "opt.pdf", "opt.png", nPage, 200, &nWritePipe, &nGSPid );

    pWritePipe = fdopen( nWritePipe, "w" );
  }
  
  fprintf( pWritePipe, "/FirstPage {%d} bind def\n", nPage );
  fprintf( pWritePipe, "/LastPage {%d} bind def\n", nPage );
  fprintf( pWritePipe, "(%s) run\n", g_pzFileName );
  fflush( pWritePipe );
//  fclose( pWritePipe );
//  close( nWritePipe );
    
  int nTotLen = 0;
  while( nTotLen < 3*2 ) {
    int nLen =  read( nFile, anBuffer + nTotLen, 3*2 - nTotLen );
    if ( nLen < 0 ) {
      printf( "Read error!\n" );
      break;
    }
    nTotLen += nLen;
  }
  
  for ( int y = 0 ; y < nSrcHeight / 2 ; ++y )
  {
    int nTotLen = 0;

    if ( m_nPage != nPage ) {
      close( nFile );
      close( nWritePipe );
      fclose( pWritePipe );
      
      kill( nGSPid, SIGINT );

      printf( "Waitpid\n" );
      int nStatus;
      waitpid( nGSPid, &nStatus, 0 );

      nGSPid = -1;
      printf( "Waitpid done\n" );
      
      goto restart;
    }
    int nSrcBytesPerLine = nSrcWidth*2*3;
//    int nSrcBytesPerLine = nSrcWidth*3 / 2;
    while( nTotLen < nSrcBytesPerLine ) {
      int nLen =  read( nFile, anBuffer + nTotLen, nSrcBytesPerLine - nTotLen );
      if ( nLen < 0 ) {
	printf( "Read error!\n" );
	break;
      }
      nTotLen += nLen;
    }


    for ( int x = 0 ; x < nSrcWidth / 2 ; ++x ) {
      for ( int i = 2 ; i >= 0 ; --i ) {
	*pRaster++ = (pRowBuffer1[ x*6 + i] + pRowBuffer1[ x*6+3 + i] +
		      pRowBuffer2[ x*6 + i] + pRowBuffer2[ x*6+3 + i]) / 4;
      }
      *pRaster++ = 0;
    }
/*    
    for ( int x = 0 ; x < nSrcWidth / 2 ; ++x ) {
      for ( int i = 0 ; i < 3 ; ++i ) {
	*pRaster++ = anBuffer[ x*3 + 2 - i];
      }
      *pRaster++ = 0;
    }
    */    
    if ( (y % 20) == 0 ) {
      Window* pcWnd = GetWindow();
      assert( pcWnd != NULL );
      pcWnd->Lock();
      Rect cFrame = m_pcBitmap->GetBounds();
      cFrame.top = y - 20;
      cFrame.bottom = y;
      DrawBitmap( m_pcBitmap, cFrame, cFrame );
      pcWnd->Flush();
      pcWnd->Unlock();
    }
  }
  nTotLen = 0;
  while( nTotLen < 3*1 ) {
    int nLen =  read( nFile, anBuffer + nTotLen, 3*3 - nTotLen );
    if ( nLen < 0 ) {
      printf( "Read error!\n" );
      break;
    }
    nTotLen += nLen;
  }
  
//  int nStatus;
//  close( nFile );
//  waitpid( nGSPid, &nStatus, 0 );
}

static int32 render_thread( void* pData )
{
  PDFView* pcView = (PDFView*) pData;

  for ( ;; ) {
    int nPage = pcView->m_nPage;

    pcView->ReadBitmap();

    while( nPage == pcView->m_nPage ) snooze( 100000 );
  }
  
  Window* pcWnd = pcView->GetWindow();

  
  
  assert( pcWnd != NULL );
  pcWnd->Lock();
  pcView->m_hRenderThread = -1;
  pcWnd->Unlock();
  return( 0 );
}

void PDFView::LoadPage()
{
  if ( m_hRenderThread == -1 ) {
    m_hRenderThread = spawn_thread( "render_thread", render_thread, 0, 0, this );
    if ( m_hRenderThread >= 0 ) {
      resume_thread( m_hRenderThread );
    }
  }

//  delete m_pcBitmap;
//  m_pcBitmap = read_png( "opt.png" );
//  m_pcBitmap->Sync();
//  close( nFile );
//  Paint( GetBounds() );
//  Flush();
}

void PDFView::MouseDown( const Point& cPosition, uint32 nButtons )
{
  MakeFocus( true );
}

void PDFView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    switch( pzString[0] )
    {
	case VK_UP_ARROW:
	{
	    float vDist = -GetScrollOffset().y;

	    if ( vDist > 15 ) {
		vDist = 15;
	    }
	
	    ScrollBy( 0, vDist );
	    Flush();
	    break;
	}
	case VK_DOWN_ARROW:
	{
	    float vDist =  GetBounds().bottom - m_pcBitmap->GetBounds().bottom;
	    if ( vDist < -15 ) {
		vDist = -15;
	    }
	    ScrollBy( 0, vDist );
	    Flush();
	    break;
	}
	case VK_LEFT_ARROW:
	{
	    float vDist = -GetScrollOffset().x;

	    if ( vDist > 15 ) {
		vDist = 15;
	    }
	
	    ScrollBy( vDist, 0 );
	    Flush();
	    break;
	}
	case VK_RIGHT_ARROW:
	{
	    float vDist =  GetBounds().right - m_pcBitmap->GetBounds().right;
	    if ( vDist < -15 ) {
		vDist = -15;
	    }
	    ScrollBy( vDist, 0 );
	    Flush();
	    break;
	}
	case VK_PAGE_UP:
	    if ( m_nPage > 1 ) {
		m_nPage--;
		LoadPage();
	    }
	    break;
	case VK_PAGE_DOWN:
	    m_nPage++;
	    LoadPage();
	    break;
	
	default:
	    View::KeyDown( pzString, pzRawString, nQualifiers );
	    break;
    }
}
