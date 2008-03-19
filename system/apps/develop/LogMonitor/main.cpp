/*
 *  ATail - Graphic replacement for tail
 *  Copyright (C) 2001 Henrik Isaksson
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include <gui/window.h>
#include <gui/view.h>
#include <gui/rect.h>
#include <gui/desktop.h>
#include <util/message.h>
#include <util/application.h>
#include <util/optionparser.h>
#include <iostream>
#include <cstdlib>

using namespace os;
using namespace std;

#include "tailwin.h"
#include "resources/LogMonitor.h"

class MyApp:public Application
{
	public:
	MyApp();
	~MyApp();

	void Initialize();
	void SetDesktop(uint32 d) { m_DesktopMask = d?(1L << (d-1)):~0; }
	void SetFileName(const string &f) { m_FileName = f; }
	void SetFullscreen() { m_Fullscreen = true; }

	void SetX(uint32 x) { m_X = x; m_SetX = true; }
	void SetY(uint32 x) { m_Y = x; m_SetY = true; }
	void SetW(uint32 x) { m_W = x; m_SetW = true; }
	void SetH(uint32 x) { m_H = x; m_SetH = true; }

	private:
	Window	*m_Window;
	uint32	m_DesktopMask;
	string	m_FileName;
	bool	m_Fullscreen;

	bool	m_SetX;
	bool	m_SetY;
	bool	m_SetW;
	bool	m_SetH;
	uint32	m_X;
	uint32	m_Y;
	uint32	m_W;
	uint32	m_H;
};

MyApp::MyApp()
	:Application("application/x-vnd.syllable.Monitor")
{
	m_Window = NULL;
	m_DesktopMask = CURRENT_DESKTOP;
	m_Fullscreen = false;
	m_SetX = m_SetY = m_SetW = m_SetH = false;
	m_FileName = "/var/log/kernel";
	
	SetCatalog( "LogMonitor.catalog" );
}

void MyApp::Initialize()
{
	Desktop desktop;
	Rect trect(50,50,desktop.GetResolution().x-100,200);

	if(m_SetX)
		trect.left = m_X;
	if(m_SetY)
		trect.top = m_Y;
	if(m_SetW)
		trect.right = trect.left + m_W;
	if(m_SetH)
		trect.bottom = trect.top + m_H;

	if(m_Fullscreen) {
		trect.left = 0;
		trect.top = 0;
		trect.right = desktop.GetResolution().x;
		trect.bottom = desktop.GetResolution().y;
	}

	m_Window = new TailWin(trect, m_FileName, m_DesktopMask);
	m_Window->Show();
}

MyApp::~MyApp()
{
}

int main(int ac, char **av)
{
	MyApp *ThisApp = new MyApp();

	OptionParser args(ac, av);
	
	args.AddArgMap("desktop", 'd', STR_OPTION_DESKTOP);
	args.AddArgMap("fullscreen", 'f', STR_OPTION_FULLSCREEN);
	args.AddArgMap("help", 'h', STR_OPTION_HELP);
	args.AddArgMap("version", 'v', STR_OPTION_VERSION);

	args.AddArgMap("left", 'x', STR_OPTION_LEFT);
	args.AddArgMap("top", 'y', STR_OPTION_TOP);
	args.AddArgMap("width", 'w', STR_OPTION_WIDTH);
	args.AddArgMap("height", 'h', STR_OPTION_HEIGHT);
	
	args.ParseOptions("dfhvxywh");

	const struct OptionParser::option *opt;

	if((opt = args.FindOption('v'))) {
		std::cout << "Monitor: " << ATAIL_VERSION << endl;
		return 0;
	}
	
	if((opt = args.FindOption('h')) /*|| args.GetFileCount() == 0*/) {
		cout << STR_USAGE.c_str() << endl;
		cout << STR_EXAMPLE.c_str() << endl;
		cout << endl;
		args.PrintHelpText();
		return 0;
	}
	
	args.RewindOptions();
	while((opt = args.GetNextOption())) {
		uint32 intval = atoi(opt->value.c_str());
		switch(opt->opt) {
			case 'd':
				if(opt->has_value && intval < 32) {
					ThisApp->SetDesktop(intval);
				} else {
					cout << STR_ERROR_DESKTOP_NUMBER.c_str() << endl;
				}
				break;
			case 'f':
				ThisApp->SetFullscreen();
				break;
			case 'x':
				ThisApp->SetX(intval);
				break;
			case 'y':
				ThisApp->SetY(intval);
				break;
			case 'w':
				if(intval > 50) {
					ThisApp->SetW(intval);
				} else {
					cout << STR_ERROR_WINDOW_WIDTH.c_str() << endl;
				}
				break;
			case 'h':
				if(intval > 20) {
					ThisApp->SetH(intval);
				} else {
					cout << STR_ERROR_WINDOW_HEIGHT.c_str() << endl;
				}
				break;
		}
	}

	if (args.GetFileCount() != 0)
		ThisApp->SetFileName(args[0]);
	
	ThisApp->Initialize();
	ThisApp->Run();

	return 0;
}






