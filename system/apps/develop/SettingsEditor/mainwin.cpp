#include "mainwin.h"
#include "editwin.h"
#include "app.h"

#include <util/application.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/desktop.h>
#include <gui/dropdownmenu.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <storage/file.h>
#include <util/resources.h>
#include <util/settings.h>
#include <util/resources.h>
#include <gui/image.h>
#include <atheos/image.h>

#include <iostream>
#include <stdio.h>

#include "msgtotext.h"
#include "strreq.h"

struct Settings_FileHeader {
	uint32	nMagic;
	uint32  nHeaderSize;
	uint32	nVersion;
	uint32	nSize;
};


class VariableTreeNode: public TreeViewStringNode {
	public:
	VariableTreeNode() { }
	
	void RemoveFromMessage( Message* pcMessage ) {
		if( m_nIndex == -1 ) {
			pcMessage->RemoveName( m_cName.c_str() );
		} else {
			pcMessage->RemoveData( m_cName.c_str(), m_nIndex );
		}
	}
	
	String		m_cName;
	int			m_nIndex;
};

void MainWin::HandleMessage(Message *msg)
{
	switch(msg->GetCode()) {
		case ID_NEW:
			delete m_Message;
			m_Message = new Settings(0);
			_BuildList();
			break;

		case ID_OPEN:
			{
				String cSettingsDir = String( getenv("HOME") ) + String( "/Settings/" );
				FileRequester *fr = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this), cSettingsDir.c_str());
				fr->Show();
				fr->CenterInWindow(this);
				fr->MakeFocus();
			}
			break;
		case M_LOAD_REQUESTED:
			{
				String tmpstring;

				if(msg->FindString("file/path", &tmpstring) == 0) {
					File *f = new File( tmpstring, O_RDONLY );
					if( m_Message ) delete m_Message;
					m_Message = new Settings( f );
					((Settings *)m_Message)->Load();
					_BuildList();
				}				
			}
			break;

		case ID_SAVE:
			if( m_Message ) m_Message->Save();
			break;
			
		case ID_SAVE_AS:
			{
				FileRequester *fr = new FileRequester(FileRequester::SAVE_REQ, new Messenger(this), getenv("$HOME"));
				fr->Show();
				fr->CenterInWindow(this);
				fr->MakeFocus();
			}
			break;
		case M_SAVE_REQUESTED:
			if(m_Message) {
				String tmpstring;

				if(msg->FindString("file/path", &tmpstring) == 0) {
					File f( tmpstring, O_CREAT | O_TRUNC );

					Settings_FileHeader fh;
					fh.nMagic = SETTINGS_MAGIC;
					fh.nVersion = SETTINGS_VERSION;
					fh.nHeaderSize = sizeof( fh );
					fh.nSize = m_Message->GetFlattenedSize();
					f.Write( &fh, sizeof( fh ) );
					uint8* bfr = new uint8[fh.nSize];
					m_Message->Flatten( bfr, fh.nSize );
					f.Write( bfr, fh.nSize );
					delete bfr;

/*					Settings *pcSaveSettings = new Settings( &f );
					*pcSaveSettings = m_Message;
					pcSaveSettings->Save();
					delete pcSaveSettings;*/
				}				
			}
			break;

		case ID_EDIT_ITEM:
			{
				int sel = m_pcContents->GetFirstSelected();
				if(sel != -1) {
					TreeViewStringNode *lvr = (TreeViewStringNode *)m_pcContents->GetRow(sel);
					lvr->SetIsSelectable(false);

					Window *w = new EditWin(Rect(0,0,300, 200), lvr->GetString(0), m_Message, this, lvr->GetString(3));
					w->CenterInWindow(this);
					w->Show();
				}
			}
			break;
		case ID_REFRESH:
			{
				String s;
				if(msg->FindString("id", &s) == 0) {
					int nIndex;
					TreeViewStringNode *lvr;
					if( ( lvr = _GetRowByID( s, &nIndex ) ) ) {
						String cName = lvr->GetString(0);
						m_pcContents->RemoveNode( nIndex );
						_AddVariable( cName, -1, true );
					}
				}
			}			
			break;
		case ID_QUIT:
			if( m_bEmbedded && m_pcMessenger ) {
				Message *m = new Message( ID_UNLOCK );
				m->AddString( "id", m_cID );
				m_pcMessenger->SendMessage( m );
				Close();
			} else {
				Application::GetInstance()->PostMessage(M_QUIT);
			}
			break;

		case ID_UNLOCK:
			{
				String id;

				if(msg->FindString("id", &id) == 0) {
					TreeViewStringNode *lvr = _GetRowByID( id );
					if( lvr ) lvr->SetIsSelectable(true);
				}
			}
			break;

		case ID_DELETE_ITEM:
			{
				int sel = m_pcContents->GetFirstSelected();
				if( sel != -1 ) {
					VariableTreeNode *lvr = (VariableTreeNode *)m_pcContents->GetRow(sel);
					if( lvr->IsSelectable() ) {
						lvr->RemoveFromMessage( m_Message );
						do {
							m_pcContents->RemoveNode( sel, true );
						} while( lvr->m_nIndex == -1 && (((VariableTreeNode *)m_pcContents->GetRow(sel))->GetIndent() > 1 ));
						delete lvr;
					}
				}
			}
			break;

		case ID_ADD_VARIABLE:
			{
				int32 nType;
				msg->FindInt32("_Type", &nType);
				String cTypeName = DataTypeToText( nType );
				String cTitle = String().Format(ID_ADD_DIALOG_WINDOW_TITLE.c_str(),cTypeName.c_str());
				String cBody = String().Format(ID_ADD_DIALOG_SENTENCE.c_str(),cTypeName.c_str());
				StringRequester *sr = new StringRequester( cTitle, cBody );
				
				Message *invokemsg = new Message( ID_ADD_VARIABLE_NAME );
				invokemsg->AddInt32("_Type", nType);
				Invoker *inv = new Invoker(invokemsg);
				inv->SetTarget( this );
				sr->CenterInWindow(this);
				sr->Go( inv );
			}
			break;
		case ID_ADD_VARIABLE_NAME:
			if(m_Message) {
				const char *pzName = NULL;
				int32 nType;
				status_t nUnique;
				
				msg->FindString("_string", &pzName);
				msg->FindInt32("_Type", &nType);
				
				nUnique = m_Message->GetNameInfo( pzName );

				if( strcmp(pzName, "") && nUnique == -1 ) {
					_AddToMessage( pzName, nType );
					int sel = m_pcContents->GetFirstSelected();
					m_pcContents->ClearSelection();
					_AddVariable(pzName, sel, true);
				} else if( nUnique == 0 ) {
					Alert *a = new Alert( ID_ERROR_TITLE.c_str(),
						String().Format(ID_ERROR_UNIQUE.c_str(),pzName),
						Alert::ALERT_WARNING, ID_ERROR_BUTTON.c_str(), NULL );
					a->Go( new Invoker(NULL) );
				} else {
					Alert *a = new Alert( ID_ERROR_TITLE.c_str(),
						ID_ERROR_VARIABLE.c_str(),
						Alert::ALERT_WARNING, ID_ERROR_BUTTON.c_str(), NULL );
					a->Go( new Invoker(NULL) );
				}
			}
			break;

		default:
			Window::HandleMessage(msg);
	}
}

