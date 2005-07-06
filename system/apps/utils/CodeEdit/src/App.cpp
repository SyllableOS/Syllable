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
#include "App.h"
#include "EditWin.h"
#include "SettingsDialog.h"
#include "Dialogs.h"

#include <codeview/format_c.h>
#include <codeview/format_java.h>
#include <codeview/format_perl.h>
#include <codeview/format_html.h>

#include <memory>
#include <iostream>
#include <unistd.h> /* for get_current_dir_name() */

#include <gui/filerequester.h>
#include <util/messenger.h>
#include <storage/file.h>
#include <storage/directory.h>

using namespace cv;
using namespace std;

App::App()
        : os::Application("application/x-wnd-nikrowd-" APP_NAME),
        options(NULL), loadRequester(NULL), loadRequesterVisible(false)
{
	zCWD = get_current_dir_name();
	zCWD = (char*) realloc(zCWD, strlen(zCWD) + 2);
	zCWD[strlen(zCWD)+1] = '\0';
	zCWD[strlen(zCWD)] = '/';
	
	loadSettings();
}

App::~App()
{
	 if(!storeSettings())
           cerr << APP_NAME ": Failed to write settings to disk!\n";
	if ( zCWD != NULL ) free(zCWD);
}

bool App::OkToQuit()
{
    if(windowList)
    {
        if(windowList->SafeLock()==0)
        {
            windowList->PostMessage(os::M_QUIT, windowList);
            windowList->Unlock();
        }
        return false;
    }
    else
    {
        return true;
    }
}

void App::init(int argc, char** argv)
{
    if(argc==1)
    {
        openWindow();
    }
    else
    {
        for(int a=1;a<argc;++a)
            openWindow(argv[a]);
    }
}

void App::openWindow(const char* path)
{
    //EditWin *win=new EditWin(this, path);
    windowList = new EditWin(this,path);

    if (path !=NULL)
    {
        windowList->setFavorites(path);
        windowList->UpdateFavorites();
    }
}


void App::openTab(const char* path)
{
    if (path !=NULL)
    {
        // for (uint i=0; i < windowList.size(); ++i)
        //{
        Message *msg = new Message( M_OPEN_FILE_IN_TAB );
        msg->AddString( "path", path );
        windowList->PostMessage( msg, windowList);
        //}
    }
}

void App::windowClosed(void* win)
{
    /* for(uint a=0;a<windowList.size();++a)
     {
         if(windowList[a]==win)
         {
             windowList.erase(&windowList[a]);
             break;
         }
     }

     if(windowList.empty())
         PostMessage(os::M_QUIT);*/
}

void App::setFormatList(const vector<FormatSet>& s)
{
    formatList=s;

    // for(uint a=0;a<windowList.size();++a)
    //{
    if(windowList->SafeLock()==0)
    {
        windowList->PostMessage(EditWin::ID_FORMAT_CHANGED, windowList);
        windowList->Unlock();
    }
    //}
}

bool App::storeSettings()
{
    //first put data into message
    keymap.store(&settings);

    settings.RemoveName("formatCount");
    settings.RemoveName("formatName");
    settings.RemoveName("formatFilter");
    settings.RemoveName("formatFormat");
    settings.RemoveName("formatUseTab");
    settings.RemoveName("formatTabSize");
    settings.RemoveName("formatUseTab");
    settings.RemoveName("formatAutoindent");
    settings.RemoveName("formatFgColor");
    settings.RemoveName("formatBgColor");

    settings.AddInt32("formatCount", (int32)formatList.size());
    for(uint a=0;a<formatList.size();++a)
    {
        settings.AddString("formatName", formatList[a].name);
        settings.AddString("formatFilter", formatList[a].filter);
        settings.AddString("formatFormat", formatList[a].formatName);
        settings.AddInt32("formatTabSize", formatList[a].tabSize);
        settings.AddBool("formatUseTab", formatList[a].useTab);
        settings.AddBool("formatAutoindendt", formatList[a].autoindent);
        settings.AddColor32("formatBgColor", formatList[a].bgColor);
        settings.AddColor32("formatFgColor", formatList[a].fgColor);

        if(formatList[a].format)
        {
            char tmp[20];
            sprintf(tmp, "format%iStyle", a);
            settings.RemoveName(tmp);
            for(uint b=0;b<formatList[a].format->GetStyleCount();++b)
                settings.AddColor32(tmp, formatList[a].format->GetStyle(b).sColor);
        }
    }

    //then write message to disk
    if(settings.Save() != 0)
        return false;
    return true;
}

