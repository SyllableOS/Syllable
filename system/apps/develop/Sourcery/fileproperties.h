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

#ifndef _FILE_PROPERTIES_H
#define _FILE_PROPERTIES_H

#include <gui/window.h>
#include <gui/stringview.h>
#include <util/string.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <storage/file.h>
#include <gui/button.h>
#include <gui/font.h>
#include "commonfuncs.h"
#include "file.h"

using namespace os;

class FileProp : public Window
{
public:
	FileProp(Window*, Message*);
	virtual void HandleMessage(Message*);
	
private:
	void _Layout();
	
	LayoutView* pcLVMain;
	VLayoutNode* pcVLMain, *pcVLFilePath;
	HLayoutNode* pcHLTimeStamp, *pcHLButton, *pcHLSizeAndLines, *pcHLRelative, *pcHLAbsolute; 	
	FrameView* pcFVFilePath;
	
	StringView *pcSVRelativeOne,*pcSVRelativeTwo;
	StringView* pcSVAbsoluteOne,*pcSVAbsoluteTwo;
	StringView* pcSVSizeOne, *pcSVSizeTwo;
	StringView* pcSVLinesOne,*pcSVLinesTwo;
	StringView* pcSVCommentCountOne, *pcSVCommentCountTwo;
	StringView* pcSVActualCodeOne, *pcSVActualCodeTwo;
	StringView* pcSVTimeOne, *pcSVTimeTwo;
	Button* pcOk;
	Message* pcFileMessage;
	Window* pcParentWindow;

	Font* pBoldFont;
}; 

#endif  //_FILE_PROPERTIES_H
