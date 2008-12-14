#include "Settings.h"

NewEditWindow::NewEditWindow(bool bIsNew,Window* pcParent) : Window(Rect(0,0,250,100),"newedit_wind","NewEditWindow",WND_NOT_RESIZABLE)
{
	bNewWindow = bIsNew;
	pcParentWindow = pcParent;
	
	if (!bIsNew)
	{
		SetTitle("Editting...");
	}
	else
		SetTitle("New...");
		 
	Layout();
}

void NewEditWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"main_layout",NULL,CF_FOLLOW_ALL);
	VLayoutNode* pcRoot = new VLayoutNode("main_layout_node");
	pcRoot->SetBorders(Rect(5,5,10,5));
	
	HLayoutNode* pcNameNode = new HLayoutNode("name_node");
	pcNameNode->AddChild(pcTagStringView = new StringView(Rect(),"name_string","Tag:",ALIGN_LEFT,CF_FOLLOW_NONE));
	pcNameNode->AddChild(new HLayoutSpacer("",5,5));
	pcNameNode->AddChild(pcTagTextView = new TextView(Rect(),"text_name","",CF_FOLLOW_NONE),100);
	
	HLayoutNode* pcTextNode = new HLayoutNode("text_node");
	pcTextNode->AddChild(pcWebStringView = new StringView(Rect(),"insert_string","Website:",ALIGN_LEFT,CF_FOLLOW_NONE));
	pcTextNode->AddChild(new HLayoutSpacer("",5,5));
	pcTextNode->AddChild(pcWebTextView = new TextView(Rect(),"text_insert","",CF_FOLLOW_ALL));
	
	HLayoutNode* pcButtonNode = new HLayoutNode("button_node");
	pcButtonNode->SetHAlignment(ALIGN_RIGHT);
	pcButtonNode->AddChild(new HLayoutSpacer("",50));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_NEW_CANCEL),CF_FOLLOW_NONE));
	pcButtonNode->AddChild(new HLayoutSpacer("",5));	
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"ok_but","_O.K",new Message(M_NEW_OK),CF_FOLLOW_NONE));
	pcButtonNode->SameWidth("ok_but","cancel_but",NULL);
	
	
	pcRoot->AddChild(pcNameNode);
	pcRoot->AddChild(new VLayoutSpacer("",5));
	pcRoot->AddChild(pcTextNode);
	pcRoot->AddChild(new VLayoutSpacer("",5));
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",5));	
	pcRoot->SameWidth("insert_string", "name_string",NULL);

	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
	
	pcTagTextView->SetTabOrder(NEXT_TAB_ORDER);
	pcWebTextView->SetTabOrder(NEXT_TAB_ORDER);
	pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	
	SetFocusChild(pcTagTextView);
	SetDefaultButton(pcOkButton);
}

void NewEditWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_NEW_OK:
			{
				Message* pcMsg = new Message(M_NEW_OK);
				pcMsg->AddString("tag",pcTagTextView->GetBuffer()[0]);
				pcMsg->AddString("website",pcWebTextView->GetBuffer()[0]);
				pcMsg->AddBool("new",bNewWindow);
				pcParentWindow->PostMessage(pcMsg,pcParentWindow);
				delete(pcMsg);
				break;
			}
			
		case M_NEW_CANCEL:
			pcParentWindow->PostMessage(M_NEW_CANCEL,pcParentWindow);
			break;
	}
}

void NewEditWindow::SetEditDetails(String cTag,String cWebsite)
{
	pcTagTextView->Set(cTag.c_str());
	pcWebTextView->Set(cWebsite.c_str());
}

SettingsWindow::SettingsWindow(View* pcParent,int32 nDefault) : Window(Rect(0,0,250,200),"address_sets","Address Settings...")
{
	pcParentView = pcParent;
	pcNewEditWindow = NULL;
	nEditted = -1;
	Layout();
	LoadAddresses();
	pcDefaultDrop->SetSelection(nDefault);
}

void SettingsWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"main_layout_view",NULL);

	VLayoutNode* pcRoot = new VLayoutNode("main_layout");
	pcRoot->SetBorders(Rect(5,5,5,10));
	
	
	HLayoutNode* pcListNode = new HLayoutNode("list_node");
	pcListNode->AddChild(pcListView = new ListView(Rect(0,0,0,0),"edit_list",CF_FOLLOW_ALL));
	pcListView->InsertColumn("Tag",40);
	pcListView->InsertColumn("Website",(int)GetBounds().Width()/2-10);
	pcListNode->SetWeight(120);
	
	HLayoutNode* pcListButtonNode = new HLayoutNode("list_buttons");
	pcListButtonNode->AddChild(new HLayoutSpacer("",20));
	pcListButtonNode->AddChild(pcNewButton = new Button(Rect(),"new_but","_New",new Message(M_SETTINGS_NEW),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",10));
	pcListButtonNode->AddChild(pcEditButton = new Button(Rect(),"edit_but","_Edit",new Message(M_SETTINGS_EDIT),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",10));
	pcListButtonNode->AddChild(pcDeleteButton = new Button(Rect(),"del_but","_Delete",new Message(M_SETTINGS_DELETE),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcListButtonNode->AddChild(new HLayoutSpacer("",20));
	pcListButtonNode->SameWidth("del_but","edit_but","new_but",NULL);
	
	
	HLayoutNode* pcDefaultNode = new HLayoutNode("default_node");
	pcDefaultNode->AddChild(new StringView(Rect(),"default_string","Default:"));
	pcDefaultNode->AddChild(new HLayoutSpacer("",5,5));
	pcDefaultNode->AddChild(pcDefaultDrop = new DropdownMenu(Rect(),"default_drop"));
	pcDefaultDrop->SetMinPreferredSize(12);
	pcDefaultDrop->SetMaxPreferredSize(12);
	pcDefaultNode->SameHeight("default_string","default_drop",NULL);
	pcDefaultDrop->AppendItem("Google");
	pcDefaultDrop->AppendItem("Wikipedia");
	pcDefaultDrop->AppendItem("Yahoo");
	pcDefaultDrop->AppendItem("Thesaurus");
	pcDefaultDrop->AppendItem("Dictionary");
	pcDefaultDrop->SetReadOnly(true);
	
	HLayoutNode* pcOtherButtonNode = new HLayoutNode("other_buttons");
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",90));	
	pcOtherButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_SETTINGS_CANCEL)));	
	pcOtherButtonNode->AddChild(new HLayoutSpacer("",5));
	pcOtherButtonNode->AddChild(pcOkButton = new Button(Rect(),"save_but","_Save",new Message(M_SETTINGS_OK)));
	pcOtherButtonNode->SameWidth("save_but","cancel_but",NULL);
	pcOtherButtonNode->SameHeight("save_but","cancel_but",NULL);
	
	pcRoot->AddChild(pcListNode);
	pcRoot->AddChild(pcListButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",10,10));
	pcRoot->AddChild(pcDefaultNode);
	pcRoot->AddChild(new VLayoutSpacer("",10,10));
	pcRoot->AddChild(pcOtherButtonNode);
	
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
	
	pcNewButton->SetTabOrder(NEXT_TAB_ORDER);
	pcEditButton->SetTabOrder(NEXT_TAB_ORDER);
	pcDeleteButton->SetTabOrder(NEXT_TAB_ORDER);
	pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
		
	SetDefaultButton(pcOkButton);
	SetFocusChild(pcNewButton);
	
}

void SettingsWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_SETTINGS_NEW:
		{
			if (pcNewEditWindow == NULL)
			{
				pcNewEditWindow = new NewEditWindow(true,this);
				pcNewEditWindow->CenterInWindow(this);
				pcNewEditWindow->Show();
				pcNewEditWindow->MakeFocus();
			}
			else
				pcNewEditWindow->MakeFocus();
			break;
		}
		
		case M_SETTINGS_EDIT:
		{
			if (pcNewEditWindow == NULL)
			{
				nEditted = pcListView->GetLastSelected();
				if (nEditted != -1)
				{
					ListViewStringRow* pcRow = (ListViewStringRow*) pcListView->GetRow(pcListView->GetLastSelected());
					pcNewEditWindow = new NewEditWindow(false,this);
					pcNewEditWindow->SetEditDetails(pcRow->GetString(0),pcRow->GetString(1));
					pcNewEditWindow->CenterInWindow(this);
					pcNewEditWindow->Show();
					pcNewEditWindow->MakeFocus();
				}
			}
			else
				pcNewEditWindow->MakeFocus();
			break;	
		}
		
		case M_SETTINGS_DELETE:
		{
			nEditted = pcListView->GetLastSelected();
			if (nEditted != -1)
			{
				pcListView->RemoveRow(nEditted);
			}	
			break;
		}
			
		case M_NEW_OK:
		{
			String cTag;
			String cWeb;
			bool bNewWindow;
			
			pcMessage->FindString("tag",&cTag);
			pcMessage->FindString("website",&cWeb);
			pcMessage->FindBool("new",&bNewWindow);
			
			if (bNewWindow)
			{
				ListViewStringRow* pcRow = new ListViewStringRow();
				pcRow->AppendString(cTag);
				pcRow->AppendString(cWeb);
				pcListView->InsertRow(pcRow);
			}
			else
			{
				ListViewStringRow* pcRow = (ListViewStringRow*)pcListView->GetRow(nEditted);
				pcRow->SetString(0,cTag);
				pcRow->SetString(1,cWeb);
			}
			pcNewEditWindow->PostMessage(M_TERMINATE,pcNewEditWindow);
			pcNewEditWindow = NULL;
			break;
		}
		
		case M_NEW_CANCEL:
		{
			pcNewEditWindow->PostMessage(M_TERMINATE,pcNewEditWindow);
			pcNewEditWindow = NULL;
			break;
		}
		
		case M_SETTINGS_CANCEL:
		{
			pcParentView->GetLooper()->PostMessage(M_SETTINGS_CANCEL,pcParentView);
			break;
		}
		
		case M_SETTINGS_OK:
		{
			SaveAddresses();
			Message* pcMsg = new Message(M_SETTINGS_PASSED);
			pcMsg->AddInt32("default",pcDefaultDrop->GetSelection());
			pcParentView->GetLooper()->PostMessage(pcMsg,pcParentView);
			delete(pcMsg);
			break;
		}	
	}
}

void SettingsWindow::SaveAddresses()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	cSettingsPath += "/Addresses";
	
	os::File* pcFile = new File(cSettingsPath,O_RDWR | O_CREAT);
	os::Settings* pcSettings = new Settings(pcFile);
	
	
	for (int i=0; i<pcListView->GetRowCount(); i++)
	{
		ListViewStringRow* pcRow = (ListViewStringRow*)pcListView->GetRow(i);
		pcSettings->AddString(pcRow->GetString(0).c_str(),pcRow->GetString(1));
	}
	pcSettings->Save();
	delete (pcSettings);
}

void SettingsWindow::LoadAddresses()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	mkdir( cSettingsPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	
	try
	{
		cSettingsPath += "/Addresses";
		os::File* pcFile = new File(cSettingsPath);
		os::Settings* pcSettings = new Settings(pcFile);
		pcSettings->Load();
	
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			ListViewStringRow* pcRow = new ListViewStringRow();
			String cTag = pcSettings->GetName(i);
			String cAddress;
			
			pcSettings->FindString(cTag.c_str(),&cAddress);
			
			pcRow->AppendString(cTag);
			pcRow->AppendString(cAddress);
			pcListView->InsertRow(pcRow);
		}
		delete (pcSettings);	
	}
	
	catch (...)
	{
		printf("cannot load\n");
	}	
}










