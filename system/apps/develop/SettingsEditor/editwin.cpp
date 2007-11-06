#include "editwin.h"
#include "editstring.h"
#include "mainwin.h"
#include "app.h"

#include <util/application.h>
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <gui/dropdownmenu.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <storage/file.h>
#include <util/resources.h>
#include <util/settings.h>
#include <iostream>
#include <stdio.h>

#include "msgtotext.h"
#include "resources/SettingsEditor.h"

void EditWin::_RebuildArray()
{
	int nType;
	int nCount;
	int i;
	nCount = m_pcItems->GetRowCount();
	m_pcMessage->GetNameInfo(m_cName.c_str(), &nType, NULL);
	m_pcMessage->RemoveName(m_cName.c_str());
	for(i = 0; i < nCount; i++) {
		ListViewStringRow *lvs = (ListViewStringRow *)m_pcItems->GetRow(i);
		switch(nType) {
			case T_STRING:
				m_pcMessage->AddString(m_cName.c_str(), lvs->GetString(1));
				break;
			case T_INT8:
				m_pcMessage->AddInt8(m_cName.c_str(), atol(lvs->GetString(1).c_str()) & 0xFF);
				break;
			case T_INT16:
				m_pcMessage->AddInt16(m_cName.c_str(), atol(lvs->GetString(1).c_str()) & 0xFFFF);
				break;
			case T_INT32:
				m_pcMessage->AddInt32(m_cName.c_str(), atol(lvs->GetString(1).c_str()));
				break;
			case T_INT64:	// BUG: only 32 bits here!
				m_pcMessage->AddInt64(m_cName.c_str(), atol(lvs->GetString(1).c_str()));
				break;
			case T_FLOAT:
				m_pcMessage->AddFloat(m_cName.c_str(), atof(lvs->GetString(1).c_str()));
				break;
			case T_DOUBLE:
				{
					double val = 0;
					sscanf( lvs->GetString(1).c_str(), "%lf", &val );
					m_pcMessage->AddDouble( m_cName.c_str(), val );
				}
				break;
			case T_POINT:
				{
					float x = 0, y = 0;
					sscanf( lvs->GetString(1).c_str(), "%f, %f", &x, &y );
					m_pcMessage->AddPoint( m_cName.c_str(), Point( x, y ) );
				}
				break;
			case T_IPOINT:
				{
					int x = 0, y = 0;
					sscanf( lvs->GetString(1).c_str(), "%d, %d", &x, &y );
					m_pcMessage->AddIPoint( m_cName.c_str(), IPoint( x, y ) );
				}
				break;
			case T_RECT:
				{
					float a = 0, b = 0, c = 0, d = 0;
					sscanf( lvs->GetString(1).c_str(), "%f, %f, %f, %f", &a, &b, &c, &d );
					m_pcMessage->AddRect( m_cName.c_str(), Rect( a, b, c, d ) );
				}
				break;
			case T_IRECT:
				{
					int a = 0, b = 0, c = 0, d = 0;
					sscanf( lvs->GetString(1).c_str(), "%d, %d, %d, %d", &a, &b, &c, &d );
					m_pcMessage->AddIRect( m_cName.c_str(), IRect( a, b, c, d ) );
				}
				break;
		}
	}
}

void EditWin::_EditItem( int nIndex )
{
	ListViewStringRow *lvr = (ListViewStringRow *)m_pcItems->GetRow( nIndex );
	lvr->SetIsSelectable(false);

	Window *w = NULL;

	switch( m_nType ) {
		case T_STRING:
		case T_INT16:
		case T_INT32:
		case T_INT64:
		case T_INT8:
		case T_FLOAT:
		case T_DOUBLE:
		case T_POINT:
		case T_IPOINT:
		case T_RECT:
		case T_IRECT:
			w = new EditStringWin(Rect(0,0,250, 100), m_cName, lvr->GetString(2), lvr->GetString(1), this);
			w->CenterInWindow(this);
			break;
		case T_MESSAGE:
			{
				Settings* pcMsg = new Settings(0);
				if( m_pcMessage->FindMessage( m_cName.c_str(), (Message *)pcMsg, nIndex ) == 0 ) {
					w = new MainWin( Rect(0,0,250, 100), (Message *)pcMsg, this, lvr->GetString(2) );
					w->CenterInWindow(this);
				}
			}
			break;
	}

	if( w ) w->Show();
}

void EditWin::_ReNumber()
{
	ListViewStringRow *lvr;
	char bfr[16];

	for(int i = 0; i < (int)m_pcItems->GetRowCount(); i++) {
		lvr = (ListViewStringRow *)m_pcItems->GetRow(i);
		sprintf(bfr, "%d", i);
		lvr->SetString(0, bfr);
		m_pcItems->InvalidateRow(i, 0);
	}

}

