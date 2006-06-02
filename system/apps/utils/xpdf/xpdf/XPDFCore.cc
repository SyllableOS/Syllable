//========================================================================
//
// XPDFCore.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "gmem.h"
#include "GString.h"
#include "GList.h"
#include "Error.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "ErrorCodes.h"
#include "GfxState.h"
#include "PSOutputDev.h"
#include "TextOutputDev.h"
#include "SplashPattern.h"
#include "XSplashOutputDev.h"
#include "XPDFCore.h"


//------------------------------------------------------------------------

#define highlightNone     0
#define highlightNormal   1
#define highlightSelected 2

//------------------------------------------------------------------------

GString *XPDFCore::currentSelection = NULL;
XPDFCore *XPDFCore::currentSelectionOwner = NULL;


class XPDFView : public os::View
{
public:
	XPDFView( XPDFCore* pcCore, const os::Rect& cFrame, const os::String& cTitle,
	  uint32 nResizeMask ) : os::View( cFrame, cTitle, nResizeMask, os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE )
	{
		m_pcCore = pcCore;
		m_pcImage = new os::Bitmap( (int)cFrame.Width() + 1, (int)cFrame.Height() + 1, os::CS_RGB32 );
	}
	~XPDFView()
	{
		delete( m_pcImage );
	}
	void Paint( const os::Rect& cUpdateRect )
	{
		os::Rect cUpdate = cUpdateRect;
		m_pcCore->redrawRectangle( (int)cUpdate.left, (int)cUpdate.top, (int)cUpdate.Width() + 1, (int)cUpdate.Height() + 1 );
	}
	void FrameSized( const os::Point& cDelta )
	{
		delete( m_pcImage );
		m_pcImage = new os::Bitmap( (int)GetBounds().Width() + 1, (int)GetBounds().Height() + 1, os::CS_RGB32 );
		m_pcCore->resizeCbk( m_pcCore );
		os::View::FrameSized( cDelta );
	}
	void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
	{
		m_pcCore->inputCbk( m_pcCore, os::M_MOUSE_MOVED, cNewPos- os::Point( m_pcCore->getScrollX(), m_pcCore->getScrollY() ), nButtons );
		os::View::MouseMove( cNewPos , nCode, nButtons, pcData );
	}
    void MouseDown( const os::Point& cPosition, uint32 nButtons )
	{
		MakeFocus();
		m_pcCore->inputCbk( m_pcCore, os::M_MOUSE_DOWN, cPosition - os::Point( m_pcCore->getScrollX(), m_pcCore->getScrollY() ), nButtons );
		os::View::MouseDown( cPosition, nButtons );
	}
    void MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData )
	{
		m_pcCore->inputCbk( m_pcCore, os::M_MOUSE_UP, cPosition - os::Point( m_pcCore->getScrollX(), m_pcCore->getScrollY() ), nButtons );
		os::View::MouseUp( cPosition, nButtons, pcData );
	}
	void WheelMoved( const os::Point& cDelta )
	{
		if( cDelta.y > 0 )
			m_pcCore->scrollDown( 1 );
		else
			m_pcCore->scrollUp( 1 );
		os::View::WheelMoved( cDelta );
	}
	void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
	{
		m_pcCore->keyPress( pzString, pzRawString, nQualifiers );
		os::View::KeyDown( pzString, pzRawString, nQualifiers );
	}
	void HandleMessage( os::Message* pcMessage )
	{
		switch( pcMessage->GetCode() )
		{
			case MSG_SCROLLBAR_H:
			{
				os::Variant cVar;
				if( pcMessage->FindVariant( "value", &cVar ) == 0 )
				{
					m_pcCore->scrollTo( cVar.AsInt32(), m_pcCore->getScrollY() );
				}
			}
			break;
			case MSG_SCROLLBAR_V:
			{
				os::Variant cVar;
				if( pcMessage->FindVariant( "value", &cVar ) == 0 )
				{
					m_pcCore->scrollTo( m_pcCore->getScrollX(), cVar.AsInt32() );
				}
			}
			break;
			default:
				os::View::HandleMessage( pcMessage );
		}
	}
	os::Bitmap* GetBitmap()
	{
		return( m_pcImage );
	}
	void Redraw()
	{
		m_pcCore->outputDevRedrawCbk( m_pcCore );
	}
private:
	os::Bitmap* m_pcImage;
	XPDFCore* m_pcCore;
};

//------------------------------------------------------------------------
// XPDFCore
//------------------------------------------------------------------------

XPDFCore::XPDFCore(os::Window* shellA, os::View* parentWidgetA,
		   SplashRGB8 paperColorA, GBool fullScreenA,
		   GBool reverseVideo, GBool installCmap, int rgbCubeSize) {
  GString *initialZoom;
  SplashColor paperColor2;
  int i;

  shell = shellA;
  parentWidget = parentWidgetA;
 
  paperColor = paperColorA;
  fullScreen = fullScreenA;

  scrolledWin = NULL;
  hScrollBar = NULL;
  vScrollBar = NULL;
  //drawAreaFrame = NULL;
  //drawArea = NULL;
  out = NULL;

  doc = NULL;
  page = 0;
  rotate = 0;

  // get the initial zoom value
  initialZoom = globalParams->getInitialZoom();
  if (!initialZoom->cmp("page")) {
    zoom = zoomPage;
  } else if (!initialZoom->cmp("width")) {
    zoom = zoomWidth;
  } else {
    zoom = atoi(initialZoom->getCString());
    if (zoom <= 0) {
      zoom = defZoom;
    }
  }
  delete initialZoom;

  scrollX = 0;
  scrollY = 0;
  linkAction = NULL;
  selectXMin = selectXMax = 0;
  selectYMin = selectYMax = 0;
  dragging = gFalse;
  lastDragLeft = lastDragTop = gTrue;

  panning = gFalse;


  // no history yet
  historyCur = xpdfHistorySize - 1;
  historyBLen = historyFLen = 0;
  for (i = 0; i < xpdfHistorySize; ++i) {
    history[i].fileName = NULL;
  }

  // optional features default to on
  hyperlinksEnabled = gTrue;
  selectEnabled = gTrue;

  // do X-specific initialization and create the widgets
  initWindow();

  // create the OutputDev
  paperColor2.rgb8 = paperColor;
  out = new XSplashOutputDev(shellA,
			     reverseVideo, paperColor2,
			     installCmap, rgbCubeSize, gTrue,
			     &outputDevRedrawCbk, this);
  out->startDoc(NULL);
}

XPDFCore::~XPDFCore() {
  int i;
	
  if (out) {
    delete out;
  }

  if (doc) {
    delete doc;
  }
  if (currentSelectionOwner == this && currentSelection) {
    delete currentSelection;
    currentSelection = NULL;
    currentSelectionOwner = NULL;
  }
  for (i = 0; i < xpdfHistorySize; ++i) {
    if (history[i].fileName) {
      delete history[i].fileName;
    }
  }
  
#ifndef __SYLLABLE__
  if (busyCursor) {
    XFreeCursor(display, busyCursor);
  }
  if (linkCursor) {
    XFreeCursor(display, linkCursor);
  }
  if (selectCursor) {
    XFreeCursor(display, selectCursor);
  }
#endif
}

//------------------------------------------------------------------------
// loadFile / displayPage / displayDest
//------------------------------------------------------------------------

