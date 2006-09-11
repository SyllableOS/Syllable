//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
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

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/checkbox.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/button.h>

#ifndef _FINDDIALOG_H_
#define _FINDDIALOG_H_

namespace os
{

class FindDialog : public Window
{
	public:
		FindDialog(const Rect& cFrame, Window* pcParent);
		os::String GetFindText();
		void ShowDialog();
		bool OkToQuit();
	private:
		bool m_bShown;
		Window* pcParentWindow;

		LayoutView* pcMainLayoutView;
		HLayoutNode* pcHLayoutNode;
		VLayoutNode* pcButtonNode;
		VLayoutNode* pcInputNode;

		Button* pcFindButton;
		TextView* pcFindTextView;

		StringView* pcFindLabel;
};

};

#endif
