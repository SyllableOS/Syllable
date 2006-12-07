#include <gui/colorrequester.h>
#include <gui/colorselector.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/gfxtypes.h>
#include <util/application.h>
#include <util/message.h>
#include <gui/guidefines.h>

using namespace os;



class ColorRequester::Private
{
public:
	Private(os::Messenger* pcTarget)
	{
		
		if (pcTarget == NULL)
		{
			m_pcTarget = new os::Messenger(os::Application::GetInstance());
			m_bDeleteTarget = true;
		}
		else
		{
			m_pcTarget = pcTarget;
			m_bDeleteTarget = false;
		}
		
		m_pcSelector = NULL;
		m_pcOkButton = NULL;
		m_pcCancelButton = NULL;
		m_pcView = NULL;
	}
	
	~Private()
	{
		if (m_bDeleteTarget)
			delete m_pcTarget;
	}
	
	ColorSelector* m_pcSelector;
	Messenger* m_pcTarget;
	Button* m_pcOkButton;
	Button* m_pcCancelButton;
	os::LayoutView* m_pcView;
	bool m_bDeleteTarget;
};

ColorRequester::ColorRequester(os::Messenger* pcTarget) : os::Window(os::Rect(0,0,300,200), "color_requester_window","Select a color...")
{
	m = new Private(pcTarget);
	_Layout();
}

void ColorRequester::_Layout()
{
	m->m_pcView = new LayoutView(GetBounds(),"",NULL,CF_FOLLOW_ALL);
	VLayoutNode* pcRoot = new VLayoutNode("color_requester_main_view_layout");
	
	HLayoutNode* pcButtonNode = new HLayoutNode("color_requester_button_node");
	pcButtonNode->AddChild(m->m_pcCancelButton = new Button(Rect(),"color_requester_cancel_but","_Cancel",new Message(M_COLOR_REQUESTER_CANCELED)));
	pcButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcButtonNode->AddChild(m->m_pcOkButton = new Button(Rect(),"color_requester_ok_but","_Okay",new Message(M_COLOR_REQUESTED)));
	pcButtonNode->SameWidth("color_requester_ok_but","color_requester_cancel_but",NULL);	
	pcButtonNode->LimitMaxSize(pcButtonNode->GetPreferredSize(false));	
	
	
	pcRoot->AddChild(m->m_pcSelector = new os::ColorSelector(os::Rect(0,0,1,1),"",new os::Message(M_COLOR_REQUESTER_CHANGED)));
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));

	m->m_pcView->SetRoot(pcRoot);
	AddChild(m->m_pcView);
	
	SetDefaultButton(m->m_pcOkButton);
	SetFocusChild(m->m_pcSelector);
	
	CenterInScreen();
}

ColorRequester::~ColorRequester()
{
	delete m;
}

void ColorRequester::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_COLOR_REQUESTED:
		{
			/*a simple test for null...  it should never be null, but you never know :)*/
			if (m->m_pcTarget != NULL)
			{
				Message cMsg = Message( M_COLOR_REQUESTED );
				cMsg.AddPointer( "source", this );
				cMsg.AddColor32("color",m->m_pcSelector->GetValue().AsColor32());
				m->m_pcTarget->SendMessage( &cMsg );
			}
			else
				dbprintf("No place to send messages to.\n");

			Show(false);			
			break;
		}
		
		case M_COLOR_REQUESTER_CANCELED:
		{
			/*a simple test for null...  it should never be null, but you never know :)*/
			if (m->m_pcTarget != NULL)
			{
				Message cMsg = Message( M_COLOR_REQUESTER_CANCELED );
				cMsg.AddPointer( "source", this );
				m->m_pcTarget->SendMessage( &cMsg );
			}
			else
				dbprintf("No place to send messages to.\n");
			
			Show(false);
			break;
		}
		
		case M_COLOR_REQUESTER_CHANGED:
		{
			if (m->m_pcTarget != NULL)
			{
				Message cMsg = Message( M_COLOR_REQUESTER_CHANGED );
				cMsg.AddColor32("color",m->m_pcSelector->GetValue().AsColor32());
				m->m_pcTarget->SendMessage( &cMsg );
			}
			else
				dbprintf("No place to send messages to.\n");

			break;
		}
				
		default:
			Window::HandleMessage(pcMessage);
			break;
	}
}

bool ColorRequester::OkToQuit()
{
	/*a simple test for null...  it should never be null, but you never know :)*/
	if (m->m_pcTarget != NULL)
	{
		Message cMsg = Message( M_COLOR_REQUESTER_CANCELED );
		cMsg.AddPointer( "source", this );
		m->m_pcTarget->SendMessage( &cMsg );
	}
	else
		dbprintf("No place to send messages to.\n");

	Show(false);
	return ( false );
}



