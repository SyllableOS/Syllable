//========================================================================
//
// XPDFViewer.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "gmem.h"
#include "gfile.h"
#include "GString.h"
#include "GList.h"
#include "Error.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "ErrorCodes.h"
#include "Outline.h"
#include "UnicodeMap.h"
#include "XPDFApp.h"
#include "XPDFViewer.h"
#include "PSOutputDev.h"
#include "config.h"


//------------------------------------------------------------------------
// GUI includes
//------------------------------------------------------------------------
#ifndef __SYLLABLE__
#include "xpdfIcon.xpm"
#include "leftArrow.xbm"
#include "leftArrowDis.xbm"
#include "dblLeftArrow.xbm"
#include "dblLeftArrowDis.xbm"
#include "rightArrow.xbm"
#include "rightArrowDis.xbm"
#include "dblRightArrow.xbm"
#include "dblRightArrowDis.xbm"
#include "backArrow.xbm"
#include "backArrowDis.xbm"
#include "forwardArrow.xbm"
#include "forwardArrowDis.xbm"
#include "find.xbm"
#include "findDis.xbm"
#include "print.xbm"
#include "printDis.xbm"
#include "about.xbm"
#endif
#include "about-text.h"


//------------------------------------------------------------------------

struct ZoomMenuInfo {
  char *label;
  double zoom;
};

static ZoomMenuInfo zoomMenuInfo[nZoomMenuItems] = {
  { "400%",      400 },
  { "200%",      200 },
  { "150%",      150 },
  { "125%",      125 },
  { "100%",      100 },
  { "50%",        50 },
  { "25%",        25 },
  { "12.5%",      12.5 },
  { "fit page",  zoomPage },
  { "fit width", zoomWidth }
};

#define maxZoomIdx   0
#define defZoomIdx   3
#define minZoomIdx   7
#define zoomPageIdx  8
#define zoomWidthIdx 9

#include "finddialog.cpp"
#include "statusbar.cpp"
#include "resources/xpdf.h"

class XPDFLooper : public os::Looper
{
public:
	XPDFLooper( XPDFViewer* pcViewer ) : os::Looper( "xpdf_looper" )
	{
		m_pcViewer = pcViewer;
	}
	virtual void HandleMessage( os::Message* pcMessage )
	{
		switch( pcMessage->GetCode() )
		{
			case MSG_PREVIOUS_PAGE:
				m_pcViewer->prevPageCbk( m_pcViewer );
			break;
			case MSG_NEXT_PAGE:
				m_pcViewer->nextPageCbk( m_pcViewer );
			break;
			case MSG_SET_ZOOM:
			{
				int32 nZoom;
				if( pcMessage->FindInt32( "zoom_idx", &nZoom ) == 0 )
				{
					m_pcViewer->setZoomIdx( nZoom );
					m_pcViewer->displayPage(m_pcViewer->core->getPageNum(), zoomMenuInfo[nZoom].zoom,
						m_pcViewer->core->getRotate(), gTrue, gFalse);
				}
			}
			break;
			case MSG_PAGE_SELECT:
			{
				int32 nPage;
				if( pcMessage->FindInt32( "page", &nPage ) == 0 )
				{
					m_pcViewer->pageNumCbk( m_pcViewer, nPage );
				}
			}
			break;
			default:
				os::Looper::HandleMessage( pcMessage );
		}
	}
private:
	XPDFViewer* m_pcViewer;
};

//------------------------------------------------------------------------
class XPDFWindow : public os::Window
{
public:
	XPDFWindow( XPDFViewer* pcViewer, const os::Rect& cFrame, const os::String& cName, const os::String& cTitle )
				: os::Window( cFrame, cName, cTitle )
	{
		m_pcViewer = pcViewer;
		m_pcLooper = new XPDFLooper( pcViewer );
		m_pcLooper->Run();

		/* Create menubar */
		os::Menu* pcMenuBar = new os::Menu( GetBounds(), "xpdf_menu_bar", os::ITEMS_IN_ROW );

		/* Application menu */
		os::Menu* pcAppMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_APPLICATION, os::ITEMS_IN_COLUMN );
		pcAppMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new os::Message( MSG_ABOUT ) );
		pcAppMenu->AddItem( new os::MenuSeparator() );
		pcAppMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_QUIT, new os::Message( MSG_QUIT ) );
		pcMenuBar->AddItem( pcAppMenu );

		/* File menu */
		os::Menu* pcFileMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_FILE, os::ITEMS_IN_COLUMN );
		pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_OPEN, new os::Message( MSG_OPEN ) );
		pcMenuBar->AddItem( pcFileMenu );

		/* Page menu */
		os::Menu* pcPageMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_PAGE, os::ITEMS_IN_COLUMN );
		pcPageMenu->AddItem( MSG_MAINWND_MENU_PAGE_PREV, new os::Message( MSG_PREVIOUS_PAGE ) );
		pcPageMenu->AddItem( MSG_MAINWND_MENU_PAGE_NEXT, new os::Message( MSG_NEXT_PAGE ) );
		pcPageMenu->AddItem( new os::MenuSeparator() );
		for( int i = 0; i < nZoomMenuItems; i++ )
		{
			os::Message* pcZoomMsg = new os::Message( MSG_SET_ZOOM );
			pcZoomMsg->AddInt32( "zoom_idx", i );
			if( i == zoomPageIdx )
				pcPageMenu->AddItem( MSG_MAINWND_MENU_ZOOM_PAGE, pcZoomMsg );
			else if( i == zoomWidthIdx )
				pcPageMenu->AddItem( MSG_MAINWND_MENU_ZOOM_WIDTH, pcZoomMsg );
			else
				pcPageMenu->AddItem( zoomMenuInfo[i].label, pcZoomMsg );
		}
		pcMenuBar->AddItem( pcPageMenu );

		/* Edit menu */
		os::Menu* pcEditMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_EDIT, os::ITEMS_IN_COLUMN );
		pcEditMenu->AddItem( MSG_MAINWND_MENU_EDIT_COPY, new os::Message( MSG_COPY ) );
		pcEditMenu->AddItem( new os::MenuSeparator() );
		pcEditMenu->AddItem( MSG_MAINWND_MENU_EDIT_FIND, new os::Message( MSG_SHOW_FIND ) );
		pcMenuBar->AddItem( pcEditMenu );

		os::Rect cMenuFrame = pcMenuBar->GetFrame();
		cMenuFrame.bottom = cMenuFrame.top + pcMenuBar->GetPreferredSize( false ).y - 1;
		pcMenuBar->SetFrame( cMenuFrame );
		pcMenuBar->SetTargetForItems( this );
		AddChild( pcMenuBar );

		/* Toolbar */
		os::ToolBar* pcToolBar = new os::ToolBar( os::Rect( 0, cMenuFrame.bottom + 1, GetBounds().Width(), 38 + cMenuFrame.bottom ), "xpdf_toolbar" );
		pcToolBar->AddChild( new os::ImageView( os::Rect(), "breaker", GetImageFromResource( "breaker.png" ) ), os::ToolBar::TB_FIXED_MINIMUM );
		pcToolBar->AddChild( new os::ImageButton( os::Rect(), "open", MSG_MAINWND_MENU_FILE_OPEN, new os::Message( MSG_OPEN ), GetImageFromResource( "fileopen.png" ), os::ImageButton::IB_TEXT_BOTTOM, true, false, true ), os::ToolBar::TB_FIXED_MINIMUM );
		m_pcPrevButton = pcToolBar->AddChild( new os::ImageButton( os::Rect(), "prev", MSG_MAINWND_MENU_PAGE_PREV, new os::Message( MSG_PREVIOUS_PAGE ), GetImageFromResource( "previous.png" ), os::ImageButton::IB_TEXT_BOTTOM, true, false, true ), os::ToolBar::TB_FIXED_MINIMUM );
		
		static_cast<os::Control*>(m_pcPrevButton->GetView())->SetEnable( false );
		
		m_pcPageSelect = new os::TextView( os::Rect(), "page_select", NULL );
		m_pcPageSelect->SetMultiLine( false );
		m_pcPageSelect->SetNumeric( true );
		m_pcPageSelect->SetReadOnly( false );
		m_pcPageSelect->SetMinPreferredSize( 5, 1 );
		m_pcPageSelect->SetMaxPreferredSize( 5, 1 );
		m_pcPageSelect->SetMessage( new os::Message( MSG_PAGE_SELECT ) );
		m_pcPageSelect->SetTarget( this );
		pcToolBar->AddChild( m_pcPageSelect, os::ToolBar::TB_FIXED_MINIMUM );

		m_pcNextButton = pcToolBar->AddChild( new os::ImageButton( os::Rect(), "prev", MSG_MAINWND_MENU_PAGE_NEXT, new os::Message( MSG_NEXT_PAGE ), GetImageFromResource( "next.png" ), os::ImageButton::IB_TEXT_BOTTOM, true, false, true ), os::ToolBar::TB_FIXED_WIDTH );
		static_cast<os::Control*>(m_pcNextButton->GetView())->SetEnable( false );
		
		AddChild( pcToolBar );

		m_pcStatusBar = new os::StatusBar( GetBounds(), "xpdf_statusbar", 3 );
		m_pcStatusBar->ConfigurePanel( 0, os::StatusBar::CONSTANT, 50 );
		m_pcStatusBar->ConfigurePanel( 1, os::StatusBar::CONSTANT, 50 );
		m_pcStatusBar->ConfigurePanel( 2, os::StatusBar::FILL, 0 );
		os::Rect cStatusFrame = m_pcStatusBar->GetFrame();
		cStatusFrame.top = cStatusFrame.bottom - m_pcStatusBar->GetPreferredSize( false ).y + 1;
		m_pcStatusBar->SetFrame( cStatusFrame );
		AddChild( m_pcStatusBar );

		m_pcForm = new os::View( os::Rect( 0, pcToolBar->GetFrame().bottom + 1, GetBounds().right, cStatusFrame.top - 1 ), "xpdf_form", os::CF_FOLLOW_ALL );
		AddChild( m_pcForm );

		
		/* Set Icon */
		os::Resources cCol( get_image_id() );
		os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
		os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
		SetIcon( pcIcon->LockBitmap() );
		delete( pcIcon );
		delete( pcStream );

		SetSizeLimits( os::Point( 150, 120 ), os::Point( MAX_SIZE, MAX_SIZE ) );

		m_bOpenDialogShown = false;
	}
	~XPDFWindow()
	{
		m_pcLooper->Quit();
	}
	virtual void HandleMessage( os::Message* pcMessage )
	{
		switch( pcMessage->GetCode() )
		{
			case MSG_ABOUT:
			{
				os::Alert* pcAlert = new os::Alert( "About Xpdf " xpdfVersion, aboutWinText, os::Alert::ALERT_INFO, 0, "Ok", NULL );
				pcAlert->Go( new os::Invoker( new os::Message( MSG_REQUESTER_CLOSED ) ) );
			}
			break;
			case MSG_QUIT:
				PostMessage( os::M_QUIT, this );
			break;
			case MSG_PREVIOUS_PAGE:
			case MSG_NEXT_PAGE:
			case MSG_SET_ZOOM:
				m_pcLooper->PostMessage( pcMessage, m_pcLooper );
			break;
			case MSG_OPEN:
				m_pcViewer->mapOpenDialog( gFalse );
			break;
			case MSG_FIND:
				m_pcViewer->doFind( gFalse );
			break;
			case MSG_PAGE_SELECT:
			{
				int32 nEvent;
				if( pcMessage->FindInt32( "events", &nEvent ) == 0 )
				{
					if( nEvent & os::TextView::EI_ENTER_PRESSED )
					{
						pcMessage->AddInt32( "page", m_pcPageSelect->GetValue().AsInt32() );
						m_pcLooper->PostMessage( pcMessage, m_pcLooper );
					}
				}
			}
			break;
			case MSG_COPY:
			{
				if( m_pcViewer->core->getCurrentSelection() ) {
					os::Clipboard cClipboard;

					cClipboard.Lock();
					cClipboard.Clear();
					os::Message *pcData = cClipboard.GetData();

					pcData->AddString( "text/plain", m_pcViewer->core->getCurrentSelection()->getCString() );
					cClipboard.Commit();
					cClipboard.Unlock();
				}
			}
			break;
			case MSG_SHOW_FIND:
				if (m_pcViewer->core->getDoc()) {
					m_pcViewer->findDialog->Lock();
					m_pcViewer->findDialog->ShowDialog();
					m_pcViewer->findDialog->Unlock();
				}
			break;
			case os::M_LOAD_REQUESTED:
			{
				os::String zPath;
				if( pcMessage->FindString( "file/path", &zPath ) == 0 )
				{
					GString* fileNameStr = new GString( zPath.c_str() );
					if (m_pcViewer->openInNewWindow) {
						m_pcViewer->app->open(fileNameStr);
					} else {
						if (m_pcViewer->loadFile(fileNameStr)) {
							m_pcViewer->displayPage(1, m_pcViewer->core->getZoom(),
							m_pcViewer->core->getRotate(), gTrue, gTrue);
						} else {
							os::Alert* pcAlert = new os::Alert( "Xpdf", zPath + " " + MSG_ERRWND_CANTOPEN, os::Alert::ALERT_WARNING, 0, "Ok", NULL );
							pcAlert->Go( new os::Invoker( new os::Message( MSG_REQUESTER_CLOSED ) ) );
						}
					}
					delete( fileNameStr );
				}
				m_bOpenDialogShown = false;
			}
			break;
			case os::M_FILE_REQUESTER_CANCELED:
			{
				m_bOpenDialogShown = false;
			}
			break;
			
			default:
				os::Window::HandleMessage( pcMessage );
		}
	}
	bool OkToQuit()
	{
		os::Application::GetInstance()->PostMessage( os::M_QUIT );
		return( true );
	}
	os::View* GetForm()
	{
		return( m_pcForm );
	}
	bool GetOpenDialogShown()
	{
		return( m_bOpenDialogShown );
	}
	void SetOpenDialogShown()
	{
		m_bOpenDialogShown = true;
	}
	void SetStatusBarText( os::String cLabel, int nPanel )
	{
		m_pcStatusBar->SetText( cLabel, nPanel, 0 );
	}
	void EnableButtons( bool bNext, bool bPrev )
	{
		static_cast<os::Control*>(m_pcNextButton->GetView())->SetEnable( bNext );
		static_cast<os::Control*>(m_pcPrevButton->GetView())->SetEnable( bPrev );
	}
	void SetPage( int nPage )
	{
		m_pcPageSelect->SetValue( nPage );
	}
	os::Image* GetImageFromResource( os::String zResource )
	{
		os::File cSelf( open_image_file( get_image_id() ) );
		os::Resources cCol( &cSelf );		
		os::ResStream *pcStream = cCol.GetResourceStream( zResource );
		os::BitmapImage* pcImage = new os::BitmapImage();
		pcImage->Load( pcStream );
		delete( pcStream );
		return( pcImage );
	}
