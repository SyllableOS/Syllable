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
#include "SettingsTab_Key.h"

#include "App.h"
#include "KeyMap.h"
#include "EditWin.h"

#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/listview.h>
#include <gui/checkbox.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>

#include <util/message.h>


const SettingsTab_Key::KeyMapping SettingsTab_Key::mapList[]=
    {
        {"Application / Quit", EditWin::ID_QUIT
        },
        {"Application / About", EditWin::ID_ABOUT},
        {"Application / Options", EditWin::ID_OPTIONS},
        {"File / New", EditWin::ID_FILE_NEW},
        {"File / Open", EditWin::ID_FILE_OPEN},
        {"File / Save", EditWin::ID_FILE_SAVE},
        {"File / Save As", EditWin::ID_FILE_SAVE_AS},
        {"File / Close", EditWin::ID_FILE_CLOSE},
        {"Edit / Undo", EditWin::ID_EDIT_UNDO},
        {"Edit / Redo", EditWin::ID_EDIT_REDO},
        {"Edit / Cut", EditWin::ID_EDIT_CUT},
        {"Edit / Copy", EditWin::ID_EDIT_COPY},
        {"Edit / Paste", EditWin::ID_EDIT_PASTE},
        {"Edit / Delete", EditWin::ID_EDIT_DELETE},
        {"Search / Find", EditWin::ID_FIND},
        {"Search / Find next", EditWin::ID_FIND_NEXT},
        {"Search / Find previous", EditWin::ID_FIND_PREVIOUS},
        /*	{"Search / Replace", EditWin::ID_REPLACE},
        	{"Search / Replace next", EditWin::ID_REPLACE_NEXT},
        	{"Search / Replace all", EditWin::ID_REPLACE_ALL},
        */	{"Search / Goto line", EditWin::ID_GOTO_LINE},
        {"", -1}//end marker
    };

class KeyTextView : public os::TextView
{
private:
    string key;
public:
    KeyTextView(const os::Rect &r, const char *s, uint i)
            : os::TextView(r, s, "", i)
    {}

    void setKey(const string &s)
    {
        key=s;
        if(!key.empty())
            key[0]='0';
        Set(KeyMap::getDisplayString(key).c_str());
    }

    string getKey()
    {
        return key;
    }

    virtual void KeyDown(const char *str, const char *rawstr, uint32 qual)
    {
        if(!*rawstr)
            return;
        os::Looper *looper=GetLooper();
        if(!looper)
            return;
        os::Message *m=looper->GetCurrentMessage();
        if(!m)
            return;
        setKey(KeyMap::encodeKey(m));
    }
};

const float pageW=400;
const float pageH=400;

