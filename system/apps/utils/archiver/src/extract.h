#ifndef _EXTRACT_H
#define _EXTRACT_H

#include <gui/window.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/desktop.h>
#include <util/message.h>
#include <fstream.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <gui/requesters.h>
#include <iostream.h>
#include <gui/filerequester.h>
#include <storage/filereference.h>
#include <sys/stat.h>
#include "messages.h"

using namespace os;


class ExtractWindow : public Window
{

public:

    ExtractWindow(Window* pcParent);
    ~ExtractWindow();
    Button* m_pcExtractButton;
    TextView* m_pcExtractView;
    Button*  m_pcBrowseButton;
    Button*  m_pcCloseButton;
    void mExtract();
    void HandleMessage(Message* pcMessage);

private:
    String cBuf;
    FileRequester* m_pcBrowse;
    const char* pzExtractPath;
    const char* pzAPath;
    const char* pzDPath;
    const char* pzDestPath;
    char pzPath[0x900];
    bool Read();
    char junk[1024];
    Window* pcParentWindow;
    char lineExtract[1024];
    //char currentLine3[1024];
    ifstream ConfigFile;
    char zConfigFile[128];
};

#endif;

