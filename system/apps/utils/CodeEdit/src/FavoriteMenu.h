#ifndef __F_GUI_RECENT_MENU_H
#define __F_GUI_RECENT_MENU_H
#include <gui/menu.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <util/message.h>
#include <gui/view.h>


enum fm_layout{
    ID_CLEAR_HISTORY = 830,
    ID_RECENT_ITEM = 831,
    ID_ADD_TO_FAVORITES = 832,
    ID_UPDATE_FAVORITES = 833
};

using namespace os;
//using namespace std;

class FavoriteMenu : public Menu
{
public:
    FavoriteMenu(const char* pzName,Message* pcMessage, int nRecent, bool bShowAddFavorites=false);
    ~FavoriteMenu();
    const char* GetMenuItemName();
    void RefreshList();

    void SetMenuLimit();
    int GetMenuLimit();
    const char* GetClicked(Message* pcMessage);
    void ClearHistory();
    const char* GetFileName();
    void AddItems();
    void AddNewItem(Message* pcMsge);
    void SetTarget(Handler* pcHandler);

private:
    int nRecentLimit;
    const char* pzFileName;
    const char* m_pzFileName;
    void* vMenuItem;
    Message* pcMsg;
    MenuItem* m_pcRecent;
    std::string pzArray[1024];
    bool bHistory;
    bool bAddFavorites;
};

#endif

































