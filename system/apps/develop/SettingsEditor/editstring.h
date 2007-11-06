#ifndef EDITSTRING_H
#define EDITSTRING_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <util/message.h>

using namespace os;

class EditStringWin: public Window {
	public:
	EditStringWin(const Rect & r, const String& name, const String& cID, const String& cData, Looper *pcParent);
	~EditStringWin();
	bool OkToQuit();
	void HandleMessage(Message *);
	private:
	TextView*	m_pcText;
	String		m_cData;
	String		m_cName;
	String		m_cID;
	Looper*		m_pcParent;
	Messenger*	m_pcMessenger;
};

#endif