int XPDFCore::loadFile(GString *fileName, GString *ownerPassword,
		       GString *userPassword) {
  PDFDoc *newDoc;
  GString *password;
  GBool again;
  int err;

  // busy cursor
  //setCursor(busyCursor);

  // open the PDF file
  newDoc = new PDFDoc(fileName->copy(), ownerPassword, userPassword);
  if (!newDoc->isOk()) {
    err = newDoc->getErrorCode();
    delete newDoc;
    if (err != errEncrypted /*|| !reqPasswordCbk*/) {
      //setCursor(None);
      return err;
    }

    // try requesting a password
    again = ownerPassword != NULL || userPassword != NULL;
	
    while (1) {
#ifndef __SYLLABLE__
      if (!(password = (*reqPasswordCbk)(reqPasswordCbkData, again))) {
	setCursor(None);
	return errEncrypted;
      }
#endif
      newDoc = new PDFDoc(fileName->copy(), password, password);
      if (newDoc->isOk()) {
	break;
      }
      err = newDoc->getErrorCode();
      delete newDoc;
      if (err != errEncrypted) {
	//setCursor(None);
	return err;
      }
      again = gTrue;
    }
  }

  // replace old document
  if (doc) {
    delete doc;
  }
  doc = newDoc;
  if (out) {
    out->startDoc(doc->getXRef());
  }

  // nothing displayed yet
  page = -99;

  // save the modification time
  modTime = getModTime(doc->getFileName()->getCString());

  // update the parent window

  if (updateCbk) {
    (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		 doc->getNumPages(), NULL);
  }

  static_cast<XPDFView*>(scrolledWin)->Redraw();
  // back to regular cursor
  //setCursor(None);

  return errNone;
}

int XPDFCore::loadFile(BaseStream *stream, GString *ownerPassword,
		       GString *userPassword) {
  PDFDoc *newDoc;
  GString *password;
  GBool again;
  int err;

  // busy cursor
  //setCursor(busyCursor);

  // open the PDF file
  newDoc = new PDFDoc(stream, ownerPassword, userPassword);
  if (!newDoc->isOk()) {
    err = newDoc->getErrorCode();
    delete newDoc;
    if (err != errEncrypted /*|| !reqPasswordCbk*/) {
     //setCursor(None);
      return err;
    }

    // try requesting a password
    again = ownerPassword != NULL || userPassword != NULL;
    while (1) {
#ifndef __SYLLABLE__
      if (!(password = (*reqPasswordCbk)(reqPasswordCbkData, again))) {
	setCursor(None);
	return errEncrypted;
      }
#endif
      newDoc = new PDFDoc(stream, password, password);
      if (newDoc->isOk()) {
	break;
      }
      err = newDoc->getErrorCode();
      delete newDoc;
      if (err != errEncrypted) {
	//setCursor(None);
	return err;
      }
      again = gTrue;
    }
  }

  // replace old document
  if (doc) {
    delete doc;
  }
  doc = newDoc;
  if (out) {
    out->startDoc(doc->getXRef());
  }

  // nothing displayed yet
  page = -99;

  // save the modification time
  modTime = getModTime(doc->getFileName()->getCString());


  // update the parent window
  if (updateCbk) {
    (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		 doc->getNumPages(), NULL);
  }

  static_cast<XPDFView*>(scrolledWin)->Redraw();
  // back to regular cursor
  //setCursor(None);

  return errNone;
}

void XPDFCore::resizeToPage(int pg) {
  float width, height;
  double width1, height1;
  float topW, topH, topBorder, daW, daH;
  float displayW, displayH;

  os::Desktop cDesktop;

  displayW = cDesktop.GetResolution().x;
  displayH = cDesktop.GetResolution().y;

  if (fullScreen) {
    width = displayW;
    height = displayH;
  } else {
    if (pg < 0 || pg > doc->getNumPages()) {
      width1 = 612;
      height1 = 792;
    } else if (doc->getPageRotate(pg) == 90 ||
	       doc->getPageRotate(pg) == 270) {
      width1 = doc->getPageHeight(pg);
      height1 = doc->getPageWidth(pg);
    } else {
      width1 = doc->getPageWidth(pg);
      height1 = doc->getPageHeight(pg);
    }
    if (zoom == zoomPage || zoom == zoomWidth) {
      width = (float)(width1 * 0.01 * defZoom + 0.5);
      height = (float)(height1 * 0.01 * defZoom + 0.5);
    } else {
      width = (float)(width1 * 0.01 * zoom + 0.5);
      height = (float)(height1 * 0.01 * zoom + 0.5);
    }
    if (width > displayW - 100) {
      width = displayW - 100;
    }
    if (height > displayH - 150) {
      height = displayH - 150;
    }
  }


  os::Rect cFrame = shell->GetFrame();
  if( fullScreen )
  {
	cFrame.left = cFrame.top = 0;
  }
  cFrame.right = cFrame.left + width - 1 + ( fullScreen ? 0 : ( vScrollBar->GetBounds().Width() + 1 ) );
  cFrame.bottom = cFrame.top + height - 1 + ( fullScreen ? 0 : ( hScrollBar->GetBounds().Height() + 1 ) );
  shell->SetFrame( cFrame );
}

void XPDFCore::clear() {
  if (!doc) {
    return;
  }

  // no document
  delete doc;
  doc = NULL;
  out->clear();

  // no page displayed
  page = -99;

  // redraw
  scrollX = scrollY = 0;
  updateScrollBars();
  redrawRectangle(scrollX, scrollY, drawAreaWidth, drawAreaHeight);
}

void XPDFCore::displayPage(int pageA, double zoomA, int rotateA,
			   GBool scrollToTop, GBool addToHist) {
  double hDPI, vDPI;
  int rot;
  XPDFHistory *h;
  GBool newZoom;
  time_t newModTime;
  int oldScrollX, oldScrollY;

  // update the zoom and rotate values
  newZoom = zoomA != zoom;
  zoom = zoomA;
  rotate = rotateA;

  // check for document and valid page number
  if (!doc || pageA <= 0 || pageA > doc->getNumPages()) {
    return;
  }

  // busy cursor
  //setCursor(busyCursor);


  // check for changes to the file
  newModTime = getModTime(doc->getFileName()->getCString());
  if (newModTime != modTime) {
    if (loadFile(doc->getFileName()) == errNone) {
      if (pageA > doc->getNumPages()) {
	pageA = doc->getNumPages();
      }
    }
    modTime = newModTime;
  }

  // new page number
  page = pageA;

  // scroll to top
  if (scrollToTop) {
    scrollY = 0;
  }

  // if zoom level changed, scroll to the top-left corner
  if (newZoom) {
    scrollX = scrollY = 0;
  }

  // initialize mouse-related stuff
  linkAction = NULL;
  selectXMin = selectXMax = 0;
  selectYMin = selectYMax = 0;
  dragging = gFalse;
  lastDragLeft = lastDragTop = gTrue;

  // draw the page
  rot = rotate + doc->getPageRotate(page);
  if (rot >= 360) {
    rot -= 360;
  } else if (rotate < 0) {
    rot += 360;
  }
  if (zoom == zoomPage) {
    if (rot == 90 || rot == 270) {
      hDPI = (drawAreaWidth / doc->getPageHeight(page)) * 72;
      vDPI = (drawAreaHeight / doc->getPageWidth(page)) * 72;
    } else {
      hDPI = (drawAreaWidth / doc->getPageWidth(page)) * 72;
      vDPI = (drawAreaHeight / doc->getPageHeight(page)) * 72;
    }
    dpi = (hDPI < vDPI) ? hDPI : vDPI;
  } else if (zoom == zoomWidth) {
    if (rot == 90 || rot == 270) {
      dpi = (drawAreaWidth / doc->getPageHeight(page)) * 72;
    } else {
      dpi = (drawAreaWidth / doc->getPageWidth(page)) * 72;
    }
  } else {
    dpi = 0.01 * zoom * 72;
  }
  doc->displayPage(out, page, dpi, dpi, rotate, gTrue, gTrue);
  oldScrollX = scrollX;
  oldScrollY = scrollY;
  updateScrollBars();
  if (scrollX != oldScrollX || scrollY != oldScrollY) {
    redrawRectangle(scrollX, scrollY, drawAreaWidth, drawAreaHeight);
  }


  // add to history
  if (addToHist) {
    if (++historyCur == xpdfHistorySize) {
      historyCur = 0;
    }
    h = &history[historyCur];
    if (h->fileName) {
      delete h->fileName;
    }
    if (doc->getFileName()) {
      h->fileName = doc->getFileName()->copy();
    } else {
      h->fileName = NULL;
    }
    h->page = page;
    if (historyBLen < xpdfHistorySize) {
      ++historyBLen;
    }
    historyFLen = 0;
  }

  // update the parent window

if (updateCbk) {
    (*updateCbk)(updateCbkData, NULL, page, -1, "");
  }

  static_cast<XPDFView*>(scrolledWin)->Redraw();
  // back to regular cursor
  //setCursor(None);
}

