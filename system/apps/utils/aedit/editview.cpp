//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
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

#include "editview.h"
#include "messages.h"

#include <util/message.h>

#include <gui/textview.h>

EditView::EditView(const Rect& cFrame) : TextView(cFrame,"edit_view","",CF_FOLLOW_BOTTOM | CF_FOLLOW_TOP |  CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT)//,WID_WILL_DRAW)
{
	// Create a context menu
	pcContextMenu=new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);
	pcContextMenu->AddItem("Cut", new Message(M_MENU_EDIT_CUT));
	pcContextMenu->AddItem("Copy", new Message(M_MENU_EDIT_COPY));
	pcContextMenu->AddItem("Paste", new Message(M_MENU_EDIT_PASTE));
	pcContextMenu->AddItem(new MenuSeparator());
	pcContextMenu->AddItem("Select all", new Message(M_MENU_EDIT_SELECT_ALL));
#ifdef ENABLE_UNDO
	pcContextMenu->AddItem(new MenuSeparator());
	pcContextMenu->AddItem("Undo", new Message(M_MENU_EDIT_UNDO));
	pcContextMenu->AddItem("Redo", new Message(M_MENU_EDIT_REDO));
#endif
}

void EditView::AttachedToWindow(void)
{
	pcContextMenu->SetTargetForItems(GetWindow());
}

void EditView::MouseDown(const Point &cPosition, uint32 nButtons)
{
	if(nButtons == 2)
		pcContextMenu->Open(ConvertToScreen(cPosition));	// Open the menu where the mouse is
	else
		TextView::MouseDown(cPosition, nButtons);
}

void EditView::KeyDown(const char* pzString, const char* pzRawString, uint32 nQualifiers)
{
	if(nQualifiers & QUAL_CTRL)
	{
		if(!(nQualifiers & QUAL_REPEAT))
		{

			switch(*pzString)
			{
				case 15:	// Open (Ctrl+O)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_FILE_OPEN));
					break;
				}

				case 19:	// Save (Ctrl+S)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_FILE_SAVE));
					break;
				}

				case 1:	// Save As (Ctrl+A)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_FILE_SAVE_AS));
					break;
				}

				case 6:	// Find (Ctrl+F)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_FIND_FIND));
					break;
				}

				case 18:	// Replace (Ctrl+R)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_FIND_REPLACE));
					break;
				}

				case 7:	// Goto Line (Ctrl+G)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_FIND_GOTO));
					break;
				}

				case 17:	// Exit (Ctrl+Q)
				{
					GetWindow()->HandleMessage(new Message(M_MENU_APP_QUIT));
					break;
				}

				default:
					TextView::KeyDown(pzString,pzRawString,nQualifiers);

			}	// End of switch() block

		}	// End of if() testing the repeat

	}
	else
	{
		TextView::KeyDown(pzString,pzRawString,nQualifiers);
	}
}

int EditView::Find(std::string zOriginal, std::string zFind, bool bCaseSensitive, int nStartPosition){
// bCaseSensitive == True, do a case sensitive search
// bCaseSensitive == False, do a case insensitive search

	unsigned int nPosition1, nPosition2=0;

	const char* cOriginal=zOriginal.c_str();
	const char* cFind=zFind.c_str();
	bool bMatched=true;

	nPosition1=nStartPosition;

	if(bCaseSensitive)
	{

		for(;nPosition1<=zOriginal.size();nPosition1++)
		{
			if(cOriginal[nPosition1]==cFind[0]){

				bMatched=true;
				for(nPosition2=0;nPosition2<zFind.size();nPosition2++)
				{
					if(cOriginal[nPosition1+nPosition2]!=cFind[nPosition2])
					{
						bMatched=false;
						break;
					}
				}

				if(bMatched)
					return(nPosition1);

			}	// End of if()
		}	// End of for()
	}	// End of outer if()

	else	// Do a case insensitive search
	{
		for(;nPosition1<=zOriginal.size();nPosition1++)
		{
			if(tolower(cOriginal[nPosition1])==tolower(cFind[0]))
			{
				bMatched=true;

				for(nPosition2=0;nPosition2<zFind.size();nPosition2++)
				{
					if(tolower(cOriginal[nPosition1+nPosition2])!=tolower(cFind[nPosition2]))
					{
						bMatched=false;
						break;
					}
				}

				if(bMatched)
					return(nPosition1);

			}	// End of if()
		}	// End of for()
	}	// End of else() block

	return(-1);
}

void EditView::Undo(void)
{
	TextView::Undo();
}

void EditView::Redo(void)
{
	TextView::Redo();
}

int EditView::GetLineCount(void)
{
	return TextView::GetBuffer().size();
}

std::string EditView::GetLine(int32 nLineNo)
{
	return TextView::GetBuffer()[nLineNo];
}






