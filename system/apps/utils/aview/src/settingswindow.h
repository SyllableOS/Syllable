#ifndef SETTINGS_WIN_H
#define SETTINGS_WIN_H

#include <gui/window.h>
#include <gui/checkbox.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/imagebutton.h>
#include <gui/button.h>
#include <gui/filerequester.h>
#include <gui/requesters.h>
#include "mainapp.h"
#include "messages.h"

using namespace os;

class SettingsWindow : public Window
{
public:
    SettingsWindow(ImageApp*,Window*);
    virtual void HandleMessage(Message*);

private:
    Button* pcOk;
    Button* pcCancel;
    ImageButton* pcSearch;
    TextView* pcOpenFieldTView;
    StringView* pcOpenFieldSView;
    CheckBox* pcSaveSizeCBView;
    FileRequester* m_pcLoadRequester;
    ImageApp* pcApplication;
    Window* pcParent;
    bool bSaveSize;
    std::string sOpenTo;
    void _Init();
};
#endif






