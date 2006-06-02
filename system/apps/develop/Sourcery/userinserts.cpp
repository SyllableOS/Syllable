// Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#include "userinserts.h"
#include "messages.h"
#include "commonfuncs.h"

#include <gui/imagebutton.h>
#include <gui/requesters.h>

using namespace os;

UserInsertsEditWindow::UserInsertsEditWindow(Window* pcParent,bool bNew) : Window(Rect(0,0,200,200),"edit_win","New Insert...",WND_NOT_RESIZABLE)
{
	pcParentWindow = pcParent;
	bIsNewWindow = bNew;
	
	Layout();
	ResizeTo(pcView->GetPreferredSize(false));
}

void UserInsertsEditWindow::Layout()
{
	pcView = new LayoutView(GetBounds(),"main_layout",NULL,CF_FOLLOW_ALL);
	VLayoutNode* pcRoot = new VLayoutNode("main_layout_node");
	pcRoot->SetBorders(Rect(5,5,10,5));
	
	HLayoutNode* pcNameNode = new HLayoutNode("name_node");
	pcNameNode->AddChild(pcNameString = new StringView(Rect(),"name_string","Name:",ALIGN_LEFT,CF_FOLLOW_NONE));
	pcNameNode->AddChild(new HLayoutSpacer("",5));
	pcNameNode->AddChild(pcNameText = new TextView(Rect(),"text_name","",CF_FOLLOW_NONE),100);
	
	HLayoutNode* pcTextNode = new HLayoutNode("text_node");
	pcTextNode->AddChild(pcInsertedString = new StringView(Rect(),"insert_string","Text:",ALIGN_LEFT,CF_FOLLOW_NONE));
	pcTextNode->AddChild(new HLayoutSpacer("",5));
	pcTextNode->AddChild(pcInsertedText = new TextView(Rect(),"text_insert","",CF_FOLLOW_ALL));
	pcInsertedText->SetMultiLine(true);	
	pcInsertedText->SetMaxPreferredSize(25,8);
	pcInsertedText->SetMinPreferredSize(25,8);

	
	HLayoutNode* pcButtonNode = new HLayoutNode("button_node");
	pcButtonNode->SetHAlignment(ALIGN_RIGHT);
	pcButtonNode->AddChild(new HLayoutSpacer("",170));
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"ok_but","_OK",new Message(M_USER_INSERT_SAVE),CF_FOLLOW_NONE));
	pcButtonNode->AddChild(new HLayoutSpacer("",5));	
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_USER_INSERT_CANCEL),CF_FOLLOW_NONE));
	pcButtonNode->SameWidth("ok_but","cancel_but",NULL);
	
	
	pcRoot->AddChild(pcNameNode);
	pcRoot->AddChild(new VLayoutSpacer("",5));
	pcRoot->AddChild(pcTextNode);
	pcRoot->AddChild(new VLayoutSpacer("",5));
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",5));	
	pcRoot->SameWidth("insert_string", "name_string",NULL);
	pcRoot->SameWidth("text_name","text_insert",NULL);
	pcView->SetRoot(pcRoot);
	AddChild(pcView);
	
	pcNameText->SetTabOrder(NEXT_TAB_ORDER);
	pcInsertedText->SetTabOrder(NEXT_TAB_ORDER);
	pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	
	SetFocusChild(pcNameText);
	SetDefaultButton(pcOkButton);
}

bool UserInsertsEditWindow::OkToQuit()
{
	Close();
	return false;
}

void UserInsertsEditWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_USER_INSERT_SAVE:
		{
			Message* pcMsg;
			String cInsertedText="";
			for (uint i=0; i<pcInsertedText->GetBuffer().size(); i++)
			{
				cInsertedText += pcInsertedText->GetBuffer()[i];
				cInsertedText += "\n";
			}
			
			if (bIsNewWindow)
			{
				pcMsg = new Message(M_USER_INSERT_NEW_PASSED);
				pcMsg->AddString("name",pcNameText->GetBuffer()[0]);
				pcMsg->AddString("text",cInsertedText);
			}
			
			else
			{
				pcMsg = new Message(M_USER_INSERT_EDIT_PASSED);
				pcMsg->AddString("name",pcNameText->GetBuffer()[0]);
				pcMsg->AddString("text",cInsertedText);			
			}
			pcParentWindow->PostMessage(pcMsg,pcParentWindow);
			OkToQuit();
			break;
		}
		
		case M_USER_INSERT_CANCEL:
		{
			OkToQuit();
			break;
		}
	}
}

