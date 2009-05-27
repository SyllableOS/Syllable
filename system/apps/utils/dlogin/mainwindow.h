#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string>

#include <util/application.h>
#include <gui/window.h>
#include <util/message.h>
#include <storage/nodemonitor.h>

#include "loginimageview.h"
#include "loginview.h"
#include "appsettings.h"

using namespace os;

class MainWindow : public os::Window
{
public:
	MainWindow(const Rect&, AppSettings* pcAppSettings);

	void HandleMessage(os::Message*);
	void TimerTick(int nID);
	void Init();
	void ClearPassword();
	void Authorize( const String& zLoginName, const String& zPassword, const String& zKeymap );
	void ScreenModeChanged( const os::IPoint& cRes, os::color_space eSpace );

	void Refresh()
	{
		pcLoginView->Reload();
	}
	
private:
	bool OkToQuit();
	
	LoginImageView* pcLoginImageView;
	LoginView* pcLoginView;
	NodeMonitor* pcMonitorFile;
	
	AppSettings* pcSettings;
};
#endif






