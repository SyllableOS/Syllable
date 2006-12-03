//  AEdit -:-  (C)opyright 2005-2006 Jonas Jarvoll
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

#ifndef __ABOUT_DIALOG_H__
#define __ABOUT_DIALOG_H__

#include "appwindow.h"

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/imageview.h>
#include <gui/image.h>
#include <gui/stringview.h>

class AboutDialog : public os::Window
{
public:
	AboutDialog();
	~AboutDialog();

	void HandleMessage( os::Message* pcMessage );
	void FrameSized( const os::Point& cDelta );	
	bool OkToQuit( void );
	void Raise();

private:
	void _Close();

	// Layouts
	os::LayoutView* pcLayoutView;
	os::LayoutNode* pcRoot;

	// Widgets
	os::Image* pcAEditImage;
	os::ImageView* pcAEditImageView;

	os::StringView* pcVersionLabel;
	os::StringView* pcTextEditorLabel;

	os::Button* pcCloseButton;
};

#endif
