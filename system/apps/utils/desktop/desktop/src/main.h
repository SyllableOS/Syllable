#ifndef MAIN_H
#define MAIN_H


#include <util/application.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/desktop.h>

#include <storage/file.h>
#include <storage/fsnode.h>
#include "include.h"
#include "bitmapwindow.h"
#include <vector>


class DeskApp : public Application
{
	public:
		DeskApp();
		~DeskApp();
	private:
		BitmapWindow* pcWindow;
		
};

#endif










































