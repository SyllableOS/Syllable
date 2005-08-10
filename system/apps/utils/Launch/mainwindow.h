#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <gui/layoutview.h>
#include <gui/window.h>
#include <gui/image.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/imageview.h>
#include <util/resources.h>
#include <atheos/image.h>
#include <gui/filerequester.h>
#include <gui/dropdownmenu.h>
#include <util/settings.h>

#include <vector>
using namespace os;

class MainWindow : public Window
{
public:
	enum
	{
		M_BROWSE,
		M_LAUNCH,
		M_CANCEL
	};
	
	MainWindow();
	~MainWindow();

	void HandleMessage(Message* pcMessage);
	void Layout();
	bool OkToQuit();
	os::BitmapImage* GetImage(const String&);
	int GetSelection(const String&);
	void LoadBuffer();
	void SaveBuffer();

private:
	FileRequester* pcFileRequester;
	
	Button* pcOkButton;
	Button* pcCancelButton;
	Button* pcBrowseButton;
	
	StringView* pcDescString;
	StringView* pcRunString;
	StringView* pcParamString;
	
	TextView* pcParamTextView;
	DropdownMenu* pcTextDrop;

	LayoutView* pcLayoutView;
	
	std::vector<String> m_cBuffer;
};

#endif //_MAIN_WINDOW_H