private:
	XPDFViewer* m_pcViewer;
	os::View* m_pcForm;
	os::StatusBar* m_pcStatusBar;
	bool m_bOpenDialogShown;
	os::LayoutNode* m_pcNextButton;
	os::LayoutNode* m_pcPrevButton;
	os::TextView* m_pcPageSelect;
	XPDFLooper* m_pcLooper;
};


//------------------------------------------------------------------------

XPDFViewer::XPDFViewer(XPDFApp *appA, GString *fileName,
		       int pageA, GString *destName,
		       GString *ownerPassword, GString *userPassword) {
  LinkDest *dest;
  int pg;
  double z;
  GString *dir;

  app = appA;
  win = NULL;
  core = NULL;
  password = NULL;
  ok = gFalse;
#ifndef DISABLE_OUTLINE
  outlineLabels = NULL;
  outlineLabelsLength = outlineLabelsSize = 0;
#endif
 
  // do Motif-specific initialization and create the window;
  // this also creates the core object
  initWindow();
  initAboutDialog();
  initOpenDialog();
  initFindDialog();
  initSaveAsDialog();
  initPrintDialog();
  initPasswordDialog();

  dest = NULL; // make gcc happy
  pg = pageA; // make gcc happy

  if (fileName) {
    if (loadFile(fileName, ownerPassword, userPassword)) {
      getPageAndDest(pageA, destName, &pg, &dest);
#ifndef DISABLE_OUTLINE
      if (!app->getFullScreen() &&
	  core->getDoc()->getOutline()->getItems() &&
	  core->getDoc()->getOutline()->getItems()->getLength() > 0) {
	XtVaSetValues(outlineScroll, XmNwidth, 175, NULL);
      }
#endif
      if (pg > 0) {
	core->resizeToPage(pg);
      }
      dir = makePathAbsolute(grabPath(fileName->getCString()));
      setOpenDialogDir(dir->getCString());
      setSaveAsDialogDir(dir->getCString());
      delete dir;
    } else {
      return;
    }
  }

  // map the window -- we do this after calling resizeToPage to avoid
  // an annoying on-screen resize
  mapWindow();


  // display the first page
  z = app->getFullScreen() ? zoomPage : core->getZoom();
  if (dest) {
    displayDest(dest, z, core->getRotate(), gTrue);
    delete dest;
  } else {
    displayPage(pg, z, core->getRotate(), gTrue, gTrue);
  }


  ok = gTrue;
}

XPDFViewer::~XPDFViewer() {
  delete core;
#ifndef __SYLLABLE__
XmFontListFree(aboutBigFont);
  XmFontListFree(aboutVersionFont);
  XmFontListFree(aboutFixedFont);

  closeWindow();
#endif
#ifndef DISABLE_OUTLINE
  if (outlineLabels) {
    gfree(outlineLabels);
  }
#endif
  if (password) {
    delete password;
  }
}

void XPDFViewer::open(GString *fileName, int pageA, GString *destName) {
  LinkDest *dest;
  int pg;
  double z;

  if (!core->getDoc() || fileName->cmp(core->getDoc()->getFileName())) {
    if (!loadFile(fileName, NULL, NULL)) {
      return;
    }
  }
  getPageAndDest(pageA, destName, &pg, &dest);
  z = app->getFullScreen() ? zoomPage : core->getZoom();
  if (dest) {
    displayDest(dest, z, core->getRotate(), gTrue);
    delete dest;
  } else {
    displayPage(pg, z, core->getRotate(), gTrue, gTrue);
  }
}

void XPDFViewer::clear() {
  char *title;
//  XmString s;

  core->clear();

  // set up title, number-of-pages display
  title = app->getTitle() ? app->getTitle()->getCString()
                          : (char *)xpdfAppName;
  

  win->SetTitle( title );
  static_cast<XPDFWindow*>(win)->SetPage( 0 );
  static_cast<XPDFWindow*>(win)->EnableButtons( false, false );
#ifndef __SYLLABLE__
  XtVaSetValues(win, XmNtitle, title, XmNiconName, title, NULL);
  s = XmStringCreateLocalized("");
  XtVaSetValues(pageNumText, XmNlabelString, s, NULL);
  XmStringFree(s);
  s = XmStringCreateLocalized(" of 0");
  XtVaSetValues(pageCountLabel, XmNlabelString, s, NULL);
  XmStringFree(s);

  // disable buttons
  XtVaSetValues(prevTenPageBtn, XmNsensitive, False, NULL);
  XtVaSetValues(prevPageBtn, XmNsensitive, False, NULL);
  XtVaSetValues(nextTenPageBtn, XmNsensitive, False, NULL);
  XtVaSetValues(nextPageBtn, XmNsensitive, False, NULL);
#endif
  // remove the old outline
#ifndef DISABLE_OUTLINE
  setupOutline();
#endif
}

//------------------------------------------------------------------------
// load / display
//------------------------------------------------------------------------

GBool XPDFViewer::loadFile(GString *fileName, GString *ownerPassword,
			   GString *userPassword) {
  return core->loadFile(fileName, ownerPassword, userPassword) == errNone;
}

void XPDFViewer::reloadFile() {
  int pg;

  if (!core->getDoc()) {
    return;
  }
  pg = core->getPageNum();
  loadFile(core->getDoc()->getFileName());
  if (pg > core->getDoc()->getNumPages()) {
    pg = core->getDoc()->getNumPages();
  }
  displayPage(pg, core->getZoom(), core->getRotate(), gFalse, gFalse);
}

void XPDFViewer::displayPage(int pageA, double zoomA, int rotateA,
			     GBool scrollToTop, GBool addToHist) {
  core->displayPage(pageA, zoomA, rotateA, scrollToTop, addToHist);
}

void XPDFViewer::displayDest(LinkDest *dest, double zoomA, int rotateA,
			     GBool addToHist) {
  core->displayDest(dest, zoomA, rotateA, addToHist);
}

void XPDFViewer::getPageAndDest(int pageA, GString *destName,
				int *pageOut, LinkDest **destOut) {
  Ref pageRef;

  // find the page number for a named destination
  *pageOut = pageA;
  *destOut = NULL;
  if (destName && (*destOut = core->getDoc()->findDest(destName))) {
    if ((*destOut)->isPageRef()) {
      pageRef = (*destOut)->getPageRef();
      *pageOut = core->getDoc()->findPage(pageRef.num, pageRef.gen);
    } else {
      *pageOut = (*destOut)->getPageNum();
    }
  }

  if (*pageOut <= 0) {
    *pageOut = 1;
  }
  if (*pageOut > core->getDoc()->getNumPages()) {
    *pageOut = core->getDoc()->getNumPages();
  }
}

//------------------------------------------------------------------------
// password dialog
//------------------------------------------------------------------------
#ifndef __SYLLABLE__
GString *XPDFViewer::reqPasswordCbk(void *data, GBool again) {
  XPDFViewer *viewer = (XPDFViewer *)data;

  viewer->getPassword(again);
  return viewer->password;
}
#endif
//------------------------------------------------------------------------
// actions
//------------------------------------------------------------------------

void XPDFViewer::actionCbk(void *data, char *action) {
  XPDFViewer *viewer = (XPDFViewer *)data;

  if (!strcmp(action, "Quit")) {
    viewer->app->quit();
  }
}

//------------------------------------------------------------------------
// keyboard/mouse input
//------------------------------------------------------------------------

