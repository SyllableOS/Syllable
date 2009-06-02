
#ifndef _LOGIN_VIEW_H
#define _LOGIN_VIEW_H

#include <gui/frameview.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/iconview.h>
#include <gui/window.h>
#include <util/datetime.h>
#include <gui/textview.h>
#include <util/message.h>
#include <util/settings.h>

#include "keymap.h"
#include "appsettings.h"

using namespace os;

class LoginView : public View
{
public:	
	LoginView(Rect cRect, Window*, AppSettings* pcAppSettings );
	
	void UpdateTime();
	void PopulateIcons();
	void AllAttached();
	bool GetUserNameAndPass(String*,String*);
	void ClearPassword();
	void FindUser(const String&);
	void FocusPassword();
	void SetKeymapForUser( const String& );
	void Reload();
	
	os::String GetKeymap()
	{
		return pcKeymapSelector->GetKeymap();
	}
	
	void HandleMessage(os::Message* pcMessage);	
private:
	void Layout();
	
	LayoutView* pcLayoutView;
	
	StringView* pcDateString;
	
	StringView* pcPassString;
	TextView* pcPassText;
	
	KeymapSelector* pcKeymapSelector;
	
	Button* pcLoginButton;
	Button* pcShutdownButton;
	
	IconView* pcUserIconView;

	Window* pcParentWindow;
	
	AppSettings* pcSettings;
};	

#endif










