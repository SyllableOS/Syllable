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

#include <gui/button.h> //libsyllable headers

#include "aboutbox.h"
#include "commonfuncs.h"
#include "messages.h"
#include <gui/requesters.h>

using namespace os;

/*************************************************
* Description: View that will be displayed in About Box
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
AboutBoxView::AboutBoxView() : View(Rect(0,0,310,200),"")
{
	pcBitmap = LoadImageFromResource("icon48x48.png");
	Button* pcOk = new Button(Rect(100,100,125,125),"ok","_Okay",new Message(M_OK));
	Button* pcContributors = new Button(Rect(100,100,125,125),"","_Credits",new Message(M_CREDITS));

	pcContributors->SetFrame(Rect(250-pcContributors->GetPreferredSize(false).x-5,160,250,pcContributors->GetPreferredSize(false).y+160));
	pcOk->SetFrame(Rect(59,160,155,pcContributors->GetPreferredSize(false).y+160));

	AddChild(pcOk);
	AddChild(pcContributors);
	
	pcOk->SetTabOrder(NEXT_TAB_ORDER);
	pcContributors->SetTabOrder(NEXT_TAB_ORDER);
	
	float vWidth;
	vWidth = 20 + pcBitmap->GetBounds().Width();
	Link* pcLink = new Link("http://www.syllable-desk.tk/sourcery.html","Sourcery v0.1",new Message(M_LINK));
	pcLink->SetFrame(Rect(vWidth,20,pcLink->GetPreferredSize(true).x+vWidth,pcLink->GetPreferredSize(true).y+20));	
	AddChild(pcLink);
	
}

AboutBoxView::~AboutBoxView()
{
	delete( pcBitmap );
}

/*************************************************
* Description: Paints the view
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void AboutBoxView::Paint(const Rect& cUpdateRect)
{
	float vWidth;
	vWidth = 20 + pcBitmap->GetBounds().Width();
	
	Rect cPaintRect = Rect(0,0,pcBitmap->GetBounds().Width()+5,GetBounds().Height());
	FillRect( GetBounds(), get_default_color( COL_NORMAL ) );
	

	SetDrawingMode( DM_BLEND );
	SetFgColor(Tint(Color32_s(120,120,120,255),-2));
	FillRect(cPaintRect);
	pcBitmap->Draw(Point(5,5),this);


	SetFgColor(0,0,0,0);

	String cInfo=(String)"Code Editor for Syllable\nCopyright 2003-2004 Rick Caudill\n\nSourcery is released under the GNU General\nPublic License. Please see the file COPYING,\ndistrubuted with Sourcery, or http:://gnu.org\nfor more information.";
	SetDrawingMode( DM_OVER );
	DrawText(Rect(vWidth,10,GetBounds().Width(), GetBounds().Height()),cInfo,DTF_ALIGN_LEFT);
}

/*************************************************
* Description: About box window
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
AboutBoxWindow::AboutBoxWindow(Window* pcParent) : Window(Rect(0,0,310,200),"",String("About ") + String(APP_NAME) + String(" ") + String(APP_VERSION),WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT)
{
	os::BitmapImage* pcIcon = LoadImageFromResource("icon24x24.png");
	SetIcon(pcIcon->LockBitmap());
	delete( pcIcon );
	pcParentWindow = pcParent;
	pcView = new AboutBoxView();
	AddChild(pcView);	
}

/*************************************************
* Description: handles the messages in the aboutbox window
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void AboutBoxWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_OK:
			OkToQuit();
			break;
	
		case M_CREDITS:
		{
			os::BitmapImage* pcImage = LoadImageFromResource("icon48x48.png");
			Alert* pcCredits = new Alert("Credits...","Thanks goes out to:\n\n    Brent P. Newhall      -   For his API Browser\n\n    Henrik Isakson         -   For his ColorWheel and Wisdom\n\n    The Syllable Team -   For always being there when I\n                                           didn't know something\n\n",pcImage->LockBitmap(),0,"Okay",NULL);
			pcCredits->CenterInWindow(this);
			pcCredits->Go(new Invoker());
			pcCredits->MakeFocus();
			break;
		}
		case M_LINK:
			break;
	}
}

/*************************************************
* Description: Called when the dialog quits
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
bool AboutBoxWindow::OkToQuit()
{
	pcParentWindow->PostMessage(new Message(M_APP_ABOUT_REQUESTED),pcParentWindow);
	return true;
}