void XPDFViewer::keyPressCbk(void *data, const char *s, const char* raw,
			     uint32 modifiers) {
  XPDFViewer *viewer = (XPDFViewer *)data;
  int z;

  if (s[0]) {
    switch (s[0]) {
    case 'O':
    case 'o':
      viewer->mapOpenDialog(gFalse);
      break;
    case 'R':
    case 'r':
      viewer->reloadFile();
      break;
    case 'F':
    case 'f':
    case '\006':		// ctrl-F
      if (viewer->core->getDoc()) {
		viewer->findDialog->Lock();
		
		viewer->findDialog->ShowDialog();
		viewer->findDialog->Unlock();
      }
      break;
    case '\007':		// ctrl-G
      if (viewer->core->getDoc()) {
	viewer->doFind(gTrue);
      }
      break;
    case '\020':		// ctrl-P
      if (viewer->core->getDoc()) {
	//XtManageChild(viewer->printDialog);
      }
      break;
    case 'N':
    case 'n':
      viewer->core->gotoNextPage(1, (modifiers & os::QUAL_CTRL/*Mod5Mask*/));
      break;
    case 'P':
    case 'p':
      viewer->core->gotoPrevPage(1, !(modifiers & os::QUAL_CTRL/*Mod5Mask*/), gFalse);
      break;
    case ' ':
      if (viewer->app->getFullScreen()) {
	viewer->core->gotoNextPage(1, gTrue);
      } else {
	viewer->core->scrollPageDown();
      }
      break;
    case '\b':			// bs
    case '\177':		// del
      if (viewer->app->getFullScreen()) {
	viewer->core->gotoPrevPage(1, gTrue, gFalse);
      } else {
	viewer->core->scrollPageUp();
      }
      break;
    case 'v':
      viewer->core->goForward();
      break;
    case 'b':
      viewer->core->goBackward();
      break;
    case 'g':
      if (!viewer->app->getFullScreen()) {
#ifndef __SYLLABLE__
	XmTextFieldSetSelection(
            viewer->pageNumText, 0,
	    strlen(XmTextFieldGetString(viewer->pageNumText)),
	    XtLastTimestampProcessed(viewer->display));
	XmProcessTraversal(viewer->pageNumText, XmTRAVERSE_CURRENT);
#endif
      }
      break;
    case 'h':			// vi-style left
      if (!viewer->app->getFullScreen() && viewer->app->getViKeys()) {
	viewer->core->scrollLeft();
      }
      break;
    case 'l':			// vi-style right
      if (!viewer->app->getFullScreen() && viewer->app->getViKeys()) {
	viewer->core->scrollRight();
      }
      break;
    case 'k':			// vi-style up
      if (!viewer->app->getFullScreen() && viewer->app->getViKeys()) {
	viewer->core->scrollUp();
      }
      break;
    case 'j':			// vi-style down
      if (!viewer->app->getFullScreen() && viewer->app->getViKeys()) {
	viewer->core->scrollDown();
      }
      break;
    case '0':
      if (!viewer->app->getFullScreen() &&
	  viewer->core->getZoom() != defZoom) {
	viewer->setZoomIdx(defZoomIdx);
	viewer->displayPage(viewer->core->getPageNum(), defZoom,
			    viewer->core->getRotate(), gTrue, gFalse);
      }
      break;
    case '+':

      if (!viewer->app->getFullScreen()) {
	z = viewer->getZoomIdx();
	if (z <= minZoomIdx && z > maxZoomIdx) {
	  --z;
	  viewer->setZoomIdx(z);
	  viewer->displayPage(viewer->core->getPageNum(),
			      zoomMenuInfo[z].zoom,
			      viewer->core->getRotate(), gTrue, gFalse);
	}
      }
      break;
    case '-':

      if (!viewer->app->getFullScreen()) {
	z = viewer->getZoomIdx();
	if (z < minZoomIdx && z >= maxZoomIdx) {
	  ++z;
	  viewer->setZoomIdx(z);
	  viewer->displayPage(viewer->core->getPageNum(),
			      zoomMenuInfo[z].zoom,
			      viewer->core->getRotate(), gTrue, gFalse);
	}
      }
      break;
    case 'z':
      if (!viewer->app->getFullScreen() &&
	  viewer->core->getZoom() != zoomPage) {
	viewer->setZoomIdx(zoomPageIdx);
	viewer->displayPage(viewer->core->getPageNum(), zoomPage,
			    viewer->core->getRotate(), gTrue, gFalse);
      }
      break;
    case 'w':
      if (!viewer->app->getFullScreen() &&
	  viewer->core->getZoom() != zoomWidth) {
	viewer->setZoomIdx(zoomWidthIdx);
	viewer->displayPage(viewer->core->getPageNum(), zoomWidth,
			    viewer->core->getRotate(), gTrue, gFalse);
      }
      break;
    case '\014':		// ^L
      viewer->displayPage(viewer->core->getPageNum(), viewer->core->getZoom(),
			  viewer->core->getRotate(), gFalse, gFalse);
      break;
    case '\027':		// ^W
      viewer->app->close(viewer, gFalse);
      break;
    case '?':
      //XtManageChild(viewer->aboutDialog);
      break;
    case 'Q':
    case 'q':
      viewer->app->quit();
      break;
    }
  }
}
#ifndef __SYLLABLE__
void XPDFViewer::mouseCbk(void *data, XEvent *event) {
  XPDFViewer *viewer = (XPDFViewer *)data;

  if (event->type == ButtonPress && event->xbutton.button == 3) {
    XmMenuPosition(viewer->popupMenu, &event->xbutton);
    XtManageChild(viewer->popupMenu);

    // this is magic (taken from DDD) - weird things happen if this
    // call isn't made (this is done in two different places, in hopes
    // of squashing this stupid bug)
    XtUngrabButton(viewer->core->getDrawAreaWidget(), AnyButton, AnyModifier);
  }
}
#endif
//------------------------------------------------------------------------
// GUI code: main window
//------------------------------------------------------------------------

void XPDFViewer::initWindow() {
  //Widget btn, label, lastBtn, zoomWidget;
#ifndef DISABLE_OUTLINE
  Widget clipWin;
#endif
  //Colormap colormap;
  //XColor xcol;
  //Arg args[20];
  int n;
  char *title;
  //XmString s, s2, emptyString;
  int i;

  //display = XtDisplay(app->getAppShell());
  //screenNum = XScreenNumberOfScreen(XtScreen(app->getAppShell()));

  // private colormap
  if (app->getInstallCmap()) {
    printf("app->getInstallCmap()\n" );
	#ifndef __SYLLABLE__
    XtVaGetValues(app->getAppShell(), XmNcolormap, &colormap, NULL);
    // ensure that BlackPixel and WhitePixel are reserved in the
    // new colormap
    xcol.red = xcol.green = xcol.blue = 0;
    XAllocColor(display, colormap, &xcol);
    xcol.red = xcol.green = xcol.blue = 65535;
    XAllocColor(display, colormap, &xcol);
    colormap = XCopyColormapAndFree(display, colormap);
	#endif
  }

  // top-level window
  n = 0;
  title = app->getTitle() ? app->getTitle()->getCString()
                          : (char *)xpdfAppName;

  win = new XPDFWindow( this, os::Rect( 50, 50, 700, 500 ), "xpdf_window", title );
#ifndef __SYLLABLE__

  XtSetArg(args[n], XmNtitle, title); ++n;
  XtSetArg(args[n], XmNiconName, title); ++n;
  XtSetArg(args[n], XmNminWidth, 100); ++n;
  XtSetArg(args[n], XmNminHeight, 100); ++n;
  XtSetArg(args[n], XmNbaseWidth, 0); ++n;
  XtSetArg(args[n], XmNbaseHeight, 0); ++n;
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); ++n;
  if (app->getFullScreen()) {
    XtSetArg(args[n], XmNmwmDecorations, 0); ++n;
    XtSetArg(args[n], XmNgeometry, "+0+0"); ++n;
  } else if (app->getGeometry()) {
    XtSetArg(args[n], XmNgeometry, app->getGeometry()->getCString()); ++n;
  }
  win = XtCreatePopupShell("win", topLevelShellWidgetClass,
			   app->getAppShell(), args, n);
  if (app->getInstallCmap()) {
    XtVaSetValues(win, XmNcolormap, colormap, NULL);
  }
  XmAddWMProtocolCallback(win, XInternAtom(display, "WM_DELETE_WINDOW", False),
			  &closeMsgCbk, this);

  // form

  n = 0;
  form = XmCreateForm(win, "form", args, n);
  XtManageChild(form);
#endif
  form = static_cast<XPDFWindow*>(win)->GetForm();

  // toolbar
#ifndef __SYLLABLE__
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  toolBar = XmCreateForm(form, "toolBar", args, n);
  XtManageChild(toolBar);

  // create an empty string -- this is used for buttons that will get
  // pixmaps later
  emptyString = XmStringCreateLocalized("");

  // page movement buttons
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  backBtn = XmCreatePushButton(toolBar, "back", args, n);
  XtManageChild(backBtn);
  XtAddCallback(backBtn, XmNactivateCallback,
		&backCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, backBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  prevTenPageBtn = XmCreatePushButton(toolBar, "prevTenPage", args, n);
  XtManageChild(prevTenPageBtn);
  XtAddCallback(prevTenPageBtn, XmNactivateCallback,
		&prevTenPageCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, prevTenPageBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  prevPageBtn = XmCreatePushButton(toolBar, "prevPage", args, n);
  XtManageChild(prevPageBtn);
  XtAddCallback(prevPageBtn, XmNactivateCallback,
		&prevPageCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, prevPageBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  nextPageBtn = XmCreatePushButton(toolBar, "nextPage", args, n);
  XtManageChild(nextPageBtn);
  XtAddCallback(nextPageBtn, XmNactivateCallback,
		&nextPageCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, nextPageBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  nextTenPageBtn = XmCreatePushButton(toolBar, "nextTenPage", args, n);
  XtManageChild(nextTenPageBtn);
  XtAddCallback(nextTenPageBtn, XmNactivateCallback,
		&nextTenPageCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, nextTenPageBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  forwardBtn = XmCreatePushButton(toolBar, "forward", args, n);
  XtManageChild(forwardBtn);
  XtAddCallback(forwardBtn, XmNactivateCallback,
		&forwardCbk, (XtPointer)this);

  // page number display
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, forwardBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  s = XmStringCreateLocalized("Page ");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  label = XmCreateLabel(toolBar, "pageLabel", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, label); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 3); ++n;
  XtSetArg(args[n], XmNmarginHeight, 3); ++n;
  XtSetArg(args[n], XmNcolumns, 5); ++n;
  pageNumText = XmCreateTextField(toolBar, "pageNum", args, n);
  XtManageChild(pageNumText);
  XtAddCallback(pageNumText, XmNactivateCallback,
		&pageNumCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, pageNumText); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  s = XmStringCreateLocalized(" of 00000");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); ++n;
  XtSetArg(args[n], XmNrecomputeSize, False); ++n;
  pageCountLabel = XmCreateLabel(toolBar, "pageCountLabel", args, n);
  XmStringFree(s);
  XtManageChild(pageCountLabel);
  s = XmStringCreateLocalized(" of 0");
  XtVaSetValues(pageCountLabel, XmNlabelString, s, NULL);
  XmStringFree(s);

  // zoom menu
#if USE_COMBO_BOX
  XmString st[nZoomMenuItems];
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, pageCountLabel); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 0); ++n;
  XtSetArg(args[n], XmNmarginHeight, 0); ++n;
  XtSetArg(args[n], XmNcomboBoxType, XmDROP_DOWN_COMBO_BOX); ++n;
  XtSetArg(args[n], XmNpositionMode, XmONE_BASED); ++n;
  XtSetArg(args[n], XmNcolumns, 7); ++n;
  for (i = 0; i < nZoomMenuItems; ++i) {
    st[i] = XmStringCreateLocalized(zoomMenuInfo[i].label);
  }
  XtSetArg(args[n], XmNitems, st); ++n;
  XtSetArg(args[n], XmNitemCount, nZoomMenuItems); ++n;
  zoomComboBox = XmCreateComboBox(toolBar, "zoomComboBox", args, n);
  for (i = 0; i < nZoomMenuItems; ++i) {
    XmStringFree(st[i]);
  }
  XtAddCallback(zoomComboBox, XmNselectionCallback,
		&zoomComboBoxCbk, (XtPointer)this);
  XtManageChild(zoomComboBox);
  zoomWidget = zoomComboBox;
#else
  Widget menuPane;
  char buf[16];
  n = 0;
  menuPane = XmCreatePulldownMenu(toolBar, "zoomMenuPane", args, n);
  for (i = 0; i < nZoomMenuItems; ++i) {
    n = 0;
    s = XmStringCreateLocalized(zoomMenuInfo[i].label);
    XtSetArg(args[n], XmNlabelString, s); ++n;
    XtSetArg(args[n], XmNuserData, (XtPointer)i); ++n;
    sprintf(buf, "zoom%d", i);
    btn = XmCreatePushButton(menuPane, buf, args, n);
    XmStringFree(s);
    XtManageChild(btn);
    XtAddCallback(btn, XmNactivateCallback,
		  &zoomMenuCbk, (XtPointer)this);
    zoomMenuBtns[i] = btn;
  }
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, pageCountLabel); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 0); ++n;
  XtSetArg(args[n], XmNmarginHeight, 0); ++n;
  XtSetArg(args[n], XmNsubMenuId, menuPane); ++n;
  zoomMenu = XmCreateOptionMenu(toolBar, "zoomMenu", args, n);
  XtManageChild(zoomMenu);
  zoomWidget = zoomMenu;
