#ifndef TOOLS_DIALOG_H
#define TOOLS_DIALOG_H

#include <gui/listview.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/imagebutton.h>
#include <util/message.h>
#include <gui/textview.h>

using namespace os;

class ToolsEditWindow : public Window
{
public:
	//if message == NULL new and if not edit
	ToolsEditWindow(Window* pcParent, Message* pcMessage);
	
	void HandleMessage(Message*);
	bool OkToQuit();
	
	void ChangeTo(Message*);
private:
	void Layout();
	void SetTabs();
	
	TextView* pcNameText;
	TextView* pcCommandText;
	TextView* pcArgumentsText;
	
	StringView* pcNameString;
	StringView* pcCommandString;
	StringView* pcArgumentsString;
	
	Button* pcOkButton;
	Button* pcCancelButton;
	
	LayoutView* pcLayoutView;
	Window* pcParentWindow;
	Message* pcSaveMessage;
	
	String cCommand;
	String cArguments;
	String cName;
	int nSelected;
};

class ToolsWindow : public Window
{
public:
	ToolsWindow(Window*);
	
	void HandleMessage(Message*);
	bool OkToQuit();
	
	void SaveTools();
	void LoadTools();
private:
	void Layout();
	void SetTabs();
	
	ListView* pcListView;
	LayoutView* pcLayoutView;
	ToolsEditWindow* pcToolsEditWindow;
	
	Button* pcNewButton;
	Button* pcDeleteButton;
	Button* pcEditButton;
	Button* pcSaveButton;
	Button* pcCancelButton;
	
	Window* pcParentWindow;
	
	bool bSaved;
};

#endif








