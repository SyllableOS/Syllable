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

#ifndef _ADDWINDOW_H_
#define _ADDWINDOW_H_

class AddWindow : public os::Window
{
	public:
		AddWindow( const os::Rect& cFrame, project* pcProject, std::vector<os::String> azPaths, os::Window* pcProjectWindow );
		~AddWindow();
		virtual bool OkToQuit( void );
		void RefreshList();
		virtual void HandleMessage( os::Message* pcMessage );
	private:
		os::LayoutView*		m_pcView;
		os::Button*			m_pcAdd;
		os::Button*			m_pcCancel;
		os::TextView*		m_pcInput;
		os::DropdownMenu*	m_pcGroup;
		os::DropdownMenu*	m_pcType;
		os::Window*			m_pcProjectWindow;
		project*		m_pcProject;
		std::vector<os::String> m_azPaths;
};


#endif