#endif

  // find/print/about buttons
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, zoomWidget); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  findBtn = XmCreatePushButton(toolBar, "find", args, n);
  XtManageChild(findBtn);
  XtAddCallback(findBtn, XmNactivateCallback,
		&findCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, findBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  printBtn = XmCreatePushButton(toolBar, "print", args, n);
  XtManageChild(printBtn);
  XtAddCallback(printBtn, XmNactivateCallback,
		&printCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, printBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  XtSetArg(args[n], XmNlabelString, emptyString); ++n;
  aboutBtn = XmCreatePushButton(toolBar, "about", args, n);
  XtManageChild(aboutBtn);
  XtAddCallback(aboutBtn, XmNactivateCallback,
		&aboutCbk, (XtPointer)this);
  lastBtn = aboutBtn;

  // quit button
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNmarginWidth, 6); ++n;
  s = XmStringCreateLocalized("Quit");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  quitBtn = XmCreatePushButton(toolBar, "quit", args, n);
  XmStringFree(s);
  XtManageChild(quitBtn);
  XtAddCallback(quitBtn, XmNactivateCallback,
		&quitCbk, (XtPointer)this);

  // link label
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, lastBtn); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNrightWidget, quitBtn); ++n;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  s = XmStringCreateLocalized("");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNrecomputeSize, True); ++n;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); ++n;
  linkLabel = XmCreateLabel(toolBar, "linkLabel", args, n);
  XmStringFree(s);
  XtManageChild(linkLabel);

#ifndef DISABLE_OUTLINE
  if (app->getFullScreen()) {
#endif
#endif
    // core
    core = new XPDFCore(win, form, app->getPaperRGB(),
			app->getFullScreen(), app->getReverseVideo(),
			app->getInstallCmap(), app->getRGBCubeSize());

    core->setUpdateCbk(&updateCbk, this);
    core->setActionCbk(&actionCbk, this);
    core->setKeyPressCbk(&keyPressCbk, this);
#ifndef __SYLLABLE__
    core->setMouseCbk(&mouseCbk, this);
    core->setReqPasswordCbk(&reqPasswordCbk, this);
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); ++n;
    XtSetArg(args[n], XmNbottomWidget, toolBar); ++n;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
    XtSetValues(core->getWidget(), args, n);
#endif
#ifndef DISABLE_OUTLINE
  } else {

    // paned window
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); ++n;
    XtSetArg(args[n], XmNbottomWidget, toolBar); ++n;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
#if defined(__sgi) && (XmVERSION <= 1)
    panedWin = SgCreateHorzPanedWindow(form, "panedWin", args, n);
#else
    panedWin = XmCreatePanedWindow(form, "panedWin", args, n);
#endif
    XtManageChild(panedWin);

    // scrolled window for outline container
    n = 0;
    XtSetArg(args[n], XmNpositionIndex, 0); ++n;
    XtSetArg(args[n], XmNallowResize, True); ++n;
    XtSetArg(args[n], XmNpaneMinimum, 1); ++n;
    XtSetArg(args[n], XmNpaneMaximum, 10000); ++n;
#if !(defined(__sgi) && (XmVERSION <= 1))
    XtSetArg(args[n], XmNwidth, 1); ++n;
#endif
    XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); ++n;
    outlineScroll = XmCreateScrolledWindow(panedWin, "outlineScroll", args, n);
    XtManageChild(outlineScroll);
    XtVaGetValues(outlineScroll, XmNclipWindow, &clipWin, NULL);
    XtVaSetValues(clipWin, XmNbackground, app->getPaperColor(), NULL);

    // outline tree
    n = 0;
    XtSetArg(args[n], XmNbackground, app->getPaperColor()); ++n;
    outlineTree = XPDFCreateTree(outlineScroll, "outlineTree", args, n);
    XtManageChild(outlineTree);
    XtAddCallback(outlineTree, XPDFNselectionCallback, &outlineSelectCbk,
		  (XtPointer)this);

    // core
    core = new XPDFCore(win, panedWin, app->getPaperRGB(),
			app->getFullScreen(), app->getReverseVideo(),
			app->getInstallCmap(), app->getRGBCubeSize());
    core->setUpdateCbk(&updateCbk, this);
    core->setActionCbk(&actionCbk, this);
    core->setKeyPressCbk(&keyPressCbk, this);
    core->setMouseCbk(&mouseCbk, this);
    core->setReqPasswordCbk(&reqPasswordCbk, this);
    n = 0;
    XtSetArg(args[n], XmNpositionIndex, 1); ++n;
    XtSetArg(args[n], XmNallowResize, True); ++n;
    XtSetArg(args[n], XmNpaneMinimum, 1); ++n;
    XtSetArg(args[n], XmNpaneMaximum, 10000); ++n;
    XtSetValues(core->getWidget(), args, n);
  }
#endif

  // set the zoom menu to match the initial zoom setting
  setZoomVal(core->getZoom());

  // set traversal order
#ifndef __SYLLABLE__
  XtVaSetValues(core->getDrawAreaWidget(),
		XmNnavigationType, XmEXCLUSIVE_TAB_GROUP, NULL);
  XtVaSetValues(backBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(prevTenPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(prevPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(nextPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(nextTenPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(forwardBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(pageNumText, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(zoomWidget, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(findBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(printBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(aboutBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);
  XtVaSetValues(quitBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		NULL);

  // popup menu
  n = 0;
#if XmVersion < 1002
  // older versions of Motif need this, newer ones choke on it,
  // sometimes not displaying the menu at all, maybe depending on the
  // state of the NumLock key (taken from DDD)
  XtSetArg(args[n], XmNmenuPost, "<Btn3Down>"); ++n;
#endif
  popupMenu = XmCreatePopupMenu(core->getDrawAreaWidget(), "popupMenu",
				args, n);
  n = 0;
  s = XmStringCreateLocalized("Open...");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  s2 = XmStringCreateLocalized("O");
  XtSetArg(args[n], XmNacceleratorText, s2); ++n;
  btn = XmCreatePushButton(popupMenu, "open", args, n);
  XmStringFree(s);
  XmStringFree(s2);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&openCbk, (XtPointer)this);
  n = 0;
  s = XmStringCreateLocalized("Open in new window...");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  btn = XmCreatePushButton(popupMenu, "openInNewWindow", args, n);
  XmStringFree(s);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&openInNewWindowCbk, (XtPointer)this);
  n = 0;
  s = XmStringCreateLocalized("Reload");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  s2 = XmStringCreateLocalized("R");
  XtSetArg(args[n], XmNacceleratorText, s2); ++n;
  btn = XmCreatePushButton(popupMenu, "reload", args, n);
  XmStringFree(s);
  XmStringFree(s2);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&reloadCbk, (XtPointer)this);
  n = 0;
  s = XmStringCreateLocalized("Save as...");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  btn = XmCreatePushButton(popupMenu, "saveAs", args, n);
  XmStringFree(s);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&saveAsCbk, (XtPointer)this);
  n = 0;
  btn = XmCreateSeparator(popupMenu, "sep1", args, n);
  XtManageChild(btn);
  n = 0;
  s = XmStringCreateLocalized("Rotate counterclockwise");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  btn = XmCreatePushButton(popupMenu, "rotateCCW", args, n);
  XmStringFree(s);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&rotateCCWCbk, (XtPointer)this);
  n = 0;
  s = XmStringCreateLocalized("Rotate clockwise");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  btn = XmCreatePushButton(popupMenu, "rotateCW", args, n);
  XmStringFree(s);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&rotateCWCbk, (XtPointer)this);
  n = 0;
  btn = XmCreateSeparator(popupMenu, "sep2", args, n);
  XtManageChild(btn);
  n = 0;
  s = XmStringCreateLocalized("Close");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  s2 = XmStringCreateLocalized("Ctrl+W");
  XtSetArg(args[n], XmNacceleratorText, s2); ++n;
  btn = XmCreatePushButton(popupMenu, "close", args, n);
  XmStringFree(s);
  XmStringFree(s2);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&closeCbk, (XtPointer)this);
  n = 0;
  s = XmStringCreateLocalized("Quit");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  s2 = XmStringCreateLocalized("Q");
  XtSetArg(args[n], XmNacceleratorText, s2); ++n;
  btn = XmCreatePushButton(popupMenu, "quit", args, n);
  XmStringFree(s);
  XmStringFree(s2);
  XtManageChild(btn);
  XtAddCallback(btn, XmNactivateCallback,
		&quitCbk, (XtPointer)this);

  // this is magic (taken from DDD) - weird things happen if this
  // call isn't made
  XtUngrabButton(core->getDrawAreaWidget(), AnyButton, AnyModifier);

  XmStringFree(emptyString);
#endif
}

void XPDFViewer::mapWindow() {
#ifdef HAVE_X11_XPM_H
  Pixmap iconPixmap;
#endif
  int depth;
  //Pixel fg, bg, arm;
  // show the window
  win->Show();

  win->MakeFocus();

//  XtPopup(win, XtGrabNone);
  core->takeFocus();



  // create the icon
#ifndef __SYLLABLE__
#ifdef HAVE_X11_XPM_H
  if (XpmCreatePixmapFromData(display, XtWindow(win), xpdfIcon,
			      &iconPixmap, NULL, NULL) == XpmSuccess) {
    XtVaSetValues(win, XmNiconPixmap, iconPixmap, NULL);
  }
#endif

  // set button bitmaps (must be done after the window is mapped)
  XtVaGetValues(backBtn, XmNdepth, &depth,
		XmNforeground, &fg, XmNbackground, &bg,
		XmNarmColor, &arm, NULL);
  XtVaSetValues(backBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)backArrow_bits,
					    backArrow_width,
					    backArrow_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)backArrow_bits,
					    backArrow_width,
					    backArrow_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)backArrowDis_bits,
					    backArrowDis_width,
					    backArrowDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(prevTenPageBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)dblLeftArrow_bits,
					    dblLeftArrow_width,
					    dblLeftArrow_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)dblLeftArrow_bits,
					    dblLeftArrow_width,
					    dblLeftArrow_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)dblLeftArrowDis_bits,
					    dblLeftArrowDis_width,
					    dblLeftArrowDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(prevPageBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)leftArrow_bits,
					    leftArrow_width,
					    leftArrow_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)leftArrow_bits,
					    leftArrow_width,
					    leftArrow_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)leftArrowDis_bits,
					    leftArrowDis_width,
					    leftArrowDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(nextPageBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)rightArrow_bits,
					    rightArrow_width,
					    rightArrow_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)rightArrow_bits,
					    rightArrow_width,
					    rightArrow_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)rightArrowDis_bits,
					    rightArrowDis_width,
					    rightArrowDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(nextTenPageBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)dblRightArrow_bits,
					    dblRightArrow_width,
					    dblRightArrow_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)dblRightArrow_bits,
					    dblRightArrow_width,
					    dblRightArrow_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)dblRightArrowDis_bits,
					    dblRightArrowDis_width,
					    dblRightArrowDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(forwardBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)forwardArrow_bits,
					    forwardArrow_width,
					    forwardArrow_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)forwardArrow_bits,
					    forwardArrow_width,
					    forwardArrow_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)forwardArrowDis_bits,
					    forwardArrowDis_width,
					    forwardArrowDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(findBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)find_bits,
					    find_width,
					    find_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)find_bits,
					    find_width,
					    find_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)findDis_bits,
					    findDis_width,
					    findDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(printBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)print_bits,
					    print_width,
					    print_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)print_bits,
					    print_width,
					    print_height,
					    fg, arm, depth),
		XmNlabelInsensitivePixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)printDis_bits,
					    printDis_width,
					    printDis_height,
					    fg, bg, depth),
		NULL);
  XtVaSetValues(aboutBtn, XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)about_bits,
					    about_width,
					    about_height,
					    fg, bg, depth),
		XmNarmPixmap,
		XCreatePixmapFromBitmapData(display, XtWindow(toolBar),
					    (char *)about_bits,
					    about_width,
					    about_height,
					    fg, arm, depth),
		NULL);
