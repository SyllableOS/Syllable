//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#include "splash.h"
#include "commonfuncs.h"

using namespace os;

/*************************************************************
* Description: SplashView Constructor... SplashView is just an image in a view
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:05:09 
*************************************************************/
SplashView::SplashView() : View(Rect(0,0,1,1),"",WID_WILL_DRAW)
{
	pcImage = LoadImageFromResource("sourcery.png");
	ResizeTo(pcImage->GetSize());	
}

/*************************************************************
* Description: SplashView Destructor... deletes the image
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:06:10 
*************************************************************/
SplashView::~SplashView()
{
	delete pcImage;
}

/*************************************************************
* Description: Typical Paint()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:05:49 
*************************************************************/
void SplashView::Paint(const Rect& cUpdateRect)
{
	SetDrawingMode(DM_BLEND);
	pcImage->Draw(Point(0,0),this);
	SetDrawingMode(DM_COPY);
}	

/*************************************************************
* Description: SplashWindow Constructor... nothing major
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:06:48 
*************************************************************/
SplashWindow::SplashWindow() : Window(Rect(),"","",WND_NOT_RESIZABLE | WND_NO_BORDER)
{
	pcView = new SplashView();
	AddChild(pcView);
	ResizeTo(pcView->GetBounds().Width(),pcView->GetBounds().Height());
}

/*************************************************************
* Description: Called when the window closes
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:07:16 
*************************************************************/
bool SplashWindow::OkToQuit()
{
	return true;
}
