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

#include "toolsdialog.h"
#include "messages.h"
#include "toolhandle.h"

#include <util/settings.h>
using namespace os;

/*************************************************************
* Description: Simple window that lets you edit/create a new tool
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:57:23 
*************************************************************/
ToolsEditWindow::ToolsEditWindow(Window* pcParent, Message* pcMessage) : Window(Rect(0,0,1,1),"tools_edit","New Command...",WND_NOT_RESIZABLE)
{
	if (pcMessage != NULL)
	{
		pcMessage->FindString("command",&cCommand);
		pcMessage->FindString("name",&cName);
		pcMessage->FindString("arguments",&cArguments);
		pcMessage->FindInt("selected",&nSelected);
	}
	
	else
	{
		cCommand = "";
		cName = "";
		cArguments = "";
		nSelected = -1;
	}
	
	pcParentWindow = pcParent;
	Layout();
	SetTabs();
	ResizeTo(pcLayoutView->GetPreferredSize(false));
}

/*************************************************************
* Description: Sets the taborder of all elements
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:00:25 
*************************************************************/
void ToolsEditWindow::SetTabs()
{
	pcNameText->SetTabOrder(NEXT_TAB_ORDER);
	pcCommandText->SetTabOrder(NEXT_TAB_ORDER);
	pcArgumentsText->SetTabOrder(NEXT_TAB_ORDER);
	pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	
	SetDefaultButton(pcOkButton);
	SetFocusChild(pcNameText);
}

/*************************************************************
* Description: Changes to the current tool
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 05:22:05 
*************************************************************/
void ToolsEditWindow::ChangeTo(Message* pcMessage)
{
	if (pcMessage != NULL)
	{
		pcMessage->FindString("command",&cCommand);
		pcMessage->FindString("name",&cName);
		pcMessage->FindString("arguments",&cArguments);
		pcMessage->FindInt("selected",&nSelected);
	}	
	
	pcNameText->Set(cName.c_str());
	pcCommandText->Set(cCommand.c_str());
	pcArgumentsText->Set(cArguments.c_str());
	
}

/*************************************************************
* Description: Lays the editor window out
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:00:50 
*************************************************************/
void ToolsEditWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"main_layout_view",NULL,CF_FOLLOW_ALL);
	
	VLayoutNode* pcRoot = new VLayoutNode("main_layout");
	pcRoot->SetBorders(Rect(5,5,5,10));
	
	HLayoutNode* pcNameNode = new HLayoutNode("name_node");
	pcNameNode->AddChild(pcNameString = new StringView(Rect(),"name_string","Title:"));
	pcNameNode->AddChild(new HLayoutSpacer("",5,2));
	pcNameNode->AddChild(pcNameText = new TextView(Rect(),"name_text",cName.c_str()));
	
	HLayoutNode* pcCommandNode = new HLayoutNode("command_node");
	pcCommandNode->AddChild(pcCommandString = new StringView(Rect(),"command_string","Command:"));
	pcCommandNode->AddChild(new HLayoutSpacer("",5,2));
	pcCommandNode->AddChild(pcCommandText = new TextView(Rect(),"command_text",cCommand.c_str()));
	
	HLayoutNode* pcArgumentsNode = new HLayoutNode("arguments_node");
	pcArgumentsNode->AddChild(pcArgumentsString = new StringView(Rect(),"arguments_string","Arguments:"));
	pcArgumentsNode->AddChild(new HLayoutSpacer("",5,2));
	pcArgumentsNode->AddChild(pcArgumentsText = new TextView(Rect(),"arguments_text",cArguments.c_str()));
		
	HLayoutNode* pcOtherButtonNode = new HLayoutNode("other_buttons");
	pcOtherButtonNode->SetHAlignment(ALIGN_RIGHT);
	pcOtherButtonNode->SetVAlignment(ALIGN_RIGHT);
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",100));	
	pcOtherButtonNode->AddChild(pcOkButton = new Button(Rect(),"save_but","_Save",new Message(M_TOOLS_SAVE)));
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcOtherButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_TOOLS_CANCEL)));
	pcOtherButtonNode->SameWidth("save_but","cancel_but",NULL);	
	
	pcRoot->AddChild(pcNameNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcCommandNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));	
	pcRoot->AddChild(pcArgumentsNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));	
	pcRoot->AddChild(pcOtherButtonNode);
	
	pcRoot->SameWidth("arguments_string","command_string","name_string",NULL);
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
}

