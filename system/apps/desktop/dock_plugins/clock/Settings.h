#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <gui/window.h>
#include <gui/button.h>
#include <gui/view.h>
#include <gui/checkbox.h>
#include <gui/dropdownmenu.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <util/message.h>

#include "ColorSelector.h"
#include "colorbutton.h"

using namespace os;

class DockClockSettings : public Window
{
public:
	DockClockSettings(Message* pcMessage,View* pcPlugin);
		
	void HandleMessage(Message*);
	bool OkToQuit();
	void Layout();
	void SetFontTypes();
	void LoadTimes();
	void LoadDates();
	void SaveSettings();
	void LoadSettings();
	
	String GetTimeFormat();
	String GetDateFormat();
	
private:
	LayoutView* pcLayoutView;
	Message* pcSendMessage;
	View* pcParentPlugin;
		
	Button* pcOkButton;
	Button* pcDefaultButton;
	Button* pcCancelButton;
	
	ColorSelectorWindow* pcSelector;
	CheckBox* pcDrawBox;
	
	StringView* pcFontString;
	DropdownMenu* pcFontTypeDrop;
	DropdownMenu* pcFontSizeDrop;
	ColorButton* pcFontColorButton;
	
	StringView* pcTimeString;
	DropdownMenu* pcTimeDrop;
	
	StringView* pcDateString;
	DropdownMenu* pcDateDrop;
	
	CheckBox* pcFrameCheck;
	
	bool bFrame;
	Color32_s cTextColor;
	float vFontSize;
	String cFontType;
	String cTimeFormat;
	String cDateFormat;
};

#endif









