
/*  Syllable Media Preferences
 *  Copyright (C) 2003 Arno Klenke
 *  Based on the preferences code by Daryl Dudey
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __MAIN_H_
#define __MAIN_H_

#include <util/application.h>
#include "mainwindow.h"

class PrefsMediaApp:public os::Application
{
      public:
	PrefsMediaApp();
	~PrefsMediaApp();
	bool OkToQuit();
      private:
	MainWindow * pcWindow;
	// Media manager
	os::MediaManager * m_pcManager;
};

#endif
