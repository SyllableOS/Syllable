#include "editstring.h"
#include "app.h"
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
#include "msgtotext.h"

// - EditStringWin ---------------------------------------------------------------

void EditStringWin::HandleMessage(Message *msg)
{
	switch(msg->GetCode()) {
		case ID_EDIT_CANCEL:
			{
				Message *m = new Message( ID_UNLOCK );
				m->AddString( "id", m_cID );
				m_pcMessenger->SendMessage( m );
			}
			Close();
			break;
		case ID_EDIT_SAVE:
			{
				Message *m = new Message(ID_SAVE_INDEX);
				m->AddString("data", m_pcText->GetBuffer()[0]);
				m->AddString("id", m_cID);
				m_pcMessenger->SendMessage(m);
			}
			Close();
			break;
		default:
			Window::HandleMessage(msg);
	}
}

EditStringWin::EditStringWin(const Rect & r, const String& name, const String& cID, const String& cData, Looper *pcParent)
	:Window(r, "EditStringWin", name.const_str() + std::string(" - ASE"), 0, CURRENT_DESKTOP)
{
	m_cData = cData;
	m_cName = name;
	m_cID = cID;
	m_pcParent = pcParent;
	m_pcMessenger = new Messenger(m_pcParent, m_pcParent);

	Rect bounds = GetBounds();

/*	int nType;
	int nCount;

	pcMsg->GetNameInfo(name.c_str(), &nType, &nCount);*/


 // --- Root layout node ---------------------------------------------------------

	LayoutNode* pcRoot = new VLayoutNode("pcRoot");

 // --- ListView -----------------------------------------------------------------

	m_pcText = new TextView(Rect(), "m_pcText", cData.c_str()); //MsgDataToText(name, nIndex, pcMsg).c_str());
	m_pcText->SetMultiLine(true);

	pcRoot->AddChild(m_pcText);

 // --- Buttons ------------------------------------------------------------------

	pcRoot->AddChild(new VLayoutSpacer("ButtonTopSpacer", 8, 8));
	LayoutNode *pcButtons = new HLayoutNode("pcButtons", 0.0f);
	pcButtons->AddChild(new HLayoutSpacer("ButtonLeftSpacer", 0, COORD_MAX));
 	pcButtons->AddChild(new Button(Rect(), "pcSave", "Save", new Message(ID_EDIT_SAVE)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonIntermediateSpacer", 8, 8));
 	pcButtons->AddChild(new Button(Rect(), "pcCancel", "Cancel", new Message(ID_EDIT_CANCEL)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonRightSpacer", 8, 8));
 	pcButtons->SameWidth("pcSave", "pcCancel", NULL);
 	pcRoot->AddChild(pcButtons);
	pcRoot->AddChild(new VLayoutSpacer("ButtonBottomSpacer", 8, 8));
	
	AddChild(new LayoutView(bounds, "pcLayout", pcRoot));
}

EditStringWin::~EditStringWin()
{
	delete m_pcMessenger;
}

bool EditStringWin::OkToQuit(void)
{
	Message *m = new Message( ID_UNLOCK );
	m->AddString( "id", m_cID );
	m_pcMessenger->SendMessage( m );
	return true;
}

