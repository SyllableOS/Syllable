#ifndef APPLICATION_H
#define APPLICATION_H

#include <util/application.h>
#include <storage/file.h>

#include "appsettings.h"
#include "mainwindow.h"

class App:public os::Application
{
public:
	App( int nArgc, char* apzArgv[] );
	
	void HandleMessage(Message*);
	void Authorize( const String& zUserName, const String& zPassword, const String& zKeymap );
	void AutoLogin( const String& zUserName );
	
	void LoadSettings();
	
	bool OkToQuit()
	{
		return false;
	}
	
	AppSettings* GetSettings();
private:
	MainWindow* m_pcMainWindow;
	
	AppSettings* m_pcSettings;
};

#endif

