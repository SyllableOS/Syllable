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
#ifndef __GOTO_DIALOG_H__
#define __GOTO_DIALOG_H__

#include "appwindow.h"
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/textview.h>

using namespace os;

class AEditWindow;

class GotoDialog : public Window
{
	public:
		GotoDialog(const Rect& cFrame, AEditWindow* pcParent);
		void HandleMessage(Message* pcMessage);
		bool OkToQuit(void);
		void SetEnable(bool bEnable);
		void MakeFocus();
		void Raise();

	private:
		AEditWindow* pcTarget;

		LayoutView* pcMainLayoutView;
		HLayoutNode* pcHLayoutNode;
		VLayoutNode* pcInputNode;
		VLayoutNode* pcButtonNode;

		Button* pcGotoButton;
		Button* pcCloseButton;

		TextView* pcLineNoTextView;
};

#endif
