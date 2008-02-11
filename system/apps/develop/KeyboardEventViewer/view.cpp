#include "view.h"
#include "common.h"

EventView::EventView() : os::TreeView(os::Rect(0,0,1,1),"event_view")
{
	InsertColumn("Event",150);
	InsertColumn("Shortcut",100);
	InsertColumn("Application",150);
}

void EventView::Refresh()
{
	Clear();
	
	//get the events
	vector<os::KeyboardEvent> table;
	os::Application::GetInstance()->GetCurrentKeyShortcuts(&table);
	
	
	//add them
	for (int i=0; i<table.size(); i++)
	{
		os::TreeViewStringNode* node = new os::TreeViewStringNode();
		node->AppendString(table[i].GetEventName().c_str());
		node->AppendString( ShortcutKeyToString(table[i].GetShortcutKey()).c_str() );
		node->AppendString(table[i].GetApplicationName().c_str());
		InsertNode(node);
	}
}





