
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

#ifndef __PLUGINWINDOW_H_
#define __PLUGINWINDOW_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/slider.h>
#include <media/output.h>
#include <media/manager.h>
#include <media/server.h>
#include <vector.h>

class PluginWindow:public os::Window
{
      public:
	PluginWindow( const os::Rect & cFrame );
	 ~PluginWindow();
	virtual void HandleMessage( os::Message * pcMessage );

	bool OkToQuit( void );

      private:
	void ShowData();
	void Close();
	void PluginChange();

	uint32 nInputStart;
	uint32 nOutputStart;
	uint32 nCodecStart;

	os::View * m_pcConfigView;

	// Refresh flag/custom or not

	os::ListView * m_pcPluginList;
};

#endif /* __PLUGINWINDOW_H_ */
