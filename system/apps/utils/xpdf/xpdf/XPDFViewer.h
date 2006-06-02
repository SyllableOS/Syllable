//========================================================================
//
// XPDFViewer.h
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFVIEWER_H
#define XPDFVIEWER_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <gui/window.h>
#include <gui/view.h>
#include <gui/menu.h>
#include <gui/toolbar.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/image.h>
#include <gui/imageview.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/imagebutton.h>
#include <gui/textview.h>
#include <util/resources.h>
#include <util/clipboard.h>
#include "gtypes.h"
#include "XPDFCore.h"
#include "finddialog.h"

#define DISABLE_OUTLINE

class GString;
class GList;
class UnicodeMap;
class LinkDest;
class XPDFApp;

//------------------------------------------------------------------------

// NB: this must match the defn of zoomMenuBtnInfo in XPDFViewer.cc
#define nZoomMenuItems 10

//------------------------------------------------------------------------
// XPDFViewer
//------------------------------------------------------------------------

class XPDFViewer {
public:

  XPDFViewer(XPDFApp *appA, GString *fileName,
	     int pageA, GString *destName,
	     GString *ownerPassword, GString *userPassword);
  GBool isOk() { return ok; }
  ~XPDFViewer();

  void open(GString *fileName, int pageA, GString *destName);
  void clear();
  void reloadFile();

  os::Window* getWindow() { return win; }

private:

  //----- load / display
  GBool loadFile(GString *fileName, GString *ownerPassword = NULL,
		 GString *userPassword = NULL);
  void displayPage(int pageA, double zoomA, int rotateA,
                   GBool scrollToTop, GBool addToHist);
  void displayDest(LinkDest *dest, double zoomA, int rotateA,
		   GBool addToHist);
  void getPageAndDest(int pageA, GString *destName,
		      int *pageOut, LinkDest **destOut);
#ifndef __SYLLABLE__
  //----- password dialog
  static GString *reqPasswordCbk(void *data, GBool again);
#endif
  //----- actions
  static void actionCbk(void *data, char *action);

  //----- keyboard/mouse input

  static void keyPressCbk(void *data, const char *s, const char* raw,
			  uint32 modifiers);
#ifndef __SYLLABLE__		  
	static void mouseCbk(void *data, XEvent *event);
#endif			  
  

  //----- GUI code: main window
  void initWindow();
  void mapWindow();
  void closeWindow();
  int getZoomIdx();
  void setZoomIdx(int idx);
  void setZoomVal(double z);

  static void prevPageCbk(void* ptr);
  static void prevTenPageCbk(void* ptr);
  static void nextPageCbk(void* ptr);
  static void nextTenPageCbk(void* ptr);
#ifndef __SYLLABLE__
  static void backCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void forwardCbk(Widget widget, XtPointer ptr,
			 XtPointer callData);
#if USE_COMBO_BOX
  static void zoomComboBoxCbk(Widget widget, XtPointer ptr,
			      XtPointer callData);
#else
  static void zoomMenuCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
#endif
  static void findCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void printCbk(Widget widget, XtPointer ptr,
		       XtPointer callData);
  static void aboutCbk(Widget widget, XtPointer ptr,
		       XtPointer callData);
  static void quitCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void openCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void openInNewWindowCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void reloadCbk(Widget widget, XtPointer ptr,
			XtPointer callData);
  static void saveAsCbk(Widget widget, XtPointer ptr,
			XtPointer callData);
  static void rotateCCWCbk(Widget widget, XtPointer ptr,
			   XtPointer callData);
  static void rotateCWCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  static void closeCbk(Widget widget, XtPointer ptr,
		       XtPointer callData);
  static void closeMsgCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
#endif	
  static void pageNumCbk( void* ptr, int pg );
		
  static void updateCbk(void *data, GString *fileName,
			int pageNum, int numPages, char *linkString);

