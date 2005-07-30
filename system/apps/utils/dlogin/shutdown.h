#ifndef _SHUTDOWN_H
#define _SHUTDOWN_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/imageview.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <util/message.h>

using namespace os;

class ShutdownWindow : public Window
{
public:	
	ShutdownWindow(Window*);
	
	bool OkToQuit();
	void HandleMessage(Message*);
	void Layout();
	String GetTagLine();
	void AddDropItems();
	void Quit(int);
private:
	LayoutView* pcLayoutView;
	
	DropdownMenu* pcShutDrop;
	StringView* pcShutString;
	
	ImageView* pcImageView;
	StringView* pcTagString;
	
	Button* pcOkButton;
	Button* pcCancelButton;
	
	Window* pcParentWindow;
};

#endif	



