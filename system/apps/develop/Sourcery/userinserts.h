  #ifndef _USER_INSERTS_H
#define _USER_INSERTS_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/font.h>
#include <gui/stringview.h>
#include <gui/listview.h>
#include <util/settings.h>
#include <storage/file.h>
#include <gui/imageview.h>

using namespace os;

class UserInsertsEditWindow : public Window
{
public:
	UserInsertsEditWindow(Window* pcParent, bool);
	
	void Insert(const String& cName, const String& cText);

private:
	void Layout();
	void HandleMessage(Message* pcMessage);
	bool OkToQuit();
	
	Window* pcParentWindow;
	bool bIsNewWindow;
	
	LayoutView* pcView;
	
	TextView* pcNameText;
	TextView* pcInsertedText;
	
	StringView* pcNameString;
	StringView* pcInsertedString;
	
	Button* pcOkButton;
	Button* pcCancelButton;
};

//This is a simple dialog for user defined inserts
class UserInserts : public Window
{
public:
	UserInserts(Window* pcParent);
	~UserInserts() {}
private:
	void Layout();
	void HandleMessage(Message*);
	void LoadInserts();
	void SaveInserts();
	bool OkToQuit();
	
	LayoutView* pcView;
	
	Window* pcParentWindow;
	ListView* pcInsertsList;
	
	StringView* pcInsertNameString;
	TextView* pcInsertNameText;
	
	StringView* pcInsertTextString;
	TextView* pcInsertText;
	
	StringView* pcFormatsString;
	
	Button* pcOkButton;
	Button* pcCancelButton;
	Button* pcNewButton;
	Button* pcEditButton;
	Button* pcDeleteButton;

	Settings* pcInsertSettings;
	File* pcFile;
};

#endif //_USER_INSERTS_H















