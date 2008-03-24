#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <gui/image.h>
#include <util/message.h>
#include <util/resources.h>

class MainWindow : public os::Window
{
public:
	MainWindow();
	void HandleMessage( os::Message* );
private:
	bool OkToQuit(); // Obsolete?
};

#endif

