/*
    LBrowser - A simple file manager for AtheOS
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/directoryview.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/checkbox.h>
#include <util/message.h>
#include <util/application.h>
#include <util/clipboard.h>
#include <storage/fsnode.h>
#include <storage/file.h>
#include <unistd.h>
#include "../include/launcher_plugin.h"
#include "../include/launcher_mime.h"

enum {
	IE_DIR_CHANGED = 10001,
	IE_SELECTION_CHANGED,
	IE_ICON_CLICKED,
	IE_ICON_CHANGED,
	IE_BTN_CLEAR,
	IE_BTN_NEW,
	IE_BTN_RENAME,
	IE_BTN_DELETE,
	IE_BTN_CUT,
	IE_BTN_COPY,
	IE_BTN_PASTE,
	IE_BTN_CANCEL,
	IE_BTN_LOAD,
	IE_BTN_SAVE,
	IE_BTN_OK,
	IE_BTN_REFRESH,
	IE_BTN_PARENT,
	IE_BTN_HOME,
	IE_DOUBLE_CLICK,
	
	IE_MNU_NEW_WINDOW,
	IE_MNU_QUIT,
	IE_MNU_COPY,
	IE_MNU_PASTE,
	IE_MNU_SET_ICON,
	IE_MNU_DEL_ICON,
	IE_MNU_PARENT,
	IE_MNU_REFRESH,
	IE_MNU_NEWDIR,
	IE_MNU_RENAME,
	IE_MNU_DELETE,
	IE_MNU_ABOUT,
	IE_MNU_HOME,
	IE_MNU_OPEN_PREFS,
		
	IE_BTN_DIALOG_OK,
	IE_BTN_DIALOG_CANCEL,
	
	IE_DIALOG_MODE_RENAME,
	IE_DIALOG_MODE_NEWDIR,
	IE_DIALOG_MODE_DELETE,
	
	IE_SET_PREFS,
	IE_CANCEL_PREFS
};

#define CONFIG_FILE "~/config/LBrowse.cfg"

using namespace os;

class LBrowser;
class IconEditor;          // For setting icons.
class LBrowsePrefs;


class LBrowser : public Application
{
	public:
		LBrowser( int argc, char **argv );
		~LBrowser();
		bool OkToQuit( void );
		
	private:
		IconEditor *m_pcWindow;
};

class IconEditor : public Window
{
	
	public:
		IconEditor( string zDirectory );
		~IconEditor();
		void HandleMessage( Message *pcMessage );
		bool OkToQuit( void );
		
	private:
		string m_zDirectory;
		string m_zFile;
		TextView *m_pcTVDir;
		DirectoryView *m_pcDVDir;
		FileRequester *m_pcFRIcon;
		ImageButton *m_pcIcon;
		VLayoutNode *m_pcRoot;
		HLayoutNode *m_pcToolBar;
		LBrowsePrefs *m_pcPrefsWindow;
		TextView *m_pcTVDialog;
		StringView *m_pcSVDialog;
		StringView *m_pcStatusString;
		Window *m_pcDialog;
		uint m_nDialogMode;
		bool m_bIconLabels;
		
		void GetIcon( void );
		void SetIcon( string zIcon );
		void SizeWindow( void );
		void OpenRenameDialog( void );
		void OpenNewDirDialog( void );
		void OpenDeleteDialog( void );
		void CompleteDialog( Message *pcMessage );
		void ParentDir( void );
		void Launch( void );
		void OpenNewWindow( void );
		void CopyFiles( void );
		void PasteFiles( void );
		void OpenAbout( void );
		void OpenPrefs( void );
		void LoadPrefs( void );
		void SavePrefs( void );
		void SetStatus( void );
		
		ImageButton *m_pcNewBtn;
		ImageButton *m_pcRenameBtn;
		ImageButton *m_pcDeleteBtn;
		ImageButton *m_pcClearBtn;
		ImageButton *m_pcCopyBtn;
		ImageButton *m_pcPasteBtn;
		ImageButton *m_pcHomeBtn;
		ImageButton *m_pcRefreshBtn;
		ImageButton *m_pcParentBtn;
		
};

class LBrowsePrefs : public Window
{
	public:
		LBrowsePrefs( IconEditor *pcParent, Message *pcPrefs );
		~LBrowsePrefs( );
		bool OkToQuit( void );
		void HandleMessage( Message *pcMessage );
		
	private:
		void SetPrefs( void );
		IconEditor *m_pcParent;
		CheckBox *m_pcCBIconLabels;
};
