//========================================================================
//
// XPDFCore.h
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFCORE_H
#define XPDFCORE_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <util/message.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/scrollbar.h>
#include <gui/desktop.h>
#include <aconf.h>
#include "gtypes.h"
#include "gfile.h" // for time_t
#include "SplashTypes.h"

class GString;
class GList;
class BaseStream;
class PDFDoc;
class LinkAction;
class LinkDest;
class XSplashOutputDev;

//------------------------------------------------------------------------
// zoom factor
//------------------------------------------------------------------------

#define zoomPage  -1
#define zoomWidth -2
#define defZoom   125

//------------------------------------------------------------------------
// XPDFHistory
//------------------------------------------------------------------------

struct XPDFHistory {
  GString *fileName;
  int page;
};

#define xpdfHistorySize 50

//------------------------------------------------------------------------
// XPDFRegion
//------------------------------------------------------------------------

struct XPDFRegion {
  int page;
  double xMin, yMin, xMax, yMax;
  SplashRGB8 color;
  SplashRGB8 selectColor;
  GBool selectable;
};

//------------------------------------------------------------------------
// callbacks
//------------------------------------------------------------------------

typedef void (*XPDFUpdateCbk)(void *data, GString *fileName,
				int pageNum, int numPages, char *linkLabel);

typedef void (*XPDFActionCbk)(void *data, char *action);

typedef void (*XPDFKeyPressCbk)(void *data, const char *s, const char* raw,
				uint32 modifiers);

typedef void (*XPDFMouseCbk)(void *data, uint32 nMsg, os::Point cPos, uint32 nButtons);
#ifndef __SYLLABLE__
typedef GString *(*XPDFReqPasswordCbk)(void *data, GBool again);
#endif

enum
{
	MSG_PASSWORD,
	MSG_SCROLLBAR_H,
	MSG_SCROLLBAR_V,
	
	MSG_ABOUT,
	MSG_QUIT,
	MSG_PREVIOUS_PAGE,
	MSG_NEXT_PAGE,
	MSG_SET_ZOOM,
	MSG_OPEN,
	MSG_REQUESTER_CLOSED,
	MSG_FIND,
	MSG_COPY,
	MSG_SHOW_FIND,
	MSG_PAGE_SELECT
};


//------------------------------------------------------------------------
// XPDFCore
//------------------------------------------------------------------------

class XPDFCore {
public:

  // Create viewer core inside <parentWidgetA>.
  XPDFCore(os::Window* shellA, os::View* parentWidgetA,
	   SplashRGB8 paperColorA, GBool fullScreenA,
	   GBool reverseVideo, GBool installCmap, int rgbCubeSize);

  ~XPDFCore();

  //----- loadFile / displayPage / displayDest

  // Load a new file.  Returns pdfOk or error code.
  int loadFile(GString *fileName, GString *ownerPassword = NULL,
	       GString *userPassword = NULL);

  // Load a new file, via a Stream instead of a file name.  Returns
  // pdfOk or error code.
  int loadFile(BaseStream *stream, GString *ownerPassword = NULL,
	       GString *userPassword = NULL);

  // Resize the window to fit page <pg> of the current document.
  void resizeToPage(int pg);

  // Clear out the current document, if any.
  void clear();

  // Display (or redisplay) the specified page.  If <scrollToTop> is
  // set, the window is vertically scrolled to the top; otherwise, no
  // scrolling is done.  If <addToHist> is set, this page change is
  // added to the history list.
  void displayPage(int pageA, double zoomA, int rotateA,
		   GBool scrollToTop, GBool addToHist);

  // Display a link destination.
  void displayDest(LinkDest *dest, double zoomA, int rotateA,
		   GBool addToHist);

  //----- page/position changes

  void gotoNextPage(int inc, GBool top);
  void gotoPrevPage(int dec, GBool top, GBool bottom);
  void goForward();
  void goBackward();
  void scrollLeft(int nCols = 1);
  void scrollRight(int nCols = 1);
  void scrollUp(int nLines = 1);
  void scrollDown(int nLines = 1);
  void scrollPageUp();
  void scrollPageDown();
  void scrollTo(int x, int y);

  //----- selection

  void setSelection(int newXMin, int newYMin, int newXMax, int newYMax);
  void moveSelection(int mx, int my);
  void copySelection();
#ifdef __SYLLABLE__
  GString* getCurrentSelection();
#endif
  GBool getSelection(int *xMin, int *yMin, int *xMax, int *yMax);
  GString *extractText(int xMin, int yMin, int xMax, int yMax);
  GString *extractText(int pageNum, int xMin, int yMin, int xMax, int yMax);

  //----- hyperlinks

  void doAction(LinkAction *action);


  //----- find

  void find(char *s, GBool next = gFalse);

  //----- simple modal dialogs

  GBool doQuestionDialog(char *title, GString *msg);
  void doInfoDialog(char *title, GString *msg);
  void doErrorDialog(char *title, GString *msg);

  //----- misc access

  os::View* getWidget() { return scrolledWin; }
  //os::View* getDrawAreaWidget() { return drawArea; }
  PDFDoc *getDoc() { return doc; }
  XSplashOutputDev *getOutputDev() { return out; }
  int getPageNum() { return page; }
  double getZoom() { return zoom; }
  double getZoomDPI() { return dpi; }
  int getRotate() { return rotate; }
  GBool canGoBack() { return historyBLen > 1; }
  GBool canGoForward() { return historyFLen > 0; }
  int getScrollX() { return scrollX; }
  int getScrollY() { return scrollY; }
  int getDrawAreaWidth() { return drawAreaWidth; }
  int getDrawAreaHeight() { return drawAreaHeight; }
  void setBusyCursor(GBool busy);
  //Cursor getBusyCursor() { return busyCursor; }
  void takeFocus();
  void enableHyperlinks(GBool on) { hyperlinksEnabled = on; }
  void enableSelect(GBool on) { selectEnabled = on; }

