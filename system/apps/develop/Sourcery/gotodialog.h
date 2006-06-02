//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
//
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

#ifndef _GOTO_DIALOG_H
#define _GOTO_DIALOG_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/checkbox.h>
#include <util/message.h>

using namespace os;

class GoToDialog : public Window
{
public:
	GoToDialog(Window*, bool);
	~GoToDialog();
	
	void HandleMessage(Message*);
	bool OkToQuit();
private:
	void _Layout();
	void _SetTabs();
	LayoutView* pcMainLayout;
	HLayoutNode* pcHLMain;
	VLayoutNode* pcVLMain;
	VLayoutNode* pcVLButton;
	HLayoutNode* pcHLLine;
	HLayoutNode* pcHLCol;
	HLayoutNode* pcHLCheck;

	CheckBox* pcAdvancedCheckBox;
	TextView* pcLineTextView;
	TextView* pcColTextView;
	StringView* pcColStringView;
	StringView* pcLineStringView;

	Button* pcGoToButton;
	Button* pcCancelButton;
	Window* pcParentWindow;
	bool bAdvanced;
};

#endif