void UserInsertsEditWindow::Insert(const String& cName, const String& cText)
{
	pcNameText->Set(cName.c_str());
	pcInsertedText->Set(cText.c_str());
	SetTitle(String("Editing: ") + cName);
	bIsNewWindow = false;
}

UserInserts::UserInserts(Window* pcParent) : Window(Rect(0,0,200,200),"User Inserts...","User Inserts...",WND_NOT_RESIZABLE)
{
	pcParentWindow = pcParent;
	
	Layout();
	LoadInserts();
	
	ResizeTo(Point(pcView->GetPreferredSize(false).x,pcView->GetPreferredSize(false).y+100));
}

void UserInserts::Layout()
{
	pcView = new LayoutView(GetBounds(),"main_layout_view",NULL);

	VLayoutNode* pcRoot = new VLayoutNode("main_layout");
	pcRoot->SetBorders(Rect(5,5,5,10));
	
	HLayoutNode* pcFormatsNode = new HLayoutNode("formats_node");
	BitmapImage* pcArrows = LoadImageFromResource("arrows.png");
	
	pcFormatsNode->AddChild(pcFormatsString = new StringView(Rect(),"","%D - Displays the date\n%T - Displays the time",ALIGN_RIGHT,CF_FOLLOW_LEFT|CF_FOLLOW_TOP, WID_WILL_DRAW | WID_TRANSPARENT));
	pcFormatsNode->AddChild(new HLayoutSpacer("",2));	
	pcFormatsNode->AddChild(new ImageButton(Rect(),"","",new Message(M_USER_INSERT_MACROS),pcArrows,ImageButton::IB_TEXT_BOTTOM,true,false,true));
	pcFormatsNode->AddChild(new HLayoutSpacer("",7));	
	
	HLayoutNode* pcListNode = new HLayoutNode("list_node");
	pcListNode->AddChild(pcInsertsList = new ListView(Rect(0,0,0,0),"edit_list",CF_FOLLOW_ALL));
	pcInsertsList->InsertColumn("Name",GetBounds().Width()/2-10);
	pcInsertsList->InsertColumn("Text Inserted",GetBounds().Width()/2-10);
	pcListNode->SetWeight(120);
	
	HLayoutNode* pcListButtonNode = new HLayoutNode("list_buttons");
	pcListButtonNode->AddChild(new HLayoutSpacer("",20));
	pcListButtonNode->AddChild(pcNewButton = new Button(Rect(),"new_but","_New",new Message(M_USER_INSERT_NEW),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",10));
	pcListButtonNode->AddChild(pcEditButton = new Button(Rect(),"edit_but","_Edit",new Message(M_USER_INSERT_EDIT),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",10));
	pcListButtonNode->AddChild(pcDeleteButton = new Button(Rect(),"del_but","_Delete",new Message(M_USER_INSERT_DEL),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",20));
	pcListButtonNode->SameWidth("del_but","edit_but","new_but",NULL);
	
	HLayoutNode* pcOtherButtonNode = new HLayoutNode("other_buttons");
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",100));	
	pcOtherButtonNode->AddChild(pcOkButton = new Button(Rect(),"save_but","_Save",new Message(M_USER_INSERT_SAVE)));
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",5));
	pcOtherButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_USER_INSERT_CANCEL)));
	pcOtherButtonNode->SameWidth("save_but","cancel_but",NULL);
	
	pcRoot->AddChild(pcFormatsNode);
	pcRoot->AddChild(pcListNode);
	pcRoot->AddChild(pcListButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",10));
	pcRoot->AddChild(pcOtherButtonNode);
	pcView->SetRoot(pcRoot);
	AddChild(pcView);
	
	pcNewButton->SetTabOrder(NEXT_TAB_ORDER);
	pcEditButton->SetTabOrder(NEXT_TAB_ORDER);
	pcDeleteButton->SetTabOrder(NEXT_TAB_ORDER);
	pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
		
	SetDefaultButton(pcOkButton);
	SetFocusChild(pcNewButton);
}

void UserInserts::LoadInserts()
{
	try
	{
		os::String zPath = getenv( "HOME" );
		zPath += "/Settings/Sourcery/Inserts";
		os::Settings* pcSettings = new os::Settings( new os::File( zPath, O_RDWR | O_CREAT ) );
		pcSettings->Load();
		
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			String cName = pcSettings->GetName(i);
			String cInsertText;
			pcSettings->FindString(cName.c_str(),&cInsertText);
			
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(cName);
			pcRow->AppendString(cInsertText);
			pcInsertsList->InsertRow(pcRow);
		}
		delete pcSettings;
	}
	
	catch(...)
	{
	}
}

bool UserInserts::OkToQuit()
{
	pcParentWindow->PostMessage(new Message(M_USER_INSERT_QUIT),pcParentWindow);
	return false;
}

void UserInserts::SaveInserts()
{
	try
	{
		os::String zPath = getenv( "HOME" );
		zPath += "/Settings/Sourcery/Inserts";
		os::Settings* pcSettings = new os::Settings( new os::File( zPath ) );

		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			pcSettings->RemoveName(pcSettings->GetName(i).c_str());
		}
		
		for (int i=0; i<pcInsertsList->GetRowCount(); i++)
		{
				ListViewStringRow* pcRow = (ListViewStringRow*) pcInsertsList->GetRow(i);
				pcSettings->SetString(pcRow->GetString(0).c_str(),pcRow->GetString(1));
		}
		pcSettings->Save();
		delete( pcSettings );
	}
	catch (...)
	{
	}
}

void UserInserts::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_USER_INSERT_SAVE:
			SaveInserts();
			OkToQuit();
			break;
	
		case M_USER_INSERT_CANCEL:
			OkToQuit();
			break;
			
		case M_USER_INSERT_NEW:
		{
			UserInsertsEditWindow* pcNew = new UserInsertsEditWindow(this,true);	
			pcNew->CenterInWindow(this);
			pcNew->Show();
			pcNew->MakeFocus();	
			break;
		}
		
		case M_USER_INSERT_EDIT:
		{
			uint nSelected = pcInsertsList->GetLastSelected();
			
			if (nSelected >= 0 && pcInsertsList->GetRowCount() > 0)
			{
				ListViewStringRow* pcRow = (ListViewStringRow*)pcInsertsList->GetRow(nSelected);
				String cName = pcRow->GetString(0);
				String cText = pcRow->GetString(1);
				
				UserInsertsEditWindow* pcEdit = new UserInsertsEditWindow(this,false);
				pcEdit->Insert(cName,cText);
				pcEdit->CenterInWindow(this);
				pcEdit->Show();
				pcEdit->MakeFocus();
			}
			break;
		}
		
		case M_USER_INSERT_DEL:
		{
			uint nSelected = pcInsertsList->GetLastSelected();
			if (nSelected >= 0 && pcInsertsList->GetRowCount() > 0)
				pcInsertsList->RemoveRow(nSelected);
			break;
		}
		
		case M_USER_INSERT_EDIT_PASSED:
		{
			uint nSelected = pcInsertsList->GetLastSelected();
			String cName;
			String cText;
			
			pcMessage->FindString("name",&cName);
			pcMessage->FindString("text",&cText);
			
			ListViewStringRow* pcRow = (ListViewStringRow*)pcInsertsList->GetRow(nSelected);
			pcRow->SetString(0,cName);
			pcRow->SetString(1,cText);
			break;
		}
		
		case M_USER_INSERT_NEW_PASSED:
		{
			String cName;
			String cText;
			
			pcMessage->FindString("name",&cName);
			pcMessage->FindString("text",&cText);
			
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(cName);
			pcRow->AppendString(cText);
			pcInsertsList->InsertRow(pcRow);
			break;
		}
		
		case M_USER_INSERT_MACROS:
		{
			String cMacros = "%C - Displays a comment\n%D - Displays the current date\n%F - Displays the current file name\n%N - Displays the name of the user(Change in Users Program)\n%S - Displays the system info\n%T - Displays the time";
			Alert* pcAlert = new Alert("Insert Macros...",cMacros,LoadImageFromResource("icon48x48.png")->LockBitmap(),0,"Okay",NULL);
			pcAlert->CenterInWindow(this);
			pcAlert->Go(new Invoker());
			break;
		}
	}
}