void XPDFCore::displayDest(LinkDest *dest, double zoomA, int rotateA,
			   GBool addToHist) {
  Ref pageRef;
  int pg;
  int dx, dy;

  if (dest->isPageRef()) {
    pageRef = dest->getPageRef();
    pg = doc->findPage(pageRef.num, pageRef.gen);
  } else {
    pg = dest->getPageNum();
  }
  if (pg <= 0 || pg > doc->getNumPages()) {
    pg = 1;
  }
  if (pg != page) {
    displayPage(pg, zoomA, rotateA, gTrue, addToHist);
  }

  if (fullScreen) {
    return;
  }
  switch (dest->getKind()) {
  case destXYZ:
    out->cvtUserToDev(dest->getLeft(), dest->getTop(), &dx, &dy);
    if (dest->getChangeLeft() || dest->getChangeTop()) {
      scrollTo(dest->getChangeLeft() ? dx : scrollX,
	       dest->getChangeTop() ? dy : scrollY);
    }
    //~ what is the zoom parameter?
    break;
  case destFit:
  case destFitB:
    //~ do fit
    scrollTo(0, 0);
    break;
  case destFitH:
  case destFitBH:
    //~ do fit
    out->cvtUserToDev(0, dest->getTop(), &dx, &dy);
    scrollTo(0, dy);
    break;
  case destFitV:
  case destFitBV:
    //~ do fit
    out->cvtUserToDev(dest->getLeft(), 0, &dx, &dy);
    scrollTo(dx, 0);
    break;
  case destFitR:
    //~ do fit
    out->cvtUserToDev(dest->getLeft(), dest->getTop(), &dx, &dy);
    scrollTo(dx, dy);
    break;
  }
}

//------------------------------------------------------------------------
// page/position changes
//------------------------------------------------------------------------

void XPDFCore::gotoNextPage(int inc, GBool top) {
  int pg;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  if (page < doc->getNumPages()) {
    if ((pg = page + inc) > doc->getNumPages()) {
      pg = doc->getNumPages();
    }
    displayPage(pg, zoom, rotate, top, gTrue);
  } else {
    //XBell(display, 0);
  }
}

void XPDFCore::gotoPrevPage(int dec, GBool top, GBool bottom) {
  int pg;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  if (page > 1) {
    if (!fullScreen && bottom) {
      scrollY = out->getBitmapHeight() - drawAreaHeight;
      if (scrollY < 0) {
	scrollY = 0;
      }
      // displayPage will call updateScrollBars()
    }
    if ((pg = page - dec) < 1) {
      pg = 1;
    }
    displayPage(pg, zoom, rotate, top, gTrue);
  } else {
    //XBell(display, 0);
  }
}

void XPDFCore::goForward() {
  if (historyFLen == 0) {
    //XBell(display, 0);
    return;
  }
  if (++historyCur == xpdfHistorySize) {
    historyCur = 0;
  }
  --historyFLen;
  ++historyBLen;
  if (!doc || history[historyCur].fileName->cmp(doc->getFileName()) != 0) {
    if (loadFile(history[historyCur].fileName) != errNone) {
      //XBell(display, 0);
      return;
    }
  }
  displayPage(history[historyCur].page, zoom, rotate, gFalse, gFalse);
}

void XPDFCore::goBackward() {
  if (historyBLen <= 1) {
    //XBell(display, 0);
    return;
  }
  if (--historyCur < 0) {
    historyCur = xpdfHistorySize - 1;
  }
  --historyBLen;
  ++historyFLen;
  if (!doc || history[historyCur].fileName->cmp(doc->getFileName()) != 0) {
    if (loadFile(history[historyCur].fileName) != errNone) {
      //XBell(display, 0);
      return;
    }
  }
  displayPage(history[historyCur].page, zoom, rotate, gFalse, gFalse);
}

void XPDFCore::scrollLeft(int nCols) {
  scrollTo(scrollX - nCols * 16, scrollY);
}

void XPDFCore::scrollRight(int nCols) {
  scrollTo(scrollX + nCols * 16, scrollY);
}

void XPDFCore::scrollUp(int nLines) {
  scrollTo(scrollX, scrollY - nLines * 16);
}

void XPDFCore::scrollDown(int nLines) {
  scrollTo(scrollX, scrollY + nLines * 16);
}

void XPDFCore::scrollPageUp() {
  if (scrollY == 0) {
    gotoPrevPage(1, gFalse, gTrue);
  } else {
    scrollTo(scrollX, scrollY - drawAreaHeight);
  }
}

void XPDFCore::scrollPageDown() {
  if (scrollY >= out->getBitmapHeight() - drawAreaHeight) {
    gotoNextPage(1, gTrue);
  } else {
    scrollTo(scrollX, scrollY + drawAreaHeight);
  }
}

void XPDFCore::scrollTo(int x, int y) {
  GBool needRedraw;
  int maxPos, pos;

  needRedraw = gFalse;

  //printf( "XPDFCore::scrollTo() %i %i\n", out->getBitmapWidth(), out->getBitmapHeight() );

  maxPos = out ? out->getBitmapWidth() : 1;
  if (maxPos < drawAreaWidth) {
    maxPos = drawAreaWidth;
  }
  if (x < 0) {
    pos = 0;
  } else if (x > maxPos - drawAreaWidth) {
    pos = maxPos - drawAreaWidth;
  } else {
    pos = x;
  }
  if (scrollX != pos) {
    scrollX = pos;
    hScrollBar->SetValue( scrollX, false );
//	XmScrollBarSetValues(hScrollBar, scrollX, drawAreaWidth, 16,
	//		 drawAreaWidth, False);
    needRedraw = gTrue;
  }

  maxPos = out ? out->getBitmapHeight() : 1;
  if (maxPos < drawAreaHeight) {
    maxPos = drawAreaHeight;
  }
  if (y < 0) {
    pos = 0;
  } else if (y > maxPos - drawAreaHeight) {
    pos = maxPos - drawAreaHeight;
  } else {
    pos = y;
  }
  if (scrollY != pos) {
    scrollY = pos;
	vScrollBar->SetValue( scrollY, false );
    //XmScrollBarSetValues(vScrollBar, scrollY, drawAreaHeight, 16,
	//		 drawAreaHeight, False);
    needRedraw = gTrue;
  }

  scrolledWin->GetWindow()->Lock();
  scrolledWin->ScrollTo( -scrollX, -scrollY );
  scrolledWin->Flush();
  scrolledWin->GetWindow()->Unlock();

/*  if (needRedraw) {
    redrawRectangle(scrollX, scrollY, drawAreaWidth, drawAreaHeight);
  }*/
}

//------------------------------------------------------------------------
// selection
//------------------------------------------------------------------------

