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
#ifndef _KEYMAP_H_
#define _KEYMAP_H_

#include <map>
#include <string>
#include <util/settings.h>
#include <util/message.h>
#include <stdio.h>

using namespace os;
using namespace std;

class KeyMap
{
    friend class SettingsTab_Key;
private:
    map<string, int> keymap;
public:

    void addMapping(const string &key, int msg);
    void removeMapping(const string& key);
    void clear();

    bool findKey(const string& key, int *msg);

    bool store(os::Settings *m);
    bool load(os::Settings *m);

    static string encodeKey(os::Message *m);
    static string encodeKey(const string &rawstr, uint32 qualifiers, uint32 rawkey);
    static string getDisplayString(const string &key);
};


#define KEYMAP_DISPATCH(m) \
	if((m)->GetCode()==os::M_KEY_DOWN){ \
		string str=KeyMap::encodeKey(m); \
		int msg; \
		if(!str.empty() && findKey(str, &msg)){ \
			PostMessage(msg, this); \
			return; \
		} \
	}


#endif




