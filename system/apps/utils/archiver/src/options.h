#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <gui/window.h>
#include <gui/button.h>
#include <gui/desktop.h>
#include <gui/textview.h>
#include <gui/checkbox.h>
#include <gui/view.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <util/message.h>
#include <fstream.h>
#include <storage/filereference.h>
#include <gui/tabview.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/dropdownmenu.h>
#include <gui/checkbox.h>
#include <gui/slider.h>
#include <gui/imagebutton.h>
#include "messages.h"
#include <unistd.h>
using namespace os;

class AppSettings;

class OptionsPathView : public FrameView
{
public:
    OptionsPathView();
};

class OptionsSettingsView : public FrameView
{
public:
	OptionsSettingsView();
private:
};

class OptionsGeneralView : public View
{
public:
    OptionsGeneralView();

    StringView* m_pcOpenToView;
    ImageButton* m_pcOpenToBut;
    TextView* m_pcOpenToTextView;
    ImageButton* m_pcExtractToBut;
    StringView* m_pcExtractToView;
    TextView* m_pcExtractToTextView;
    ImageButton* m_pcSelectButtonBarPath;
    StringView* m_pcButtonBarView;
    TextView* m_pcButtonBarTextView;
    CheckBox* m_pcExtractClose;
    CheckBox* m_pcNewClose;
};

class OptionsMiscSettingsView : public View{
public:
	OptionsMiscSettingsView();
	
	StringView* m_pcSlideView;
	Slider* m_pcSlider;
	StringView* m_pcSetShortView;
	DropdownMenu* m_pcSetShort;
	StringView* m_pcRecentView;
	DropdownMenu* m_pcRecentDrop;
};


class OptionsTabView : public TabView
{
public:
	OptionsTabView(const Rect&);
    OptionsGeneralView* m_pcGenView;
    OptionsMiscSettingsView* m_pcMisc;
};


class OptionsWindow : public Window
{
public:
    OptionsWindow(Window* pcParent, AppSettings*);
    ~OptionsWindow();
private:
    void 	HandleMessage(Message* pcMessage);
    void	SaveOptions();
	void	_Init();

    float 	GetSlide();
    void 	SetSlide(float);
    bool 	ExtractClose();
    bool 	NewClose();
    
	void 	Tab();
	
	OptionsTabView* m_pcTabView;
    Button* m_pcCancelButton;
    Button* m_pcSaveButton;
    
    Window* pcParentWindow;

    String cExtractPath;
    String cOpenPath;

    bool Extract;
    bool Open;


    Button* m_pcExtractToBut;
    StringView* m_pcExtractToView;
    TextView* m_pcExtractToTextView;

    //Open Path Option Controls
    Button* m_pcOpenToBut;
    StringView* m_pcOpenToView;
    TextView* m_pcOpenToTextView;
    FileRequester* m_pcPathRequester;

	AppSettings* pcAppSettings;

    CheckBox* m_pcExtractClose;
    CheckBox* m_pcRecentOption;
};
#endif





