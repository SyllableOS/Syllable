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
#include <iostream>

#include <gui/filerequester.h>
#include <storage/filereference.h>
#include <sys/stat.h>

#include "messages.h"

using namespace os;


class ExtractWindow : public Window
{
public:

    ExtractWindow(Window* pcParent, const String&, const String&);
    ~ExtractWindow();

    void 	Extract();
    void 	HandleMessage(Message* pcMessage);

private:
    Button* 		m_pcExtractButton;
    TextView* 		m_pcExtractView;
    Button*  		m_pcBrowseButton;
    Button*  		m_pcCloseButton;

    String 			cBuf;
	String 			cFilePath;
	String			cExtractPath;

    FileRequester* 	m_pcBrowse;

    Window* pcParentWindow;
};

#endif;

