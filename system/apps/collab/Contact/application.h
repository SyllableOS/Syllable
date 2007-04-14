#ifndef APPLICATION_H
#define APPLICATION_H

#include <storage/registrar.h>
#include <util/application.h>
#include <gui/window.h>

class App : public os::Application
{
public:
	App( const os::String& cArgv );
private:
	os::Window* m_pcMainWindow;
};

#endif

