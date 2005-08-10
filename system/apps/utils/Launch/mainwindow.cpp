#include "mainwindow.h"
#include "commonfuncs.h"
#include "resources/Launch.h"

#include <util/application.h>
#include <gui/requesters.h>

MainWindow::MainWindow() : Window(Rect(0,0,1,1),"run_window",MSG_WINDOW_TITLE,WND_NOT_RESIZABLE)
{
	pcFileRequester = new FileRequester(FileRequester::LOAD_REQ,new os::Messenger(this));
	
	os::BitmapImage* pcIcon = GetImage("icon48x48.png");
	pcIcon->SetSize(os::Point(24,24));
	SetIcon(pcIcon->LockBitmap());

	Layout();
}

MainWindow::~MainWindow()
{
}

os::BitmapImage* MainWindow::GetImage(const String& cString)
{
	os::BitmapImage* pcImage;
	os::File cFile(open_image_file(get_image_id()));
	os::Resources cRes(&cFile);
	os::ResStream* pcStream = cRes.GetResourceStream(cString);
	pcImage = new BitmapImage(pcStream);
	return pcImage;
}

void MainWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"run_layout",NULL);

	VLayoutNode* pcRoot = new VLayoutNode("run_root");

	HLayoutNode* pcButtonNode = new HLayoutNode("button_node");
	pcButtonNode->SetVAlignment(ALIGN_CENTER);
	pcButtonNode->SetHAlignment(ALIGN_CENTER);
	pcButtonNode->SetBorders(Rect(5,0,5,0));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(0,0,1,1),"cancel_but",MSG_BUTTON_CANCEL,new Message(M_CANCEL)));
	pcButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(0,0,1,1),"ok_but",MSG_BUTTON_LAUNCH,new os::Message(M_LAUNCH)));
	pcButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcButtonNode->AddChild(pcBrowseButton = new Button(Rect(),"browse",MSG_BUTTON_BROWSE,new os::Message(M_BROWSE)));
	pcButtonNode->SameWidth("browse","ok_but","cancel_but",NULL);
	
	HLayoutNode* pcTextLayout = new HLayoutNode("text_layout");
	pcTextLayout->SetBorders(Rect(5,0,5,0));
	pcTextLayout->AddChild(pcRunString = new StringView(Rect(),"open_string",MSG_STRING_LAUNCH));
	pcTextLayout->AddChild(new HLayoutSpacer("",5,5));
	pcTextLayout->AddChild(pcTextDrop = new DropdownMenu(Rect(),"text"));	
	pcTextDrop->SetMinPreferredSize(30);
	pcTextDrop->SetMaxPreferredSize(30);
	LoadBuffer();
	pcTextDrop->SetSelection(0);
	HLayoutNode* pcStringLayout = new HLayoutNode("string_layout");
	pcStringLayout->SetBorders(Rect(0,0,5,0));
	pcStringLayout->AddChild(new ImageView(Rect(),"image_view",GetImage("icon48x48.png")));
	pcStringLayout->AddChild(new HLayoutSpacer("",5,5));
	pcStringLayout->AddChild(pcDescString = new StringView(Rect(),"desc_string",MSG_STRING_DESC));

	pcRoot->AddChild(pcStringLayout);
	pcRoot->AddChild(pcTextLayout);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer(""));
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
	ResizeTo(pcLayoutView->GetPreferredSize(false));
	
	SetDefaultButton(pcOkButton);
	SetFocusChild(pcTextDrop);

	
	pcTextDrop->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
	pcBrowseButton->SetTabOrder(NEXT_TAB_ORDER);
}

void MainWindow::HandleMessage(os::Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_BROWSE:
		{
			pcFileRequester->CenterInWindow(this);
			pcFileRequester->Show();
			pcFileRequester->MakeFocus();
			break;
		}
		
		case M_LAUNCH:
		{
			String cFile = pcTextDrop->GetCurrentString();
			
			if (cFile != "")
			{
				if (LaunchFile(cFile)==false)
				{
					os::String cError = MSG_ERROR_LAUNCHING_ONE + cFile + MSG_ERROR_LAUNCHING_TWO;
					os::Alert* pcError = new os::Alert(MSG_WINDOW_TITLE.c_str(),cError,os::Alert::ALERT_INFO,0,MSG_ALERT_BUTTON.c_str(),NULL);
					pcError->CenterInWindow(this);
					pcError->MakeFocus();
					pcError->Go(new os::Invoker());
				}
				else
				{
					m_cBuffer.insert(m_cBuffer.begin(),cFile);
					SaveBuffer();
					OkToQuit();
				}
			}
			break;
		}
		
		case M_CANCEL:
		{
			os::Application::GetInstance()->PostMessage(os::M_QUIT);
			break;
		}
		
		case M_LOAD_REQUESTED:
		{
			String cString;
			
			if (pcMessage->FindString("file/path",&cString) == 0)
			{
				int nSelection;
				if ( (nSelection = GetSelection(cString)) != -1)
				{
					pcTextDrop->SetSelection(GetSelection(cString));
				}	
				else
				{
					pcTextDrop->InsertItem(0,cString.c_str());
					pcTextDrop->SetSelection(0);
				}
			}
			break;
		}
	}
}

int MainWindow::GetSelection(const String& cString)
{
	for (int i=0; i<pcTextDrop->GetItemCount(); i++)
	{
		if (pcTextDrop->GetItem(i) == cString)
		{
			return i;
		}
	}
	return -1;
}


void MainWindow::LoadBuffer()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Launch");
	mkdir( cSettingsPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	try
	{
		cSettingsPath += "/Buffer";
		os::File* pcFile = new File(cSettingsPath);
		os::Settings* pcSettings = new Settings(pcFile);
		pcSettings->Load();
	
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			String cName = pcSettings->GetName(i);
			m_cBuffer.push_back(cName);
			pcTextDrop->AppendItem(cName);
		}
		delete (pcSettings);	
	}
	
	catch (...)
	{
	}	
}

void MainWindow::SaveBuffer()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Launch");
	cSettingsPath += "/Buffer";
	
	os::File* pcFile = new File(cSettingsPath,O_RDWR | O_CREAT);
	os::Settings* pcSettings = new Settings(pcFile);
	
	
	for (int i=0; i<m_cBuffer.size(); i++)
	{
		pcSettings->AddString(m_cBuffer[i].c_str(),m_cBuffer[i]);
	}
	pcSettings->Save();
	delete (pcSettings);	
}

bool MainWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage(os::M_QUIT);
	return true;
}





