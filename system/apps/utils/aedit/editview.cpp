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
#include "appwindow.h"
#include "resources/aedit.h"

EditView::EditView(const Rect& cFrame, AEditWindow *pcMain) : TextView(cFrame,"edit_view","",CF_FOLLOW_BOTTOM | CF_FOLLOW_TOP |  CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT)//,WID_WILL_DRAW)
{
	pcMainWindow=pcMain;

	// Create a context menu
	pcContextMenu=new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);
	pcContextMenu->AddItem(MSG_MENU_EDIT_CUT, new Message(M_MENU_EDIT_CUT));
	pcContextMenu->AddItem(MSG_MENU_EDIT_COPY, new Message(M_MENU_EDIT_COPY));
	pcContextMenu->AddItem(MSG_MENU_EDIT_PASTE, new Message(M_MENU_EDIT_PASTE));
	pcContextMenu->AddItem(new MenuSeparator());
	pcContextMenu->AddItem(MSG_MENU_EDIT_SELECT_ALL, new Message(M_MENU_EDIT_SELECT_ALL));
	pcContextMenu->AddItem(new MenuSeparator());
	pcContextMenu->AddItem(MSG_MENU_EDIT_UNDO, new Message(M_MENU_EDIT_UNDO));
	pcContextMenu->AddItem(MSG_MENU_EDIT_REDO, new Message(M_MENU_EDIT_REDO));
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

// We implement this function to be able to do drag and drop of files
void EditView::MouseUp(const Point& cPosition, uint32 nButtons, Message* pcData)
{
	if(pcData!=NULL)
		pcMainWindow->DragAndDrop(pcData);
	else
		TextView::MouseUp(cPosition, nButtons, pcData);
}

void EditView::KeyDown(const char* pzString, const char* pzRawString, uint32 nQualifiers)
{
// This is not a nice way of doing shortcuts. Best would be to first check if the shortcut belongs to a menuitem
// and if not pass the shortcut to TextView. Unfortunetly there is no(?) way of knowing (except going through
// all menuitems) if the shortcut did have a menuitem attached to it or not.
// The main probem with the below solution is that the shortcut is defined in two places, this place but
// also where you create the menuitem.

	if((nQualifiers & QUAL_CTRL) && (nQualifiers & QUAL_SHIFT) && !(nQualifiers & QUAL_ALT) )
	{
		switch(*pzRawString)
		{
			case VK_TAB:	    // Prev tab (Shift+Ctrl+Tab)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_PREV_TAB));
				return;
			}

			//default:
				/* Fall through to be handled by TextView::KeyDown() later */

		}	// End of switch() block
	}
	else if((nQualifiers & QUAL_CTRL) && !(nQualifiers & QUAL_SHIFT) && !(nQualifiers & QUAL_ALT))
	{
		switch(*pzRawString)
		{
			case 'n':	// New (Ctrl+N)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_NEW));
				return;
			}

			case 'o':	// Open (Ctrl+O)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_OPEN));
				return;
			}

			case 's':	// Save (Ctrl+S)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_SAVE));
				return;
			}

			case 'l':	// Save All (Ctrl+L)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_SAVE_ALL));
				return;
			}

			case 'w':	// Close (Ctrl+W)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_CLOSE));
				return;
			}

			case 'f':	// Find (Ctrl+F)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FIND_FIND));
				return;
			}

			case 'r':	// Replace (Ctrl+R)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FIND_REPLACE));
				return;
			}

			case 'g':	// Goto Line (Ctrl+G)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FIND_GOTO));
				return;
			}

			case 'q':	// Exit (Ctrl+Q)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_APP_QUIT));
				return;
			}

			case 'a':	// Select all (Ctrl+A)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_EDIT_SELECT_ALL));
				return;
			}
			case ',':	// Prev tab (Ctrl+,)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_PREV_TAB));
				return;
			}

			case VK_TAB:	    // (Ctrl+Tab)
			case '.':	// Next tab (Ctrl+.)
			{
				if( !(nQualifiers & QUAL_REPEAT) ) GetWindow()->HandleMessage(new Message(M_MENU_FILE_NEXT_TAB));
				return;
			}
			//default:
				/* fall through to TextView::KeyDown() */
		}

	}
	TextView::KeyDown(pzString,pzRawString,nQualifiers);
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






