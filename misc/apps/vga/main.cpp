#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/wait.h>
#include <atheos/time.h>
#include <gui/window.h>
#include <gui/bitmap.h>

#include <util/application.h>
#include <util/message.h>

#include "pngloader.h"
#include "pdfview.h"

#include <string>
/*
extern "C" {
#include <jpeglib.h>
}
*/
char* g_pzFileName;

using namespace os;


class PDFWindow : public Window
{
public:
  PDFWindow( const Rect& cFrame, const char* pzPath );
  
private:
  PDFView*	m_pcPDFView;
  std::string	m_cPath;
};

PDFWindow::PDFWindow( const Rect& cFrame, const char* pzPath ) : Window( cFrame, "main_window", "PDF View" )
{
  m_pcPDFView = new PDFView( GetBounds() );
  AddChild( m_pcPDFView );
  m_pcPDFView->MakeFocus();
  MakeFocus();
  Show();
}

int main( int argc, char** argv )
{
  if ( argc != 2 ) {
    printf( "Usage: vga file.pdf" );
    exit( 1 );
  }
  g_pzFileName = argv[1];
  Application* pcApp = new Application( "application/x-vnd.KHS-VGA" );

  new PDFWindow( Rect( 20, 20, 700, 600 ), g_pzFileName );

  pcApp->Run();
  return( 0 );
#if 0  
//  int nFile = RunGS( "opt.pdf", "opt.png", 1, 200 );
  Bitmap* pcBitmap;
#if 0  
  pcBitmap = new Bitmap( 1024, 768, CS_RGB32 );
  uint8* pDstRaster = pcBitmap->LockRaster();

//  int nFile = open( "/tmp/tst.bin", O_RDONLY );

  assert( nFile >= 0 );

  uint8 anBuffer[1024 * 3];

  for ( int i = 0 ; i < 768 ; ++i ) {
//    printf( "Read line %d\n", i );

    int nTotLen = 0;
    
    while( nTotLen < 1024 * 3 ) {
      int nLen =  read( nFile, anBuffer + nTotLen, 1024 * 3 - nTotLen );

      if ( nLen < 0 ) {
	printf( "Read error!\n" );
	break;
      }
      nTotLen += nLen;
    }

    for ( int x = 0 ; x < 1024 ; ++x ) {
      *pDstRaster++ = anBuffer[x*3+2];
      *pDstRaster++ = anBuffer[x*3+1];
      *pDstRaster++ = anBuffer[x*3+0];
      *pDstRaster++ = 0;
    }
  }
  close( nFile );

#else
//  pcBitmap = read_png( "opt.png" );
#endif
//  pcBitmap->Sync();

  pcBitmap = new Bitmap( 850, 1100, CS_RGB32 );

  Window* pcWnd = new Window( Rect( 20, 20, 400, 600 ), "_gs_view","AtheOS PDF viewer" );

  View* pcView = new PDFView( pcWnd->GetBounds(), pcBitmap );
  pcWnd->Lock();
  pcWnd->AddChild( pcView );
  pcWnd->Unlock();
  
  pcApp->Run();
#endif	
  return 0;
}
