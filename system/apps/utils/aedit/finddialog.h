//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//             (C)opyright 2004-2006 Jonas Jarvoll
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

#ifndef __FIND_DIALOG_H__
#define __FIND_DIALOG_H__

#include "dialog.h"

#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/imagebutton.h>
#include <gui/checkbox.h>
#include <util/message.h>

class FindDialog : public Dialog
{
public:
	FindDialog();
	~FindDialog();

	void FrameSized( const os::Point& cDelta );
	void HandleMessage( os::Message* pcMessage );
	void AllAttached();
	os::Point GetPreferredSize( bool bSize ) const;

	void Init();

	void SetEnable( bool bEnable );

private:
	void _FindFirst();
	void _FindNext();
	void _Layout();

	// Layouts
	os::LayoutView* pcLayoutView;
	os::LayoutNode* pcRoot;
	os::LayoutNode* pcTopPanel;
	os::LayoutNode* pcBottomPanel;

	// Widgets
	os::Button* pcFirstButton;
	os::Button* pcNextButton;
	os::ImageButton* pcCloseButton;
	os::StringView* pcFindString;
	os::TextView* pcFindTextView;
	os::CheckBox* pcCaseCheckbox;
};

#endif
