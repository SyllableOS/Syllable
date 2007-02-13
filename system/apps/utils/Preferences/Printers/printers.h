/*
 *  Syllable Printer configuration preferences
 *
 *  (C)opyright 2006 - Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PRINTERS_H
#define PRINTERS_H

#include <gui/rect.h>
#include <gui/window.h>
#include <gui/treeview.h>
#include <gui/frameview.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/splitter.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/toolbar.h>
#include <gui/exceptions.h>

#include <config.h>
#include <print_view.h>

enum messages
{
	M_APP_ABOUT,
	M_ALERT_DONE,
	M_WINDOW_SAVE,
	M_WINDOW_CANCEL,
	M_ADD_PRINTER,
	M_DEFAULT_PRINTER,
	M_REMOVE_PRINTER,
	M_REMOVE_PRINTER_DIALOG,
	M_SELECT_PRINTER
};

class PrintersWindow : public os::Window
{
	public:
		PrintersWindow( const os::Rect &cFrame );
		~PrintersWindow();
		void HandleMessage( os::Message *pcMessage );
		bool OkToQuit( void );

	private:
		void LoadPrinters( void );
		void AppendPrinter( CUPS_Printer *pcPrinter );
		void RemovePrinter( uint nIndex );
		void ReloadPrinters( void );

		status_t ReadConfig( void );
		status_t PushKeyPair( os::String cKey, os::String cValue, CUPS_Printer *pcPrinter );
		status_t WriteConfig( void );
		status_t InstallPPD( os::String cPPD, os::String cName );
		status_t GetPPD( os::String cPPD );
		status_t RenamePPD( os::String cOriginal, os::String cNew );
		status_t CopyPPD( os::String cFrom, os::String cTo );

		os::Menu *m_pcMenuBar;
		float m_vMenuHeight;

		os::ToolBar *m_pcToolBar;

		os::TreeView *m_pcTreeView;

		os::View *m_pcView;
		os::LayoutView *m_pcLayoutView;
		os::Splitter *m_pcSplitter;

		std::vector< CUPS_Printer *> m_vpcPrinters;
		std::vector< PrintView *> m_vpcPrintViews;
		PrintView *m_pcCurrentPrintView;
};

#endif	/* PRINTERS_H */

