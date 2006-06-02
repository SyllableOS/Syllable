// sIDE		(C) 2001-2005 Arno Klenke
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

#ifndef _GROUPWINDOW_H_
#define _GROUPWINDOW_H_

#include <gui/textview.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/separator.h>

class GroupWindow : public os::Window
{
	public:
		GroupWindow( const os::Rect& cFrame, project* pcProject, os::Window* pcProjectWindow );
		~GroupWindow();
		virtual bool OkToQuit( void );
		virtual void HandleMessage( os::Message* pcMessage );
		void RefreshList();
	private:
		#include "GroupWindowLayout.h"
		os::LayoutView*		m_pcView;
		os::Window*			m_pcProjectWindow;
		project*			m_pcProject;
		
};


#endif







