// AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//		      (C)opyright 2004 Jonas Jarvoll
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

#include "button_bar.h"
#include "status_bar.h"
#include "aboutdialog.h"
#include "gotodialog.h"
#include "finddialog.h"
#include "replacedialog.h"
#include "mytabview.h"

#include <util/message.h>
#include <util/string.h>
#include <util/resources.h>
#include <util/settings.h>

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/image.h>
#include <gui/menu.h>
#include <gui/filerequester.h>
#include <gui/requesters.h>

using namespace os;

class AboutDialog;
class FindDialog;
class ReplaceDialog;
class GotoDialog;
class MyTabView;

class AEditWindow : public Window
{
	friend class Buffer;

	public:
		AEditWindow(const Rect& cFrame);
		virtual void HandleMessage(Message* pcMessage);
		virtual bool OkToQuit(void);
		void LoadOnStart(char* pzFilename);
		void SetTitle(std::string zTtitle);
		void SetStatus(std::string zTitle);
		void UpdateMenuItemStatus(void);
		void Load(char* pzFileName);		
		void CreateNewBuffer(void);
		void DragAndDrop(Message* pcData);

		// Pointer to current viewed buffer. NULL if no buffer has been opened
		Buffer* pcCurrentBuffer;

	private:
		void SetupMenus();	

		LayoutView* pcAppWindowLayout;
		VLayoutNode* pcVLayoutRoot;

		ButtonBar* pcButtonBar;
		MyTabView* pcMyTabView;
		StatusBar *pcStatusBar;

		// Button bar buttons
		ImageButton* pcImageFileNew;
		ImageButton* pcImageFileOpen;
		ImageButton* pcImageFileSave;
		ImageButton* pcImageEditCut;
		ImageButton* pcImageEditCopy;
		ImageButton* pcImageEditPaste;
		ImageButton* pcImageFindFind;
		ImageButton* pcImageEditUndo;
		ImageButton* pcImageEditRedo;

		// Menu stuff
		Menu* pcMenuBar;
		Menu* pcAppMenu;

		// The file menu and its menuitems
		Menu* pcFileMenu;
		MenuItem *pcFileNew;
		MenuItem *pcFileOpen;
		MenuItem *pcFileClose;
		MenuItem *pcFileSave;
		MenuItem *pcFileSaveAs;
		MenuItem *pcFileSaveAll;
		MenuItem *pcFileNextTab;
		MenuItem *pcFilePrevTab;

		// The edit menu and its menuitem
		Menu* pcEditMenu;
		MenuItem* pcEditUndo;
		MenuItem* pcEditRedo;
		MenuItem* pcEditCut;
		MenuItem* pcEditCopy;
		MenuItem* pcEditPaste;
		MenuItem* pcEditSelectAll;

		// The find menu and its menuitem
		Menu* pcFindMenu;
		MenuItem* pcFindFind;
		MenuItem* pcFindReplace;
		MenuItem* pcFindGoto;
	
		// The font menu
		Menu* pcFontMenu;
		
		// The setting menu
		Menu* pcSettingsMenu;
		MenuItem* pcSettingsSaveOnClose;

		FileRequester* pcLoadRequester;
		FileRequester* pcSaveRequester;
		FileRequester* pcSaveAsRequester;

		AboutDialog* pcAboutDialog;			// About dialog
		GotoDialog* pcGotoDialog;			// "Goto Line" dialog
		FindDialog* pcFindDialog;			// Find dialog
		ReplaceDialog* pcReplaceDialog;		// Replace dialog
		Alert* pcCloseAlert;
		Alert* pcSaveErrorAlert;
		Alert* pcQuitAlert;

		String LatestOpenPath;
		String LatestSavePath;

		Settings cSettings;
};

#endif /* __APPWINDOW_H_ */


