// Locale Preferences :: (C)opyright 2004 Henrik Isaksson
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

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/listview.h>
#include <gui/slider.h>
#include <gui/textview.h>

using namespace os;

class MainWindow : public Window
{
	public:
	MainWindow();
	~MainWindow();

	virtual void HandleMessage( Message * pcMessage );

	bool OkToQuit( void );

	private:
	void _ShowData();
	void _Apply();
	void _Undo();
	void _Default();
	void _AddLang();
	void _RemLang();
	ListViewRow* _RemoveRow( const String& cName );
	
	void _LoadLanguages();

	ListView*		m_pcAvailable;
	ListView*		m_pcPreferred;
	Button*			m_pcApply;
	Button*			m_pcRevert;
	Button*			m_pcDefault;
	Button*			m_pcAddLang;
	Button*			m_pcRemLang;
};

#endif /* __MAINWINDOW_H_ */

