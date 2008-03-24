#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/image.h>
#include <util/resources.h>
#include <util/application.h>
#include <util/message.h>

class MainWindow : public os::Window
{
public:
	MainWindow();
	void HandleMessage( os::Message* );
private:
	bool OkToQuit();  // Obsolete?
	#include "mainwindowLayout.h"
};

#endif