void EditWin::HandleMessage(Message *msg)
{
	switch(msg->GetCode()) {
		case ID_EDIT_ADD:
			{
				int sel = m_pcItems->GetFirstSelected();
				ListViewStringRow *lvs = new ListViewStringRow;
				char bfr[16];
				sprintf(bfr, "%d", 0);
				lvs->AppendString(bfr);
				lvs->AppendString("");
				sprintf(bfr, "%d", ++m_nIDCtr);	
				lvs->AppendString(bfr);
				m_pcItems->ClearSelection();
				m_pcItems->InsertRow(sel, lvs);
				_ReNumber();
			}
			break;
		case ID_EDIT_DELETE:
			{
				int sel = m_pcItems->GetFirstSelected();
				if(sel != -1) {
					m_pcItems->RemoveRow(sel);
					_ReNumber();
				}
			}
			break;
		case ID_EDIT_ITEM:
			{
				int sel = m_pcItems->GetFirstSelected();
				if(sel != -1) {
					_EditItem( sel );
				}
			}
			break;
		case ID_EDIT_CANCEL:
			{
				Message *m = new Message( ID_UNLOCK );
				m->AddString( "id", m_cID );
				m_pcMessenger->SendMessage( m );
			}
			Close();
			break;
		case ID_EDIT_SAVE:
			_RebuildArray();
			{
				Message *m = new Message( ID_REFRESH );
				m->AddString( "id", m_cID );
				m_pcMessenger->SendMessage( m );
			}
			Close();
			break;
		case ID_SAVE_INDEX:
			{
				String id;
				String s;
				if(msg->FindString("id", &id) == 0) {
					ListViewStringRow *lvr;
					for(int i = 0; i < (int)m_pcItems->GetRowCount(); i++) {
						lvr = (ListViewStringRow *)m_pcItems->GetRow(i);
						if( lvr->GetString(2) == id ) {
							lvr->SetIsSelectable(true);				
							if(msg->FindString("data", &s) == 0) {
								lvr->SetString(1, s);
							}
							m_pcItems->InvalidateRow(i, 0);
							break;
						}
					}
				}
//				cout << "SAVE_INDEX" << endl;
			}
			break;
		case ID_UNLOCK:
			{
				String id;
				if(msg->FindString("id", &id) == 0) {
					ListViewStringRow *lvr;
					for(int i = 0; i < m_pcItems->GetRowCount(); i++) {
						lvr = (ListViewStringRow *)m_pcItems->GetRow(i);
						if( lvr->GetString(2) == id ) {
							lvr->SetIsSelectable(true);				
							break;
						}
					}
				}
			}
			break;
		default:
			Window::HandleMessage(msg);
	}
}

EditWin::EditWin(const Rect & r, const String& name, Message *pcMsg, Looper *pcParent, const String& id)
	:Window(r, "EditWin", String().Format("%s - %s", name.c_str(),ID_SETTINGS_EDITOR_WINDOW.c_str()), 0, CURRENT_DESKTOP)
{
	Rect bounds = GetBounds();

	m_pcMessage = pcMsg;
	m_cName = name;
	m_cID = id;
	m_nIDCtr = 0;
	m_pcMessenger = new Messenger(pcParent, pcParent);

	int nCount;

	pcMsg->GetNameInfo(name.c_str(), &m_nType, &nCount);


 // --- Root layout node ---------------------------------------------------------

	LayoutNode* pcRoot = new VLayoutNode("pcRoot");
 
 // --- ListView -----------------------------------------------------------------

	m_pcItems = new ListView(Rect(), "m_pcListView", ListView::F_NO_AUTO_SORT|ListView::F_RENDER_BORDER);
	
	m_pcItems->InsertColumn(ID_LISTVIEW_INDEX.c_str(), 50);
	m_pcItems->InsertColumn(ID_LISTVIEW_VALUE.c_str(), 250);

	for(int i = 0; i < nCount; i++) {
		ListViewStringRow *lvs = new ListViewStringRow;
		char bfr[16];
		sprintf(bfr, "%d", i);
		lvs->AppendString(bfr);
		lvs->AppendString(MsgDataToText(name, i, pcMsg));
		sprintf(bfr, "%d", ++m_nIDCtr);	
		lvs->AppendString(bfr);
		m_pcItems->InsertRow(lvs);
	}
	
	m_pcItems->SetInvokeMsg(new Message(ID_EDIT_ITEM));

	pcRoot->AddChild(m_pcItems);

 // --- Buttons ------------------------------------------------------------------

	pcRoot->AddChild(new VLayoutSpacer("ButtonTopSpacer", 8, 8));
	LayoutNode *pcButtons = new HLayoutNode("pcButtons", 0.0f);
	pcButtons->AddChild(new HLayoutSpacer("ButtonLeftSpacer", 8, 8));
 	pcButtons->AddChild(new Button(Rect(), "pcAdd", ID_BUTTON_PLUS, new Message(ID_EDIT_ADD)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonIntermediateSpacer", 8, 8));
 	pcButtons->AddChild(new Button(Rect(), "pcRem", ID_BUTTON_MINUS, new Message(ID_EDIT_DELETE)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonIntermediateSpacer", 0, COORD_MAX));
 	pcButtons->AddChild(new Button(Rect(), "pcSave", ID_BUTTON_SAVE, new Message(ID_EDIT_SAVE)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonIntermediateSpacer", 8, 8));
 	pcButtons->AddChild(new Button(Rect(), "pcCancel", ID_BUTTON_CLOSE, new Message(ID_EDIT_CANCEL)));
	pcButtons->AddChild(new HLayoutSpacer("ButtonRightSpacer", 8, 8));
 	pcButtons->SameWidth("pcSave", "pcCancel", NULL);
 	pcButtons->SameWidth("pcRem", "pcAdd", NULL);
 	pcRoot->AddChild(pcButtons);
	pcRoot->AddChild(new VLayoutSpacer("ButtonBottomSpacer", 8, 8));
	
	AddChild(new LayoutView(bounds, "pcLayout", pcRoot));
}

EditWin::~EditWin()
{
	delete m_pcMessenger;
}

bool EditWin::OkToQuit(void)
{
	Message *m = new Message( ID_UNLOCK );
	m->AddString( "id", m_cID );
	m_pcMessenger->SendMessage( m );
	return true;
}

