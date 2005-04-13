#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <gui/button.h>
#include <gui/window.h>
#include <gui/listview.h>
#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <util/message.h>
#include <util/settings.h>
#include <storage/file.h>
#include <gui/dropdownmenu.h>

#include "messages.h"

using namespace os;

class NewEditWindow : public Window
{
public:
	NewEditWindow(bool bIsNew,Window*);
	
	void HandleMessage(Message*);
	void SetEditDetails(String,String);
private:
	void Layout();
	
	LayoutView* pcLayoutView;
	
	Button* pcOkButton;
	Button* pcCancelButton;
	
	TextView* pcTagTextView;
	TextView* pcWebTextView;
	
	StringView* pcTagStringView;
	StringView* pcWebStringView;
	
	Window* pcParentWindow;
	Message* pcMainMessage;
	bool bNewWindow;
};

class SettingsWindow : public Window
{
public:	
	SettingsWindow(View* pcParent,int32 nDefault);
	
	void Layout();
	void HandleMessage(Message*);
	void SaveAddresses();
	void LoadAddresses();
private:
	LayoutView* pcLayoutView;
	
	Button* pcDeleteButton;
	Button* pcEditButton;
	Button* pcNewButton;
	Button* pcOkButton;
	Button* pcApplyButton;
	Button* pcCancelButton;
	
	ListView* pcListView;
	NewEditWindow* pcNewEditWindow;
	View* pcParentView;
	DropdownMenu* pcDefaultDrop;
	int nEditted;
};

#endif	
	





