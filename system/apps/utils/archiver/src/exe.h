#ifndef _EXE_H
#define _EXE_H

#include <gui/window.h>
#include <gui/view.h>
#include <stdio.h>
#include <util/message.h>
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <fstream.h>
#include <string.h>
#include <unistd.h>
#include "messages.h"
using namespace os;



class ExeWindow : public Window
{
public:
    ExeWindow(Window* pcWindow);
    ~ExeWindow();
    Button* m_pcMakeBut;
    Button* m_pcCancelBut;
    virtual void HandleMessage(Message* pcMessage);
    virtual bool OkToQuit();

private:
    TextView* m_pcFileView;
    StringView* m_pcFileSView;
    TextView* m_pcNameView;
    StringView* m_pcNameSView;
    TextView* m_pcLabelView;
    StringView* m_pcLabelSView;
    Window* pcParentWindow;
};

#endif


