//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//             (C)opyright 2004 Jonas Jarvoll
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

#ifndef __REPLACE_DIALOG_H__
#define __REPLACE_DIALOG_H__

#include "appwindow.h"
#include <gui/window.h>
#include <gui/textview.h>
#include <gui/layoutview.h>
#include <gui/checkbox.h>
#include <gui/stringview.h>
#include <gui/button.h>

using namespace os;

class AEditWindow;

class ReplaceDialog : public Window
{
	public:
		ReplaceDialog(const Rect& cFrame, AEditWindow* pcParent);
		void HandleMessage(Message* pcMessage);
		bool OkToQuit(void);
		void SetEnable(bool bEnable);
		void Raise();

	private:
		AEditWindow* pcTarget;

		LayoutView* pcMainLayoutView;

		HLayoutNode* pcHLayoutNode;
		VLayoutNode* pcButtonNode;
		VLayoutNode* pcInputNode;

		Button* pcFindButton;
		Button* pcNextButton;
		Button* pcReplaceButton;
		Button* pcCloseButton;

		TextView* pcFindTextView;
		TextView* pcReplaceTextView;

		CheckBox* pcCaseCheckBox;

		StringView* pcFindLabel;
		StringView* pcReplaceLabel;
};

#endif
