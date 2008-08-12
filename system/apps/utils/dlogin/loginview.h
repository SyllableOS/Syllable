
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
#include "keymap.h"

using namespace os;

class LoginView : public View
{
public:	
	LoginView(Rect cRect, Window*);
	
	void UpdateTime();
	void PopulateIcons();
	bool GetUserNameAndPass(String*,String*);
	void ClearPassword();
	void Focus();
	void AttachedToWindow();
	void FindUser(const String&);
	void Reload()
	{
		String cIcon;
		for (uint i=0; i<pcUserIconView->GetIconCount(); i++)
		{
			if (pcUserIconView->GetIconSelected(i))
				cIcon = pcUserIconView->GetIconString(i,0);
		}
		pcUserIconView->Clear();
		PopulateIcons();
		pcUserIconView->Flush();
		pcUserIconView->Sync();
		pcUserIconView->Paint(pcUserIconView->GetBounds());
		FindUser(cIcon);
	}
	
	os::String GetKeymap()
	{
		return selector->GetKeymap();
	}
	
	void HandleMessage(os::Message* pcMessage);	
private:
	void Layout();
	
	LayoutView* pcLayoutView;
	
	StringView* pcDateString;
	
	StringView* pcPassString;
	TextView* pcPassText;
	
	KeymapSelector* selector;
	
	Button* pcLoginButton;
	Button* pcShutdownButton;
	
	IconView* pcUserIconView;

	Window* pcParentWindow;
};	

#endif










