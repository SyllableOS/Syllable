#include "loginview.h"
#include "commonfuncs.h"
#include "messages.h"
#include "keymap.h"

#include <stdio.h>
#include <gui/requesters.h>
#include <pwd.h>

LoginView::LoginView(Rect cRect,Window* pcParent) : View(cRect,"login_view")
{	
	pcParentWindow = pcParent;
	Layout();
	
	String cName = GetHighlightName();
	if (cName != "")
	{
		FindUser(cName);
	}


	pcParentWindow->SetFocusChild(pcPassText);
	pcParentWindow->SetDefaultButton(pcLoginButton);
	SetTabOrder( NO_TAB_ORDER );
}

void LoginView::Layout()
{
	VLayoutNode* pcRoot = new VLayoutNode("root");
	
	pcUserIconView = new os::IconView(Rect(1,2,2000,82),"",CF_FOLLOW_ALL);
	pcUserIconView->SetSelChangeMsg(new Message(M_SEL_CHANGED));
	PopulateIcons();
	pcUserIconView->SetTabOrder( 1 );

	FrameView* pcFrameView = new FrameView(Rect(0,0,2000,84),"","");
	pcFrameView->AddChild(pcUserIconView);
	AddChild(pcFrameView);
	
	pcLayoutView = new LayoutView(Rect(0,89,GetBounds().Width(),GetBounds().Height()),"layout_view",NULL);
	
	HLayoutNode* pcPassNode = new HLayoutNode("pass_node");
	pcPassNode->AddChild(new HLayoutSpacer("",2,2));
	pcPassNode->AddChild(pcPassString = new StringView(Rect(),"pass_string","Password:"));
	pcPassNode->AddChild(new HLayoutSpacer("",2,2));
	pcPassNode->AddChild(pcPassText = new TextView(Rect(),"pass_text",""));
	pcPassText->SetPasswordMode(true);
	pcPassText->SetMaxPreferredSize(28,1);
	pcPassText->SetMinPreferredSize(28,1);
	pcPassText->SetTabOrder( 2 );
	pcPassNode->AddChild(new HLayoutSpacer("",5,5));
	pcPassNode->AddChild(pcLoginButton = new Button(Rect(),"login_but","_Login",new Message(M_LOGIN)));
	pcLoginButton->SetTabOrder( 3 );
	pcPassNode->AddChild(new HLayoutSpacer("",2,2));
	pcPassNode->SameHeight("login_but",NULL);
	pcPassNode->ExtendMaxSize(pcPassNode->GetPreferredSize(false));
	

	HLayoutNode* pcKeymapNode = new HLayoutNode("keymap_node");
	pcKeymapNode->AddChild(selector = new KeymapSelector(os::Messenger(pcParentWindow)));
	
	
	HLayoutNode* pcOtherNode = new HLayoutNode("other_node");
	pcOtherNode->SetHAlignment(ALIGN_RIGHT);	
	pcOtherNode->AddChild(pcShutdownButton = new Button(Rect(),"shut_but","_Shutdown",new Message(M_SHUTDOWN)));
	pcShutdownButton->SetTabOrder( 4 );
	pcOtherNode->AddChild(new HLayoutSpacer("",150,150));
	pcOtherNode->AddChild(pcDateString = new StringView(Rect(),"","9 Aug 05, 10:00am"));
	UpdateTime();
	
	pcRoot->AddChild(pcPassNode);
	pcRoot->AddChild(new os::VLayoutSpacer("",10,10));
	pcRoot->AddChild(pcKeymapNode);
	pcRoot->AddChild(new os::VLayoutSpacer("",10,10));
	pcRoot->AddChild(pcOtherNode);
	
	pcRoot->SameHeight("shut_but","login_but",NULL);
	pcRoot->SameWidth("shut_but","login_but",NULL);
	
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
	
	ResizeTo(os::Point(pcLayoutView->GetPreferredSize(false).x,GetBounds().Height()));
}

void LoginView::PopulateIcons()
{
 	try
 	{
 		FILE* fp = fopen("/etc/passwd","r");
 		struct passwd* psPwd;
  		int nIcon = 0;
  		while((psPwd = fgetpwent( fp )) != NULL ) 
  		{	
  			// Let's remove users that don't need to be there and then add them to the iconview
  			// System users (eg "www", "mail") have uids between 1 and 99. Filter them out.
			if( psPwd->pw_uid >= 100 || psPwd->pw_uid == 0 )
  			{
    			pcUserIconView->AddIcon(GetImageFromIcon(psPwd->pw_name),new IconData());
  				pcUserIconView->AddIconString(nIcon,psPwd->pw_name);
            	nIcon++;
            }	
  		}
		pcUserIconView->Layout();
  	}
  	
  	catch (...)
  	{
  		Alert* pcError = new Alert("Unable to load users","I am unable to load users, please contact the Syllable development group.",0x00,"Oh no!!!",NULL);
  		pcError->Go(new Invoker());
  	}	
}

void LoginView::UpdateTime()
{
	DateTime cDate = DateTime::Now();
	pcDateString->SetString(cDate.GetDate());
}

bool LoginView::GetUserNameAndPass(String* cName, String* cPass)
{
	String cIconName;
	int nIcon=-1;
	bool bSelected=false;
	
	for (int i=0; i<pcUserIconView->GetIconCount(); i++)
	{
		bSelected = pcUserIconView->GetIconSelected(i);
		
		if (bSelected == true)
		{
			nIcon = i;
			cIconName = pcUserIconView->GetIconString(nIcon,0);
			break;
		}
	}
	
	if (nIcon != -1)
	{ 
		*cName = String(cIconName);
		*cPass = String(pcPassText->GetBuffer()[0]);
	}
	
	return bSelected;	
}

void LoginView::ClearPassword()
{
	pcPassText->Clear();
}

void LoginView::Focus()
{
	pcPassText->MakeFocus( true );
}

void LoginView::AttachedToWindow()
{
	Focus();
}

void LoginView::FindUser(const String& cName)
{
	for (uint i=0; i< pcUserIconView->GetIconCount(); i++)
	{
		if (pcUserIconView->GetIconString(i,0) == cName)
		{
			pcUserIconView->SetIconSelected(i,true);
		}
	}
}

void LoginView::HandleMessage(os::Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case KeymapSelector::M_SELECT:
        {
        	printf("changed\n");
        	break;
        }
	}
}







