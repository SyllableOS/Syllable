#ifndef _FIND_DIALOG_H
#define _FIND_DIALOG_H

#include <gui/window.h>
#include <gui/button.h>
#include <gui/radiobutton.h>
#include <gui/textview.h>
#include <util/string.h>
#include <gui/frameview.h>
#include <gui/layoutview.h>
#include <util/message.h>
#include "messages.h"

using namespace os;

class FindDialog : public Window
{
public:
	FindDialog( Window* pcParent, const String& = "", bool = true, bool = true, bool = false );

	virtual void HandleMessage( os::Message * message );

private:
	void Init();
	void Layout();
	void SetTabs();

	os::Button * findButton, *closeButton;
	os::RadioButton * rUp, *rDown, *rCase, *rNoCase, *rBasic, *rExtended;
	os::TextView * text;
	Window *pcParentWindow;
};

#endif  //_FIND_DIALOG_H