/*************************************************************
* Description: Handles all messages passed through the edit window
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:01:04 
*************************************************************/
void ToolsEditWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_TOOLS_SAVE:
		{
			pcSaveMessage = new Message(M_TOOLS_QUIT);
			pcSaveMessage->AddBool("saved",true);
			pcSaveMessage->AddString("command",pcCommandText->GetBuffer()[0]);
			pcSaveMessage->AddString("name", pcNameText->GetBuffer()[0]);
			pcSaveMessage->AddString("arguments",pcArgumentsText->GetBuffer()[0]);
			pcSaveMessage->AddInt32("selected",nSelected);
			OkToQuit();
			break;
		}
			
		case M_TOOLS_CANCEL:
			pcSaveMessage = new Message(M_TOOLS_QUIT);
			OkToQuit();
			break;
	}
}

/*************************************************************
* Description: Called when the window is closed with the close/cancel buttons
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:01:38 
*************************************************************/
bool ToolsEditWindow::OkToQuit()
{
	pcParentWindow->PostMessage(pcSaveMessage,pcParentWindow);
	return true;
}

/*************************************************************
* Description: ToolsWindow lets the user have external tools they can
*              from within Sourcery.
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:03:07 
*************************************************************/
ToolsWindow::ToolsWindow(Window* pcParent) : Window(Rect(0,0,1,1), "external_tools","External Tools..", WND_NOT_RESIZABLE)
{
	pcParentWindow = pcParent;
	pcToolsEditWindow = NULL;
	bSaved = false;
	
	Layout();
	SetTabs();
	ResizeTo(Point(pcLayoutView->GetPreferredSize(false).x+90,pcLayoutView->GetPreferredSize(false).y+100));
}

