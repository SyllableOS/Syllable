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
#include "KeyMap.h"

#include <util/message.h>
#include <util/settings.h>
#include <gui/guidefines.h>

void KeyMap::addMapping(const string &key, int msg)
{
    keymap.insert(map<string, int>::value_type(key, msg));
}

void KeyMap::removeMapping(const string &key)
{
    map<string, int>::iterator it=keymap.find(key);
    if(it!=keymap.end())
        keymap.erase(it);
}

void KeyMap::clear()
{
    keymap.clear();
}

bool KeyMap::findKey(const string &key, int *msg)
{
    map<string, int>::iterator it=keymap.find(key);
    if(it==keymap.end())
        return false;
    if(msg)
        *msg=(*it).second;
    return true;
}

bool KeyMap::load(os::Settings *m)
{
    string str;
    if(m->FindString("KeyMap.key", &str)==0)
    {
        clear();
        uint i=0;
        int32 val;
        do
        {
            if(m->FindInt32("KeyMap.value", &val, i)==0)
                addMapping(str, val);
            ++i;
        }
        while(m->FindString("KeyMap.key", &str, i)==0);
        return true;
    }
    return false;
}

bool KeyMap::store(os::Settings *m)
{
    m->RemoveName("KeyMap.key");
    m->RemoveName("KeyMap.value");

    map<string, int>::iterator it=keymap.begin();
    while(it!=keymap.end())
    {
        m->AddString("KeyMap.key", (*it).first);
        m->AddInt32("KeyMap.value", (*it).second);
        ++it;
    }
    return true;
}

string KeyMap::encodeKey(os::Message *m)
{
    if(m->GetCode()!=os::M_KEY_DOWN)
        return "";

    string rawstr;
    int32 qual, rawkey;

    if(m->FindString("_raw_string", &rawstr)!=0 ||
            m->FindInt32("_qualifiers", &qual)!=0 ||
            m->FindInt32("_raw_key", &rawkey)!=0)
        return "";

    return encodeKey(rawstr, qual, rawkey);
}

string KeyMap::encodeKey(const string &rawstr, uint32 qualifiers, uint32 rawkey)
{
    char mod=0;
    if(qualifiers&os::QUAL_SHIFT)
        mod|=1;
    if(qualifiers&os::QUAL_CTRL)
        mod|=2;
    if(qualifiers&os::QUAL_ALT)
        mod|=4;
    mod+='0';

    string ret;
    ret+=mod;

    if(rawstr[0]==os::VK_FUNCTION_KEY)
    {
        ret+=rawstr[0];
        ret+=(char)rawkey;
    }
    else
    {
        ret+=rawstr;
    }

    return ret;
}

string KeyMap::getDisplayString(const string &key)
{
    if(key.size()<2)
        return "<none>";

    string tmp;
    char mod=key[0]-'0';
    if(mod&2)
        tmp+="CTRL + ";
    if(mod&4)
        tmp+="ALT + ";
    if(mod&1)
        tmp+="SHIFT + ";

    string k=key.substr(1);

    switch(k[0])
    {
    case os::VK_BACKSPACE:
        k="BACKSPACE";
        break;
    case os::VK_ENTER:
        k="ENTER";
        break;
    case os::VK_SPACE:
        k="SPACE";
        break;
    case os::VK_TAB:
        k="TAB";
        break;
    case os::VK_ESCAPE:
        k="ESCAPE";
        break;
    case os::VK_LEFT_ARROW:
        k="LEFT";
        break;
    case os::VK_RIGHT_ARROW:
        k="RIGHT";
        break;
    case os::VK_UP_ARROW:
        k="UP";
        break;
    case os::VK_DOWN_ARROW:
        k="DOWN";
        break;
    case os::VK_INSERT:
        k="INSERT";
        break;
    case os::VK_DELETE:
        k="DELETE";
        break;
    case os::VK_HOME:
        k="HOME";
        break;
    case os::VK_END:
        k="END";
        break;
    case os::VK_PAGE_UP:
        k="PAGE UP";
        break;
    case os::VK_PAGE_DOWN:
        k="PAGE DOWN";
        break;
    case os::VK_FUNCTION_KEY:
        if(k.size()==2)
        {
            char buf[10];
            sprintf(buf, "F%i", k[1]-1);
            k=buf;
        }
        break;
    }

    return tmp+k;
}





