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
#ifndef _SETTINGSTAB_FORMATS_H_
#define _SETTINGSTAB_FORMATS_H_


namespace os
{
class Button;
class ListView;
class TextView;
class DropdownMenu;
class Spinner;
};

#include <gui/view.h>

#include <vector>

#include "App.h"

class SettingsTab_Formats : public os::View
{
private:
    App* app;

    enum{ ID_ADD, ID_REMOVE, ID_FORMAT_CHANGE,
          ID_FORMATER_CHANGE, ID_STYLE_CHANGE,
          ID_COLOR_CHANGE
        };

    os::DropdownMenu *mFormat;
    os::Button *bAdd, *bRemove;

    os::TextView *tName, *tFilter, *tTabsize;
    os::DropdownMenu *mFormater, *mUseTabs;

    os::ListView *lStyles;
    os::Spinner *sRed, *sGreen, *sBlue;

    vector<FormatSet> formatList;

    int iFormat;

    void loadValues(int i);
    void storeValues(int i);
public:
    SettingsTab_Formats(App*);

    void apply();

    virtual void HandleMessage(os::Message*);
    virtual void AllAttached();
};

#endif




