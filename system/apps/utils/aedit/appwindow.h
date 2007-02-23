// AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//		      (C)opyright 2004-2006 Jonas Jarvoll
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


#include "dialog.h"
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
#include <gui/checkmenu.h>
#include <gui/filerequester.h>
#include <gui/requesters.h>
#include <gui/toolbar.h>
#include <gui/statusbar.h>
#include <gui/fontrequester.h>

class AboutDialog;
class FindDialog;
class ReplaceDialog;
class GotoDialog;
class MyTabView;
class Buffer;

class AEditWindow : public os::Window
{
	friend class Buffer;

	public:
		AEditWindow( const os::Rect& cFrame );
		~AEditWindow();
		virtual void HandleMessage( os::Message* pcMessage );
		virtual bool OkToQuit(void);
		virtual void FrameSized( const os::Point& cDelta );

		void LoadOnStart( char* pzFilename );
		void SetStatus( os::String zTitle, ... );
		void UpdateMenuItemStatus(void);
		void Load( char* pzFileName );
		void CreateNewBuffer(void);
		void DragAndDrop( os::Message* pcData);

		void SetDialog( Dialog* dialog );

		// Pointer to current viewed buffer. NULL if no buffer has been opened
		Buffer* pcCurrentBuffer;

	private:
		void _SetupMenus();
		void _Layout();

		os::LayoutView* pcAppWindowLayout;
		os::VLayoutNode* pcVLayoutRoot;

		os::ToolBar* pcToolBar;
		MyTabView* pcMyTabView;
		os::StatusBar *pcStatusBar;

		// Button bar buttons
		os::ImageButton* pcImageFileNew;
		os::ImageButton* pcImageFileOpen;
		os::ImageButton* pcImageFileSave;
		os::ImageButton* pcImageEditCut;
		os::ImageButton* pcImageEditCopy;
		os::ImageButton* pcImageEditPaste;
		os::ImageButton* pcImageFindFind;
		os::ImageButton* pcImageEditUndo;
		os::ImageButton* pcImageEditRedo;

		// Menu stuff
		os::Menu* pcMenuBar;
		os::Menu* pcAppMenu;

		// The file menu and its menuitems
		os::Menu* pcFileMenu;
		os::MenuItem *pcFileNew;
		os::MenuItem *pcFileOpen;
		os::MenuItem *pcFileClose;
		os::MenuItem *pcFileSave;
		os::MenuItem *pcFileSaveAs;
		os::MenuItem *pcFileSaveAll;
		os::MenuItem *pcFileNextTab;
		os::MenuItem *pcFilePrevTab;

		// The edit menu and its menuitem
		os::Menu* pcEditMenu;
		os::MenuItem* pcEditUndo;
		os::MenuItem* pcEditRedo;
		os::MenuItem* pcEditCut;
		os::MenuItem* pcEditCopy;
		os::MenuItem* pcEditPaste;
		os::MenuItem* pcEditSelectAll;

		// The view menu and its menuitem
		os::Menu* pcViewMenu;
		os::CheckMenu* pcViewToolbar;
		os::CheckMenu* pcViewStatusbar;
		os::MenuItem* pcViewFont;
		os::Menu* pcTabsMenu;
		os::CheckMenu* pcViewTabAutomatic;
		os::CheckMenu* pcViewTabShow;
		os::CheckMenu* pcViewTabHide;

		// The find menu and its menuitem
		os::Menu* pcFindMenu;
		os::MenuItem* pcFindFind;
		os::MenuItem* pcFindReplace;
		os::MenuItem* pcFindGoto;

		// The setting menu
		os::Menu* pcSettingsMenu;
		os::MenuItem* pcSettingsSaveOnClose;

		os::FileRequester* pcLoadRequester;
		os::FileRequester* pcSaveRequester;
		os::FileRequester* pcSaveAsRequester;
		os::FontRequester* pcFontRequester;

		AboutDialog* pcAboutDialog;			// About dialog
		GotoDialog* pcGotoDialog;			// "Goto Line" dialog
		FindDialog* pcFindDialog;			// Find dialog
		ReplaceDialog* pcReplaceDialog;		// Replace dialog
		Dialog* pcCurrentDialog;			// Pointer to current dialog

		os::Alert* pcCloseAlert;
		os::Alert* pcSaveErrorAlert;
		os::Alert* pcQuitAlert;

		os::String LatestOpenPath;
		os::String LatestSavePath;

		os::Settings cSettings;
};

#endif /* __APPWINDOW_H_ */