#endif

}

void XPDFViewer::closeWindow() {
//  XtPopdown(win);
//  XtDestroyWidget(win);
  win->Close();
}

int XPDFViewer::getZoomIdx() {
#ifndef __SYLLABLE__
#if USE_COMBO_BOX
  int z;

  XtVaGetValues(zoomComboBox, XmNselectedPosition, &z, NULL);
  return z - 1;
#else
  Widget w;
  int i;

  XtVaGetValues(zoomMenu, XmNmenuHistory, &w, NULL);
  for (i = 0; i < nZoomMenuItems; ++i) {
    if (w == zoomMenuBtns[i]) {
      return i;
    }
  }
  // this should never happen
  return 0;
#endif
#endif
  return( zoom );
  return( 0 );
}

void XPDFViewer::setZoomIdx(int idx) {
#ifndef __SYLLABLE__
#if USE_COMBO_BOX
  XtVaSetValues(zoomComboBox, XmNselectedPosition, idx + 1, NULL);
#else
  XtVaSetValues(zoomMenu, XmNmenuHistory, zoomMenuBtns[idx], NULL);
#endif
#endif
	zoom = idx;
}

void XPDFViewer::setZoomVal(double z) {
#ifndef __SYLLABLE__
#if USE_COMBO_BOX
  char buf[32];
  XmString s;
  int i;

  for (i = 0; i < nZoomMenuItems; ++i) {
    if (z == zoomMenuInfo[i].zoom) {
      XtVaSetValues(zoomComboBox, XmNselectedPosition, i + 1, NULL);
      return;
    }
  }
  sprintf(buf, "%d", (int)z);
  s = XmStringCreateLocalized(buf);
  XtVaSetValues(zoomComboBox, XmNselectedItem, s, NULL);
  XmStringFree(s);
#else
  int i;

  for (i = 0; i < nZoomMenuItems; ++i) {
    if (z == zoomMenuInfo[i].zoom) {
      XtVaSetValues(zoomMenu, XmNmenuHistory, zoomMenuBtns[i], NULL);
      return;
    }
  }
  for (i = maxZoomIdx; i < minZoomIdx; ++i) {
    if (z > zoomMenuInfo[i].zoom) {
      break;
    }
  }
  XtVaSetValues(zoomMenu, XmNmenuHistory, zoomMenuBtns[i], NULL);
#endif
#endif
  int i;
   for (i = 0; i < nZoomMenuItems; ++i) {
    if (z == zoomMenuInfo[i].zoom) {
	  zoom = i;
      break;
    }
  }
}

void XPDFViewer::prevPageCbk( void* ptr ) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->core->gotoPrevPage(1, gTrue, gFalse);
  viewer->core->takeFocus();
}

void XPDFViewer::prevTenPageCbk( void* ptr ) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->core->gotoPrevPage(10, gTrue, gFalse);
  viewer->core->takeFocus();
}

void XPDFViewer::nextPageCbk( void* ptr ) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->core->gotoNextPage(1, gTrue);
  viewer->core->takeFocus();
}

void XPDFViewer::nextTenPageCbk( void* ptr ) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->core->gotoNextPage(10, gTrue);
  viewer->core->takeFocus();
}
#ifndef __SYLLABLE__
void XPDFViewer::backCbk(Widget widget, XtPointer ptr,
			 XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->core->goBackward();
  viewer->core->takeFocus();
}

void XPDFViewer::forwardCbk(Widget widget, XtPointer ptr,
			    XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->core->goForward();
  viewer->core->takeFocus();
}

#if USE_COMBO_BOX

void XPDFViewer::zoomComboBoxCbk(Widget widget, XtPointer ptr,
				 XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmComboBoxCallbackStruct *data = (XmComboBoxCallbackStruct *)callData;
  double z;
  char *s;
  XmStringContext context;
  XmStringCharSet charSet;
  XmStringDirection dir;
  Boolean sep;

  z = viewer->core->getZoom();
  if (data->item_position == 0) {
    XmStringInitContext(&context, data->item_or_text);
    if (XmStringGetNextSegment(context, &s, &charSet, &dir, &sep)) {
      z = atof(s);
      if (z <= 1) {
	z = defZoom;
      }
      XtFree(charSet);
      XtFree(s);
    }
    XmStringFreeContext(context);
  } else {
    z = zoomMenuInfo[data->item_position - 1].zoom;
  }
  // only redraw if this was triggered by an event; otherwise
  // the caller is responsible for doing the redraw
  if (z != viewer->core->getZoom() && data->event) {
    viewer->displayPage(viewer->core->getPageNum(), z,
			viewer->core->getRotate(), gTrue, gFalse);
  }
  viewer->core->takeFocus();
}

#else // USE_COMBO_BOX

void XPDFViewer::zoomMenuCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmPushButtonCallbackStruct *data = (XmPushButtonCallbackStruct *)callData;
  XtPointer userData;
  double z;

  XtVaGetValues(widget, XmNuserData, &userData, NULL);
  z = zoomMenuInfo[(int)userData].zoom;
  // only redraw if this was triggered by an event; otherwise
  // the caller is responsible for doing the redraw
  if (z != viewer->core->getZoom() && data->event) {
    viewer->displayPage(viewer->core->getPageNum(), z,
			viewer->core->getRotate(), gTrue, gFalse);
  }
  viewer->core->takeFocus();
}

#endif // USE_COMBO_BOX

void XPDFViewer::findCbk(Widget widget, XtPointer ptr,
			 XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  if (!viewer->core->getDoc()) {
    return;
  }
  XtManageChild(viewer->findDialog);
}

void XPDFViewer::printCbk(Widget widget, XtPointer ptr,
			  XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  if (!viewer->core->getDoc()) {
    return;
  }
  XtManageChild(viewer->printDialog);
}

void XPDFViewer::aboutCbk(Widget widget, XtPointer ptr,
			  XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  XtManageChild(viewer->aboutDialog);
}

void XPDFViewer::quitCbk(Widget widget, XtPointer ptr,
			 XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->app->quit();
}

void XPDFViewer::openCbk(Widget widget, XtPointer ptr,
			 XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->mapOpenDialog(gFalse);
}

void XPDFViewer::openInNewWindowCbk(Widget widget, XtPointer ptr,
				    XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->mapOpenDialog(gTrue);
}

void XPDFViewer::reloadCbk(Widget widget, XtPointer ptr,
			 XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->reloadFile();
}

void XPDFViewer::saveAsCbk(Widget widget, XtPointer ptr,
			   XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  if (!viewer->core->getDoc()) {
    return;
  }
  viewer->mapSaveAsDialog();
}

void XPDFViewer::rotateCCWCbk(Widget widget, XtPointer ptr,
			      XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  int r;

  r = viewer->core->getRotate();
  r = (r == 0) ? 270 : r - 90;
  viewer->displayPage(viewer->core->getPageNum(), viewer->core->getZoom(),
		      r, gTrue, gFalse);
}

void XPDFViewer::rotateCWCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  int r;

  r = viewer->core->getRotate();
  r = (r == 270) ? 0 : r + 90;
  viewer->displayPage(viewer->core->getPageNum(), viewer->core->getZoom(),
		      r, gTrue, gFalse);
}

void XPDFViewer::closeCbk(Widget widget, XtPointer ptr,
			  XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->app->close(viewer, gFalse);
}

void XPDFViewer::closeMsgCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->app->close(viewer, gTrue);
}
#endif
void XPDFViewer::pageNumCbk(void* ptr, int pg) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
#ifndef __SYLLABLE__
  char *s, *p;

  int pg;
  char buf[20];

  if (!viewer->core->getDoc()) {
    goto err;
  }
  s = XmTextFieldGetString(viewer->pageNumText);
  for (p = s; *p; ++p) {
    if (!isdigit(*p)) {
      goto err;
    }
  }
  pg = atoi(s);
#endif
  if (pg < 1 || pg > viewer->core->getDoc()->getNumPages()) {
    goto err;
  }
  viewer->displayPage(pg, viewer->core->getZoom(),
		      viewer->core->getRotate(), gFalse, gTrue);
  viewer->core->takeFocus();
  return;

 err:
#ifndef __SYLLABLE__
  XBell(viewer->display, 0);
  sprintf(buf, "%d", viewer->core->getPageNum());
  XmTextFieldSetString(viewer->pageNumText, buf);