  //----- GUI code: outline
#ifndef DISABLE_OUTLINE
  void setupOutline();
  void setupOutlineItems(GList *items, Widget parent, UnicodeMap *uMap);
  static void outlineSelectCbk(Widget widget, XtPointer ptr,
			       XtPointer callData);
#endif

  //----- GUI code: "about" dialog
  void initAboutDialog();

  //----- GUI code: "open" dialog
  void initOpenDialog();
  void setOpenDialogDir(char *dir);
  void mapOpenDialog(GBool openInNewWindowA);
#ifndef __SYLLABLE__
  static void openOkCbk(Widget widget, XtPointer ptr,
			XtPointer callData);
#endif

  //----- GUI code: "find" dialog
  void initFindDialog();
#ifndef __SYLLABLE__
  static void findFindCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
#endif
  void doFind(GBool next);
#ifndef __SYLLABLE__
  static void findCloseCbk(Widget widget, XtPointer ptr,
			   XtPointer callData);
#endif
  //----- GUI code: "save as" dialog
  void initSaveAsDialog();
  
  void setSaveAsDialogDir(char *dir);

  void mapSaveAsDialog();
  #if 0
  static void saveAsOkCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
#endif
  //----- GUI code: "print" dialog
  void initPrintDialog();
  void setupPrintDialog();
#ifndef __SYLLABLE__
  static void printWithCmdBtnCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void printToFileBtnCbk(Widget widget, XtPointer ptr,
				XtPointer callData);
  static void printPrintCbk(Widget widget, XtPointer ptr,
			    XtPointer callData);
#endif
  //----- GUI code: password dialog
  void initPasswordDialog();
#ifndef __SYLLABLE__
  static void passwordTextVerifyCbk(Widget widget, XtPointer ptr,
				    XtPointer callData);
  static void passwordOkCbk(Widget widget, XtPointer ptr,
			    XtPointer callData);
  static void passwordCancelCbk(Widget widget, XtPointer ptr,
				XtPointer callData);
#endif				
  void getPassword(GBool again);

  //----- Motif support
  //XmFontList createFontList(char *xlfd);

  XPDFApp *app;
  GBool ok;
#ifndef __SYLLABLE__
  Display *display;
  int screenNum;
  Widget win;			// top-level window
  Widget form;
  Widget panedWin;
#endif
  os::Window* win;
  os::View* form;
  int zoom;
  os::FileRequester* openDialog;
  os::FindDialog* findDialog;
#ifndef DISABLE_OUTLINE
  Widget outlineScroll;
  Widget outlineTree;
  Widget *outlineLabels;
  int outlineLabelsLength;
  int outlineLabelsSize;
#endif
  XPDFCore *core;
#ifndef __SYLLABLE__
  Widget toolBar;
  Widget backBtn;
  Widget prevTenPageBtn;
  Widget prevPageBtn;
  Widget nextPageBtn;
  Widget nextTenPageBtn;
  Widget forwardBtn;
  Widget pageNumText;
  Widget pageCountLabel;
#if USE_COMBO_BOX
  Widget zoomComboBox;
#else
  Widget zoomMenu;
  Widget zoomMenuBtns[nZoomMenuItems];
#endif
  Widget findBtn;
  Widget printBtn;
  Widget aboutBtn;
  Widget linkLabel;
  Widget quitBtn;
  Widget popupMenu;

  Widget aboutDialog;
  XmFontList aboutBigFont, aboutVersionFont, aboutFixedFont;

  Widget openDialog;
#endif  
  GBool openInNewWindow;
#ifndef __SYLLABLE__
  Widget findDialog;
  Widget findText;

  Widget saveAsDialog;

  Widget printDialog;
  Widget printWithCmdBtn;
  Widget printToFileBtn;
  Widget printCmdText;
  Widget printFileText;
  Widget printFirstPage;
  Widget printLastPage;

  Widget passwordDialog;
  Widget passwordMsg;
  Widget passwordText;
#endif  
  int passwordDone;
  GString *password;
  
  friend class XPDFWindow;
  friend class XPDFLooper;
};

#endif