  void setUpdateCbk(XPDFUpdateCbk cbk, void *data)
    { updateCbk = cbk; updateCbkData = data; }
  void setActionCbk(XPDFActionCbk cbk, void *data)
    { actionCbk = cbk; actionCbkData = data; }
  void setKeyPressCbk(XPDFKeyPressCbk cbk, void *data)
    { keyPressCbk = cbk; keyPressCbkData = data; }
  void setMouseCbk(XPDFMouseCbk cbk, void *data)
    { mouseCbk = cbk; mouseCbkData = data; }
#ifndef __SYLLABLE__
  void setReqPasswordCbk(XPDFReqPasswordCbk cbk, void *data)
    { reqPasswordCbk = cbk; reqPasswordCbkData = data; }
#endif 
private:

  //----- hyperlinks
  GBool doLink(int mx, int my);
  void runCommand(GString *cmdFmt, GString *arg);
  GString *mungeURL(GString *url);

  //----- selection
#ifndef __SYLLABLE__
  static Boolean convertSelectionCbk(Widget widget, Atom *selection,
				     Atom *target, Atom *type,
				     XtPointer *value, unsigned long *length,
				     int *format);

#endif
  //----- GUI code
  void initWindow();
#ifndef __SYLLABLE__
  static void hScrollChangeCbk(Widget widget, XtPointer ptr,
			       XtPointer callData);
  static void hScrollDragCbk(Widget widget, XtPointer ptr,
			     XtPointer callData);
  static void vScrollChangeCbk(Widget widget, XtPointer ptr,
			       XtPointer callData);
  static void vScrollDragCbk(Widget widget, XtPointer ptr,
			     XtPointer callData);
#endif			     
  static void resizeCbk(XPDFCore* core);
#ifndef __SYLLABLE__
  static void redrawCbk(Widget widget, XtPointer ptr, XtPointer callData);
#endif
  static void outputDevRedrawCbk(void *data);

  static void inputCbk(void* ptr, uint32 nMsg, os::Point cPos, uint32 nButtons);

  void keyPress(const char* pzString, const char* pzRawString, uint32 nQualifiers);


  void redrawRectangle(int x, int y, int w, int h);
  void updateScrollBars();
  //void setCursor(Cursor cursor);
  GBool doDialog(int type, GBool hasCancel,
		 char *title, GString *msg);
#ifndef __SYLLABLE__	 
  static void dialogOkCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  static void dialogCancelCbk(Widget widget, XtPointer ptr,
			      XtPointer callData);
#endif
  SplashRGB8 paperColor;
  GBool fullScreen;
#ifndef __SYLLABLE__
  Display *display;
  int screenNum;
  Visual *visual;
  Colormap colormap;
#endif
  os::View* parentWidget;
  os::Window* shell;
  os::ScrollBar* hScrollBar;
  os::ScrollBar* vScrollBar;
  os::View* scrolledWin;
#ifndef __SYLLABLE__
  Widget shell;			// top-level shell containing the widget
  Widget parentWidget;		// parent widget (not created by XPDFCore)
  Widget scrolledWin;
  Widget hScrollBar;
  Widget vScrollBar;
  
  Widget drawAreaFrame;
  Widget drawArea;

	
  Cursor busyCursor, linkCursor, selectCursor;
  Cursor currentCursor;
  GC drawAreaGC;		// GC for blitting into drawArea
#endif
  int drawAreaWidth, drawAreaHeight;
  int scrollX, scrollY;		// current upper-left corner

  int selectXMin, selectYMin,	// coordinates of current selection:
      selectXMax, selectYMax;	//   (xMin==xMax || yMin==yMax) means there
				//   is no selection
  GBool dragging;		// set while selection is being dragged
  GBool lastDragLeft;		// last dragged selection edge was left/right
  GBool lastDragTop;		// last dragged selection edge was top/bottom
  static GString *currentSelection;  // selected text
  static XPDFCore *currentSelectionOwner;
  //static Atom targetsAtom;

  GBool panning;
  int panMX, panMY;

  XPDFHistory			// page history queue
    history[xpdfHistorySize];
  int historyCur;               // currently displayed page
  int historyBLen;              // number of valid entries backward from
                                //   current entry
  int historyFLen;              // number of valid entries forward from
                                //   current entry

  PDFDoc *doc;			// current PDF file
  int page;			// current page number
  double zoom;			// current zoom level, in percent of 72 dpi
  double dpi;			// current zoom level, in DPI
  int rotate;			// current page rotation
  time_t modTime;		// last modification time of PDF file

  LinkAction *linkAction;	// mouse cursor is over this link


  XPDFUpdateCbk updateCbk;
  void *updateCbkData;
  XPDFActionCbk actionCbk;
  void *actionCbkData;
  XPDFKeyPressCbk keyPressCbk;
  void *keyPressCbkData;

  XPDFMouseCbk mouseCbk;
  void *mouseCbkData;
#ifndef __SYLLABLE__
  XPDFReqPasswordCbk reqPasswordCbk;
  void *reqPasswordCbkData;
#endif
  GBool hyperlinksEnabled;
  GBool selectEnabled;

  XSplashOutputDev *out;

  int dialogDone;
  
  friend class XPDFView;
};

#endif
