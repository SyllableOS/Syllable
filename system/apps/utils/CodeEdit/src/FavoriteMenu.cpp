#include "FavoriteMenu.h"

/**
 * Default constructor
 * \author Rick Caudill (cau0730@cup.edu)
 * \param pzName  - The name of the menu. 
 * \param pzFile  - The file the Recentmenu will open(if the file does not exsis, it will create one).
 * \param nRecent - The number of menuitems you want. 
 *****************************************************************************/
FavoriteMenu::FavoriteMenu(const char* pzName,Message* pcMessage, int nRecent, bool bShowAddFavorites) : Menu(Rect(0,0,0,0),pzName,ITEMS_IN_COLUMN)
{
    pcMsg = pcMessage;
    nRecentLimit = nRecent;
    bAddFavorites = bShowAddFavorites;

    if (bAddFavorites == true)
    {
        AddItem(new MenuItem("Delete all...", new Message(ID_ADD_TO_FAVORITES)));
        AddItem(new MenuSeparator());
    }


    AddItems();
}

FavoriteMenu::~FavoriteMenu()
{}

/** \internal */
void FavoriteMenu::AddItems()
{
    int nIncrement;
    std::string pzArray;
    int nDec = pcMsg->CountNames() -1;
    for (nIncrement=0; nIncrement <pcMsg->CountNames(); nIncrement++)
    {
        if (GetItemCount() < GetMenuLimit()+2)
        {
            pcMsg->FindString(pcMsg->GetName(nIncrement).c_str(),&pzArray);
            AddItem(new MenuItem(pzArray.c_str(), new Message(ID_RECENT_ITEM)));
        }
    }
}
/** Get MenuItem Limit
 * \par		Description:
 *		Returns the number of menuitems that the favoritemenu will see.
 * \author Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
int FavoriteMenu::GetMenuLimit()
{
    return (nRecentLimit);
}



const char* FavoriteMenu::GetClicked(Message* pcMessage)
{

    pcMessage->FindPointer("source",&vMenuItem);
    m_pcRecent = static_cast <MenuItem*> (vMenuItem);
    pzFileName = m_pcRecent->GetLabel().c_str();

    return (pzFileName);
}


void FavoriteMenu::ClearHistory()
{
    int nIncrement=0;
    if (bAddFavorites == false)
    {
        for (nIncrement=0; nIncrement< pcMsg->CountNames(); nIncrement++)
        {
            pcMsg->RemoveName(pcMsg->GetName(nIncrement).c_str());
            RemoveItem(nIncrement);
        }
    }

    else
    {
        for (nIncrement=2; nIncrement < GetItemCount(); nIncrement++)
        {
            while (GetItemCount() > 2)
                RemoveItem(nIncrement);

        }
    }

}


void FavoriteMenu::AddNewItem(Message* pcMsge)
{
    ClearHistory();
    pcMsg = pcMsge;
    AddItems();
}

void FavoriteMenu::SetTarget(Handler* pcHandler)
{
    SetTargetForItems(pcHandler);
}









