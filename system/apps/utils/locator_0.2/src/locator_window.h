/*
    Locator - A fast file finder for Syllable
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


#include <gui/window.h>
#include <gui/tabview.h>
#include "image_button.h"
#include <gui/desktop.h>
#include <gui/point.h>
#include <gui/textview.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/radiobutton.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/listview.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/checkbox.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include <storage/file.h>

#include <time.h>

#include "locator.h"

#define MAX_HISTORY 20


enum
{
  EV_BTN_SEARCH = 50,
  EV_BTN_STOP,
  EV_BTN_CLOSE,
  EV_BTN_PREF,
  EV_BTN_COPY,

  EV_SEARCH_STRING_CHANGED,
  EV_SEARCH_ITEM_CHANGED,
  EV_SEARCH_DIR_ITEM_CHANGED,

  EV_MENU_SELECT_ALL,
  EV_MENU_SELECT_NONE,
  EV_MENU_COPY,
  EV_MENU_MATCH_DIR,
  EV_MENU_FOLLOW_DIR,
  EV_MENU_REGEX,
  EV_MENU_SORT,
  EV_MENU_ABOUT,
  EV_MENU_OPEN_PREF,
  
  EV_PREF_WINDOW_CLOSED,
  EV_PREF_BTN_OK,
  EV_PREF_BTN_CANCEL

};

using namespace os;

class LocatorPrefWindow;
class LocatorWindow;

class LocatorWindow : public Window
{

  public:
    LocatorWindow( );
    ~LocatorWindow( );
    void Close( void );
    void HandleMessage( Message *pcMessage );

  private:
    bool bFollowSubDirs;
    bool bMatchDirNames;
    bool bUseRegex;
    bool bSortResults;
    int nItemsFound;
    ListView *pcFoundItems;
    StringView *pcStatusString;
    DropdownMenu *pcSearch;
    DropdownMenu *pcSearchDir;
    Message *pcPrevSearch;
    Message *pcPrevSearchDir;
    ImageButton *pcSearchButton;
    SearchThread *pcSearchThread;
    bool bSafeToSearch;
    Menu *pcMenu;
    Menu *pcOptMenu;
	LocatorPrefWindow *m_pcPrefWindow;

    bool OkToQuit( void );
    void StartSearch( Message *pcMessage );
    void AddFoundItem( Message *pcFoundMessage );
    std::string ReadInfo( void );
    void ReadHistory( void );
    void WriteHistory( void );
    void CopySelected( void );
    void UpdateOptionMenu( void );
    void DisplayAbout( void );
};


class LocatorPrefWindow : public Window
{
	public:
		LocatorPrefWindow(Window *pcParent, Message *pcPrefs);
		~LocatorPrefWindow();
		void HandleMessage( Message *pcMessage );
		
	private:
		CheckBox *m_pcCBFollowDirs;
		CheckBox *m_pcCBMatchDirs;
		CheckBox *m_pcCBUseRegex;
		CheckBox *m_pcCBSortResults;
		Window *m_pcParent;
};
