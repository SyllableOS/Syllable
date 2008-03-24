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

#ifndef _PROJECTPREFS_H_
#define _PROJECTPREFS_H_

#include <gui/stringview.h>
#include <gui/layoutview.h>

namespace os
{
	class CheckBox;
}

class ProjectPrefs : public os::Window
{
	public:
		ProjectPrefs( const os::Rect& cFrame, project* pcProject, os::Window* pcProjectWindow );
		~ProjectPrefs();
		virtual bool OkToQuit( void );
		virtual void HandleMessage( os::Message* pcMessage );
	private:
		os::LayoutView*		m_pcView;
		os::StringView*		m_pcTargetLabel;
		os::TextView*		m_pcTarget;
		os::StringView*		m_pcCategoryLabel;
		os::TextView*		m_pcCategory;
		os::StringView*		m_pcInstallPathLabel;
		os::TextView*		m_pcInstallPath;
		os::StringView*		m_pcCFlagsLabel;
		os::TextView*		m_pcCFlags;
		os::StringView*		m_pcLFlagsLabel;
		os::TextView*		m_pcLFlags;
		os::CheckBox*		m_pcTerminalFlag;
		
		os::Button*			m_pcOk;
		os::Button*			m_pcCancel;
		
		os::Window*			m_pcProjectWindow;
		project*			m_pcProject;
		
};


#endif





