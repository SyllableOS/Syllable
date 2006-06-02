//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <util/message.h>
#include <util/resources.h>
#include <gui/image.h>
#include <util/string.h>
#include <gui/menu.h>
#include <storage/file.h>
#include <gui/requesters.h>
#include <gui/imagebutton.h>
#include <gui/popupmenu.h>
#include <gui/filerequester.h>
#include <util/settings.h>
#include <gui/checkmenu.h>
#include <gui/toolbar.h>
#include <storage/nodemonitor.h>

#include <codeview/codeview.h>


#include "editor.h"
#include "statusbar.h"
#include "convertdialog.h"
#include "settings.h"
#include "commonfuncs.h"
#include "gotodialog.h"
#include "finddialog.h"
#include "replacedialog.h"
#include "fileproperties.h"
#include "aboutbox.h"
#include "userinserts.h"
#include "browser.h"
#include "settingsdialog.h"
#include "toolsdialog.h"

using namespace os;

class SourceryFile;
struct ToolHandle;

class MainWindow : public os::Window
{
public:
	MainWindow(AppSettings*,const os::String&);
	~MainWindow();
	
	void 				HandleMessage( os::Message* );
 	void				DispatchMessage(Message*, Handler*);
 	virtual void		TimerTick(int);
	bool				Find(const String &pcString, bool bDown, bool bCaseSensitive, bool bExtended);
	void				GoTo(uint, uint);
	void				ZoomOut();
	void				ZoomIn();
	
private:
	void 				SetupMenus();
	void 				SetupToolBar();
	void 				SetupStatusBar();
	bool 				OkToQuit();
	void 				Load(const os::String&,bool);
	void				Save(const os::String&);
	void				CloseFile();
	
	void				UpdateTitle();
	void				UpdateStatus(const String&,bigtime_t);
	void				LoadSettings();
	void				LoadDefaults();
	void				SaveSettings();
	void				LoadFormat();
	void				LaunchConvertor();
	void				OpenCorrespondingFile();
	void				ShowError(int);
	
	void				AddLanguageInformation();
	void				ReloadCustomItems(bool bFirstTime);
	Message*			LoadCustomItemMessage(const String& cInsertedText);
	void				ReloadCustomTools(bool bFirstTime);
	Message*			LoadCustomToolsMessage(ToolHandle*);
	Menu*				GetInsertMenu();
	Menu*				GetFindMenu();
	void				LoadNewFormat(const String&);
	void				ReloadRecentMenu();
	vector<String>		GetRecentList();
	void				SetRecentList(const String&);
	
	void				Init();
	
	//gui elements
	Editor* 			pcEditor;
	ToolBar* 			m_pcToolBar;
	StatusBar* 			m_pcStatusBar;
	APIBrowserWindow*   m_pcBrowserWindow;
	SettingsDialog*		m_pcSettingsDialog;
	Menu* 				m_pcPopMenu;
	Menu* 				m_pcContextMenu;
	Menu* 				m_pcMenu;
	Menu* 				m_pcAppMenu;
	Menu* 				m_pcFileMenu;
	Menu* 				m_pcEditMenu;
	Menu* 				m_pcViewMenu;
	Menu* 				m_pcFindMenu;
	Menu* 				m_pcHelpMenu;
	Menu* 				m_pcInsertMenu;
	Menu* 				m_pcInsertMenuTwo;	
	Menu* 				m_pcCommentMenu;	
	Menu*				m_pcLanguageMenu;
	Menu*				m_pcInsertCustomMenu;
	Menu*				m_pcDateMenu;
	Menu*				m_pcToolMenu;
	Menu*				m_pcTemplateMenu;
	Menu*				pcRecentMenu;
	Menu*				m_pcFormatMenu;
	
	CheckMenu*			m_pcMainToolBarCheck;
	CheckMenu*			m_pcFindToolBarCheck;
	CheckMenu*			m_pcShowLineNumberCheck;
	
	FileRequester* 		pcOpenRequester;
	FileRequester*		pcSaveRequester;
	Settings*			pcSettings;
	ConvertDialog*		pcConvertDialog;
	GoToDialog*			pcGoToDialog;
	FindDialog*			pcFindDialog;
	FileProp*			pcFileProp;
	ReplaceDialog*		pcReplaceDialog;
	AboutBoxWindow*		pcAbout;
	UserInserts*		pcUserInserts;
	ToolsWindow*		pcToolsWindow;
	
	//settings variables
	AppSettings* pcMainSettings;
	
	//file variables
	SourceryFile* sFile;
	String cFile;
	String cConvertor;
	String cSearchText;
	NodeMonitor* pcMonitorFile;

	bool bChanged;
	bool bNamed;
	bool bFound;
	bool bSearchDown;
	bool bSearchCase;
	bool bSearchExtended;
	int nMonitor;
	Message* pcPassedMessage;
	
	// events
	os::Event* pcGetFileEvent;
	os::Event* pcMakeFocusEvent;
};

#endif
































