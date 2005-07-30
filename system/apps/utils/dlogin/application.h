#ifndef APPLICATION_H
#define APPLICATION_H

#include <util/application.h>

#include "mainwindow.h"

class App:public os::Application
{
public:
	App();
	
	void HandleMessage(Message*);
private:
	MainWindow* m_pcMainWindow;
};

#endif

