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
#ifndef _SETTINGSTAB_KEY_H_
#define _SETTINGSTAB_KEY_H_

using namespace std;

namespace os
{
class Button;
class ListView;
class CheckBox;
class TextView;
class DropdownMenu;
};

#include <gui/view.h>

#include <map>

class App;
class KeyTextView;

class SettingsTab_Key : public os::View
{
private:
    struct KeyMapping
    {
        const string desc;
        const int msg;
    };
    static const KeyMapping mapList[];

    App* app;

    enum{ ID_SET, ID_UNSET, ID_LIST_CHANGE };

    os::ListView *list;
    os::Button *setBut, *unsetBut;
    KeyTextView *text;
    os::DropdownMenu *func;
    os::CheckBox *shift, *ctrl, *alt;
    map<string, int> mapText;
    map<int, string> mapMsg;

    int msgFromText(const string&);
    string textFromMsg(int);

    void addBinding(const string&, int);
    void removeBinding(const string&);

public:
    SettingsTab_Key(App*);

    void apply();

    virtual void HandleMessage(os::Message*);
    virtual void AllAttached();
};

#endif

