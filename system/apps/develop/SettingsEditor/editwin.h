#ifndef EDITWIN_H
#define EDITWIN_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <util/message.h>

using namespace os;

class EditWin: public Window {
	public:
	EditWin( const Rect & r, const String& name, Message *pcMsg, Looper *pcParent, const String& id );
	~EditWin();

	bool OkToQuit();

	void HandleMessage(Message *);
	
	protected:
	void _RebuildArray();
	void _ReNumber();
	void _EditItem( int nIndex );

	private:
	ListView*	m_pcItems;
	Message*	m_pcMessage;
	String		m_cName;
	String		m_cID;
	Messenger*	m_pcMessenger;
	uint32		m_nIDCtr;
	int		m_nType;
};

#endif

