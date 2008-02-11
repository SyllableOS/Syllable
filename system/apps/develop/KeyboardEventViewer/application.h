#ifndef APPLICATION_H
#define APPLICATION_H

#include <util/application.h>
#include <gui/window.h>

class App : public os::Application
{
public:
	App();
private:
	os::Window* m_pcMainWindow;
};

#endif