bool App::loadSettings()
{
    if(settings.Load() != 0)
        loadDefaults();

    keymap.load(&settings);

    int32 c;
    if(settings.FindInt32("formatCount", &c)==0)
    {
        formatList.resize(c);
        for(int a=0;a<c;++a)
        {
            settings.FindString("formatName", &formatList[a].name, a);
            settings.FindString("formatFilter", &formatList[a].filter, a);
            settings.FindString("formatFormat", &formatList[a].formatName, a);
            if(formatList[a].formatName=="C formater")
                formatList[a].format=new Format_C();
            else if(formatList[a].formatName=="Java formater")
                formatList[a].format=new Format_java();
            settings.FindInt32("formatTabSize", &formatList[a].tabSize, a);
            settings.FindBool("formatUseTab", &formatList[a].useTab, a);
            settings.FindBool("formatAutoindent", &formatList[a].autoindent, a);
            settings.FindColor32("formatBgColor", &formatList[a].bgColor, a);
            settings.FindColor32("formatFgColor", &formatList[a].fgColor, a);

            if(formatList[a].format)
            {
                char tmp[20];
                sprintf(tmp, "format%iStyle", a);
                Color32_s c;
                for(uint b=0;b<formatList[a].format->GetStyleCount();++b)
                {
                    if(settings.FindColor32(tmp, &c, b)==0)
                    {
                        CodeViewStyle cs;
                        cs.sColor = c;
                        formatList[a].format->SetStyle(b, cs);
                    }
                }
            }
        }
    }

    return true;
}

void App::loadDefaults()
{
    keymap.addMapping("4o", EditWin::ID_OPTIONS);
    keymap.addMapping("2n", EditWin::ID_FILE_NEW);
    keymap.addMapping("2s", EditWin::ID_FILE_SAVE);
    keymap.addMapping("3s", EditWin::ID_FILE_SAVE_AS);
    keymap.addMapping("2o", EditWin::ID_FILE_OPEN);
    keymap.addMapping("2q", EditWin::ID_FILE_CLOSE);
    keymap.addMapping("2f", EditWin::ID_FIND);
    keymap.addMapping("0\20\4", EditWin::ID_FIND_NEXT);//F3
    keymap.addMapping("2r", EditWin::ID_REPLACE);
    keymap.addMapping("2g", EditWin::ID_GOTO_LINE);

    formatList.resize(5);
    formatList[0].name="(default)";
    formatList[0].filter="N/A";
    formatList[0].formatName="Null formater";
    formatList[1].name="C/C++";
    formatList[1].filter=".c;.h;.C;.cpp;.cxx";
    formatList[1].format=new Format_C();
    formatList[1].formatName="C formater";
    formatList[2].name="Java";
    formatList[2].filter=".java";
    formatList[2].format=new Format_java();
    formatList[2].formatName="Java formater";
    formatList[3].name="Perl";
    formatList[3].filter=".perl;.pm;";
    formatList[3].format=new Format_Perl();
    formatList[3].formatName="Perl formater";
    formatList[4].name="HTML";
    formatList[4].filter=".htm;.html;.HTM;.HTML";
    formatList[4].format=new Format_HTML();
    formatList[4].formatName="HTML formater";
}


void App::HandleMessage(os::Message *m)
{
    switch(m->GetCode())
    {
    case ID_OPEN_FILE:
        {
            m->FindBool("open in win",&bWin);
            if(loadRequester)
            {
                loadRequester->Lock();
                if(!loadRequesterVisible)
                    loadRequester->Show();
                loadRequesterVisible=true;
                loadRequester->MakeFocus();
                loadRequester->Unlock();
            }
            else
            {
                loadRequester =new FileReq(os::FileRequester::LOAD_REQ,
                                           new os::Messenger(this), zCWD, os::FileRequester::NODE_FILE,
                                           true, new os::Message(ID_DO_OPEN_FILE), NULL, true);

                loadRequester->Show();
                loadRequesterVisible=true;
                loadRequester->MakeFocus();
            }
            break;
        }
    case ID_DO_OPEN_FILE:
        {
            loadRequesterVisible=false;
            string s;
            for(int i=0;m->FindString("file/path", &s, i)==0;++i)
            {
                openTab(s.c_str());
                windowList->setFavorites(s.c_str());
            }
            break;
        }

    case os::M_FILE_REQUESTER_CANCELED:
        loadRequesterVisible=false;
        break;
    case ID_NEW_FILE:
        openWindow();
        break;
    case ID_SHOW_OPTIONS:
        if(!options)
        {
            options=new SettingsDialog(this);
            options->CenterInWindow(windowList );
            options->Show();
        }
        options->MakeFocus();
        break;
    case ID_OPTIONS_CLOSED:
        options=NULL;
        break;
    case ID_WINDOW_CLOSING:
        {
            void *p;
            if(m->FindPointer("source", &p)==0)
                windowClosed(p);
            break;
        }
#ifndef NDEBUG
    default:
        cout << "App: Unhandled Message: " << m->GetCode() << endl;
        for(int a=0;a<m->CountNames();++a)
            cout << "\t" << m->GetName(a).str() << endl;
#endif

    }
}


int main(int argc, char** argv)
{
    App *app=new App();

    app->init(argc, argv);
    app->Run();

    return 0;
}






