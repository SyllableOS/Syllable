#ifndef NEW_H
#define NEW_H

#include <gui/window.h>
#include <gui/frameview.h>
#include <gui/font.h>
#include <gui/stringview.h>
#include <gui/textview.h> 
#include <gui/button.h>
#include <gui/view.h>
#include <gui/filerequester.h>
#include <util/message.h>
#include <gui/dropdownmenu.h>
#include <gui/desktop.h>
#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <gui/imagebutton.h>
#include "messages.h"
#include <unistd.h>
#include <sys/stat.h>
using namespace os;

class NewDrop : public DropdownMenu{
public:
	NewDrop();
};

class NewFrameView : public FrameView{
public:
	NewFrameView(Rect& r);
	NewDrop* m_pcDrop;
	StringView* m_pcDropView;
	StringView* m_pcFileStringView;
    TextView* m_pcFileTextView;
    ImageButton* m_pcFileButton;
	TextView* m_pcDirectoryTextView;
    StringView* m_pcDirectoryStringView;
    ImageButton* m_pcDirectoryButton;
};

class NewView : public View{
public:
	NewView(const Rect& r);
	NewFrameView* m_pcFrameView;
	Button* m_pcCancel;
	Button* m_pcCreate;
};

class NewWindow : public Window{
public:
	NewWindow(Window* pcParent, bool);
	~NewWindow();
	
	void HandleMessage(Message* pcMessage);

private:
	void Create(int Sel);
	
	Window* pcParentWindow;
	NewView* m_pcView;

	FileRequester* m_pcOpenSelect;
	FileRequester* m_pcSaveSelect;
	
	const char *pzSelectPath;
	bool bCloseAfterCreation;
};
#endif
	


