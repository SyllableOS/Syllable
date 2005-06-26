#include <gui/fontrequester.h>

#include <util/application.h>
#include <util/message.h>

#include <gui/font.h>
#include <gui/guidefines.h>
#include <util/shortcutkey.h>

#include "fontrequesterview.h"
#include "fonthandle.h"


class FontRequester::Private
{
public:
	Private()
	{
		m_bShowAdvanced = false;
		m_pcTarget = NULL;  //target is null to start
		m_pcFont = new Font();
//		m_pcFont->AddRef();  //add a reference to the font
	}
	
	~Private()
	{
		//if( m_pcFont ) m_pcFont->Release();
	}
	
	Messenger* m_pcTarget;
	Font* m_pcFont;
	bool m_bShowAdvanced;
};


FontRequester::FontRequester(Messenger* pcTarget, bool bEnableAdvanced) : Window(Rect(0,0,1,1),"font_requester","Select A Font...")
{
	Lock();

	m = new Private();
   	pcTarget == NULL ? m->m_pcTarget = new os::Messenger(os::Application::GetInstance()) : m->m_pcTarget = pcTarget;
	m->m_bShowAdvanced = bEnableAdvanced;
	
	
	LayoutView* pcView = new LayoutView(GetBounds(),"",NULL,CF_FOLLOW_ALL);
	VLayoutNode* pcRoot = new VLayoutNode("font_requester_main_view_layout");
	
	HLayoutNode* pcButtonNode = new HLayoutNode("fontrequester_button_node");
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"fontrequester_cancel_but","_Cancel",new Message(M_FONT_REQUESTER_CANCELED)));
	pcButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"fontrequester_ok_but","_Okay",new Message(M_FONT_REQUESTED)));
	pcButtonNode->SameWidth("fontrequester_ok_but","fontrequester_cancel_but",NULL);	
	pcButtonNode->LimitMaxSize(pcButtonNode->GetPreferredSize(false));	
	
	pcRoot->AddChild(pcRequesterView = new FontRequesterView(GetDefaultFontHandle(),m->m_bShowAdvanced));
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));

	pcView->SetRoot(pcRoot);
	AddChild(pcView);
	
	SetDefaultButton(pcOkButton);
	SetFocusChild(pcRequesterView);
	AddShortcut(os::ShortcutKey(27), new os::Message(M_FILE_REQUESTER_CANCELED));

	ResizeTo(pcView->GetPreferredSize(false).x+350,pcView->GetPreferredSize(false).y+75);
	CenterInScreen();

	Unlock();
}

FontRequester::~FontRequester()
{
	delete m;  //delete private
}

void FontRequester::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_FONT_REQUESTED:
		{
			/*a simple test for null...  it should never be null, but you never know :)*/
			if (m->m_pcTarget != NULL)
			{
				m->m_pcFont->Release();
				m->m_pcFont = new Font(GetFontProperties(pcRequesterView->GetFontHandle()));
				//m->m_pcFont->AddRef();

				Message *pcMsg = new Message( M_FONT_REQUESTED );
				pcMsg->AddPointer( "source", this );
				pcMsg->AddObject("font",*m->m_pcFont);
				m->m_pcTarget->SendMessage( pcMsg );
			}
			else
				dbprintf("No place to send messages to.\n");

			Show(false);			
			break;
		}
		
		case M_FONT_REQUESTER_CANCELED:
		{
			/*a simple test for null...  it should never be null, but you never know :)*/
			if (m->m_pcTarget != NULL)
			{
				Message *pcMsg = new Message( M_FONT_REQUESTER_CANCELED );
				pcMsg->AddPointer( "source", this );
				m->m_pcTarget->SendMessage( pcMsg );
			}
			else
				dbprintf("No place to send messages to.\n");
			
			Show(false);
			break;
		}
				
		default:
			Window::HandleMessage(pcMessage);
			break;
	}
}

bool FontRequester::OkToQuit()
{
	/*a simple test for null...  it should never be null, but you never know :)*/
	if (m->m_pcTarget != NULL)
	{
		Message *pcMsg = new Message( M_FONT_REQUESTER_CANCELED );
		pcMsg->AddPointer( "source", this );
		m->m_pcTarget->SendMessage( pcMsg );
	}
	else
		dbprintf("No place to send messages to.\n");

	Show(false);
	return ( false );
}


	






