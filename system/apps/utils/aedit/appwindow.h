// AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
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

#ifndef __APPWINDOW_H_
#define __APPWINDOW_H_

#include "editview.h"
#include "button_bar.h"
#include "gotodialog.h"
#include "finddialog.h"
#include "replacedialog.h"
#include "settings.h"

#include <util/message.h>

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/menu.h>
#include <gui/filerequester.h>

#include <codeview/Format_C.h>
#include <codeview/Format_java.h>

using namespace os;

class AEditWindow : public Window
{
	public:
		AEditWindow(const Rect& cFrame);
		virtual void DispatchMessage(Message* pcMessage, Handler* pcHandler);
		virtual void HandleMessage(Message* pcMessage);
		virtual bool OkToQuit(void);
		void LoadOnStart(char* pzFilename);

	private:
		void Load(char* pzFileName);
		void Save(const char* pzFileName);
		bool MapKey(const char* pzString, int32 nQualifiers);

		LayoutView* pcAppWindowLayout;
		VLayoutNode* pcVLayoutRoot;

		ButtonBar* pcButtonBar;
		EditView* pcEditView;

		Menu* pcMenuBar;
		Menu* pcAppMenu;
		Menu* pcFileMenu;
		Menu* pcEditMenu;
		Menu* pcFindMenu;
		Menu* pcFormatMenu;
		Menu* pcSettingsMenu;
		Menu* pcHelpMenu;

		char nFormator;

		Format_C* pcFormatC;
		Format_java* pcFormatJava;

		FileRequester* pcLoadRequester;
		FileRequester* pcSaveRequester;

		std::string zWindowTitle;
		std::string zCurrentFile;

		bool bContentChanged;

		GotoDialog* pcGotoDialog;			// "Goto Line" dialog
		FindDialog* pcFindDialog;			// Find dialog
		ReplaceDialog* pcReplaceDialog;		// Replace dialog

		std::string zFindText;				// All used for find & replace
		std::string zReplaceText;
		uint32 nTextPosition,nCurrentLine, nTotalLines;
		bool bHasFind;
		bool bCaseSensitive;
		bool bSaveOnExit;
		
		AEditSettings* pcSettings;

};

#endif /* __APPWINDOW_H_ */

