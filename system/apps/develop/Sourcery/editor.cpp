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

#include "editor.h"
#include "messages.h"

/********************************************
* Description: Editor's constructor, once I figure 
*              shortcuts out this will be utilized.
* Author: 	   Rick Caudill
* Date:        Thu Mar 18 23:10:02 2004
********************************************/
Editor::Editor(const Rect& cRect,Window* pcParent) : cv::CodeView(cRect,"","",CF_FOLLOW_ALL)
{
	pcParentWindow = pcParent;
	SetLineNumberFgColor(get_default_color(COL_SHADOW));
}

/*************************************************************
* Description: Simple mouseup, this is used for drag and drop
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:37:56 
*************************************************************/
void Editor::MouseUp(const os::Point& cPosition, uint32 nButtons, os::Message* pcData)
{
	const char* pzPath;
	if (nButtons == 1 && pcData != NULL && pcData->FindString("file/path",&pzPath)==0)
	{
		Message* pcMessage = new Message(M_LOAD_REQUESTED);
		pcMessage->AddString("file/path",pzPath);
		pcParentWindow->PostMessage(pcMessage,pcParentWindow);
		delete( pcMessage );
	}
}








