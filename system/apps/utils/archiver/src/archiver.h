#ifndef _ARCHIVER_H
#define _ARCHIVER_H

#include <gui/listview.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/filerequester.h>
#include <gui/desktop.h>
#include <gui/requesters.h>
#include <gui/imagebutton.h>
#include <fstream.h>
#include <gui/menu.h>
#include <util/application.h>

#include <gui/image.h>
#include <util/resources.h>
#include <stdio.h>
#include <util/locker.h>


#include "new.h"
#include "options.h"
#include "exe.h"
#include "extract.h"
#include "statusbar.h"
#include "messages.h"
#include "loadbitmap.h"
#include "toolbar.h"
using namespace os;

class AppSettings;

class AListView : public ListView{
public:
    AListView(const Rect & r);
};


class ArcWindow : public Window{
public:
    ArcWindow(AppSettings*);
    ~ArcWindow();
    AListView* pcView;
    void LoadAtStart( char* cFileName );
private:
    void 	Load(char* cFileName);
    void 	HandleMessage( Message* pcMessage);
	void 	DispatchMessage(Message*, Handler*);
    bool 	OkToQuit();
    void 	ListFiles( char* cFileName);
	void 	SetupToolBar();
	void 	SetupMenus();
	void 	SetupStatusBar();


    OptionsWindow* m_pcOptions;
    StatusBar* pcStatus;
    ToolBar* pcButtonBar;
    Menu* m_pcMenu;
    Menu*  aMenu;
    Menu*  fMenu;
    Menu*  actMenu;
    Menu*  hMenu;
    NewWindow* m_pcNewWindow;
    ExeWindow* m_pcExeWindow;
    ExtractWindow* m_pcExtractWindow;
    FileRequester* pcOpenRequest;

	bool Lst;
	Rect cMenuFrame;
	const char* pzFPath;
	char pzStorePath[1024];

	char tmp[8][1024];
	char trash[2048];
	int jas;

	AppSettings* pcAppSettings;
};

#endif


































