#ifndef MAINWIN_H
#define MAINWIN_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/treeview.h>
#include <util/message.h>
#include <util/string.h>
#include <gui/requesters.h>

#include "resources/SettingsEditor.h"

using namespace os;

class MainWin: public Window {
	public:
	MainWin( const Rect & r, Message* pcMsg, Looper *pcParent, const std::string& cID );
	~MainWin();

	bool OkToQuit();

	void HandleMessage(Message *);
	
	protected:

	private:
	void			_BuildList();
	void			_AddVariable( const String &name, int pos = -1, bool bUpdate = false );
	void			_AddToMessage( const char *pzName, int32 nType );
	Message*		_AddMsg( int nType );
	String			_GetUniqueID();
	TreeViewStringNode*	_GetRowByID( String& cID, int *nIndex = 0 );

	Settings*	m_Message;

	TreeView*	m_pcContents;
	Menu*		m_pcMenuBar;
	bool		m_bEmbedded;
	Messenger*	m_pcMessenger;
	std::string	m_cID;
};

#endif





