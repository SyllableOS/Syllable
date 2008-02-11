#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <gui/image.h>
#include <util/message.h>
#include <util/resources.h>
#include <gui/layoutview.h>
#include <gui/button.h>

#include <appserver/keymap.h>
#include "view.h"



class MainWindow : public os::Window
{
public:
	MainWindow();
	void HandleMessage( os::Message* );

private:
	bool OkToQuit();
	
	os::LayoutView* pcLayout;
	EventView* pcView;
	os::Button* pcRefresh;
};

#endif