#endif
	static_cast<XPDFWindow*>(viewer->win)->SetPage( viewer->core->getPageNum() );
}

void XPDFViewer::updateCbk(void *data, GString *fileName,
			   int pageNum, int numPages, char *linkString) {
  XPDFViewer *viewer = (XPDFViewer *)data;
  GString *title;
  char buf[20];
//  XmString s;

  if (fileName) {
    //if (!(title = viewer->app->getTitle())) {
      title = (new GString(xpdfAppName))->append(" : ")->append(fileName);
    //}
	viewer->win->SetTitle( title->getCString() );
    //if (!viewer->app->getTitle()) {
      delete title;
    //}
#ifndef DISABLE_OUTLINE
    if (!viewer->app->getFullScreen()) {
      viewer->setupOutline();
    }
#endif
    viewer->setupPrintDialog();
  }

  if (pageNum >= 0) {
	sprintf(buf, "%s %d", MSG_STATUS_PAGE.c_str(), pageNum);
	static_cast<XPDFWindow*>(viewer->win)->SetStatusBarText( buf, 0 );
	static_cast<XPDFWindow*>(viewer->win)->SetStatusBarText( "", 2 );
	static_cast<XPDFWindow*>(viewer->win)->EnableButtons( pageNum < viewer->core->getDoc()->getNumPages(),
															pageNum > 1 );
#ifndef __SYLLABLE__
    s = XmStringCreateLocalized("");
    XtVaSetValues(viewer->linkLabel, XmNlabelString, s, NULL);
    XmStringFree(s);
    sprintf(buf, "%d", pageNum);
    XmTextFieldSetString(viewer->pageNumText, buf);
    XtVaSetValues(viewer->prevTenPageBtn, XmNsensitive,
		  pageNum > 1, NULL);
    XtVaSetValues(viewer->prevPageBtn, XmNsensitive,
		  pageNum > 1, NULL);
    XtVaSetValues(viewer->nextTenPageBtn, XmNsensitive,
		  pageNum < viewer->core->getDoc()->getNumPages(), NULL);
    XtVaSetValues(viewer->nextPageBtn, XmNsensitive,
		  pageNum < viewer->core->getDoc()->getNumPages(), NULL);
    XtVaSetValues(viewer->backBtn, XmNsensitive,
		  viewer->core->canGoBack(), NULL);
    XtVaSetValues(viewer->forwardBtn, XmNsensitive,
		  viewer->core->canGoForward(), NULL);
#endif
	static_cast<XPDFWindow*>(viewer->win)->SetPage( pageNum );
  }

  if (numPages >= 0) {
    sprintf(buf, "%s %d", MSG_STATUS_PAGE_OF.c_str(), numPages);
	static_cast<XPDFWindow*>(viewer->win)->SetStatusBarText( buf, 1 );
#ifndef __SYLLABLE__
	s = XmStringCreateLocalized(buf);
    XtVaSetValues(viewer->pageCountLabel, XmNlabelString, s, NULL);
    XmStringFree(s);
#endif
  }

  if (linkString) {
	static_cast<XPDFWindow*>(viewer->win)->SetStatusBarText( linkString, 2 );
#ifndef __SYLLABLE__
    s = XmStringCreateLocalized(linkString);
    XtVaSetValues(viewer->linkLabel, XmNlabelString, s, NULL);
    XmStringFree(s);
#endif
  }

}


//------------------------------------------------------------------------
// GUI code: outline
//------------------------------------------------------------------------

#ifndef DISABLE_OUTLINE

void XPDFViewer::setupOutline() {
  GList *items;
  UnicodeMap *uMap;
  GString *enc;
  int i;

  // unmanage and destroy the old labels
  if (outlineLabels) {
    XtUnmanageChildren(outlineLabels, outlineLabelsLength);
    for (i = 0; i < outlineLabelsLength; ++i) {
      XtDestroyWidget(outlineLabels[i]);
    }
    gfree(outlineLabels);
    outlineLabels = NULL;
    outlineLabelsLength = outlineLabelsSize = 0;
  }

  if (core->getDoc()) {

    // create the new labels
    items = core->getDoc()->getOutline()->getItems();
    if (items && items->getLength() > 0) {
      enc = new GString("Latin1");
      uMap = globalParams->getUnicodeMap(enc);
      delete enc;
      setupOutlineItems(items, NULL, uMap);
      uMap->decRefCnt();
    }

    // manage the new labels
    XtManageChildren(outlineLabels, outlineLabelsLength);
  }
}

void XPDFViewer::setupOutlineItems(GList *items, Widget parent,
				   UnicodeMap *uMap) {
  OutlineItem *item;
  GList *kids;
  Widget label;
  Arg args[20];
  GString *title;
  char buf[8];
  XmString s;
  int i, j, n;

  for (i = 0; i < items->getLength(); ++i) {
    item = (OutlineItem *)items->get(i);
    title = new GString();
    for (j = 0; j < item->getTitleLength(); ++j) {
      n = uMap->mapUnicode(item->getTitle()[j], buf, sizeof(buf));
      title->append(buf, n);
    }
    n = 0;
    XtSetArg(args[n], XPDFNentryPosition, i); ++n;
    if (parent) {
      XtSetArg(args[n], XPDFNentryParent, parent); ++n;
    }
    XtSetArg(args[n], XPDFNentryExpanded, item->isOpen()); ++n;
    s = XmStringCreateLocalized(title->getCString());
    delete title;
    XtSetArg(args[n], XmNlabelString, s); ++n;
    XtSetArg(args[n], XmNuserData, item); ++n;
    XtSetArg(args[n], XmNmarginWidth, 0); ++n;
    XtSetArg(args[n], XmNmarginHeight, 2); ++n;
    XtSetArg(args[n], XmNshadowThickness, 0); ++n;
    XtSetArg(args[n], XmNforeground,
	     app->getReverseVideo() ? WhitePixel(display, screenNum)
	                            : BlackPixel(display, screenNum)); ++n;
    XtSetArg(args[n], XmNbackground, app->getPaperColor()); ++n;
    label = XmCreateLabelGadget(outlineTree, "label", args, n);
    XmStringFree(s);
    if (outlineLabelsLength == outlineLabelsSize) {
      outlineLabelsSize += 64;
      outlineLabels = (Widget *)grealloc(outlineLabels,
					 outlineLabelsSize * sizeof(Widget *));
    }
    outlineLabels[outlineLabelsLength++] = label;
    item->open();
    if ((kids = item->getKids())) {
      setupOutlineItems(kids, label, uMap);
    }
  }
}

void XPDFViewer::outlineSelectCbk(Widget widget, XtPointer ptr,
				  XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XPDFTreeSelectCallbackStruct *data =
      (XPDFTreeSelectCallbackStruct *)callData;
  OutlineItem *item;

  XtVaGetValues(data->selectedItem, XmNuserData, &item, NULL);
  if (item) {
    viewer->core->doAction(item->getAction());
  }
  viewer->core->takeFocus();
}

#endif // !DISABLE_OUTLINE

//------------------------------------------------------------------------
// GUI code: "about" dialog
//------------------------------------------------------------------------

void XPDFViewer::initAboutDialog() {
#ifndef __SYLLABLE__
  Widget scrolledWin, col, label, sep, closeBtn;
  Arg args[20];
  int n, i;
  XmString s;
  char buf[20];

  //----- dialog
  n = 0;
  s = XmStringCreateLocalized(xpdfAppName ": About");
  XtSetArg(args[n], XmNdialogTitle, s); ++n;
  XtSetArg(args[n], XmNwidth, 450); ++n;
  XtSetArg(args[n], XmNheight, 300); ++n;
  aboutDialog = XmCreateFormDialog(win, "aboutDialog", args, n);
  XmStringFree(s);

  //----- "close" button
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  closeBtn = XmCreatePushButton(aboutDialog, "Close", args, n);
  XtManageChild(closeBtn);
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, closeBtn); ++n;
  XtSetArg(args[n], XmNcancelButton, closeBtn); ++n;
  XtSetValues(aboutDialog, args, n);

  //----- scrolled window and RowColumn
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNbottomWidget, closeBtn); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); ++n;
  scrolledWin = XmCreateScrolledWindow(aboutDialog, "scrolledWin", args, n);
  XtManageChild(scrolledWin);
  n = 0;
  XtSetArg(args[n], XmNorientation, XmVERTICAL); ++n;
  XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); ++n;
  col = XmCreateRowColumn(scrolledWin, "col", args, n);
  XtManageChild(col);

  //----- fonts
  aboutBigFont =
    createFontList("-*-times-bold-i-normal--20-*-*-*-*-*-iso8859-1");
  aboutVersionFont =
    createFontList("-*-times-medium-r-normal--16-*-*-*-*-*-iso8859-1");
  aboutFixedFont =
    createFontList("-*-courier-medium-r-normal--12-*-*-*-*-*-iso8859-1");

  //----- heading
  n = 0;
  s = XmStringCreateLocalized("Xpdf");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNfontList, aboutBigFont); ++n;
  label = XmCreateLabel(col, "h0", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  s = XmStringCreateLocalized("Version " xpdfVersion);
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNfontList, aboutVersionFont); ++n;
  label = XmCreateLabel(col, "h1", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  s = XmStringCreateLocalized(xpdfCopyright);
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNfontList, aboutVersionFont); ++n;
  label = XmCreateLabel(col, "h2", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  s = XmStringCreateLocalized(" ");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNfontList, aboutVersionFont); ++n;
  label = XmCreateLabel(col, "h3", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
  sep = XmCreateSeparator(col, "sep", args, n);
  XtManageChild(sep);
  n = 0;
  s = XmStringCreateLocalized(" ");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  XtSetArg(args[n], XmNfontList, aboutVersionFont); ++n;
  label = XmCreateLabel(col, "h4", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;

  //----- text
  for (i = 0; aboutWinText[i]; ++i) {
    n = 0;
    s = XmStringCreateLocalized(aboutWinText[i]);
    XtSetArg(args[n], XmNlabelString, s); ++n;
    XtSetArg(args[n], XmNfontList, aboutFixedFont); ++n;
    sprintf(buf, "t%d", i);
    label = XmCreateLabel(col, buf, args, n);
    XtManageChild(label);
    XmStringFree(s);
  }
#endif
}

//------------------------------------------------------------------------
// GUI code: "open" dialog
//------------------------------------------------------------------------

void XPDFViewer::initOpenDialog() {
	openDialog = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( win ), "", os::FileRequester::NODE_FILE,
										false );
	openDialog->Start();
#ifndef __SYLLABLE__
  Arg args[20];
  int n;
  XmString s1, s2, s3;

  n = 0;
  s1 = XmStringCreateLocalized("Open");
  XtSetArg(args[n], XmNokLabelString, s1); ++n;
  s2 = XmStringCreateLocalized("*.[Pp][Dd][Ff]");
  XtSetArg(args[n], XmNpattern, s2); ++n;
  s3 = XmStringCreateLocalized(xpdfAppName ": Open");
  XtSetArg(args[n], XmNdialogTitle, s3); ++n;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ++n;
  XtSetArg(args[n], XmNautoUnmanage, True); ++n;
  openDialog = XmCreateFileSelectionDialog(win, "openDialog", args, n);
  XmStringFree(s1);
  XmStringFree(s2);
  XmStringFree(s3);
  XtUnmanageChild(XmFileSelectionBoxGetChild(openDialog,
					     XmDIALOG_HELP_BUTTON));
  XtAddCallback(openDialog, XmNokCallback,
		&openOkCbk, (XtPointer)this);
#endif
}

