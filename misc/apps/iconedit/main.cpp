#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <gui/view.h>
#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/menu.h>
#include <gui/filerequester.h>

#include <util/application.h>
#include <util/message.h>

#include "editview.h"
#include "controlpanel.h"
#include "controlid.h"

using namespace os;

class IconView;

class MainWindow : public Window
{
public:
  MainWindow( const Rect& cFrame );
  virtual void	HandleMessage( Message* pcMessage );
  void DispatchMessage( Message* pcMsg, Handler* pcHandler );
  virtual bool OkToQuit( void );
private:
  EditView* 	 m_pcEditView;
  ControlPanel*  m_pcControlPanel;
  Menu*	    	 m_pcMenuBar;
  FileRequester* m_pcLoadRequester;
  FileRequester* m_pcSaveRequester;
};

void MainWindow::DispatchMessage( Message* pcMsg, Handler* pcHandler )
{
  Window::DispatchMessage( pcMsg, pcHandler );
}

class IconApp : public Application
{
public:
  IconApp();
  ~IconApp();
private:
  MainWindow* m_pcMainWindow;
};


IconApp::IconApp() : Application( "application/x-vnd.KHS-IconEdit" )
{
  m_pcMainWindow = new MainWindow( Rect( 200, 100, 600, 400 ) );
  m_pcMainWindow->MakeFocus();
  m_pcMainWindow->Show();
}

IconApp::~IconApp()
{
  m_pcMainWindow->Close();
}

MainWindow::MainWindow( const Rect& cFrame ) : Window( cFrame, "main_window", "Icon Edit V0.1" )
{
  Rect cMenuFrame = cFrame.Bounds();
  cMenuFrame.bottom = 15;

  m_pcLoadRequester = NULL;
  m_pcSaveRequester = NULL;
  
  m_pcMenuBar  = new Menu( cMenuFrame, "main_menu", ITEMS_IN_ROW,
			   CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP, WID_FULL_UPDATE_ON_H_RESIZE );

  Menu* pcFileMenu = new Menu( Rect( 0, 0, 1, 1 ), "File", ITEMS_IN_COLUMN );
  pcFileMenu->AddItem( "Load", new Message( ID_FILE_LOAD ) );
  pcFileMenu->AddItem( "Save", new Message( ID_FILE_SAVE ) );
  pcFileMenu->AddItem( "Save as...", new Message( ID_FILE_SAVE_AS ) );

  Menu* pcViewMenu = new Menu( Rect( 0, 0, 1, 1 ), "View", ITEMS_IN_COLUMN );
  pcViewMenu->AddItem( "Grid", NULL );

  Menu* pcImageMenu = new Menu( Rect( 0, 0, 1, 1 ), "Image", ITEMS_IN_COLUMN );
  pcImageMenu->AddItem( "Generate 16x16", new Message( ID_IMAGE_GEN_16 ) );
  pcImageMenu->AddItem( "Generate 32x32", new Message( ID_IMAGE_GEN_32 ) );
  
  Menu* pcHelpMenu = new Menu( Rect( 0, 0, 1, 1 ), "Help", ITEMS_IN_COLUMN );
  pcHelpMenu->AddItem( "About...", NULL );
  
  m_pcMenuBar->AddItem( pcFileMenu );
  m_pcMenuBar->AddItem( pcViewMenu );
  m_pcMenuBar->AddItem( pcImageMenu );
  m_pcMenuBar->AddItem( pcHelpMenu );
  AddChild( m_pcMenuBar );

  m_pcMenuBar->SetTargetForItems( this );
  
  cMenuFrame.bottom = m_pcMenuBar->GetPreferredSize( false ).y;
  m_pcMenuBar->SetFrame( cMenuFrame );

  
  m_pcEditView     = new EditView( Rect( 0, cMenuFrame.bottom + 1, 327, cMenuFrame.bottom + 1 + 256 ) );
  m_pcControlPanel = new ControlPanel( Rect( 328, cMenuFrame.bottom + 1, cFrame.right, cMenuFrame.bottom + 1 + 256 ) );
  AddChild( m_pcEditView );
  AddChild( m_pcControlPanel );
}

void MainWindow::HandleMessage( Message* pcMessage )
{
  switch( pcMessage->GetCode() )
  {
    case ID_FILE_LOAD:
    {
      if ( m_pcLoadRequester == NULL ) {
	m_pcLoadRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "/atheos/" );
      }
      m_pcLoadRequester->Show();
      m_pcLoadRequester->MakeFocus();
      break;
    }
    case ID_FILE_SAVE_AS:
    {
      if ( m_pcSaveRequester == NULL ) {
	m_pcSaveRequester = new FileRequester( FileRequester::SAVE_REQ, new Messenger( this ), "/atheos/" );
      }
      m_pcSaveRequester->Show();
      m_pcSaveRequester->MakeFocus();
      break;
    }
    case M_LOAD_REQUESTED:
    {
      const char* pzPath;
      if ( pcMessage->FindString( "file/path", &pzPath ) == 0 ) {
	m_pcEditView->Load( pzPath );
      }
      break;
    }
    case M_SAVE_REQUESTED:
    {
      const char* pzPath;
      if ( pcMessage->FindString( "file/path", &pzPath ) == 0 ) {
	m_pcEditView->Save( pzPath );
      }
      break;
    }
    
    case ID_IMAGE_GEN_16:
      m_pcEditView->GenerateSmall();
      break;
    case ID_IMAGE_GEN_32:
      m_pcEditView->GenerateLarge();
      break;
    case ID_FILTER_CHANGED:
      m_pcEditView->SetFilterType( m_pcControlPanel->GetFilterType() );
      break;
    default:
      Window::HandleMessage( pcMessage );
      break;
  }
}

bool MainWindow::OkToQuit( void )
{
  Application::GetInstance()->PostMessage( M_QUIT );
  return( false );
}


bool load_win_bmp( const char* pzPath, Bitmap** ppcImg );

int main()
{
  IconApp* pcApp = new IconApp();
/*
  Bitmap* pcBitmap = NULL;
  load_win_bmp( "gfx/radio.bmp", &pcBitmap );

  if ( pcBitmap != NULL ) {
    uint8* pRaster = pcBitmap->LockRaster();
    int nSize = pcBitmap->GetBounds().Height() * pcBitmap->GetBytesPerRow();

    for ( int i = 0 ; i < nSize ; ++i ) {
      if ( i != 0 && (i % 16) == 0 ) {
	printf( "\n" );
      }
      printf( "0x%02x,", *pRaster++ );
    }
    fflush( stdout );
  }
  */  
//  load_win_icon( "icons/12-Be98 Application.ico", NULL, NULL );
//  load_win_icon( "nps83b4.ico", NULL, NULL );
//  load_win_icon( "testicon.ico", NULL, NULL );
  pcApp->Run();
  
  return( 0 );
}
