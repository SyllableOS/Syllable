//  AView 0.2 -:-  (C)opyright 2000-2001 Kristian Van Der Vliet
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free
//  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//  MA 02111-1307, USA
//

#include <util/application.h>
#include "crect.h"
#include "window.h"

using namespace os;

class ImageApp : public Application
{
public:
    ImageApp(char *fileName);
    ~ImageApp();

private:
    AppWindow* m_pcMainWindow;

};

ImageApp::ImageApp(char *fileName) : Application( "application/x-VND.RGC-AView")
{
    m_pcMainWindow = new AppWindow(CRect(400,400));

    m_pcMainWindow->AddItems(m_pcMainWindow);
    if (fileName!=NULL)
        m_pcMainWindow->Load(fileName);

    m_pcMainWindow->Show();
    m_pcMainWindow->MakeFocus();

}

ImageApp::~ImageApp()
{}