void XPDFViewer::setOpenDialogDir(char *dir) {
	openDialog->SetPath( dir );
#ifndef __SYLLABLE__
  XmString s;

  s = XmStringCreateLocalized(dir);
  XtVaSetValues(openDialog, XmNdirectory, s, NULL);
  XmStringFree(s);
#endif
}

void XPDFViewer::mapOpenDialog(GBool openInNewWindowA) {
	openDialog->Lock();
	if( !static_cast<XPDFWindow*>(win)->GetOpenDialogShown() )
	{
		openDialog->Show();
		static_cast<XPDFWindow*>(win)->SetOpenDialogShown();
	}
	openDialog->MakeFocus();
	openDialog->Unlock();
	openInNewWindow = openInNewWindowA;
#ifndef __SYLLABLE__
  openInNewWindow = openInNewWindowA;
  XmFileSelectionDoSearch(openDialog, NULL);
  XtManageChild(openDialog);
#endif
}
#ifndef __SYLLABLE__
void XPDFViewer::openOkCbk(Widget widget, XtPointer ptr,
			   XtPointer callData) {

  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmFileSelectionBoxCallbackStruct *data =
    (XmFileSelectionBoxCallbackStruct *)callData;
  char *fileName;
  XmStringContext context;
  XmStringCharSet charSet;
  XmStringDirection dir;
  Boolean sep;
  GString *fileNameStr;

  XmStringInitContext(&context, data->value);
  if (XmStringGetNextSegment(context, &fileName, &charSet, &dir, &sep)) {
    fileNameStr = new GString(fileName);
    if (viewer->openInNewWindow) {
      viewer->app->open(fileNameStr);
    } else {
      if (viewer->loadFile(fileNameStr)) {
	viewer->displayPage(1, viewer->core->getZoom(),
			    viewer->core->getRotate(), gTrue, gTrue);
      }
    }
    delete fileNameStr;
    XtFree(charSet);
    XtFree(fileName);
  }
  XmStringFreeContext(context);
}
#endif
//------------------------------------------------------------------------
// GUI code: "find" dialog
//------------------------------------------------------------------------

void XPDFViewer::initFindDialog() {
	findDialog = new os::FindDialog(os::Rect(200,200,450,310),win);
	findDialog->Start();
#ifndef __SYLLABLE__
  Widget form1, label, okBtn, closeBtn;
  Arg args[20];
  int n;
  XmString s;

  //----- dialog
  n = 0;
  s = XmStringCreateLocalized(xpdfAppName ": Find");
  XtSetArg(args[n], XmNdialogTitle, s); ++n;
  XtSetArg(args[n], XmNnavigationType, XmNONE); ++n;
  XtSetArg(args[n], XmNautoUnmanage, False); ++n;
  findDialog = XmCreateFormDialog(win, "findDialog", args, n);
  XmStringFree(s);

  //----- "find" and "close" buttons
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  XtSetArg(args[n], XmNnavigationType, XmEXCLUSIVE_TAB_GROUP); ++n;
  okBtn = XmCreatePushButton(findDialog, "Find", args, n);
  XtManageChild(okBtn);
  XtAddCallback(okBtn, XmNactivateCallback,
		&findFindCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  XtSetArg(args[n], XmNnavigationType, XmEXCLUSIVE_TAB_GROUP); ++n;
  closeBtn = XmCreatePushButton(findDialog, "Close", args, n);
  XtManageChild(closeBtn);
  XtAddCallback(closeBtn, XmNactivateCallback,
		&findCloseCbk, (XtPointer)this);

  //----- search string entry
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNtopOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNbottomWidget, okBtn); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 2); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 2); ++n;
  form1 = XmCreateForm(findDialog, "form", args, n);
  XtManageChild(form1);
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  s = XmStringCreateLocalized("Find text: ");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  label = XmCreateLabel(form1, "label", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNleftWidget, label); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  findText = XmCreateTextField(form1, "text", args, n);
  XtManageChild(findText);
#ifdef LESSTIF_VERSION
  XtAddCallback(findText, XmNactivateCallback,
		&findFindCbk, (XtPointer)this);
#endif

  //----- dialog parameters
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, okBtn); ++n;
  XtSetArg(args[n], XmNcancelButton, closeBtn); ++n;
#if XmVersion > 1001
  XtSetArg(args[n], XmNinitialFocus, findText); ++n;
#endif
  XtSetValues(findDialog, args, n);
#endif
}
#ifndef __SYLLABLE__
void XPDFViewer::findFindCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->doFind(gFalse);
}
#endif
void XPDFViewer::doFind(GBool next) {
	GString* text = new GString( findDialog->GetFindText().c_str() );
	core->find( text->getCString(), next );
	delete( text );
#ifndef __SYLLABLE__
  if (XtWindow(findDialog)) {
    XDefineCursor(display, XtWindow(findDialog), core->getBusyCursor());
  }
  core->find(XmTextFieldGetString(findText), next);
  if (XtWindow(findDialog)) {
    XUndefineCursor(display, XtWindow(findDialog));
  }
#endif
}
#ifndef __SYLLABLE__
void XPDFViewer::findCloseCbk(Widget widget, XtPointer ptr,
			      XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  XtUnmanageChild(viewer->findDialog);
}
#endif
//------------------------------------------------------------------------
// GUI code: "save as" dialog
//------------------------------------------------------------------------

void XPDFViewer::initSaveAsDialog() {
#ifndef __SYLLABLE__
  Arg args[20];
  int n;
  XmString s1, s2, s3;

  n = 0;
  s1 = XmStringCreateLocalized("Save");
  XtSetArg(args[n], XmNokLabelString, s1); ++n;
  s2 = XmStringCreateLocalized("*.[Pp][Dd][Ff]");
  XtSetArg(args[n], XmNpattern, s2); ++n;
  s3 = XmStringCreateLocalized(xpdfAppName ": Save as");
  XtSetArg(args[n], XmNdialogTitle, s3); ++n;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ++n;
  XtSetArg(args[n], XmNautoUnmanage, True); ++n;
  saveAsDialog = XmCreateFileSelectionDialog(win, "saveAsDialog", args, n);
  XmStringFree(s1);
  XmStringFree(s2);
  XmStringFree(s3);
  XtUnmanageChild(XmFileSelectionBoxGetChild(saveAsDialog,
					     XmDIALOG_HELP_BUTTON));
  XtAddCallback(saveAsDialog, XmNokCallback,
		&saveAsOkCbk, (XtPointer)this);
#endif
}

void XPDFViewer::setSaveAsDialogDir(char *dir) {
#ifndef __SYLLABLE__
  XmString s;

  s = XmStringCreateLocalized(dir);
  XtVaSetValues(saveAsDialog, XmNdirectory, s, NULL);
  XmStringFree(s);
#endif
}

void XPDFViewer::mapSaveAsDialog() {
#ifndef __SYLLABLE__
  XmFileSelectionDoSearch(saveAsDialog, NULL);
  XtManageChild(saveAsDialog);
#endif
}
#ifndef __SYLLABLE__
void XPDFViewer::saveAsOkCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmFileSelectionBoxCallbackStruct *data =
    (XmFileSelectionBoxCallbackStruct *)callData;
  char *fileName;
  GString *fileNameStr;
  XmStringContext context;
  XmStringCharSet charSet;
  XmStringDirection dir;
  Boolean sep;

  XmStringInitContext(&context, data->value);
  if (XmStringGetNextSegment(context, &fileName, &charSet, &dir, &sep)) {
    fileNameStr = new GString(fileName);
    viewer->core->getDoc()->saveAs(fileNameStr);
    delete fileNameStr;
    XtFree(charSet);
    XtFree(fileName);
  }
  XmStringFreeContext(context);
}
#endif
//------------------------------------------------------------------------
// GUI code: "print" dialog
//------------------------------------------------------------------------

void XPDFViewer::initPrintDialog() {
#ifndef __SYLLABLE__
  Widget sep1, sep2, row, label1, label2, okBtn, cancelBtn;
  Arg args[20];
  int n;
  XmString s;
  GString *psFileName;

  //----- dialog
  n = 0;
  s = XmStringCreateLocalized(xpdfAppName ": Print");
  XtSetArg(args[n], XmNdialogTitle, s); ++n;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ++n;
  printDialog = XmCreateFormDialog(win, "printDialog", args, n);
  XmStringFree(s);

  //----- "print with command"
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNtopOffset, 4); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); ++n;
  XtSetArg(args[n], XmNset, XmSET); ++n;
  s = XmStringCreateLocalized("Print with command:");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  printWithCmdBtn = XmCreateToggleButton(printDialog, "printWithCmd", args, n);
  XmStringFree(s);
  XtManageChild(printWithCmdBtn);
  XtAddCallback(printWithCmdBtn, XmNvalueChangedCallback,
		&printWithCmdBtnCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, printWithCmdBtn); ++n;
  XtSetArg(args[n], XmNtopOffset, 2); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 16); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 4); ++n;
  XtSetArg(args[n], XmNcolumns, 40); ++n;
  printCmdText = XmCreateTextField(printDialog, "printCmd", args, n);
  XtManageChild(printCmdText);

  //----- "print with command"
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, printCmdText); ++n;
  XtSetArg(args[n], XmNtopOffset, 4); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); ++n;
  XtSetArg(args[n], XmNset, XmUNSET); ++n;
  s = XmStringCreateLocalized("Print to file:");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  printToFileBtn = XmCreateToggleButton(printDialog, "printToFile", args, n);
  XmStringFree(s);
  XtManageChild(printToFileBtn);
  XtAddCallback(printToFileBtn, XmNvalueChangedCallback,
		&printToFileBtnCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, printToFileBtn); ++n;
  XtSetArg(args[n], XmNtopOffset, 2); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 16); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 4); ++n;
  XtSetArg(args[n], XmNcolumns, 40); ++n;
  XtSetArg(args[n], XmNsensitive, False); ++n;
  printFileText = XmCreateTextField(printDialog, "printFile", args, n);
  XtManageChild(printFileText);

  //----- separator
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, printFileText); ++n;
  XtSetArg(args[n], XmNtopOffset, 8); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 8); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 8); ++n;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
  sep1 = XmCreateSeparator(printDialog, "sep1", args, n);
  XtManageChild(sep1);

  //----- page range
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, sep1); ++n;
  XtSetArg(args[n], XmNtopOffset, 8); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
  XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); ++n;
  row = XmCreateRowColumn(printDialog, "row", args, n);
  XtManageChild(row);
  n = 0;
  s = XmStringCreateLocalized("Pages:");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  label1 = XmCreateLabel(row, "label1", args, n);
  XmStringFree(s);
  XtManageChild(label1);
  n = 0;
  XtSetArg(args[n], XmNcolumns, 5); ++n;
  printFirstPage = XmCreateTextField(row, "printFirstPage", args, n);
  XtManageChild(printFirstPage);
  n = 0;
  s = XmStringCreateLocalized("to");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  label2 = XmCreateLabel(row, "label2", args, n);
  XmStringFree(s);
  XtManageChild(label2);
  n = 0;
  XtSetArg(args[n], XmNcolumns, 5); ++n;
  printLastPage = XmCreateTextField(row, "printLastPage", args, n);
  XtManageChild(printLastPage);

  //----- separator
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, row); ++n;
  XtSetArg(args[n], XmNtopOffset, 8); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 8); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 8); ++n;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
  sep2 = XmCreateSeparator(printDialog, "sep2", args, n);
  XtManageChild(sep2);

  //----- "print" and "cancel" buttons
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, sep2); ++n;
  XtSetArg(args[n], XmNtopOffset, 8); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  okBtn = XmCreatePushButton(printDialog, "Print", args, n);
  XtManageChild(okBtn);
  XtAddCallback(okBtn, XmNactivateCallback,
		&printPrintCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, sep2); ++n;
  XtSetArg(args[n], XmNtopOffset, 8); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  cancelBtn = XmCreatePushButton(printDialog, "Cancel", args, n);
  XtManageChild(cancelBtn);
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, okBtn); ++n;
  XtSetArg(args[n], XmNcancelButton, cancelBtn); ++n;
  XtSetValues(printDialog, args, n);

  //----- initial values
  if ((psFileName = globalParams->getPSFile())) {
    if (psFileName->getChar(0) == '|') {
      XmTextFieldSetString(printCmdText,
			   psFileName->getCString() + 1);
    } else {
      XmTextFieldSetString(printFileText, psFileName->getCString());
    }
    delete psFileName;
  }
