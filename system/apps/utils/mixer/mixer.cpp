/*
 *  Media Mixer Controls - Instruct the Media Server to display the Mixer window
 *
 *  Copyright (C) 2005	Kristian Van Der Vliet
 *  Copyright (C) 2004	Arno Klenke
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

#include <iostream>
#include <unistd.h>
#include <media/manager.h>
#include <media/server.h>
#include <util/application.h>

using namespace os;

class Mixer : public Application
{
	public:
		Mixer();
		~Mixer(){};
		bool OkToQuit();
	private:
		os::MediaManager * m_pcManager;
};

Mixer::Mixer() : Application( "application/x-VND.Mixer")
{
	// Create media manager 
	m_pcManager = new os::MediaManager();

	if ( !m_pcManager->IsValid() )
	{
		std::cout << "Media server not running" << std::endl;
		PostMessage( os::M_QUIT );
		return;
	}

	m_pcManager->GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_SHOW_CONTROLS );

	// The mixer window is under the control of the Media Server, so we need to cause
	// ourselves to quit now
	Application::GetInstance()->PostMessage( os::M_QUIT );
}

bool Mixer::OkToQuit()
{
	if( m_pcManager )
		delete( m_pcManager );
	return ( true );
}

int main(int argc, char* argv[])
{
	Mixer* pcApp;
	pcApp = new Mixer();
	pcApp->Run();

	return EXIT_SUCCESS;
}

