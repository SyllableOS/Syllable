#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/tabview.h>
#include <gui/frameview.h>
#include <gui/checkbox.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/slider.h>
#include <gui/button.h>
#include <gui/imagebutton.h>
#include <gui/requesters.h>
#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include <storage/directory.h>
#include <storage/nodemonitor.h>
#include <appserver/keymap.h>
using namespace os;


#include <util/locale.h>
#include <util/settings.h>
#include <storage/file.h>
#include <iostream>
#include <sys/stat.h>

class MainWindow : public os::Window
{
public:
	MainWindow();
	void ShowData();
	void SetCurrent();
	void LoadLanguages();
	void Apply();
	void Undo();
	void Default();
	void HandleMessage( os::Message* );
private:
	bool OkToQuit();
	#include "mainwindowLayout.h"
	NodeMonitor* m_pcMonitor;

	os::String cKeymap;
	os::String cKeymapTemp;
	os::String cKeymapGroup;
	int iKeymapGroup;
	int iDelay, iRepeat, iDelay2, iRepeat2;
	int iOrigRow, iOrigRow2;
	int iAmericanRow;
};

#endif




