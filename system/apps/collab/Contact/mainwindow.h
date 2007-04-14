#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/tabview.h>
#include <gui/dropdownmenu.h>
#include <gui/listview.h>
#include <gui/imagebutton.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <gui/separator.h>
#include <gui/radiobutton.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/toolbar.h>
#include <util/application.h>
#include <util/message.h>
#include <util/settings.h>
#include <util/clipboard.h>
#include <storage/nodemonitor.h>

using namespace os;


class MainWindow : public os::Window
{
public:
	MainWindow( const os::String& cArgv );
	void SaveContact();
	void LoadContact();
	void ClearContact();
	void CheckChanges();
	void LoadContactList();
	void LoadCategoryList();
	void SaveSettings();
	void LoadSettings();
	void HandleMessage( os::Message* );
private:
	bool OkToQuit();
	#include "mainwindowLayout.h"
	NodeMonitor* m_pcMonitor;
	dirent *psEntry;
	DIR *pDir;

	bool bLoadSearch;
	bool bLoadContact;
	bool bLoadStartup;
	bool bContactChanged;
	os::String cChangesChecked;
	os::String cContactPath;
	os::String cContactName;
	os::String cCategory;
};

#endif