#endif
}

void XPDFViewer::setupPrintDialog() {
#ifndef __SYLLABLE__
  PDFDoc *doc;
  char buf[20];
  GString *pdfFileName, *psFileName, *psFileName2;
  char *p;

  doc = core->getDoc();
  psFileName = globalParams->getPSFile();
  if (!psFileName || psFileName->getChar(0) == '|') {
    pdfFileName = doc->getFileName();
    p = pdfFileName->getCString() + pdfFileName->getLength() - 4;
    if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")) {
      psFileName2 = new GString(pdfFileName->getCString(),
				pdfFileName->getLength() - 4);
    } else {
      psFileName2 = pdfFileName->copy();
    }
    psFileName2->append(".ps");
    XmTextFieldSetString(printFileText, psFileName2->getCString());
    delete psFileName2;
  }
  if (psFileName) {
    delete psFileName;
  }

  sprintf(buf, "%d", doc->getNumPages());
  XmTextFieldSetString(printFirstPage, "1");
  XmTextFieldSetString(printLastPage, buf);
#endif
}
#ifndef __SYLLABLE__
void XPDFViewer::printWithCmdBtnCbk(Widget widget, XtPointer ptr,
				    XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmToggleButtonCallbackStruct *data =
      (XmToggleButtonCallbackStruct *)callData;

  if (data->set != XmSET) {
    XmToggleButtonSetState(viewer->printWithCmdBtn, True, False);
  }
  XmToggleButtonSetState(viewer->printToFileBtn, False, False);
  XtVaSetValues(viewer->printCmdText, XmNsensitive, True, NULL);
  XtVaSetValues(viewer->printFileText, XmNsensitive, False, NULL);
}

void XPDFViewer::printToFileBtnCbk(Widget widget, XtPointer ptr,
				   XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmToggleButtonCallbackStruct *data =
      (XmToggleButtonCallbackStruct *)callData;

  if (data->set != XmSET) {
    XmToggleButtonSetState(viewer->printToFileBtn, True, False);
  }
  XmToggleButtonSetState(viewer->printWithCmdBtn, False, False);
  XtVaSetValues(viewer->printFileText, XmNsensitive, True, NULL);
  XtVaSetValues(viewer->printCmdText, XmNsensitive, False, NULL);
}

void XPDFViewer::printPrintCbk(Widget widget, XtPointer ptr,
			       XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  unsigned char withCmd;
  GString *psFileName;
  int firstPage, lastPage;
  PDFDoc *doc;
  PSOutputDev *psOut;

  doc = viewer->core->getDoc();
  if (!doc->okToPrint()) {
    error(-1, "Printing this document is not allowed.");
    return;
  }

  viewer->core->setBusyCursor(gTrue);

  XtVaGetValues(viewer->printWithCmdBtn, XmNset, &withCmd, NULL);
  if (withCmd) {
    psFileName = new GString(XmTextFieldGetString(viewer->printCmdText));
    psFileName->insert(0, '|');
  } else {
    psFileName = new GString(XmTextFieldGetString(viewer->printFileText));
  }

  firstPage = atoi(XmTextFieldGetString(viewer->printFirstPage));
  lastPage = atoi(XmTextFieldGetString(viewer->printLastPage));
  if (firstPage < 1) {
    firstPage = 1;
  } else if (firstPage > doc->getNumPages()) {
    firstPage = doc->getNumPages();
  }
  if (lastPage < firstPage) {
    lastPage = firstPage;
  } else if (lastPage > doc->getNumPages()) {
    lastPage = doc->getNumPages();
  }

  psOut = new PSOutputDev(psFileName->getCString(), doc->getXRef(),
			  doc->getCatalog(), firstPage, lastPage,
			  psModePS);
  if (psOut->isOk()) {
    doc->displayPages(psOut, firstPage, lastPage, 72, 72,
		      0, globalParams->getPSCrop(), gFalse);
  }
  delete psOut;
  delete psFileName;

  viewer->core->setBusyCursor(gFalse);
}
#endif
//------------------------------------------------------------------------
// GUI code: password dialog
//------------------------------------------------------------------------

void XPDFViewer::initPasswordDialog() {
#ifndef __SYLLABLE__
  Widget row, label, okBtn, cancelBtn;
  Arg args[20];
  int n;
  XmString s;

  //----- dialog
  n = 0;
  s = XmStringCreateLocalized(xpdfAppName ": Password");
  XtSetArg(args[n], XmNdialogTitle, s); ++n;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ++n;
  passwordDialog = XmCreateFormDialog(win, "passwordDialog", args, n);
  XmStringFree(s);

  //----- message
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNtopOffset, 4); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  s = XmStringCreateLocalized(" ");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  passwordMsg = XmCreateLabel(passwordDialog, "msg", args, n);
  XmStringFree(s);
  XtManageChild(passwordMsg);

  //----- label and password entry
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, passwordMsg); ++n;
  XtSetArg(args[n], XmNtopOffset, 4); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
  XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); ++n;
  row = XmCreateRowColumn(passwordDialog, "row", args, n);
  XtManageChild(row);
  n = 0;
  s = XmStringCreateLocalized("Password: ");
  XtSetArg(args[n], XmNlabelString, s); ++n;
  label = XmCreateLabel(row, "label", args, n);
  XmStringFree(s);
  XtManageChild(label);
  n = 0;
  XtSetArg(args[n], XmNcolumns, 16); ++n;
  passwordText = XmCreateTextField(row, "text", args, n);
  XtManageChild(passwordText);
  XtAddCallback(passwordText, XmNmodifyVerifyCallback,
		&passwordTextVerifyCbk, this);

  //----- "Ok" and "Cancel" buttons
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, row); ++n;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNleftOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  XtSetArg(args[n], XmNnavigationType, XmEXCLUSIVE_TAB_GROUP); ++n;
  okBtn = XmCreatePushButton(passwordDialog, "Ok", args, n);
  XtManageChild(okBtn);
  XtAddCallback(okBtn, XmNactivateCallback,
		&passwordOkCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
  XtSetArg(args[n], XmNtopWidget, row); ++n;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNrightOffset, 4); ++n;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
  XtSetArg(args[n], XmNbottomOffset, 4); ++n;
  XtSetArg(args[n], XmNnavigationType, XmEXCLUSIVE_TAB_GROUP); ++n;
  cancelBtn = XmCreatePushButton(passwordDialog, "Cancel", args, n);
  XtManageChild(cancelBtn);
  XtAddCallback(cancelBtn, XmNactivateCallback,
		&passwordCancelCbk, (XtPointer)this);
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, okBtn); ++n;
  XtSetArg(args[n], XmNcancelButton, cancelBtn); ++n;
#if XmVersion > 1001
  XtSetArg(args[n], XmNinitialFocus, passwordText); ++n;
#endif
  XtSetValues(passwordDialog, args, n);
#endif
}
#ifndef __SYLLABLE__
void XPDFViewer::passwordTextVerifyCbk(Widget widget, XtPointer ptr,
				       XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;
  XmTextVerifyCallbackStruct *data =
      (XmTextVerifyCallbackStruct *)callData;
  int i, n;

  i = (int)data->startPos;
  n = (int)data->endPos - i;
  if (i > viewer->password->getLength()) {
    i = viewer->password->getLength();
  }
  if (i + n > viewer->password->getLength()) {
    n = viewer->password->getLength() - i;
  }
  viewer->password->del(i, n);
  viewer->password->insert(i, data->text->ptr, data->text->length);

  for (i = 0; i < data->text->length; ++i) {
    data->text->ptr[i] = '*';
  }
  data->doit = True;
}

void XPDFViewer::passwordOkCbk(Widget widget, XtPointer ptr,
			       XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->passwordDone = 1;
}

void XPDFViewer::passwordCancelCbk(Widget widget, XtPointer ptr,
				   XtPointer callData) {
  XPDFViewer *viewer = (XPDFViewer *)ptr;

  viewer->passwordDone = -1;
}
#endif
void XPDFViewer::getPassword(GBool again) {
#ifndef __SYLLABLE__
  XmString s;
  XEvent event;

  if (password) {
    delete password;
  }
  password = new GString();

  XmTextFieldSetString(passwordText, "");
  s = XmStringCreateLocalized(
	  again ? (char *)"Incorrect password.  Please try again."
	        : (char *)"This document requires a password.");
  XtVaSetValues(passwordMsg, XmNlabelString, s, NULL);
  XmStringFree(s);
  XtManageChild(passwordDialog);
  passwordDone = 0;
  do {
    XtAppNextEvent(app->getAppContext(), &event);
    XtDispatchEvent(&event);
  } while (!passwordDone);

  if (passwordDone < 0) {
    delete password;
    password = NULL;
  }
#endif
}

//------------------------------------------------------------------------
// Motif support
//------------------------------------------------------------------------
#ifndef __SYLLABLE__
XmFontList XPDFViewer::createFontList(char *xlfd) {
  XmFontList fontList;

#if XmVersion <= 1001

  XFontStruct *font;
  String params;
  Cardinal nParams;

  font = XLoadQueryFont(display, xlfd);
  if (font) {
    fontList = XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
  } else {
    params = (String)xlfd;
    nParams = 1;
    XtAppWarningMsg(app->getAppContext(),
		    "noSuchFont", "CvtStringToXmFontList",
		    "XtToolkitError", "No such font: %s",
		    &params, &nParams);
    fontList = NULL;
  }

#else

  XmFontListEntry entry;

  entry = XmFontListEntryLoad(display, xlfd,
			      XmFONT_IS_FONT, XmFONTLIST_DEFAULT_TAG);
  fontList = XmFontListAppendEntry(NULL, entry);
  XmFontListEntryFree(&entry);

#endif

  return fontList;
}
#endif










