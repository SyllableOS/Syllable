/*	CodeEdit - A programmers editor for Atheos
	Copyright (c) 2002 Andreas Engh-Halstvedt
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/
#ifndef _APP_H_
#define _APP_H_

#include "KeyMap.h"

#include <util/application.h>
#include <util/message.h>
#include <util/settings.h>

#include <vector>

namespace os
{
class Window;
class FileRequester;
};
namespace cv
{
class Format;
};
class EditWin;
class SettingsDialog;


#define APP_NAME "CodeEdit"

class FormatSet
{
public:
    string name;
    string filter;
    cv::Format* format;
    string formatName;
    int32 tabSize;
    bool useTab;
    bool autoindent;
    os::Color32_s bgColor;
    os::Color32_s fgColor;

    FormatSet():
            name(), filter(), format(NULL), tabSize(4),
            useTab(true), autoindent(true),
            bgColor(255, 255, 255), fgColor(0, 0, 0)
    {}
}
;

class App : public os::Application
{
private:
    EditWin*  windowList;
    vector<FormatSet> formatList;

    os::Settings settings;
    KeyMap keymap;

    SettingsDialog* options;
    os::FileRequester* loadRequester;
    bool loadRequesterVisible;

    void openWindow(const char* =NULL);
    void openTab(const char* = NULL);
    void windowClosed(void*);

    bool loadSettings();
    void loadDefaults();
    bool storeSettings();
    bool bWin;

public:
    enum{ ID_OPEN_FILE, ID_DO_OPEN_FILE, ID_NEW_FILE,
          ID_SHOW_OPTIONS, ID_OPTIONS_CLOSED,
          ID_WINDOW_CLOSING
        };

    char *zCWD; // Initial working directory

    App();
    ~App();

    void init(int argc, char** argv);

    const os::Message& getSettings()
    {
        return settings;
    }
    KeyMap& getKeyMap()
    {
        return keymap;
    }
    vector<FormatSet> getFormatList()
    {
        return formatList;
    }
    void setFormatList(const vector<FormatSet>&);


    //overridden methods
    virtual bool OkToQuit();
    virtual void HandleMessage(os::Message*);
};

#endif




