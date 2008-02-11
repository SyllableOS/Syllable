#ifndef _EVENT_VIEW_H
#define _EVENT_VIEW_H

#include <gui/treeview.h>
#include <gui/keyevent.h>
#include <util/application.h>

#include <vector>

using namespace os;
using namespace std;

class EventView : public os::TreeView
{
public:
	EventView();
	
	void Refresh();
};

#endif
