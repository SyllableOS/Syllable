#include "loginview.h"
#include "commonfuncs.h"
#include "messages.h"
#include "keymap.h"

#include <gui/requesters.h>

#include <stdio.h>
#include <pwd.h>


class LoginIconData : public os::IconData
{
public:
	os::String m_zDisplayName;
	os::String m_zUserName;
};

LoginView::LoginView( Rect cRect, Window* pcParent, AppSettings* pcAppSettings ) : View(cRect,"login_view")
{	
	pcParentWindow = pcParent;
	pcSettings = pcAppSettings;
	Layout();
	
	pcParentWindow->SetDefaultButton(pcLoginButton);
	SetTabOrder( NO_TAB_ORDER );
}

void LoginView::Layout()
{
	VLayoutNode* pcRoot = new VLayoutNode("root");
	
	pcUserIconView = new os::IconView(Rect(1,2,GetBounds().Width()-2,82),"user_iconview",CF_FOLLOW_ALL);
	pcUserIconView->SetTarget( this );
	pcUserIconView->SetSelChangeMsg(new Message(M_SEL_CHANGED));
	PopulateIcons();
	pcUserIconView->SetTabOrder( NEXT_TAB_ORDER );

	FrameView* pcFrameView = new FrameView(Rect(0,1,GetBounds().Width()-1,84),"frame_view","", CF_FOLLOW_ALL);
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
	pcPassText->SetTabOrder( NEXT_TAB_ORDER );
	pcPassNode->AddChild(new HLayoutSpacer("",5,5));
	pcPassNode->AddChild(pcLoginButton = new Button(Rect(),"login_but","_Login",new Message(M_LOGIN)));
	pcLoginButton->SetTabOrder( NEXT_TAB_ORDER );
	pcPassNode->AddChild(new HLayoutSpacer("",2,2));
	pcPassNode->SameHeight("login_but",NULL);
	pcPassNode->ExtendMaxSize(pcPassNode->GetPreferredSize(false));
	

	HLayoutNode* pcKeymapNode = new HLayoutNode("keymap_node");
	pcKeymapSelector = new KeymapSelector( os::Messenger( pcParentWindow ) );
	pcKeymapSelector->SetTabOrder( NEXT_TAB_ORDER );
	pcKeymapNode->AddChild( pcKeymapSelector );
	
	
	HLayoutNode* pcOtherNode = new HLayoutNode("other_node");
	pcOtherNode->SetHAlignment(ALIGN_RIGHT);	
	pcOtherNode->AddChild(pcShutdownButton = new Button(Rect(),"shut_but","_Shutdown",new Message(M_SHUTDOWN)));
	pcShutdownButton->SetTabOrder( NEXT_TAB_ORDER );
	pcOtherNode->AddChild(new HLayoutSpacer("",150,150));
	pcOtherNode->AddChild(pcDateString = new StringView(Rect(),"","9 Aug 05, 10:00am"));
	UpdateTime();
	
	pcRoot->AddChild(pcPassNode);
//	pcRoot->AddChild(new os::VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcKeymapNode);
//	pcRoot->AddChild(new os::VLayoutSpacer("",5,5));
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
  			// Let's remove users that don't need to be there and then add the rest to the iconview
  			// System users (eg "www", "mail") have uids between 1 and 99. Filter them out.
			if( psPwd->pw_uid >= 100 || psPwd->pw_uid == 0 )
  			{
  				LoginIconData* pcData = new LoginIconData();
  				pcData->m_zDisplayName = psPwd->pw_gecos;
  				pcData->m_zUserName = psPwd->pw_name;
  				
    			pcUserIconView->AddIcon(GetImageFromIcon(psPwd->pw_name), pcData);
  				pcUserIconView->AddIconString(nIcon,pcData->m_zDisplayName);
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

void LoginView::AllAttached()
{
	String cName = pcSettings->GetActiveUser();
	if (cName != "")
	{
		FindUser(cName);
		pcPassText->MakeFocus( true );
	}
	else
	{
		pcUserIconView->MakeFocus( true );
	}
	
	printf( "login button height: %.0f    shutdown height: %.0f\n", pcLoginButton->GetBounds().Height(), pcShutdownButton->GetBounds().Height() );
}

void LoginView::UpdateTime()
{
	DateTime cDate = DateTime::Now();
	pcDateString->SetString(cDate.GetDate());
}

bool LoginView::GetUserNameAndPass(String* cName, String* cPass)
{
	String cUserName;
	int nIcon=-1;
	bool bSelected=false;
	
	for (int i=0; i<pcUserIconView->GetIconCount(); i++)
	{
		bSelected = pcUserIconView->GetIconSelected(i);
		
		if (bSelected == true)
		{
			nIcon = i;
			LoginIconData* pcData = static_cast<LoginIconData*>( pcUserIconView->GetIconData( nIcon ) );
			cUserName = pcData->m_zUserName;
			break;
		}
	}
	
	if (nIcon != -1)
	{ 
		*cName = String(cUserName);
		*cPass = String(pcPassText->GetBuffer()[0]);
	}
	
	return bSelected;	
}

void LoginView::ClearPassword()
{
	pcPassText->Clear();
}

/* FocusPassword(): this is so we can automatically focus the password field when the user gets a password wrong */
void LoginView::FocusPassword()
{
	pcPassText->MakeFocus( true );
}

void LoginView::Reload()
{
	String cIcon;
	for (uint i=0; i<pcUserIconView->GetIconCount(); i++)
	{
		if (pcUserIconView->GetIconSelected(i))
			cIcon = pcUserIconView->GetIconString(i,0);
	}
	pcUserIconView->Clear();
	PopulateIcons();
	pcUserIconView->Flush();
	pcUserIconView->Sync();
	pcUserIconView->Paint(pcUserIconView->GetBounds());
	FindUser(cIcon);
}

void LoginView::FindUser(const String& cName)
{
	for (uint i=0; i< pcUserIconView->GetIconCount(); i++)
	{
		LoginIconData* pcData = static_cast<LoginIconData*>( pcUserIconView->GetIconData( i ) );
		if (pcData->m_zUserName == cName)
		{
			pcUserIconView->SetIconSelected(i,true);
			pcUserIconView->ScrollToIcon( i );
			SetKeymapForUser( cName );
		}
	}
}

void LoginView::SetKeymapForUser( const String& zName )
{
	String zKeymap = pcSettings->FindKeymapForUser( zName );

	if( zKeymap != "" ) pcKeymapSelector->SelectKeymap( zKeymap );
}

void LoginView::HandleMessage(os::Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_SEL_CHANGED:
		{
			/* User has selected a different user; automatically select the right keymap for that user. */
			int nCount = pcUserIconView->GetIconCount();
			bool bFound = false;
			for( int i = 0; i < nCount; i++ )
			{
				if( pcUserIconView->GetIconSelected( i ) )
				{
					LoginIconData* pcData;
					pcData = static_cast<LoginIconData*>( pcUserIconView->GetIconData( i ) );
					SetKeymapForUser( pcData->m_zUserName );

					break;
				}
			}
			break;
		}
		default:
		{
			View::HandleMessage( pcMessage );
		}
	}
}




