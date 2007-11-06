#ifndef STRREQ_H
#define STRREQ_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <util/message.h>

using namespace os;

class StringRequester: public Window {
	public:
	StringRequester( const std::string& cTitle, const std::string& cText );
	~StringRequester();

	virtual void	HandleMessage( Message* pcMessage );
  
	int  Go();
	void Go( Invoker* pcInvoker );
	
	std::string GetString();
	void SetString( std::string& cText);
	
	protected:

	private:
	TextView*	m_pcTextView;
	Invoker*	m_pcInvoker;
};

#endif /* STRREQ */

