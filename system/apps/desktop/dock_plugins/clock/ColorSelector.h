#ifndef _COLOR_SELECTOR_H
#define _COLOR_SELECTOR_H

#include <gui/button.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/colorselector.h>
#include <util/message.h>

using namespace os;

class ColorSelectorWindow : public Window
{
public:
	ColorSelectorWindow(int,Color32_s, Window*);
	
	bool OkToQuit();
	void HandleMessage(Message*);
	void Layout();
private:
	ColorSelector* pcSelector;
	Button* pcOkButton;
	Button* pcCancelButton;
	
	LayoutView* pcLayoutView;
	Window* pcParentWindow;
	
	int nControl;
	Color32_s sControlColor;
};

#endif	