void XPDFCore::setSelection(int newXMin, int newYMin,
			    int newXMax, int newYMax) {
  int x, y;
  GBool needRedraw, needScroll;
  GBool moveLeft, moveRight, moveTop, moveBottom;
  SplashColor xorColor;


  // erase old selection on off-screen bitmap
  needRedraw = gFalse;
  if (selectXMin < selectXMax && selectYMin < selectYMax) {
    xorColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
    out->xorRectangle(selectXMin, selectYMin, selectXMax, selectYMax,
		      new SplashSolidColor(xorColor));
    needRedraw = gTrue;
  }

  // draw new selection on off-screen bitmap
  if (newXMin < newXMax && newYMin < newYMax) {
    xorColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
    out->xorRectangle(newXMin, newYMin, newXMax, newYMax,
		      new SplashSolidColor(xorColor));
    needRedraw = gTrue;
  }

  // check which edges moved
  moveLeft = newXMin != selectXMin;
  moveTop = newYMin != selectYMin;
  moveRight = newXMax != selectXMax;
  moveBottom = newYMax != selectYMax;

  // redraw currently visible part of bitmap
  if (needRedraw) {
    if (moveLeft) {
      redrawRectangle((newXMin < selectXMin) ? newXMin : selectXMin,
		      (newYMin < selectYMin) ? newYMin : selectYMin,
		      (newXMin > selectXMin) ? newXMin : selectXMin,
		      (newYMax > selectYMax) ? newYMax : selectYMax);
    }
    if (moveRight) {
      redrawRectangle((newXMax < selectXMax) ? newXMax : selectXMax,
		      (newYMin < selectYMin) ? newYMin : selectYMin,
		      (newXMax > selectXMax) ? newXMax : selectXMax,
		      (newYMax > selectYMax) ? newYMax : selectYMax);
    }
    if (moveTop) {
      redrawRectangle((newXMin < selectXMin) ? newXMin : selectXMin,
		      (newYMin < selectYMin) ? newYMin : selectYMin,
		      (newXMax > selectXMax) ? newXMax : selectXMax,
		      (newYMin > selectYMin) ? newYMin : selectYMin);
    }
    if (moveBottom) {
      redrawRectangle((newXMin < selectXMin) ? newXMin : selectXMin,
		      (newYMax < selectYMax) ? newYMax : selectYMax,
		      (newXMax > selectXMax) ? newXMax : selectXMax,
		      (newYMax > selectYMax) ? newYMax : selectYMax);
    }
  }

  // switch to new selection coords
  selectXMin = newXMin;
  selectXMax = newXMax;
  selectYMin = newYMin;
  selectYMax = newYMax;

  // scroll if necessary
  if (fullScreen) {
    return;
  }
  needScroll = gFalse;
  x = scrollX;
  y = scrollY;
  if (moveLeft && selectXMin < x) {
    x = selectXMin;
    needScroll = gTrue;
  } else if (moveRight && selectXMax >= x + drawAreaWidth) {
    x = selectXMax - drawAreaWidth;
    needScroll = gTrue;
  } else if (moveLeft && selectXMin >= x + drawAreaWidth) {
    x = selectXMin - drawAreaWidth;
    needScroll = gTrue;
  } else if (moveRight && selectXMax < x) {
    x = selectXMax;
    needScroll = gTrue;
  }
  if (moveTop && selectYMin < y) {
    y = selectYMin;
    needScroll = gTrue;
  } else if (moveBottom && selectYMax >= y + drawAreaHeight) {
    y = selectYMax - drawAreaHeight;
    needScroll = gTrue;
  } else if (moveTop && selectYMin >= y + drawAreaHeight) {
    y = selectYMin - drawAreaHeight;
    needScroll = gTrue;
  } else if (moveBottom && selectYMax < y) {
    y = selectYMax;
    needScroll = gTrue;
  }
  if (needScroll) {
    scrollTo(x, y);
  }
}

void XPDFCore::moveSelection(int mx, int my) {
  int xMin, yMin, xMax, yMax;

  // clip mouse coords
  if (mx < 0) {
    mx = 0;
  } else if (mx >= out->getBitmapWidth()) {
    mx = out->getBitmapWidth() - 1;
  }
  if (my < 0) {
    my = 0;
  } else if (my >= out->getBitmapHeight()) {
    my = out->getBitmapHeight() - 1;
  }

  // move appropriate edges of selection
  if (lastDragLeft) {
    if (mx < selectXMax) {
      xMin = mx;
      xMax = selectXMax;
    } else {
      xMin = selectXMax;
      xMax = mx;
      lastDragLeft = gFalse;
    }
  } else {
    if (mx > selectXMin) {
      xMin = selectXMin;
      xMax = mx;
    } else {
      xMin = mx;
      xMax = selectXMin;
      lastDragLeft = gTrue;
    }
  }
  if (lastDragTop) {
    if (my < selectYMax) {
      yMin = my;
      yMax = selectYMax;
    } else {
      yMin = selectYMax;
      yMax = my;
      lastDragTop = gFalse;
    }
  } else {
    if (my > selectYMin) {
      yMin = selectYMin;
      yMax = my;
    } else {
      yMin = my;
      yMax = selectYMin;
      lastDragTop = gTrue;
    }
  }

  // redraw the selection
  setSelection(xMin, yMin, xMax, yMax);
}

// X's copy-and-paste mechanism is brain damaged.  Xt doesn't help
// any, but doesn't make it too much worse, either.  Motif, on the
// other hand, adds significant complexity to the mess.  So here we
// blow off the Motif junk and stick to plain old Xt.  The next two
// functions (copySelection and convertSelectionCbk) implement the
// magic needed to deal with Xt's mechanism.  Note that this requires
// global variables (currentSelection and currentSelectionOwner).

void XPDFCore::copySelection() {
  if (!doc->okToCopy()) {
    return;
  }
  if (currentSelection) {
    delete currentSelection;
  }
  //~ for multithreading: need a mutex here
  currentSelection = out->getText(selectXMin, selectYMin,
				  selectXMax, selectYMax);
  currentSelectionOwner = this;


  
  //XtOwnSelection(drawArea, XA_PRIMARY, XtLastTimestampProcessed(display),
	//	 &convertSelectionCbk, NULL, NULL);
}

#ifdef __SYLLABLE__
GString*  XPDFCore::getCurrentSelection()
{
	return( currentSelection );
}
#endif

#ifndef __SYLLABLE__
Boolean XPDFCore::convertSelectionCbk(Widget widget, Atom *selection,
				      Atom *target, Atom *type,
				      XtPointer *value, unsigned long *length,
				      int *format) {
  Atom *array;

  // send back a list of supported conversion targets
  if (*target == targetsAtom) {
    if (!(array = (Atom *)XtMalloc(sizeof(Atom)))) {
      return False;
    }
    array[0] = XA_STRING;
    *value = (XtPointer)array;
    *type = XA_ATOM;
    *format = 32;
    *length = 1;
    return True;

  // send the selected text
  } else if (*target == XA_STRING) {
    //~ for multithreading: need a mutex here
    *value = XtNewString(currentSelection->getCString());
    *length = currentSelection->getLength();
    *type = XA_STRING;
    *format = 8; // 8-bit elements
    return True;
  }

  return False;
}
#endif
GBool XPDFCore::getSelection(int *xMin, int *yMin, int *xMax, int *yMax) {
  if (selectXMin >= selectXMax || selectYMin >= selectYMax) {
    return gFalse;
  }
  *xMin = selectXMin;
  *yMin = selectYMin;
  *xMax = selectXMax;
  *yMax = selectYMax;
  return gTrue;
}

GString *XPDFCore::extractText(int xMin, int yMin, int xMax, int yMax) {
  if (!doc->okToCopy()) {
    return NULL;
  }
  return out->getText(xMin, yMin, xMax, yMax);
}

