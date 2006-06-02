#ifndef EDITLIST_H
#define EDITLIST_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <util/message.h>

namespace os
{

class EditListWin: public Window {
	public:
	EditListWin(const Rect & r, const String& name, std::vector<os::String> acData);
	~EditListWin();
	bool OkToQuit();
	void HandleMessage(Message *);
	void SetTarget( os::Handler* pcTarget );
	std::vector<os::String> GetData();
	private:
	TextView*	m_pcText;
	ListView*	m_pcList;
	String		m_cName;
	Handler*		m_pcParent;
	Messenger*	m_pcMessenger;
};

};

#endif








