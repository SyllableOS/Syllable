/*
 *  Mixer 0.1 
 *
 *  Copyright (C) 2001	Arno Klenke
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <atheos/soundcard.h>
#include <util/application.h>
#include <util/string.h>
#include <gui/view.h>
#include <gui/frameview.h>
#include <gui/window.h>
#include <gui/checkbox.h>
#include <gui/slider.h>
#include <gui/button.h>
#include <gui/requesters.h>


#include "channel.h"
#include "view.h"
#include "window.h"

using namespace os;

class MixerApp : public Application
{
	public:
		MixerApp();
		~MixerApp();
};



int main(int argc, char* argv[]){

	MixerApp* pcApp;

	pcApp = new MixerApp();

	pcApp->Run();
	
	return(0);

}

MixerApp::MixerApp() : Application( "application/x-VND.Mixer")
{
	int nFile;
	
	nFile = open( "/dev/sound/mixer", O_RDWR );
	
	if ( nFile < 0 ) {
		Alert *pcAlert = new Alert( "Mixer", "No soundcard found", WND_NOT_RESIZABLE,
				"Ok", NULL );
		pcAlert->Go();  
	        Quit();
	} else {
		close( nFile );
		MixerWindow *pcMainWindow = new MixerWindow(Rect(100,100,500,350));
    	pcMainWindow->Show();
		pcMainWindow->MakeFocus();
	}
}

MixerApp::~MixerApp()
{
	
}