GString *XPDFCore::extractText(int pageNum,
			       int xMin, int yMin, int xMax, int yMax) {
  TextOutputDev *textOut;
  GString *s;

  if (!doc->okToCopy()) {
    return NULL;
  }
  textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
  if (!textOut->isOk()) {
    delete textOut;
    return NULL;
  }
  doc->displayPage(textOut, pageNum, dpi, dpi, rotate, gTrue, gFalse);
  s = textOut->getText(xMin, yMin, xMax, yMax);
  delete textOut;
  return s;
}

//------------------------------------------------------------------------
// hyperlinks
//------------------------------------------------------------------------

GBool XPDFCore::doLink(int mx, int my) {
  double x, y;
  LinkAction *action;

  // look for a link
  out->cvtDevToUser(mx, my, &x, &y);
  if ((action = doc->findLink(x, y))) {
    doAction(action);
    return gTrue;
  }
  return gFalse;
}

void XPDFCore::doAction(LinkAction *action) {
  LinkActionKind kind;
  LinkDest *dest;
  GString *namedDest;
  char *s;
  GString *fileName, *fileName2;
  GString *cmd;
  GString *actionName;
  Object movieAnnot, obj1, obj2;
  GString *msg;
  int i;

  switch (kind = action->getKind()) {

  // GoTo / GoToR action
  case actionGoTo:
  case actionGoToR:
    if (kind == actionGoTo) {
      dest = NULL;
      namedDest = NULL;
      if ((dest = ((LinkGoTo *)action)->getDest())) {
	dest = dest->copy();
      } else if ((namedDest = ((LinkGoTo *)action)->getNamedDest())) {
	namedDest = namedDest->copy();
      }
    } else {
      dest = NULL;
      namedDest = NULL;
      if ((dest = ((LinkGoToR *)action)->getDest())) {
	dest = dest->copy();
      } else if ((namedDest = ((LinkGoToR *)action)->getNamedDest())) {
	namedDest = namedDest->copy();
      }
      s = ((LinkGoToR *)action)->getFileName()->getCString();
      //~ translate path name for VMS (deal with '/')
      if (isAbsolutePath(s)) {
	fileName = new GString(s);
      } else {
	fileName = appendToPath(grabPath(doc->getFileName()->getCString()), s);
      }
      if (loadFile(fileName) != errNone) {
	if (dest) {
	  delete dest;
	}
	if (namedDest) {
	  delete namedDest;
	}
	delete fileName;
	return;
      }
      delete fileName;
    }
    if (namedDest) {
      dest = doc->findDest(namedDest);
      delete namedDest;
    }
    if (dest) {
      displayDest(dest, zoom, rotate, gTrue);
      delete dest;
    } else {
      if (kind == actionGoToR) {
	displayPage(1, zoom, 0, gFalse, gTrue);
      }
    }
    break;

  // Launch action
  case actionLaunch:
    fileName = ((LinkLaunch *)action)->getFileName();
    s = fileName->getCString();
    if (!strcmp(s + fileName->getLength() - 4, ".pdf") ||
	!strcmp(s + fileName->getLength() - 4, ".PDF")) {
      //~ translate path name for VMS (deal with '/')
      if (isAbsolutePath(s)) {
	fileName = fileName->copy();
      } else {
	fileName = appendToPath(grabPath(doc->getFileName()->getCString()), s);
      }
      if (loadFile(fileName) != errNone) {
	delete fileName;
	return;
      }
      delete fileName;
      displayPage(1, zoom, rotate, gFalse, gTrue);
    } else {
      fileName = fileName->copy();
      if (((LinkLaunch *)action)->getParams()) {
	fileName->append(' ');
	fileName->append(((LinkLaunch *)action)->getParams());
      }
#ifdef VMS
      fileName->insert(0, "spawn/nowait ");
#elif defined(__EMX__)
      fileName->insert(0, "start /min /n ");
#else
      fileName->append(" &");
#endif
      msg = new GString("About to execute the command:\n");
      msg->append(fileName);
      if (doQuestionDialog("Launching external application", msg)) {
	system(fileName->getCString());
      }
      delete fileName;
      delete msg;
    }
    break;

  // URI action
  case actionURI:
    if (!(cmd = globalParams->getURLCommand())) {
      error(-1, "No urlCommand defined in config file");
      break;
    }
    runCommand(cmd, ((LinkURI *)action)->getURI());
    break;

  // Named action
  case actionNamed:
    actionName = ((LinkNamed *)action)->getName();
    if (!actionName->cmp("NextPage")) {
      gotoNextPage(1, gTrue);
    } else if (!actionName->cmp("PrevPage")) {
      gotoPrevPage(1, gTrue, gFalse);
    } else if (!actionName->cmp("FirstPage")) {
      if (page != 1) {
	displayPage(1, zoom, rotate, gTrue, gTrue);
      }
    } else if (!actionName->cmp("LastPage")) {
      if (page != doc->getNumPages()) {
	displayPage(doc->getNumPages(), zoom, rotate, gTrue, gTrue);
      }
    } else if (!actionName->cmp("GoBack")) {
      goBackward();
    } else if (!actionName->cmp("GoForward")) {
      goForward();
    } else if (!actionName->cmp("Quit")) {
	  shell->PostMessage( os::M_QUIT, shell );
      if (actionCbk) {
	(*actionCbk)(actionCbkData, "Quit");
      }
    } else {
      error(-1, "Unknown named action: '%s'", actionName->getCString());
    }
    break;
	
  // Movie action
  case actionMovie:
    if (!(cmd = globalParams->getMovieCommand())) {
      error(-1, "No movieCommand defined in config file");
      break;
    }
    if (((LinkMovie *)action)->hasAnnotRef()) {
      doc->getXRef()->fetch(((LinkMovie *)action)->getAnnotRef()->num,
			    ((LinkMovie *)action)->getAnnotRef()->gen,
			    &movieAnnot);
    } else {
      doc->getCatalog()->getPage(page)->getAnnots(&obj1);
      if (obj1.isArray()) {
	for (i = 0; i < obj1.arrayGetLength(); ++i) {
	  if (obj1.arrayGet(i, &movieAnnot)->isDict()) {
	    if (movieAnnot.dictLookup("Subtype", &obj2)->isName("Movie")) {
	      obj2.free();
	      break;
	    }
	    obj2.free();
	  }
	  movieAnnot.free();
	}
	obj1.free();
      }
    }
    if (movieAnnot.isDict()) {
      if (movieAnnot.dictLookup("Movie", &obj1)->isDict()) {
	if (obj1.dictLookup("F", &obj2)) {
	  if ((fileName = LinkAction::getFileSpecName(&obj2))) {
	    if (!isAbsolutePath(fileName->getCString())) {
	      fileName2 = appendToPath(
			      grabPath(doc->getFileName()->getCString()),
			      fileName->getCString());
	      delete fileName;
	      fileName = fileName2;
	    }
	    runCommand(cmd, fileName);
	    delete fileName;
	  }
	  obj2.free();
	}
	obj1.free();
      }
    }
    movieAnnot.free();
    break;

  // unknown action type
  case actionUnknown:
    error(-1, "Unknown link action type: '%s'",
	  ((LinkUnknown *)action)->getAction()->getCString());
    break;
  }
}

// Run a command, given a <cmdFmt> string with one '%s' in it, and an
// <arg> string to insert in place of the '%s'.
void XPDFCore::runCommand(GString *cmdFmt, GString *arg) {
  GString *cmd;
  char *s;

  if ((s = strstr(cmdFmt->getCString(), "%s"))) {
    cmd = mungeURL(arg);
    cmd->insert(0, cmdFmt->getCString(),
		s - cmdFmt->getCString());
    cmd->append(s + 2);
  } else {
    cmd = cmdFmt->copy();
  }
#ifdef VMS
  cmd->insert(0, "spawn/nowait ");
#elif defined(__EMX__)
  cmd->insert(0, "start /min /n ");
#else
  cmd->append(" &");
#endif
  system(cmd->getCString());
  delete cmd;
}