/*************************************************************
* Description: Lays the window out
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:04:07 
*************************************************************/
void ToolsWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"main_layoutview",NULL,CF_FOLLOW_ALL);
	
	VLayoutNode* pcRoot = new VLayoutNode("main_layout");
	pcRoot->SetBorders(Rect(5,5,5,10));
	
	HLayoutNode* pcListNode = new HLayoutNode("list_node");
	pcListNode->AddChild(pcListView = new ListView(Rect(0,0,0,0),"edit_list",CF_FOLLOW_ALL));
	pcListView->InsertColumn("Name",100);
	pcListView->InsertColumn("Command",100);
	pcListView->InsertColumn("Arguments",100);
	pcListNode->SetWeight(120);
	
	HLayoutNode* pcListButtonNode = new HLayoutNode("list_buttons");
	pcListButtonNode->AddChild(new HLayoutSpacer("",20));
	pcListButtonNode->AddChild(pcNewButton = new Button(Rect(),"new_but","_New",new Message(M_TOOLS_NEW),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",10));
	pcListButtonNode->AddChild(pcEditButton = new Button(Rect(),"edit_but","_Edit",new Message(M_TOOLS_EDIT),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",10));
	pcListButtonNode->AddChild(pcDeleteButton = new Button(Rect(),"del_but","_Delete",new Message(M_TOOLS_DELETE),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",20));
	pcListButtonNode->SameWidth("del_but","edit_but","new_but",NULL);
	
	HLayoutNode* pcOtherButtonNode = new HLayoutNode("other_buttons");
	pcOtherButtonNode->SetHAlignment(ALIGN_RIGHT);
	pcOtherButtonNode->SetVAlignment(ALIGN_RIGHT);
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",100));	
	pcOtherButtonNode->AddChild(pcSaveButton = new Button(Rect(),"save_but","_Save",new Message(M_TOOLS_SAVE)));
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcOtherButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_TOOLS_CANCEL)));
	pcOtherButtonNode->SameWidth("save_but","cancel_but",NULL);
	
	pcRoot->AddChild(pcListNode);
	pcRoot->AddChild(pcListButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",10));
	pcRoot->AddChild(pcOtherButtonNode);
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
	
	LoadTools();
}

/*************************************************************
* Description: Sets the tab order of everything
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:04:23 
*************************************************************/
void ToolsWindow::SetTabs()
{
	pcNewButton->SetTabOrder(NEXT_TAB_ORDER);
	pcEditButton->SetTabOrder(NEXT_TAB_ORDER);
	pcDeleteButton->SetTabOrder(NEXT_TAB_ORDER);
	pcSaveButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
		
	SetDefaultButton(pcSaveButton);
	SetFocusChild(pcNewButton);	
}

/*************************************************************
* Description: handles all messages passed through the window
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:04:37 
*************************************************************/
void ToolsWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_TOOLS_SAVE:
		{	
			bSaved = true;
			SaveTools();
			OkToQuit();
			break;
		}
		
		case M_TOOLS_CANCEL:
		{	
			OkToQuit();
			break;
		}
		
		case M_TOOLS_EDIT:
		{
			if (pcListView->GetLastSelected() > -1)
			{
				ListViewStringRow* pcRow = (ListViewStringRow*)pcListView->GetRow(pcListView->GetLastSelected());
			
				Message* pcMsg = new Message();
				pcMsg->AddString("name",pcRow->GetString(0));
				pcMsg->AddString("command",pcRow->GetString(1));
				pcMsg->AddString("arguments",pcRow->GetString(2));			
				pcMsg->AddInt32("selected",pcListView->GetLastSelected());
			
				if (pcToolsEditWindow == NULL)
				{
					pcToolsEditWindow = new ToolsEditWindow(this,pcMsg);
					pcToolsEditWindow->CenterInWindow(this);
					pcToolsEditWindow->Show();
					pcToolsEditWindow->MakeFocus();
				}
			
				else
				{
					pcToolsEditWindow->ChangeTo(pcMsg);
					pcToolsEditWindow->MakeFocus(true);
				}	
			}
			break;
		}
		
		case M_TOOLS_DELETE:
		{
			uint nSelected = pcListView->GetLastSelected();
			if (nSelected >= 0)
			{
				pcListView->RemoveRow(nSelected,true);
			}
			break;
		}
		
		case M_TOOLS_NEW:
		{
			if (pcToolsEditWindow == NULL)
			{
				pcToolsEditWindow = new ToolsEditWindow(this,NULL);
				pcToolsEditWindow->CenterInWindow(this);
				pcToolsEditWindow->Show();
				pcToolsEditWindow->MakeFocus();
			}
			
			else
			{
				pcToolsEditWindow->MakeFocus(true);
			}
			break;
		}
		
		case M_TOOLS_QUIT:
		{
			if (pcToolsEditWindow != NULL)
			{
				bool bSavedTools;
				
				if (pcMessage->FindBool("saved",&bSavedTools) == 0)
				{
					String cName, cCommand, cArguments;
					int nSelected;
					pcMessage->FindString("command",&cCommand);
					pcMessage->FindString("name",&cName);
					pcMessage->FindString("arguments",&cArguments);
					pcMessage->FindInt("selected",&nSelected);
					
					if (nSelected == -1)
					{	//this means it is a new tool
						ListViewStringRow* pcRow = new ListViewStringRow();
						pcRow->AppendString(cName);
						pcRow->AppendString(cCommand);
						pcRow->AppendString(cArguments);
						pcListView->InsertRow(pcRow);
					}
					
					else
					{ //this means that a certain tool was edited
						ListViewStringRow* pcRow = (ListViewStringRow*)pcListView->GetRow(nSelected);
						pcRow->SetString(0,cName);
						pcRow->SetString(1,cCommand);
						pcRow->SetString(2,cArguments);
					}
				}
				
				pcToolsEditWindow->Close();
				pcToolsEditWindow = NULL;
			}
			break;
		}
	}
}

/*************************************************************
* Description: Called when tools window wants to quit
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:05:11 
*************************************************************/
bool ToolsWindow::OkToQuit()
{
	Message* pcMessage = new Message(M_TOOLS_QUIT);
	pcMessage->AddBool("saved",&bSaved);
	pcParentWindow->PostMessage(pcMessage,pcParentWindow);
	return false;
}

/*************************************************************
* Description: Loads all the tools
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:05:33 
*************************************************************/
void ToolsWindow::LoadTools()
{
	try
	{
		os::String zPath = getenv( "HOME" );
		zPath += "/Settings/Sourcery/Tools";
		os::Settings* pcSettings = new os::Settings( new os::File( zPath, O_RDWR | O_CREAT ) );
		pcSettings->Load();
		
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			ToolHandle* pcHandle;
			size_t nToolHandleSize;
			
			pcSettings->FindData(pcSettings->GetName(i).c_str(),T_RAW,(const void**)&pcHandle, &nToolHandleSize);
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(pcHandle->zName);
			pcRow->AppendString(pcHandle->zCommand);
			pcRow->AppendString(pcHandle->zArguments);			
			pcListView->InsertRow(pcRow);
		}
		delete pcSettings;
	}
	
	catch(...)
	{
	}	
}

/*************************************************************
* Description: Saves all the tools
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 04:06:05 
*************************************************************/
void ToolsWindow::SaveTools()
{
	try
	{
		os::String zPath = getenv( "HOME" );
		zPath += "/Settings/Sourcery/Tools";
		os::Settings* pcSettings = new os::Settings( new os::File( zPath) );

		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			pcSettings->RemoveName(pcSettings->GetName(i).c_str());
		}
		
		for (int i=0; i<pcListView->GetRowCount(); i++)
		{
			ToolHandle* pcHandle = new ToolHandle();
			ListViewStringRow* pcRow = (ListViewStringRow*) pcListView->GetRow(i);
			strcpy(pcHandle->zName, pcRow->GetString(0).c_str());
			strcpy(pcHandle->zCommand, pcRow->GetString(1).c_str());
			strcpy(pcHandle->zArguments,pcRow->GetString(2).c_str());
			pcSettings->SetData(pcRow->GetString(0).c_str(),T_RAW,(void*) pcHandle,sizeof(ToolHandle));
		}
		pcSettings->Save();
		delete( pcSettings );
	}
	catch (...)
	{
	}	
}

