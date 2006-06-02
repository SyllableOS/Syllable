// Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
// Sun Jun  6 2004 13:11:24 
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _REPLACE_DIALOG_H
#define _REPLACE_DIALOG_H

#include <gui/window.h>
#include <gui/button.h>
#include <gui/radiobutton.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <util/string.h>
#include <gui/frameview.h>
#include <gui/layoutview.h>
#include <util/message.h>
#include "messages.h"

using namespace os;

class ReplaceDialog : public Window
{
public:
	ReplaceDialog( Window* pcParent, const String& = "", bool = true, bool = true, bool = false );

	virtual void HandleMessage( os::Message*);

private:
	void Init();
	void Layout();
	void SetTabs();

	os::Button* pcReplaceButton;
	os::Button* pcReplaceAllButton;
	os::Button* pcCloseButton;
	
	os::RadioButton* pcRadioUp; 
	os::RadioButton* pcRadioDown; 
	os::RadioButton* pcRadioCase; 
	os::RadioButton* pcRadioNoCase;
	os::RadioButton* pcRadioBasic; 
	os::RadioButton* pcRadioExtended;
	
	os::TextView* pcFindText;
	os::TextView* pcReplaceText;
	
	os::StringView* pcReplaceString;
	os::StringView* pcFindString;
	
	Window *pcParentWindow;
	bool bFirst;
};

#endif