SettingsTab_Key::SettingsTab_Key(App* m)
        : os::View(os::Rect(0, 0, pageW, pageH), "SettingsTab Key", os::CF_FOLLOW_ALL),
        app(m)
{

    text=new KeyTextView(os::Rect(), "KeyText",
                         os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
    func=new os::DropdownMenu(os::Rect(), "KeyFunc",
                              os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
    func->SetReadOnly(true);
    shift=new os::CheckBox(os::Rect(), "KeyShift", "Shift", NULL,
                           os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT);
    ctrl=new os::CheckBox(os::Rect(), "KeyCtrl", "Control", NULL,
                          os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT);
    alt=new os::CheckBox(os::Rect(), "KeyAlt", "Alt", NULL,
                         os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT);
    setBut=new os::Button(os::Rect(), "keySetButton", "Set", new os::Message(ID_SET),
                          os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT);
    unsetBut=new os::Button(os::Rect(), "keyUnSetButton", "Unset", new os::Message(ID_UNSET),
                            os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT);

    list=new os::ListView(os::Rect(), "KeyList", os::ListView::F_RENDER_BORDER,
                          os::CF_FOLLOW_ALL);
    list->InsertColumn("Key Combination", (pageW-50)/2);
    list->InsertColumn("Function", (pageW-50)/2);
    list->SetSelChangeMsg(new os::Message(ID_LIST_CHANGE));

    for(uint a=0;mapList[a].msg>=0;++a)
    {
        mapText.insert(map<string, int>::value_type(mapList[a].desc, mapList[a].msg));
        mapMsg.insert(map<int, string>::value_type(mapList[a].msg, mapList[a].desc));
        func->AppendItem(mapList[a].desc.c_str());
    }

    app->Lock();
    map<string, int>::iterator it=app->getKeyMap().keymap.begin();
    map<string, int>::iterator end=app->getKeyMap().keymap.end();
    while(it!=end)
    {
        addBinding((*it).first, (*it).second);
        ++it;
    }
    app->Unlock();
    list->Sort();

    list->Select(0, 0);

    os::Point p0=setBut->GetPreferredSize(false);
    os::Point p1=text->GetPreferredSize(false);
    os::Point p2=shift->GetPreferredSize(false);
    os::Point p3=ctrl->GetPreferredSize(false);
    os::Point p4=alt->GetPreferredSize(false);
    os::Point p5=unsetBut->GetPreferredSize(false);

    float cw=max(p2.x, max(p3.x, p4.x));
    float y=pageH-5;

    alt->SetFrame(os::Rect(5, y-p4.y, 5+cw, y));
    setBut->SetFrame(os::Rect(10+cw, y-p0.y, 10+cw+p0.x, y));
    unsetBut->SetFrame(os::Rect(15+cw+p0.x, y-p0.y, 15+cw+p0.x+p5.x, y));
    y-=p4.y+5;
    ctrl->SetFrame(os::Rect(5, y-p3.y, 5+cw, y));
    y-=p3.y+5;
    shift->SetFrame(os::Rect(5, y-p2.y, 5+cw, y));
    y-=p2.y+5;

    os::StringView *sv=new os::StringView(os::Rect(), "Text0", "Function", os::ALIGN_LEFT,
                                          os::CF_FOLLOW_LEFT | os::CF_FOLLOW_BOTTOM);
    float sw=sv->GetPreferredSize(false).x+10;
    sv->SetString("Key");
    sv->SetFrame(os::Rect(5, y-p1.y, sw, y));
    AddChild(sv);
    text->SetFrame(os::Rect(5+sw, y-p1.y, pageW-5, y));
    y-=p1.y+5;
    sv=new os::StringView(os::Rect(), "Text1", "Function", os::ALIGN_LEFT,
                          os::CF_FOLLOW_LEFT | os::CF_FOLLOW_BOTTOM);
    sv->SetFrame(os::Rect(5, y-p1.y, sw, y));
    AddChild(sv);
    func->SetFrame(os::Rect(5+sw, y-p1.y, pageW-5, y));
    y-=p1.y+5;

    list->SetFrame(os::Rect(5, 5, pageW-5, y));

    AddChild(list);
    AddChild(text);
    AddChild(func);
    AddChild(setBut);
    AddChild(unsetBut);
    AddChild(shift);
    AddChild(ctrl);
    AddChild(alt);
}

void SettingsTab_Key::HandleMessage(os::Message* m)
{
    switch(m->GetCode())
    {
    case ID_LIST_CHANGE:
        {
            os::ListViewStringRow *r=static_cast<os::ListViewStringRow*>(
                                         list->GetRow(list->GetFirstSelected()));
            if(r)
            {
                func->SetCurrentString(r->GetString(1));
                string s=r->GetString(2);
                char mod=s[0]-'0';
                shift->SetValue((mod&1)!=0);
                ctrl->SetValue((mod&2)!=0);
                alt->SetValue((mod&4)!=0);
                text->setKey(s);
            }
            break;
        }
    case ID_SET:
        {
            char mod=0;
            if(shift->GetValue().AsBool())
                mod|=1;
            if(ctrl->GetValue().AsBool())
                mod|=2;
            if(alt->GetValue().AsBool())
                mod|=4;
            mod+='0';
            string key=text->getKey();
            if(key.size()<2)
                break;
            key[0]=mod;

            removeBinding(key);
            addBinding(key, msgFromText(func->GetCurrentString()));

            list->Sort();
            break;
        }
    case ID_UNSET:
        {
            char mod=0;
            if(shift->GetValue().AsBool())
                mod|=1;
            if(ctrl->GetValue().AsBool())
                mod|=2;
            if(alt->GetValue().AsBool())
                mod|=4;
            mod+='0';
            string key=text->getKey();
            if(key.size()<2)
                break;
            key[0]=mod;

            removeBinding(key);

            list->Sort();
            break;
        }
    }
}

int SettingsTab_Key::msgFromText(const string& text)
{
    map<string, int>::iterator it=mapText.find(text);
    if(it==mapText.end())
        return -1;
    return (*it).second;
}

string SettingsTab_Key::textFromMsg(int msg)
{
    map<int, string>::iterator it=mapMsg.find(msg);
    if(it==mapMsg.end())
        return "<unknown>";
    return (*it).second;
}

void SettingsTab_Key::apply()
{
    app->Lock();

    KeyMap &km=app->getKeyMap();

    km.clear();
    for(uint a=0;a<list->GetRowCount();++a)
    {
        os::ListViewStringRow *r=static_cast<os::ListViewStringRow*>(list->GetRow(a));
        km.addMapping(r->GetString(2), msgFromText(r->GetString(1)));
    }
    app->Unlock();
}

void SettingsTab_Key::AllAttached()
{
    setBut->SetTarget(this);
    unsetBut->SetTarget(this);
    list->SetTarget(this);
}

void SettingsTab_Key::addBinding(const string &key, int msg)
{
    os::ListViewStringRow *r=new os::ListViewStringRow();
    r->AppendString(KeyMap::getDisplayString(key));
    r->AppendString(textFromMsg(msg));
    r->AppendString(key);
    list->InsertRow(r);
}

void SettingsTab_Key::removeBinding(const string &key)
{
    for(uint a=0;a<list->GetRowCount();++a)
    {
        os::ListViewStringRow *r=static_cast<os::ListViewStringRow*>(list->GetRow(a));
        if(r->GetString(2)==key)
        {
            delete list->RemoveRow(a);
            break;
        }
    }
}





