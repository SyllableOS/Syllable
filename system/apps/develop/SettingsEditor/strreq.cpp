#include <util/application.h>
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <gui/dropdownmenu.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <storage/file.h>
#include <util/resources.h>
#include <iostream>
#include <stdio.h>
#include "strreq.h"

#define ID_OK		1
#define ID_CANCEL	2

StringRequester::StringRequester( const std::string& cTitle, const std::string& cText )
	:Window(Rect(0,0,300, 90), "String Requester", cTitle, 0, CURRENT_DESKTOP)
{
	Rect bounds = GetBounds();

 // --- Root layout node ---------------------------------------------------------

	LayoutNode* pcRoot = new VLayoutNode("pcRoot");

	pcRoot->AddChild(new VLayoutSpacer("TopSpacer", 8, 8));

 // --- StringView ---------------------------------------------------------------

	StringView* pcBodyText = new StringView( Rect(), "pcBodyText", cText.c_str() );

	pcRoot->AddChild( pcBodyText );

	pcRoot->AddChild(new VLayoutSpacer("MiddleSpacer", 8, 8));

 // --- ListView -----------------------------------------------------------------

	m_pcTextView = new TextView(Rect(), "m_pcTextView", "");

	pcRoot->AddChild(m_pcTextView);

 // --- Buttons ------------------------------------------------------------------

	pcRoot->AddChild(new VLayoutSpacer("ButtonTopSpacer", 8, 8));
	LayoutNode *pcButtons = new HLayoutNode("pcButtons");
	pcButtons->AddChild(new HLayoutSpacer("ButtonLeftSpacer", 0, COORD_MAX));
 	pcButtons->AddChild(new Button(Rect(), "pcOK", "OK", new Message(ID_OK)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonIntermediateSpacer", 8, 8));
 	pcButtons->AddChild(new Button(Rect(), "pcCancel", "Cancel", new Message(ID_CANCEL)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonRightSpacer", 8, 8));
 	pcButtons->SameWidth("pcOK", "pcCancel", NULL);
 	pcRoot->AddChild(pcButtons);
	pcRoot->AddChild(new VLayoutSpacer("ButtonBottomSpacer", 8, 8));
	
	AddChild(new LayoutView(bounds, "pcLayout", pcRoot));
	
	m_pcInvoker = NULL;
}

StringRequester::~StringRequester()
{
}

void StringRequester::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() ) {
		case ID_OK:
			{
				Message* msg = m_pcInvoker->GetMessage();
				msg->AddString( "_string", GetString() );
				m_pcInvoker->Invoke();
				Close();
			}
			break;
		case ID_CANCEL:
			Close();
			break;
		default:
			Window::HandleMessage( pcMessage );
	}
}
  
int  StringRequester::Go()
{
}

void StringRequester::Go( Invoker* pcInvoker )
{
	m_pcInvoker = pcInvoker;
	Show();
}
	
std::string StringRequester::GetString()
{
	if( m_pcTextView ) return m_pcTextView->GetBuffer()[0];
	else return "";
}

void StringRequester::SetString( std::string& cText)
{
//	if( m_pcTextView ) return m_pcTextView->SetText( cText );
}

