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


#include "fileproperties.h"
#include "messages.h"

#include <gui/font.h>
using namespace os;

/************************************ 
* Description: FileProperties Constructor
* Author: Rick Caudill
* Date: Tue Mar 16 09:46:37 2004
************************************/
FileProp::FileProp(Window* pcWindow, Message* pcFile) : Window(Rect(0,0,200,200),"","File Properties...",WND_NOT_RESIZABLE | WND_NO_DEPTH_BUT | WND_NO_ZOOM_BUT)
{
	pcParentWindow = pcWindow;
	pcFileMessage = pcFile;
	pBoldFont = new Font(DEFAULT_FONT_BOLD);
	_Layout();
}

/************************************ 
* Description: Lays out the gui
* Author: Rick Caudill
* Date: Tue Mar 16 09:46:37 2004
************************************/
void FileProp::_Layout()
{
	String cLines, cRel, cAbs, cTime;
	int32 nSize;
	pcFileMessage->FindString("lines",&cLines);
	pcFileMessage->FindString("rel",&cRel);
	pcFileMessage->FindString("abs",&cAbs);
	pcFileMessage->FindString("time",&cTime);
	pcFileMessage->FindInt32("size",&nSize);
	
	pcLVMain = new LayoutView(GetBounds(),"main_layout_view");
	pcVLMain = new VLayoutNode("main_vertical_node");
	
	pcHLRelative = new HLayoutNode("rel_hl");	
	pcHLAbsolute = new HLayoutNode("abs_hl");
	pcVLFilePath = new VLayoutNode("file_vl");
	pcHLRelative->AddChild(pcSVRelativeOne = new StringView(Rect(),"string_rel_one","Relative Path:"));
	pcHLRelative->AddChild(new HLayoutSpacer("",5));
	pcHLRelative->AddChild(pcSVRelativeTwo = new StringView(Rect(),"string_rel_two",cRel));

	pcHLAbsolute->AddChild(pcSVAbsoluteOne = new StringView(Rect(),"string_abs_one","Absolute Path:"));
	pcHLAbsolute->AddChild(new HLayoutSpacer("",5));
	pcHLAbsolute->AddChild(pcSVAbsoluteTwo = new StringView(Rect(),"string_abs_two",cAbs));
	
	pcVLFilePath->AddChild(pcHLAbsolute);
	pcVLFilePath->AddChild(new VLayoutSpacer("",5));
	pcVLFilePath->AddChild(pcHLRelative);
	pcVLFilePath->SameWidth("string_abs_one","string_rel_one",NULL);
	pcVLFilePath->SameWidth("string_abs_two","string_rel_two",NULL);
	pcFVFilePath = new FrameView(Rect(),"file_frame_view","Paths...");
	pcFVFilePath->SetRoot(pcVLFilePath);

	
	pcHLSizeAndLines = new HLayoutNode("hl_size_lines");
	pcHLSizeAndLines->SetHAlignment(os::ALIGN_LEFT);
	pcHLSizeAndLines->AddChild(pcSVLinesOne = new StringView(Rect(),"sv_lines_one","Line Count:"));
	pcHLSizeAndLines->AddChild(new HLayoutSpacer("",5));
	pcHLSizeAndLines->AddChild(pcSVLinesTwo = new StringView(Rect(),"sv_lines_two",ConvertIntToString(GetLineCount(cLines))));
	pcHLSizeAndLines->AddChild(new HLayoutSpacer("",10));
	pcHLSizeAndLines->AddChild(pcSVSizeOne = new StringView(Rect(),"sv_size_one","File Size:"));
	pcHLSizeAndLines->AddChild(new HLayoutSpacer("",5));
	pcHLSizeAndLines->AddChild(pcSVSizeTwo = new StringView(Rect(),"sv_size_two",ConvertSizeToString(nSize)));
	pcHLSizeAndLines->AddChild(new HLayoutSpacer("",5));	
	
	HLayoutNode* pcHLCommentAndActual = new HLayoutNode("hl_comment_actual");
	pcHLCommentAndActual->SetHAlignment(os::ALIGN_LEFT);	
	pcHLCommentAndActual->AddChild(pcSVCommentCountOne = new StringView(Rect(),"sv_comment_one","Comment Count:"));
	pcHLCommentAndActual->AddChild(new HLayoutSpacer("",5));
	pcHLCommentAndActual->AddChild(pcSVCommentCountTwo = new StringView(Rect(),"sv_comment_two",ConvertIntToString(GetCommentCount(cLines))));	
	pcHLCommentAndActual->AddChild(new HLayoutSpacer("",10));	
	pcHLCommentAndActual->AddChild(pcSVActualCodeOne = new StringView(Rect(),"sv_actual_one", "Code Lines:"));	
	pcHLCommentAndActual->AddChild(new HLayoutSpacer("",5));
	pcHLCommentAndActual->AddChild(pcSVActualCodeTwo = new StringView(Rect(),"sv_actual_two",ConvertIntToString(GetLineCount(cLines)-GetCommentCount(cLines))));		
	pcHLCommentAndActual->AddChild(new HLayoutSpacer("",5));
	
	pcHLTimeStamp = new HLayoutNode("hl_date_stamp");
	pcHLTimeStamp->SetHAlignment(os::ALIGN_LEFT);	
	pcHLTimeStamp->AddChild(pcSVTimeOne = new StringView(Rect(),"sv_time_one","Created:"));
	pcHLTimeStamp->AddChild(new HLayoutSpacer("",5));	
	pcHLTimeStamp->AddChild(pcSVTimeTwo = new StringView(Rect(),"sv_time_two",cTime));
	
	VLayoutNode* pcAllFormatsNode = new VLayoutNode("all_node");
	pcAllFormatsNode->SetBorders(Rect(5,0,5,0));
	pcAllFormatsNode->AddChild(pcHLSizeAndLines);
	pcAllFormatsNode->AddChild(new VLayoutSpacer("",5));		
	pcAllFormatsNode->AddChild(pcHLCommentAndActual);
	pcAllFormatsNode->AddChild(new VLayoutSpacer("",5));		
	pcAllFormatsNode->AddChild(pcHLTimeStamp);

	pcAllFormatsNode->SameWidth("sv_lines_one","sv_comment_one",NULL);
	pcAllFormatsNode->SameWidth("sv_time_one","sv_size_one","sv_actual_one",NULL);
	pcAllFormatsNode->SameWidth("sv_lines_two","sv_size_two","sv_actual_two","sv_comment_two",NULL);
	pcAllFormatsNode->SameWidth("hl_date_stamp","hl_comment_actual","hl_size_lines",NULL);
	

	pcHLButton = new HLayoutNode("hl_but");
	pcHLButton->AddChild(pcOk = new Button(Rect(),"but_ok","OK",new Message(M_OK)),100);

	pBoldFont->SetSize(pcSVAbsoluteOne->GetFont()->GetSize()-0.3f);
	pcSVAbsoluteTwo->SetFont(pBoldFont);
	pcSVRelativeTwo->SetFont(pBoldFont);
	pcSVLinesTwo->SetFont(pBoldFont);
	pcSVSizeTwo->SetFont(pBoldFont);
	pcSVActualCodeTwo->SetFont(pBoldFont);
	pcSVCommentCountTwo->SetFont(pBoldFont);
	pcSVTimeTwo->SetFont(pBoldFont);
	pBoldFont->Release();

	pcVLMain->AddChild(pcFVFilePath);
	pcVLMain->AddChild(new VLayoutSpacer("",5));
	pcVLMain->AddChild(pcAllFormatsNode);	
	pcVLMain->AddChild(new VLayoutSpacer("",10));
	pcVLMain->AddChild(pcHLButton);
	

	pcLVMain->SetRoot(pcVLMain);
	AddChild(pcLVMain);
	ResizeTo(pcLVMain->GetPreferredSize(false));
	SetDefaultButton(pcOk);
}

/************************************ 
* Description: Handle messsages between the dialog
* Author: Rick Caudill
* Date: Tue Mar 16 09:46:37 2004
************************************/
void FileProp::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_OK:
			pcParentWindow->PostMessage(new Message(M_EDIT_FILE_PROPERTIES_REQUESTED),pcParentWindow);
			break;
	}
}