// Escape any characters in a URL which might cause problems when
// calling system().
GString *XPDFCore::mungeURL(GString *url) {
  static char *allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz"
                         "0123456789"
                         "-_.~/?:@&=+,#%";
  GString *newURL;
  char c;
  char buf[4];
  int i;

  newURL = new GString();
  for (i = 0; i < url->getLength(); ++i) {
    c = url->getChar(i);
    if (strchr(allowed, c)) {
      newURL->append(c);
    } else {
      sprintf(buf, "%%%02x", c & 0xff);
      newURL->append(buf);
    }
  }
  return newURL;
}


//------------------------------------------------------------------------
// find
//------------------------------------------------------------------------

void XPDFCore::find(char *s, GBool next) {
  Unicode *u;
  TextOutputDev *textOut;
  int xMin, yMin, xMax, yMax;
  double xMin1, yMin1, xMax1, yMax1;
  int pg;
  GBool startAtTop;
  int len, i;

  // check for zero-length string
  if (!s[0]) {
    //XBell(display, 0);
    return;
  }

  // set cursor to watch
 // setCursor(busyCursor);

  // convert to Unicode
#if 1 //~ should do something more intelligent here
  len = strlen(s);
  u = (Unicode *)gmalloc(len * sizeof(Unicode));
  for (i = 0; i < len; ++i) {
    u[i] = (Unicode)(s[i] & 0xff);
  }
#endif

  // search current page starting at current selection or top of page
  startAtTop = !next && !(selectXMin < selectXMax && selectYMin < selectYMax);
  xMin = selectXMin + 1;
  yMin = selectYMin + 1;
  xMax = yMax = 0;
  if (out->findText(u, len, startAtTop, gTrue, next, gFalse,
		    &xMin, &yMin, &xMax, &yMax)) {
    goto found;
  }

  // search following pages
  textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
  if (!textOut->isOk()) {
    delete textOut;
    goto done;
  }
  for (pg = page+1; pg <= doc->getNumPages(); ++pg) {
    doc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
    if (textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse,
			  &xMin1, &yMin1, &xMax1, &yMax1)) {
      goto foundPage;
    }
  }

  // search previous pages
  for (pg = 1; pg < page; ++pg) {
    doc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
    if (textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse,
			  &xMin1, &yMin1, &xMax1, &yMax1)) {
      goto foundPage;
    }
  }
  delete textOut;

  // search current page ending at current selection
  if (!startAtTop) {
    xMin = yMin = 0;
    xMax = selectXMin;
    yMax = selectYMin;
    if (out->findText(u, len, gTrue, gFalse, gFalse, next,
		      &xMin, &yMin, &xMax, &yMax)) {
      goto found;
    }
  }

  // not found
  //XBell(display, 0);
  goto done;

  // found on a different page
 foundPage:
  delete textOut;
  displayPage(pg, zoom, rotate, gTrue, gTrue);
  if (!out->findText(u, len, gTrue, gTrue, gFalse, gFalse,
		     &xMin, &yMin, &xMax, &yMax)) {
    // this can happen if coalescing is bad
    goto done;
  }

  // found: change the selection
 found:
  setSelection(xMin, yMin, xMax, yMax);
#ifndef NO_TEXT_SELECT
  copySelection();
#endif

 done:
  gfree(u);

  // reset cursors to normal
  //setCursor(None);
}

//------------------------------------------------------------------------
// misc access
//------------------------------------------------------------------------

void XPDFCore::setBusyCursor(GBool busy) {
  //setCursor(busy ? busyCursor : None);
}

void XPDFCore::takeFocus() {
  //scrolledWin->MakeFocus();
  //XmProcessTraversal(drawArea, XmTRAVERSE_CURRENT);
}

//------------------------------------------------------------------------
// GUI code
//------------------------------------------------------------------------

