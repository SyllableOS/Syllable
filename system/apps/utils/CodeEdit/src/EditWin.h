/*	CodeEdit - A programmers editor for Atheos
	Copyright (c) 2002 Andreas Engh-Halstvedt
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/
#ifndef _EDITWIN_H_
#define _EDITWIN_H_

#include <gui/window.h>
#include "FavoriteMenu.h"
#include "ToolBar.h"
#include "Version.h"
#include <string>
#include <vector>
namespace os
{
class Menu;
class FileRequester;
class Alert;
class ImageButton;
class CheckMenu;
class TabView;
};
class App;

namespace cv
{
class CodeView;
};

class StatusBar;
class FindDialog;
class GotoLineDialog;
class ReplaceDialog;

using namespace std;

typedef std::vector <std::string> t_List;

class EditWin : public os::Window
{
public:
    EditWin(App*, const char* =NULL);
    ~EditWin();

    void UpdateFavorites();
    void setFavorites(const char*);
    t_List getFavorites();

    //overridden methods
    virtual bool OkToQuit();
    void DispatchMessage(os::Message*, os::Handler*);
    void HandleMessage(os::Message*);
    void fontChanged(os::Font*);
    void openTab(const char* path=NULL);
    void switchTab();
    enum {
        ID_NOTHING = 100,
        ID_ABOUT = 101,
        ID_QUIT = 102,
        ID_OPTIONS = 103,
        ID_FILE_NEW = 104,
        ID_FILE_OPEN = 105,
        ID_FILE_SAVE = 106,
        ID_FILE_SAVE_AS = 107,
        ID_FILE_CLOSE = 108,
        ID_DO_FILE_SAVE_AS = 109,
        ID_DO_FILE_SAVE = 110,
        ID_EDIT_UNDO = 111,
        ID_EDIT_REDO = 112,
        ID_EDIT_CUT = 113,
        ID_EDIT_COPY = 114,
        ID_EDIT_PASTE = 115,
        ID_EDIT_DELETE = 116,
        ID_TEXT_INVOKED = 117,
        ID_FIND = 118,
        ID_FIND_NEXT = 119,
        ID_FIND_PREVIOUS = 120 ,
        ID_REPLACE = 121,
        ID_REPLACE_NEXT = 122,
        ID_REPLACE_ALL = 123,
        ID_GOTO_LINE = 124,
        ID_DO_FIND = 125,
        ID_DO_REPLACE = 126,
        ID_DO_GOTO_LINE = 127,
        ID_FORMAT_CHANGED = 128,
        ID_CHECK_MAIN_TOOL = 129,
        ID_FILE_OPEN_WIN = 131,
        ID_FILE_NEW_WIN = 132,
        ID_TAB_CHANGED = 133,
        M_BUT_NEW = 134,
        M_BUT_OPEN = 135,
        M_BUT_SAVE = 136,
        M_BUT_SAVE_AS = 137,
        M_BUT_COPY = 138,
        M_BUT_CUT = 139,
        M_BUT_PASTE = 140,
        M_BUT_FIND = 141,
        M_BUT_REPLACE = 142,
        M_BUT_SET = 143,
        M_BUT_REDO = 144,
        M_BUT_UNDO = 145

    };

private:
    static uint unnamedCounter;
    bool saveRequesterVisible;
    std::vector <bool>changed;
    std::vector <std::string> title;
    std::vector <bool> named;
    bool searchDown;
    bool searchCase;
    bool searchExtended;



    std::string zReplaceText;
    std::string searchText;

    FindDialog* findDialog;
    ReplaceDialog* replaceDialog;
    os::FileRequester *saveRequester;
    GotoLineDialog* gotoLineDialog;
    ToolBar* pcToolBar;
    FavoriteMenu* pcFavoritesMenu;
    App* app;
    os::Menu* menu;
    StatusBar* statusbar;
    os::Message* pcFavoriteMessage;
    os::Menu* mTool;
    os::Menu *m;
    os::Menu* mFind;
    os::Menu* mReplace;
    os::ImageButton* pcNew;
    os::ImageButton* pcOpen;
    os::ImageButton* pcSave;
    os::ImageButton* pcSaveAs;
    os::ImageButton* pcSearch;
    os::ImageButton* pcReplace;
    os::ImageButton* pcPrint;
    os::ImageButton* pcSetBut;
    os::ImageButton* pcBreaker;
    os::ImageButton* pcUndo;
    os::ImageButton* pcRedo;
    os::CheckMenu* pcToolCheck;
    os::TabView* pcTabView;

    void appAbout();
    void appQuit();
    void appOptions();
    void AddFavorites();
    void setupToolbar();
    void setupMenus();
    void fileNew();
    void fileNewWin();
    void deleteTab();
    void fileOpen(bool bFlag=false);
    bool fileSave(const string &path="");
    void fileSaveAs(os::Message *parent=NULL);
    void fileClose();

    void editUndo();
    void editRedo();
    void editCopy();
    void editCut();
    void editPaste();
    void editDelete();

    void formatChanged();

    void find(const string&, bool, bool, bool);

    void updateTitle();
    void updateStatus(const string &s="", bigtime_t =5000000);
    bool load(const char* path=NULL,cv::CodeView* tEdit=NULL);
};

#endif















