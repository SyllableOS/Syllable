#include "iconmenu.h"
#include "iconmenu_messages.h"
#include <util/message.h>

IconMenu::IconMenu() : Menu(Rect (0,0,0,0), "", ITEMS_IN_COLUMN)
{
	AddItem(new MenuItem("Open",new Message(ID_ICON_OPEN) ) );
	AddItem(new MenuSeparator());
	AddItem(new MenuItem("Cut",new Message(ID_ICON_CUT) ) );
	AddItem(new MenuItem("Copy",new Message(ID_ICON_COPY) ) );
	AddItem(new MenuSeparator());
	AddItem(new MenuItem("Rename",new Message(ID_ICON_RENAME) ));
	AddItem(new MenuSeparator());
	AddItem(new MenuItem("Properties", new Message( ID_ICON_PROPERTIES) ));
	
	GetPreferredSize(true);
}



