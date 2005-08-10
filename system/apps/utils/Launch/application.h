#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <util/application.h>
#include "mainwindow.h"

using namespace os;

class RunApplication : public Application
{
public:
	RunApplication();

	MainWindow* GetWindow() { return pcMainWindow;}
private:
	MainWindow* pcMainWindow;
};

#endif
