/*
    Locator - A fast file finder for Syllable
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)
                                                                                                                                                                                                        
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                                                                                                                                        
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                                                                                                                                        
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "util.h"
#include <util/application.h>
#include <util/message.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <util/looper.h>
#include <util/string.h>
#include <util/clipboard.h>
#include <sys/stat.h>
#include <regex.h>
#include <util/invoker.h>
#include <atheos/time.h>
#include <storage/file.h>

#include <vector>
#include <string>

using namespace std;

#define APP_NAME "Locator 0.2"
#define APP_VERSION "0.2"

#define PREFS_DIR "~/config/Locator"
#define PREFS_FILE "~/config/Locator/Config"
#define INDEX_FILE "data/db.index"
#define DATA_FILE "data/db"
#define INFO_FILE "data/info"

#define L_FILE 0
#define L_DIR 1

#define DEBUG_MODE 0

enum
{
  EV_START_SEARCH = 100,
  EV_ITEM_FOUND,
  EV_SEARCH_FINISHED,

  ERR_DIR_NOT_FOUND,
  ERR_REGEX,
  ERR_DATA_FILE,
  ERR_INDEX_FILE
};


using namespace os;


class SearchThread : public Looper
{

  public:
    SearchThread( Looper *pcParent );
    ~SearchThread( );
    void HandleMessage( Message *pcMessage );

  private:
    std::vector<string> cDirList;
    bool bFollowSubDirs;
    bool bMatchDirNames;
    bool bUseRegex;
    Looper *m_pcParent;
    bool bFoundDir;
    regex_t sRX;
    string zSearch;

    int FindMatch( Message *pcMessage );
    string ResolveLinks( string zSearchDir );
    void SendFoundItem( int nType, string zFileName, string zDirName );
    void SendMessage( int nId );
    bool Match( string zString );
    void Log( string zMessage );
};
