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

#ifndef _PROJECTWINDOW_H_
#define _PROJECTWINDOW_H_

#include "resources/sIDE.h"

class ProjectTree : public os::TreeView
{
public:
	ProjectTree( const os::Rect& cFrame, const os::String& cTitle, uint32 nModeFlags = os::ListView::F_MULTI_SELECT | os::ListView::F_RENDER_BORDER,
                    uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
                    uint32 nFlags = os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE );
	void MouseUp( const os::Point & cPos, uint32 nButtons, os::Message * pcData );
};

class ProjectWindow : public os::Window
{
	public:
		ProjectWindow( const os::Rect& cFrame, const std::string &cTitle );
		~ProjectWindow();
		virtual void HandleMessage( os::Message* pcMessage );
		virtual bool OkToQuit( void );
		void RefreshList();
		void GetSelected( int16 *pnGroup, int16 *pnFile );
	private:
		friend class sIDEApp;
		os::View*		m_pcView;
		os::Menu*		m_pcMenuBar;
		os::Menu*		m_pcMenuApp;
		os::Menu*		m_pcMenuProject;
		os::Menu*		m_pcMenuBuild;
		os::TreeView*	m_pcList;
		project	m_cProject;
		GroupWindow* m_pcGrp;
		AddWindow*	m_pcAdd;
		AddLibraryWindow* m_pcAddLibrary;
		ProjectPrefs* m_pcPrefs;
		os::FileRequester* m_pcLoadRequester;
		os::FileRequester* m_pcSaveRequester;
		os::FileRequester* m_pcAddRequester;
}; 

#endif















