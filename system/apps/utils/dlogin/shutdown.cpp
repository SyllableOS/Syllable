#include "shutdown.h"
#include "commonfuncs.h"
#include "messages.h"

#include <gui/requesters.h>

#include "resources/dlogin.h"

ShutdownWindow::ShutdownWindow(Window* pcParent) : Window(Rect(0,0,300,90),"shut_window",MSG_SHUTDOWNWND_TITLE,WND_NO_DEPTH_BUT | WND_NO_ZOOM_BUT | WND_NOT_RESIZABLE)
{
	pcParentWindow = pcParent;
	Layout();
	
	SetDefaultButton(pcOkButton);
}

void ShutdownWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"layout",NULL);
	VLayoutNode* pcRoot = new VLayoutNode("root");
	pcRoot->SetBorders(Rect(20,5,5,5));
	
	HLayoutNode* pcButtonNode = new HLayoutNode("but_node");
	pcButtonNode->SetVAlignment(ALIGN_CENTER);
	pcButtonNode->SetHAlignment(ALIGN_CENTER);
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but",MSG_SHUTDOWNWND_BUTTON_CANCEL,new Message(M_CANCEL)));
	pcButtonNode->AddChild(new HLayoutSpacer("",10,10));	
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"ok_but",MSG_SHUTDOWNWND_BUTTON_OK,new Message(M_SHUTDOWN)));
	pcButtonNode->SameWidth("cancel_but","ok_but",NULL);
	pcButtonNode->SameHeight("cancel_but","ok_but",NULL);
	pcRoot->AddChild(pcShutString = new StringView(Rect(),"shut_string",MSG_SHUTDOWNWND_TEXT));
	pcRoot->AddChild(pcShutDrop = new DropdownMenu(Rect(),""));
	pcShutDrop->SetMinPreferredSize(20);
	pcShutDrop->SetMaxPreferredSize(20);
	AddDropItems();
	pcShutDrop->SetReadOnly(true);
	
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcButtonNode);
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
}

void ShutdownWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_SHUTDOWN:
		{
			int nCode = pcShutDrop->GetSelection();
			switch( nCode )
			{
				case 0:
				{
					Quit(0);
					break;
				}
				case 1:
				{
					Quit(1);
				}				
			}
			break;
		}			
		
		case M_CANCEL:
		{
			OkToQuit();
		}
	}
}

bool ShutdownWindow::OkToQuit()
{
	PostMessage(os::M_TERMINATE,this);
	return true;
}

void ShutdownWindow::AddDropItems()
{
	pcShutDrop->AppendItem(MSG_SHUTDOWNWND_DROPDOWN_SHUTDOWN);
	pcShutDrop->AppendItem(MSG_SHUTDOWNWND_DROPDOWN_REBOOT);
	pcShutDrop->SetSelection(0,false);
}

void ShutdownWindow::Quit( int nAction )
{	
	
	switch( nAction )
	{
		case 0: 
		{
			apm_poweroff();
			break;
		}
		case 1: 
		{
			reboot();
			break;
		}
		
		case 2: 
		{
			reboot();
			break;
		}
	}
}