void XPDFCore::initWindow() {
  int n;

  
#ifndef __SYLLABLE__
  // create the cursors
  busyCursor = XCreateFontCursor(display, XC_watch);
  linkCursor = XCreateFontCursor(display, XC_hand2);
  selectCursor = XCreateFontCursor(display, XC_cross);
  currentCursor = 0;
#endif


  // create the scrolled window and scrollbars
hScrollBar = new os::ScrollBar( os::Rect(), "xpdf_h_scroll", new os::Message( MSG_SCROLLBAR_H ), 0, 1, os::HORIZONTAL, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
os::Rect cScrollFrame = parentWidget->GetBounds();
cScrollFrame.top = cScrollFrame.bottom - hScrollBar->GetPreferredSize( false ).y;
hScrollBar->SetFrame( cScrollFrame );
parentWidget->AddChild( hScrollBar );

vScrollBar = new os::ScrollBar( os::Rect(), "xpdf_v_scroll", new os::Message( MSG_SCROLLBAR_V ), 0, 1, os::VERTICAL, os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
cScrollFrame.bottom = cScrollFrame.top;
cScrollFrame.top = 0;
cScrollFrame.left = cScrollFrame.right - vScrollBar->GetPreferredSize( false ).x;
vScrollBar->SetFrame( cScrollFrame );
parentWidget->AddChild( vScrollBar );

cScrollFrame.right = cScrollFrame.left - 1;
cScrollFrame.left = 0;
cScrollFrame.bottom -= 1;
scrolledWin = new XPDFView( this, cScrollFrame, "xpdf_view", os::CF_FOLLOW_ALL );
parentWidget->AddChild( scrolledWin );
hScrollBar->SetTarget( scrolledWin );
vScrollBar->SetTarget( scrolledWin );
scrolledWin->MakeFocus();

drawAreaWidth = (int)scrolledWin->GetBounds().Width() + 1;
drawAreaHeight = (int)scrolledWin->GetBounds().Height() + 1;

}

#ifndef __SYLLABLE__
void XPDFCore::hScrollChangeCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;
  XmScrollBarCallbackStruct *data = (XmScrollBarCallbackStruct *)callData;

  core->scrollTo(data->value, core->scrollY);
}

void XPDFCore::hScrollDragCbk(Widget widget, XtPointer ptr,
			      XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;
  XmScrollBarCallbackStruct *data = (XmScrollBarCallbackStruct *)callData;

  core->scrollTo(data->value, core->scrollY);
}

void XPDFCore::vScrollChangeCbk(Widget widget, XtPointer ptr,
			     XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;
  XmScrollBarCallbackStruct *data = (XmScrollBarCallbackStruct *)callData;

  core->scrollTo(core->scrollX, data->value);
}

void XPDFCore::vScrollDragCbk(Widget widget, XtPointer ptr,
			      XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;
  XmScrollBarCallbackStruct *data = (XmScrollBarCallbackStruct *)callData;

  core->scrollTo(core->scrollX, data->value);
}
#endif
void XPDFCore::resizeCbk(XPDFCore* core) {
  int w, h;
  int oldScrollX, oldScrollY;

  w = (int)core->scrolledWin->GetBounds().Width() + 1;
  h = (int)core->scrolledWin->GetBounds().Height() + 1;

  core->drawAreaWidth = (int)w;
  core->drawAreaHeight = (int)h;

  if (core->page >= 0 &&
      (core->zoom == zoomPage || core->zoom == zoomWidth)) {
    core->displayPage(core->page, core->zoom, core->rotate,
		      gFalse, gFalse);
  } else {
    oldScrollX = core->scrollX;
    oldScrollY = core->scrollY;
    core->updateScrollBars();
    if (core->scrollX != oldScrollX || core->scrollY != oldScrollY) {
      core->redrawRectangle(core->scrollX, core->scrollY,
			    core->drawAreaWidth, core->drawAreaHeight);
    }
  }

}
#ifndef __SYLLABLE__
void XPDFCore::redrawCbk(Widget widget, XtPointer ptr, XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;
  XmDrawingAreaCallbackStruct *data = (XmDrawingAreaCallbackStruct *)callData;
  int x, y, w, h;

  if (data->reason == XmCR_EXPOSE) {
    x = core->scrollX + data->event->xexpose.x;
    y = core->scrollY + data->event->xexpose.y;
    w = data->event->xexpose.width;
    h = data->event->xexpose.height;
  } else {
    x = core->scrollX;
    y = core->scrollY;
    w = core->drawAreaWidth;
    h = core->drawAreaHeight;
  }
  core->redrawRectangle(x, y, w, h);
}
#endif
void XPDFCore::outputDevRedrawCbk(void *data) {
  XPDFCore *core = (XPDFCore *)data;

  core->redrawRectangle(core->scrollX, core->scrollY,
			core->drawAreaWidth, core->drawAreaHeight);
}

void XPDFCore::inputCbk(void* ptr, uint32 nMsg, os::Point cPos, uint32 nButtons) {
  XPDFCore *core = (XPDFCore *)ptr;
  //XmDrawingAreaCallbackStruct *data = (XmDrawingAreaCallbackStruct *)callData;
  LinkAction *action;
  int mx, my;
  double x, y;
  char *s;
  //KeySym key;
  char buf[20];
  int n;

  switch (nMsg/*data->event->type*/) {
  case os::M_MOUSE_DOWN/*ButtonPress*/:
    if (nButtons/*data->event->xbutton.button*/ == 1) {
      core->takeFocus();
      if (core->doc && core->doc->getNumPages() > 0) {
	if (core->selectEnabled) {
	  mx = core->scrollX + (int)cPos.x/*data->event->xbutton.x*/;
	  my = core->scrollY + (int)cPos.y/*data->event->xbutton.y*/;
	  core->setSelection(mx, my, mx, my);
	  //core->setCursor(core->selectCursor);
	  core->dragging = gTrue;
	}
      }
    } else if (nButtons/*data->event->xbutton.button*/ == 2) {
      if (!core->fullScreen) {
	core->panning = gTrue;
	core->panMX = (int)cPos.x/*data->event->xbutton.x*/;
	core->panMY = (int)cPos.y/*data->event->xbutton.y*/;
      }
    } 
#ifndef __SYLLABLE__
else if (data->event->xbutton.button == 4) { // mouse wheel up
      if (core->fullScreen) {
	core->gotoPrevPage(1, gTrue, gFalse);
      } else if (core->scrollY == 0) {
	core->gotoPrevPage(1, gFalse, gTrue);
      } else {
	core->scrollUp(1);
      }
    } else if (data->event->xbutton.button == 5) { // mouse wheel down
      if (core->fullScreen ||
	  core->scrollY >=
	    core->out->getBitmapHeight() - core->drawAreaHeight) {
	core->gotoNextPage(1, gTrue);
      } else {
	core->scrollDown(1);
      }
    } else if (data->event->xbutton.button == 6) { // second mouse wheel left
      if (!core->fullScreen) {
	core->scrollLeft(1);
      }
    } else if (data->event->xbutton.button == 7) { // second mouse wheel right
      if (!core->fullScreen) {
	core->scrollRight(1);
      }
    } 
#endif
else {

      if (*core->mouseCbk) {
	(*core->mouseCbk)(core->mouseCbkData, nMsg, cPos, nButtons);
      }
    }

    break;
  case os::M_MOUSE_UP/*ButtonRelease*/:
    if (nButtons/*data->event->xbutton.button*/ == 1) {
      if (core->doc && core->doc->getNumPages() > 0) {
	mx = core->scrollX + (int)cPos.x/*data->event->xbutton.x*/;
	my = core->scrollY + (int)cPos.y/*data->event->xbutton.y*/;
	if (core->dragging) {
	  core->dragging = gFalse;
	  //core->setCursor(None);
	  core->moveSelection(mx, my);
#ifndef NO_TEXT_SELECT
	  if (core->selectXMin != core->selectXMax &&
	      core->selectYMin != core->selectYMax) {
	    if (core->doc->okToCopy()) {
	      core->copySelection();
	    } else {
	      error(-1, "Copying of text from this document is not allowed.");
	    }
	  }
#endif
	}
	if (core->hyperlinksEnabled) {
	  if (core->selectXMin == core->selectXMax ||
	    core->selectYMin == core->selectYMax) {
	    core->doLink(mx, my);
	  }
	}
      }
    } else if (nButtons/*data->event->xbutton.button*/ == 2) {
      core->panning = gFalse;
    } else {
#ifndef __SYLLABLE__
      if (*core->mouseCbk) {
	(*core->mouseCbk)(core->mouseCbkData, data->event);
      }
#endif
    }
    break;
  case os::M_MOUSE_MOVED/*MotionNotify*/:
    if (core->doc && core->doc->getNumPages() > 0) {
      mx = core->scrollX + (int)cPos.x/*data->event->xbutton.x*/;
      my = core->scrollY + (int)cPos.y/*data->event->xbutton.y*/;
      if (core->dragging) {
	core->moveSelection(mx, my);
      } else if (core->hyperlinksEnabled) {
	core->out->cvtDevToUser(mx, my, &x, &y);
	if ((action = core->doc->findLink(x, y))) {
	  //core->setCursor(core->linkCursor);
	  if (action != core->linkAction) {
	    core->linkAction = action;
	    if (core->updateCbk) {
	      s = "";
	      switch (action->getKind()) {
	      case actionGoTo:
		s = "[internal link]";
		break;
	      case actionGoToR:
		s = ((LinkGoToR *)action)->getFileName()->getCString();
		break;
	      case actionLaunch:
		s = ((LinkLaunch *)action)->getFileName()->getCString();
		break;
	      case actionURI:
		s = ((LinkURI *)action)->getURI()->getCString();
		break;
	      case actionNamed:
		s = ((LinkNamed *)action)->getName()->getCString();
		break;
	      case actionMovie:
		s = "[movie]";
		break;
	      case actionUnknown:
		s = "[unknown link]";
		break;
	      }
	      (*core->updateCbk)(core->updateCbkData, NULL, -1, -1, s);
	    }
	  }
	} else {
//	  core->setCursor(None);
	  if (core->linkAction) {
	    core->linkAction = NULL;
	    if (core->updateCbk) {
	      (*core->updateCbk)(core->updateCbkData, NULL, -1, -1, "");
	    }
	  }
	}
      }
    }
    if (core->panning) {
      core->scrollTo(core->scrollX - ((int)cPos.x/*data->event->xbutton.x*/ - core->panMX),
		     core->scrollY - ((int)cPos.y/*data->event->xbutton.y*/ - core->panMY));
      core->panMX = (int)cPos.x/*data->event->xbutton.x*/;
      core->panMY = (int)cPos.y/*data->event->xbutton.y*/;
    }
    break;
#ifndef __SYLLABLE__
  case KeyPress:
    n = XLookupString(&data->event->xkey, buf, sizeof(buf) - 1,
		      &key, NULL);
    core->keyPress(buf, key, data->event->xkey.state);
    break;
#endif
  }
}

void XPDFCore::keyPress(const char *s, const char* raw, uint32 modifiers) {
  switch (raw[0]) {
  case os::VK_HOME:
    if (modifiers & os::QUAL_CTRL) {
      displayPage(1, zoom, rotate, gTrue, gTrue);
    } else if (!fullScreen) {
      scrollTo(0, 0);
    }
    return;
  case os::VK_END:
    if (modifiers & os::QUAL_CTRL) {
      displayPage(doc->getNumPages(), zoom, rotate, gTrue, gTrue);
    } else if (!fullScreen) {
      scrollTo(out->getBitmapWidth() - drawAreaWidth,
	       out->getBitmapHeight() - drawAreaHeight);
    }
    return;
  case os::VK_PAGE_UP:
    if (fullScreen) {
      gotoPrevPage(1, gTrue, gFalse);
    } else {
      scrollPageUp();
    }
    return;
  case os::VK_PAGE_DOWN:
    if (fullScreen) {
      gotoNextPage(1, gTrue);
    } else {
      scrollPageDown();
    }
    return;
 case os::VK_LEFT_ARROW:
    if (!fullScreen) {
      scrollLeft();
    }
    return;
  case os::VK_RIGHT_ARROW:
    if (!fullScreen) {
      scrollRight();
    }
    return;
 case os::VK_UP_ARROW:
    if (!fullScreen) {
      scrollUp();
    }
    return;
 case os::VK_DOWN_ARROW:
    if (!fullScreen) {
      scrollDown();
    }
    return;
  }

  if (*keyPressCbk) {
    (*keyPressCbk)(keyPressCbkData, s, raw, modifiers);
  }
}

void XPDFCore::redrawRectangle(int x, int y, int w, int h) {

  // clip to window
  if (x < scrollX) {
    w -= scrollX - x;
    x = scrollX;
  }
  if (x + w > scrollX + drawAreaWidth) {
    w = scrollX + drawAreaWidth - x;
  }
  if (y < scrollY) {
    h -= scrollY - y;
    y = scrollY;
  }
  if (y + h > scrollY + drawAreaHeight) {
    h = scrollY + drawAreaHeight - y;
  }

  // create a GC for the drawing area


  // draw white background past the edges of the document
  if (x + w > out->getBitmapWidth()) {
	scrolledWin->FillRect( os::Rect( out->getBitmapWidth() /*- scrollX*/, y /*- scrollY*/,
		   x /*- scrollX*/ + w, y/* - scrollY*/ + h ) );
    w = out->getBitmapWidth() - x;
  }
  if (y + h > out->getBitmapHeight()) {
	scrolledWin->FillRect( os::Rect( x /*- scrollX*/, out->getBitmapHeight() /*- scrollY*/,
		   x /*- scrollX*/ + w, y /*- scrollY*/ + h ) );
    h = out->getBitmapHeight() - y;
  }

  // redraw
  if (w >= 0 && h >= 0) {
    out->redraw(x, y, scrolledWin, static_cast<XPDFView*>(scrolledWin)->GetBitmap()/*drawAreaWin, drawAreaGC*/, x - scrollX, y - scrollY, w, h);
  }
}

void XPDFCore::updateScrollBars() {
  int n;
  int maxPos;

  maxPos = out ? out->getBitmapWidth() : 1;
  if (maxPos < drawAreaWidth) {
    maxPos = drawAreaWidth;
  }
  if (scrollX > maxPos - drawAreaWidth) {
    scrollX = maxPos - drawAreaWidth;
  }

 
  hScrollBar->SetMinMax( 0, maxPos - drawAreaWidth );
  hScrollBar->SetProportion( (float)drawAreaWidth / (float)maxPos );
  hScrollBar->SetSteps( 30.0f, (float)drawAreaWidth * 0.8f );
  hScrollBar->SetValue( scrollX );
  maxPos = out ? out->getBitmapHeight() : 1;
  if (maxPos < drawAreaHeight) {
    maxPos = drawAreaHeight;
  }
  if (scrollY > maxPos - drawAreaHeight) {
    scrollY = maxPos - drawAreaHeight;
  }
  vScrollBar->SetMinMax( 0, maxPos - drawAreaHeight );
  vScrollBar->SetProportion( (float)drawAreaHeight / (float)maxPos );
  vScrollBar->SetSteps( 30.0f, (float)drawAreaHeight * 0.8f );
  vScrollBar->SetValue( scrollY );
}

#ifndef __SYLLABLE__
void XPDFCore::setCursor(Cursor cursor) {
  Window topWin;

  if (cursor == currentCursor) {
    return;
  }
  if (!(topWin = XtWindow(shell))) {
    return;
  }
  if (cursor == None) {
    XUndefineCursor(display, topWin);
  } else {
    XDefineCursor(display, topWin, cursor);
  }
  XFlush(display);
  currentCursor = cursor;
}
#endif
GBool XPDFCore::doQuestionDialog(char *title, GString *msg) {
  printf( "NOT IMPLEMENTED: XPDFCore::doQuestionDialog()\n" );
  //return doDialog(XmDIALOG_QUESTION, gTrue, title, msg);
}

void XPDFCore::doInfoDialog(char *title, GString *msg) {
  printf( "NOT IMPLEMENTED: XPDFCore::doInfoDialog()\n" );
  //doDialog(XmDIALOG_INFORMATION, gFalse, title, msg);
}

void XPDFCore::doErrorDialog(char *title, GString *msg) {
  printf( "NOT IMPLEMENTED: XPDFCore::doErrorDialog()\n" );
  //doDialog(XmDIALOG_ERROR, gFalse, title, msg);
}

GBool XPDFCore::doDialog(int type, GBool hasCancel,
			 char *title, GString *msg) {
#ifndef __SYLLABLE__
  Widget dialog, scroll, text;
  XtAppContext appContext;
  Arg args[20];
  int n;
  XmString s1, s2;
  XEvent event;

  n = 0;
  XtSetArg(args[n], XmNdialogType, type); ++n;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ++n;
  s1 = XmStringCreateLocalized(title);
  XtSetArg(args[n], XmNdialogTitle, s1); ++n;
  s2 = NULL; // make gcc happy
  if (msg->getLength() <= 80) {
    s2 = XmStringCreateLocalized(msg->getCString());
    XtSetArg(args[n], XmNmessageString, s2); ++n;
  }
  dialog = XmCreateMessageDialog(drawArea, "questionDialog", args, n);
  XmStringFree(s1);
  if (msg->getLength() <= 80) {
    XmStringFree(s2);
  } else {
    n = 0;
    XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); ++n;
    if (drawAreaWidth > 300) {
      XtSetArg(args[n], XmNwidth, drawAreaWidth - 100); ++n;
    }
    scroll = XmCreateScrolledWindow(dialog, "scroll", args, n);
    XtManageChild(scroll);
    n = 0;
    XtSetArg(args[n], XmNeditable, False); ++n;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); ++n;
    XtSetArg(args[n], XmNvalue, msg->getCString()); ++n;
    XtSetArg(args[n], XmNshadowThickness, 0); ++n;
    text = XmCreateText(scroll, "text", args, n);
    XtManageChild(text);
  }
  XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback(dialog, XmNokCallback,
		&dialogOkCbk, (XtPointer)this);
  if (hasCancel) {
    XtAddCallback(dialog, XmNcancelCallback,
		  &dialogCancelCbk, (XtPointer)this);
  } else {
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
  }

  XtManageChild(dialog);

  appContext = XtWidgetToApplicationContext(dialog);
  dialogDone = 0;
  do {
    XtAppNextEvent(appContext, &event);
    XtDispatchEvent(&event);
  } while (!dialogDone);

  XtUnmanageChild(dialog);
  XtDestroyWidget(dialog);

  return dialogDone > 0;
#endif
}
#ifndef __SYLLABLE__
void XPDFCore::dialogOkCbk(Widget widget, XtPointer ptr,
			   XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;

  core->dialogDone = 1;
}

void XPDFCore::dialogCancelCbk(Widget widget, XtPointer ptr,
			       XtPointer callData) {
  XPDFCore *core = (XPDFCore *)ptr;

  core->dialogDone = -1;
}
#endif
