// Eyes -:-  (C)opyright 2005 Jonas Jarvoll
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <util/application.h>
#include <gui/rect.h>
#include <gui/requesters.h>
#include <util/looper.h>

#include "eyewidget.h"
#include "messages.h"
#include <appserver/dockplugin.h>

using namespace os;



#define OUR_TIMER 0
#define UPDATE_RATE 50000

class EyeLooper : public os::Looper
{
public:
	EyeLooper(EyeWidget* pcWidget) : os::Looper("EyeLooper")
	{
		pcMainWidget = pcWidget;

		AddTimer(this, OUR_TIMER, UPDATE_RATE, false);
	}

	void TimerTick(int nCode)
	{
		if (nCode == OUR_TIMER)
			pcMainWidget->UpdateEyes();
	}
private:
	EyeWidget* pcMainWidget;			
};

class EyeDockPlugin : public os::DockPlugin
{
public:
	EyeDockPlugin()
	{
	}
	
	status_t Initialize()
	{
		pcEyeWidget = new EyeWidget("eye",CF_FOLLOW_NONE);
		pcLooper = new EyeLooper(pcEyeWidget);
		AddView(pcEyeWidget);
		pcLooper->Run();
		return( 0 );
	}
	void Delete()
	{
		pcLooper->Terminate();
		RemoveView(pcEyeWidget);
	}

	os::String GetIdentifier()
	{
		return( "Eyes" );
	}
private:
	EyeWidget* pcEyeWidget;
	EyeLooper* pcLooper;	
};

extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new EyeDockPlugin() );
}
}