MainWin::MainWin( const Rect & r, Message* pcMsg, Looper *pcParent, const std::string& cID )
	:Window(r, "MainWin", ID_SETTINGS_EDITOR_WINDOW, 0, CURRENT_DESKTOP)
{
	Rect bounds = GetBounds();
	Rect menuframe;

	m_pcMessenger = NULL;
	m_cID = cID;

	if( pcMsg ) {
		m_Message = (Settings*)pcMsg;	/// GAAAH!
		m_bEmbedded = true;
		m_pcMessenger = new Messenger(pcParent, pcParent);
	} else {
		m_Message = new Settings(0);
		m_bEmbedded = false;
	}

 // --- Menu ---------------------------------------------------------------------

	m_pcMenuBar = new Menu(GetBounds(), "m_pcMenuBar", ITEMS_IN_ROW);

	Menu* pcFileMenu = new Menu(Rect(),ID_MENU_FILE, ITEMS_IN_COLUMN);
	if( m_bEmbedded ) {
		pcFileMenu->AddItem("Import...", new Message(ID_OPEN), "CTRL+I");
		pcFileMenu->AddItem("Export...", new Message(ID_SAVE_AS), "CTRL+E");
		pcFileMenu->AddItem(new MenuSeparator());
		pcFileMenu->AddItem("Close", new Message(ID_QUIT), "CTRL+Q");
	} else {
		pcFileMenu->AddItem(ID_MENU_FILE_NEW, new Message(ID_NEW));
		pcFileMenu->AddItem(ID_MENU_FILE_OPEN, new Message(ID_OPEN), "CTRL+O");
		pcFileMenu->AddItem(ID_MENU_FILE_SAVE, new Message(ID_SAVE), "CTRL+S");
		pcFileMenu->AddItem(ID_MENU_FILE_SAVE_AS, new Message(ID_SAVE_AS));
		pcFileMenu->AddItem(new MenuSeparator());
		pcFileMenu->AddItem(ID_MENU_FILE_EXIT, new Message(ID_QUIT), "CTRL+Q");
	}
	m_pcMenuBar->AddItem(pcFileMenu);

	Menu* pcNewMenu = new Menu(Rect(),ID_MENU_EDIT_ADD, ITEMS_IN_COLUMN);
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_STRING, _AddMsg( T_STRING ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_INT8, _AddMsg( T_INT8 ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_INT16, _AddMsg( T_INT16 ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_INT32, _AddMsg( T_INT32 ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_INT64, _AddMsg( T_INT64 ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_FLOAT, _AddMsg( T_FLOAT ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_DOUBLE, _AddMsg( T_DOUBLE ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_IRECT, _AddMsg( T_IRECT ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_IPOINT, _AddMsg( T_IPOINT ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_RECT, _AddMsg( T_RECT ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_POINT, _AddMsg( T_POINT ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_COLOR32, _AddMsg( T_COLOR32 ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_RAW, _AddMsg( T_RAW ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_VARIANT, _AddMsg( T_VARIANT ) );
	pcNewMenu->AddItem(ID_MENU_EDIT_ADD_MESSAGE, _AddMsg( T_MESSAGE ) );

	Menu* pcEditMenu = new Menu(Rect(),ID_MENU_EDIT, ITEMS_IN_COLUMN);
	pcEditMenu->AddItem(pcNewMenu);
	pcEditMenu->AddItem(ID_MENU_EDIT_DELETE, new Message(ID_DELETE_ITEM), "CTRL+K");
	m_pcMenuBar->AddItem(pcEditMenu);

	menuframe = bounds;
	menuframe.bottom = menuframe.top + m_pcMenuBar->GetPreferredSize(false).y;
	bounds.top = menuframe.bottom+1;
	m_pcMenuBar->SetFrame(menuframe);
	m_pcMenuBar->SetTargetForItems(this);

	AddChild(m_pcMenuBar);

 // --- Root layout node ---------------------------------------------------------

	LayoutNode* pcRoot = new VLayoutNode("pcRoot");

 // --- ListView -----------------------------------------------------------------

	m_pcContents = new TreeView(Rect(), "m_pcContents", 0);
	
	m_pcContents->InsertColumn(ID_LISTVIEW_NAME.c_str(), 150);
	m_pcContents->InsertColumn(ID_LISTVIEW_TYPE.c_str(), 70);
	m_pcContents->InsertColumn(ID_LISTVIEW_VALUE.c_str(), 250);
	m_pcContents->SetInvokeMsg(new Message(ID_EDIT_ITEM));

	pcRoot->AddChild(m_pcContents);
	
	if( m_bEmbedded ) _BuildList();
	
	AddChild(new LayoutView(bounds, "pcLayout", pcRoot));

		/* Set the application icon */
	Resources cRes( get_image_id() );
	BitmapImage	*pcAppIcon = new BitmapImage();
	pcAppIcon->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon( pcAppIcon->LockBitmap() );
	delete( pcAppIcon );
}

void MainWin::_AddVariable(const String &name, int pos, bool bUpdate )
{
	int nType;
	int nCount;
	int i;

	m_Message->GetNameInfo(name.c_str(), &nType, &nCount);
	VariableTreeNode *lvs = new VariableTreeNode;
	
	lvs->AppendString(name);

	String cTypeStr = MsgTypeToText(name, m_Message);

	lvs->AppendString(cTypeStr);

	if(nCount == 1) {
		lvs->m_cName = name;
		lvs->m_nIndex = 0;
		lvs->AppendString(MsgDataToText(name, 0, m_Message));
	} else {
		lvs->m_cName = name;
		lvs->m_nIndex = -1;
		lvs->AppendString("");
	}

	lvs->AppendString( _GetUniqueID() );

	lvs->SetIndent( 1 );
	m_pcContents->InsertNode(lvs, bUpdate);

	if(nCount > 1) {
		char bfr[16];
		for(i = 0; i < nCount; i++) {
			lvs = new VariableTreeNode;
			lvs->m_cName = name;
			lvs->m_nIndex = i;
			lvs->SetIndent( 2 );
			sprintf(bfr, "%s[%d]", name.c_str(), i);
			lvs->AppendString( name );
			lvs->AppendString(cTypeStr);
			lvs->AppendString(MsgDataToText(name, i, m_Message));
			lvs->AppendString( _GetUniqueID() );
			m_pcContents->InsertNode(lvs, bUpdate);
		}
	}


/*	int nType;
	int nCount;
	int i;

	m_Message->GetNameInfo(name.c_str(), &nType, &nCount);
	TreeViewStringNode *lvs = new TreeViewStringNode;
	
	lvs->AppendString(name);

	String cTypeStr = MsgTypeToText(name, m_Message);

	lvs->AppendString(cTypeStr);
	
	if(nCount > 1) {
		String s = "{ ";
		for(i = 0; i < nCount; i++) {
			s += MsgDataToText(name, i, m_Message);
			if(i < nCount-1) {
				s += ", ";
			}
		}
		s += "} ";
		lvs->AppendString(s);
	} else {
		lvs->AppendString(MsgDataToText(name, 0, m_Message));
	}

	lvs->AppendString( _GetUniqueID() );

	m_pcContents->InsertNode(lvs, bUpdate);
	*/
}

String MainWin::_GetUniqueID()
{
	static uint32 counter = 0;
	String s;
	
	s.Format("%d", ++counter);

	return s;	
}

void MainWin::_BuildList()
{
	int i = m_Message->CountNames();
	
	m_pcContents->Clear();

	while(i--) {
		_AddVariable(m_Message->GetName(i));
	}
	
	m_pcContents->Invalidate();
}

MainWin::~MainWin()
{
	delete m_Message;
	if( m_pcMessenger ) delete m_pcMessenger;
}

bool MainWin::OkToQuit(void)
{
	if( !m_bEmbedded ) {
		Application::GetInstance()->PostMessage(M_QUIT);
		return true;
	} else {
		if( m_pcMessenger ) {
			Message *m = new Message( ID_UNLOCK );
			m->AddString( "id", m_cID );
			m_pcMessenger->SendMessage( m );
			Close();
		}
		return false;
	}
}

TreeViewStringNode* MainWin::_GetRowByID( String& cID, int *nIndex )
{
	TreeViewStringNode *lvr;
					
	for(int i = 0; i < (int)m_pcContents->GetRowCount(); i++) {
		lvr = (TreeViewStringNode *)m_pcContents->GetRow(i);
		if(cID == lvr->GetString(3)) {
			if( nIndex ) *nIndex = i;
			return lvr;
		}
	}
	return NULL;
}

Message* MainWin::_AddMsg( int nType )
{
	Message *m = new Message( ID_ADD_VARIABLE );
	m->AddInt32( "_Type", nType );
	return m;
}

void MainWin::_AddToMessage( const char *pzName, int32 nType )
{
	switch( nType )
	{
		case T_STRING:
			m_Message->AddString(pzName, "");
			break;
		case T_INT8:
			m_Message->AddInt8(pzName, 0);
			break;
		case T_INT16:
			m_Message->AddInt16(pzName, 0);
			break;
		case T_INT32:
			m_Message->AddInt32(pzName, 0);
			break;
		case T_INT64:
			m_Message->AddInt64(pzName, 0);
			break;
		case T_BOOL:
			m_Message->AddBool(pzName, false);
			break;
		case T_POINT:
			m_Message->AddPoint(pzName, Point());
			break;
		case T_RECT:
			m_Message->AddRect(pzName, Rect());
			break;
		case T_IPOINT:
			m_Message->AddIPoint(pzName, IPoint());
			break;
		case T_IRECT:
			m_Message->AddIRect(pzName, IRect());
			break;
		case T_FLOAT:
			m_Message->AddFloat(pzName, 0);
			break;
		case T_DOUBLE:
			m_Message->AddDouble(pzName, 0);
			break;
		case T_RAW:
			//m_Message->AddRaw(pzName, 0);
			break;
		case T_COLOR32:
			m_Message->AddColor32(pzName, Color32_s());
			break;
		case T_VARIANT:
			m_Message->AddVariant(pzName, Variant(0));
			break;
		case T_MESSAGE:
			{
				Message msg(0);
				m_Message->AddMessage(pzName, &msg);
			}
			break;
	}
}

















