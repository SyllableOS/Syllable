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

#ifndef _ADDLIBRARY_H_
#define _ADDLIBRARY_H_

class AddLibraryWindow : public os::Window
{
	public:
		AddLibraryWindow( const os::Rect& cFrame, project* pcProject, os::Window* pcProjectWindow );
		~AddLibraryWindow();
		virtual bool OkToQuit( void );
		void RefreshList();
		virtual void HandleMessage( os::Message* pcMessage );
	private:
		os::LayoutView*		m_pcView;
		os::Window*			m_pcProjectWindow;
		project*		m_pcProject;
		#include "AddLibraryLayout.h"
};


#endif








