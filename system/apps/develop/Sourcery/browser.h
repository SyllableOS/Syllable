/*
	API Browser
	1.0.0
	by Brent P. Newhall <brent@other-space.com>
*/

#ifndef _BROWSER_H
#define _BROWSER_H

#include <gui/window.h>
#include <gui/view.h>
#include <gui/rect.h>
#include <gui/treeview.h>
#include <gui/textview.h>
#include <codeview/codeview.h>
#include <codeview/format_c.h>
#include <gui/splitter.h>
#include <gui/menu.h>
#include <util/application.h>
#include <storage/directory.h>
#include <storage/file.h>
#include <vector>

#include "statusbar.h"
#include "finddialog.h"

using namespace os;
using namespace cv;

const unsigned int MSG_FILE_SELECTED = 1001;

class APIBrowserWindow : public Window
{
public:
	APIBrowserWindow(Window* pcParent, const Rect& r ); /* Constructor */
	void HandleMessage( Message *pcMessage );
	bool OkToQuit();
	void PopulateTree( const String path, const int indent );
	void SetupStatusBar();
	void SetupMenus();
	void Find(const String &pcString, bool bDown, bool bCaseSensitive, bool bExtended);
	void UpdateStatus(const String&);
private:
	Splitter *splitter;
	TreeView *treeView;
	CodeView *pcEditor;
	Menu* menu;
	StatusBar* statusbar;
	FindDialog* findDialog;
	std::vector<String> filenames;
	Window* pcParentWindow;
	String cSearchText;
	bool bSearchDown;
	bool bSearchCase;
	bool bSearchExtended;
	
};

#endif //BROWSER_H





