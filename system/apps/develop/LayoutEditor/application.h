#ifndef APPLICATION_H
#define APPLICATION_H

#include <util/application.h>
#include <gui/window.h>

class MainWindow;

class App : public os::Application
{
public:
	App( os::String zFileName, bool bLoad );
	MainWindow* m_pcMainWindow;	
private:
};

#endif




