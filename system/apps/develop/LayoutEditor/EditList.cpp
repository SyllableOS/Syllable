#include "EditList.h"
#include "messages.h"
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
//#include "msgtotext.h"
//#include "crect.h"

using namespace os;

enum
{
	ID_EDIT_OK,
	ID_EDIT_ADD,
	ID_EDIT_REMOVE
};

// - EditListWin ---------------------------------------------------------------

void EditListWin::HandleMessage(Message *msg)
{
	switch(msg->GetCode()) {
		case ID_EDIT_ADD:
		{
			ListViewStringRow* pcNewRow = new os::ListViewStringRow();
			pcNewRow->AppendString( m_pcText->GetBuffer()[0] );
			m_pcList->InsertRow( pcNewRow );
		}
		break;
		case ID_EDIT_REMOVE:
		{
			if( m_pcList->GetFirstSelected() >= 0 )
				m_pcList->RemoveRow( m_pcList->GetFirstSelected() );
		}
		break;
		default:
			Window::HandleMessage(msg);
	}
}

EditListWin::EditListWin(const Rect & r, const String& name, std::vector<os::String> acData)
	:Window(r, "EditListWin", name, 0, CURRENT_DESKTOP)
{
	m_cName = name;
	
	m_pcMessenger = NULL;

	Rect bounds = GetBounds();
 // --- Root layout node ---------------------------------------------------------

	LayoutNode* pcRoot = new VLayoutNode("pcRoot");
	pcRoot->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	
 // ---HLayoutNode ----------------------------------------------------------------
 
 	HLayoutNode* pcHLayout = new HLayoutNode( "hlayout" );

 // --- ListView -----------------------------------------------------------------

	VLayoutNode* pcVList = new VLayoutNode( "vlayout" );
			
	m_pcList = new os::ListView( os::Rect(), "list", 
							os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL );
	m_pcList->InsertColumn( "Entry", 10000 );
	m_pcList->SetAutoSort( false );
	m_pcList->SetTarget( this );

	for( uint i = 0; i < acData.size(); i++ )
	{
		ListViewStringRow* pcNewRow = new os::ListViewStringRow();
		pcNewRow->AppendString( acData[i] );
		m_pcList->InsertRow( pcNewRow );
	}
	
	pcVList->AddChild( m_pcList );
	pcVList->AddChild( new VLayoutSpacer( "", 5, 5 ) );
	
 // --- TextView ---------------------------------------------------------------

	m_pcText = new TextView(Rect(), "m_pcText", ""); 
	m_pcText->SetMultiLine(false);

	pcVList->AddChild(m_pcText);
	
	pcHLayout->AddChild( pcVList );
	pcHLayout->AddChild(new HLayoutSpacer("", 5.0f, 5.0f));
	
 // --- Buttons ------------------------------------------------------------------

	
	LayoutNode *pcButtons = new VLayoutNode("pcAddButtons", 0.0f);
 	pcButtons->AddChild(new Button(Rect(), "pcAdd", "Add", new Message(ID_EDIT_ADD)), 0.0f);
	pcButtons->AddChild(new VLayoutSpacer("", 5, 5));
 	pcButtons->AddChild(new Button(Rect(), "pcRemove", "Remove", new Message(ID_EDIT_REMOVE)), 0.0f);
	pcButtons->AddChild(new VLayoutSpacer(""));

 	pcButtons->SameWidth("pcAdd", "pcRemove", NULL);

 	pcHLayout->AddChild( pcButtons );

 	pcRoot->AddChild( pcHLayout );
	
	AddChild(new LayoutView(bounds, "pcLayout", pcRoot));
}

EditListWin::~EditListWin()
{
	delete m_pcMessenger;
}

void EditListWin::SetTarget( os::Handler* pcParent )
{
	m_pcParent = pcParent;
	m_pcMessenger = new Messenger(m_pcParent);
}

std::vector<os::String> EditListWin::GetData()
{
	std::vector<os::String> acList;
	for( uint i = 0; i < m_pcList->GetRowCount(); i++ )
	{
		os::ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcList->GetRow( i ) );
		acList.push_back( pcRow->GetString( 0 ) );
	}
	return( acList );
}

bool EditListWin::OkToQuit(void)
{
	Message *m = new Message( M_EDIT_LIST_FINISH );
	m_pcMessenger->SendMessage( m );
	return false;
}
















