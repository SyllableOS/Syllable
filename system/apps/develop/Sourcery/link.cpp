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

#include "link.h"
#include <util/application.h>
#include <util/message.h>
using namespace os;

#define MOUSE_HEIGHT 32
#define MOUSE_WIDTH 32

uint8 g_anMouseImage[]={
#include "pixmaps/mouse.h"
};

//private class for the link class
class Link::Private
{
public:
	Private()
	{
		cLink = "http://";
		cLinkName = "http://";
		sBgColor = get_default_color(COL_NORMAL);
		sFgColor = get_default_color(COL_SEL_MENU_BACKGROUND);
		sHighlightColor = Color32_s(255,255,255,0);
		m_bTracking = false;	
	}
	
	String cLink;
	String cLinkName;
	Color32_s sBgColor;
	Color32_s sFgColor;
	Color32_s sHighlightColor;
	bool m_bTracking;
};

/****************************************
* Description: Hopefully noone else uses this class because it is a horrible way to do 
* Author: Rick Caudill
* Date: Fri Mar 19 15:45:18 2004
****************************************/
Link::Link(Message* pcMessage) : Control(Rect(0,0,0,0),"cLink","",pcMessage,CF_FOLLOW_LEFT|CF_FOLLOW_TOP,WID_WILL_DRAW | WID_CLEAR_BACKGROUND )
{
	m = new Private;
}

/****************************************
* Description: Hopefully noone else uses this class because it is a horrible way to do 
* Author: Rick Caudill
* Date: Fri Mar 19 15:45:18 2004
****************************************/
Link::Link(const String& cLink, const String& cLinkName, Message* pcMessage) : Control(Rect(0,0,0,0),String(String(cLink.c_str()) + (String)"_" + String(cLinkName.c_str())),cLink,pcMessage,CF_FOLLOW_LEFT|CF_FOLLOW_TOP,WID_WILL_DRAW | WID_CLEAR_BACKGROUND)
{
	m = new Private;
	m->cLink = cLink;
	m->cLinkName = cLinkName;
}

/****************************************
* Description: Hopefully noone else uses this class because it is a horrible way to do 
* Author: Rick Caudill
* Date: Fri Mar 19 15:45:18 2004
****************************************/
Link::Link(const String& cLink, const String& cLinkName,Message* pcMessage,Color32_s sBgColor, Color32_s sFgColor) : Control(Rect(0,0,0,0),String(String(cLink.c_str()) + (String)"_" + String(cLinkName.c_str())),cLink,pcMessage,CF_FOLLOW_LEFT|CF_FOLLOW_TOP,WID_WILL_DRAW | WID_CLEAR_BACKGROUND)
{
	m = new Private;
	m->cLink = cLink;
	m->cLinkName = cLinkName;
	m->sBgColor = sBgColor;
	m->sFgColor = sFgColor;
}

Link::~Link()
{
	delete( m );
}

/*************************************************************
* Description: Simple GetPreferredSize()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:42:39 
*************************************************************/
Point Link::GetPreferredSize(bool bSize) const
{
	font_height sHeight;
	float vWidth=0.0f;
	float vHeight=0.0f;
	vWidth = GetStringWidth(m->cLinkName.c_str());
	GetFontHeight( &sHeight );
	vHeight = sHeight.ascender + sHeight.descender;
	return (Point(vWidth,vHeight+2));
}

/*************************************************************
* Description: Simple Paint()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:43:04 
*************************************************************/
void Link::Paint(const Rect& cUpdateRect)
{
	String cLink = _DrawUnderlines();
	SetFgColor(m->sBgColor);
	FillRect(GetBounds());
	
	if (m->m_bTracking == true)		
		SetFgColor(m->sHighlightColor);	
	else
		SetFgColor(m->sFgColor);

	DrawText(GetBounds(),cLink, DTF_UNDERLINES);
}

/*************************************************************
* Description: Simple MouseDown()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:44:01 
*************************************************************/
void Link::MouseDown(const Point& cPos, uint32 nButtons)
{
	if (nButtons != 1)
	{	
		View::MouseDown(cPos,nButtons);
		return;
	}
	MakeFocus( true );
	if (GetBounds().DoIntersect(cPos))
	{
		if (GetMessage() != NULL)
		{
			GetMessage()->RemoveName("link");
			GetMessage()->AddString("link",m->cLink);
		}

		SetValue(!GetValue().AsBool());
	} 
}

/*************************************************************
* Description: Simple MouseUp()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:44:20 
*************************************************************/
void Link::MouseUp(const Point& cPos, uint32 nButtons, Message *pcData )
{
	View::MouseUp(cPos,nButtons,pcData);
}

/*************************************************************
* Description: Simple MouseMove()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:44:50 
*************************************************************/
void Link::MouseMove(const Point & cPos, int nCode, uint32 nButtons, Message* pcData)
{
	if (nCode  == MOUSE_ENTERED && !m->m_bTracking)
	{						
		//Application::GetInstance()->PushCursor(MPTR_RGB32,g_anMouseImage,MOUSE_WIDTH,MOUSE_HEIGHT,IPoint((int)MOUSE_WIDTH, (int)cPos.y));			
		m->m_bTracking = true;
		Paint(GetBounds());
		Flush();
	}
	
	else if (nCode == MOUSE_EXITED)
	{
		//Application::GetInstance()->PopCursor();
		m->m_bTracking = false;
		Paint(GetBounds());
		Flush();
	}
}

/*************************************************************
* Description: Just Flushes the render queue
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:45:10 
*************************************************************/
void Link::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}

/*************************************************************
* Description: Underlines every letter
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:45:36 
*************************************************************/
String Link::_DrawUnderlines()
{
	String cReturn="";
	String cStarterString=m->cLinkName;

	for (uint i=0; i<cStarterString.size(); i++)
	{
		cReturn += "_";
		cReturn += cStarterString[i];
	}
	return cReturn;
}















