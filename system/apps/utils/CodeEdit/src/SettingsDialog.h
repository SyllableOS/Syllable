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
#ifndef _SETTINGSDIALOG_H_
#define _SETTINGSDIALOG_H_

#include <gui/window.h>

namespace os
{
class TabView;
class Button;
};

#include <map>
#include "EditWin.h"
class App;
class SettingsTab_Key;
class SettingsTab_Formats;
class SettingsTab_Fonts;

class SettingsDialog : public os::Window
{
private:
    App* app;
    SettingsTab_Key *keyTab;
    SettingsTab_Formats *formatTab;
    SettingsTab_Fonts *fontTab;

    enum{ ID_OK, ID_APPLY, ID_CANCEL };

    os::TabView *tabView;
    os::Button *okButton, *applyButton, *cancelButton;

    void apply();
public:
    SettingsDialog(App*, EditWin* p = NULL);

    virtual void HandleMessage(os::Message*);
    virtual bool OkToQuit();
};

#endif










