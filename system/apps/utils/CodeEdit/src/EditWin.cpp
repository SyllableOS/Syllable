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
#include "EditWin.h"
#include "App.h"
#include "StatusBar.h"
#include "Dialogs.h"
#include "ImageItem.h"
#include "LoadBitmap.h"
#include "Debug.h"
#include <codeview/codeview.h>
#include <codeview/format_c.h>
#include <macros.h>
#include <gui/menu.h>
#include <gui/textview.h>
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <storage/file.h>
#include <util/regexp.h>
#include <gui/imagebutton.h>
#include <gui/checkmenu.h>
#include <gui/tabview.h>
#include <util/application.h>

#include <assert.h>

#include <fstream>
#include <memory>
#include <stdio.h>
using namespace os;
using namespace cv;
static uint nTabNum = 0;


uint EditWin::unnamedCounter=0;

cv::CodeView* cEdit = NULL;

EditWin::EditWin(App* a, const char* path) : os::Window(os::Rect(50, 50, 750, 850), "EditWin", ""), app(a), searchText(""), searchDown(true), searchCase(false), searchExtended(false), findDialog(NULL), gotoLineDialog(NULL), saveRequester(NULL), saveRequesterVisible(false)
{
    if (DEBUG_ENABLED)
    {
        char zDebug[1024];
        sprintf(zDebug,"\nIn %s::EditWin(App*, const char*)\n\tOn Line:  %d\n\tGetting ready to initialize everything.", __FUNCTION__, __LINE__);
        Debug<const char*> debug("/var/log/CodeEdit",zDebug);
    }

    std::string zTitle = "CodeEdit " + (std::string)Version();
    SetTitle(zTitle);

    if (DEBUG_ENABLED)
    {
        std::string zDebug = "\tJust set the title to:  "  + zTitle;
        Debug<std::string> debug("/var/log/CodeEdit",zDebug);
    }


    pcFavoriteMessage = new os::Message();
    AddFavorites();
    os::Rect r(50, 50, 750, 850);
    os::Desktop desktop;
    os::IPoint dp=desktop.GetResolution();
    if(dp.x<r.right+50)
        r.right=dp.x-50;
    if(dp.y<r.bottom+50)
        r.bottom=dp.y-50;
    SetFrame(r);

    r=GetBounds();

    setupMenus();
    setupToolbar();

    statusbar=new StatusBar(os::Rect(), "", 3);
    os::Point  p=statusbar->GetPreferredSize(false);
    statusbar->SetFrame(os::Rect(r.left, r.bottom-p.y, r.right, r.bottom));
    r.bottom-=p.y+1;
    r.top = 47;
    AddChild(statusbar);
    statusbar->configurePanel(0, StatusBar::CONSTANT, 50);
    statusbar->configurePanel(1, StatusBar::CONSTANT, 50);
    statusbar->configurePanel(2, StatusBar::FILL, 0);

    pcTabView = new os::TabView(r,"", CF_FOLLOW_ALL);
    pcTabView->SetMessage(new Message(ID_TAB_CHANGED));
    AddChild(pcTabView);

    openTab(path);
    updateStatus();
    formatChanged();
    Show();
    MakeFocus();
}

EditWin::~EditWin()
{
    if(saveRequesterVisible)
        saveRequester->Quit();
}

bool EditWin::load(const char* path, cv::CodeView* tEdit )
{
    if (path)
    {
        try
        {
            os::File f(path);
            off_t size=f.GetSize();
            auto_ptr<char> buf(new char[size+1]);
            f.Read(buf.get(), size);//how about the return value?
            buf.get()[size]=0;
            tEdit->SetEventMask(0);
            tEdit->Set(buf.get());
            tEdit->SetEventMask(os::TextView::EI_CONTENT_CHANGED | os::TextView::EI_CURSOR_MOVED);
            title.push_back(path);
            named.push_back(true);
            changed.push_back(false);
            return true;
        }
        catch(...)
        {
            updateStatus(string("Couldn't open file '") + path + "'!\n");
            return false;
        }
    }
    else
    {
        tEdit->SetEventMask(0);
        tEdit->SetEventMask(os::TextView::EI_CONTENT_CHANGED | os::TextView::EI_CURSOR_MOVED);
        title.push_back("Untitled");
        named.push_back(false);
        changed.push_back(false);
    }
}

void EditWin::openTab(const char* path)
{

    os::Rect  r = pcTabView->GetBounds();
    cv::CodeView* tedit=new cv::CodeView(r, "", "", os::CF_FOLLOW_ALL);
    tedit->SetTarget(this);
    tedit->SetMessage(new os::Message(ID_TEXT_INVOKED));
    tedit->SetEventMask(os::TextView::EI_CONTENT_CHANGED | os::TextView::EI_CURSOR_MOVED);

    cEdit = tedit;

    load(path, tedit);
    if (path)
    {
        pcTabView->AppendTab(path,cEdit);
        setFavorites(path);
        UpdateFavorites();
    }
    else
    {
        char buf[20];
        sprintf(buf, "<untitled %i>", ++unnamedCounter);
        pcTabView->AppendTab(buf,cEdit);
    }

    pcTabView->SetSelection(nTabNum);
    formatChanged();
    nTabNum+=1;
}

void EditWin::deleteTab()
{
    uint i = pcTabView->GetSelection();
    int nt = pcTabView->GetTabCount();
    if(nt>1)
    {
        pcTabView->DeleteTab(i);
        cEdit = (cv::CodeView*)pcTabView->GetTabView(i-1);
    }
}

void EditWin::switchTab()
{
    os::Rect  r = pcTabView->GetBounds();
    uint i = pcTabView->GetSelection();
    cEdit = (cv::CodeView*)pcTabView->GetTabView(i);
    cEdit->MakeFocus();

}



void EditWin::setupMenus()
{
    Rect r = GetBounds();
    pcFavoritesMenu =  new FavoriteMenu("ReOpen", pcFavoriteMessage,7000,true);
    menu=new os::Menu(os::Rect(),	"main menu", os::ITEMS_IN_ROW);
    m=new os::Menu(os::Rect(), "Application", os::ITEMS_IN_COLUMN);

    m->AddItem(new ImageItem("Settings...", new os::Message(ID_OPTIONS), "Ctrl + B", LoadBitmapFromResource("settings.png")));
    m->AddItem(new os::MenuSeparator());
    m->AddItem(new ImageItem("About...", new os::Message(ID_ABOUT), "Ctrl + W", LoadBitmapFromResource("about.png")));
    m->AddItem(new os::MenuSeparator());
    m->AddItem(new ImageItem("Exit", new os::Message(ID_QUIT), "Ctrl + Q", LoadBitmapFromResource("close.png")));
    menu->AddItem(m);

    m=new os::Menu(os::Rect(), "File", os::ITEMS_IN_COLUMN);
    m->AddItem(new ImageItem("Open...", new os::Message(ID_FILE_OPEN),"Ctrl + O", LoadBitmapFromResource("open.png")));
    m->AddItem(new ImageItem(pcFavoritesMenu, NULL,LoadBitmapFromResource("open.png"),5.6));
    m->AddItem(new os::MenuSeparator());
    m->AddItem(new ImageItem("Print...",new os::Message(NULL), "", LoadBitmapFromResource("print.png")));
    m->AddItem(new os::MenuSeparator());
    m->AddItem(new ImageItem("New...", new os::Message(ID_FILE_NEW), "Ctrl + N", LoadBitmapFromResource("new.png")));
    m->AddItem(new os::MenuSeparator());

    m->AddItem(new ImageItem("Save", new os::Message(ID_FILE_SAVE), "Ctrl + S", LoadBitmapFromResource("save.png")));
    m->AddItem(new ImageItem("Save As...", new os::Message(ID_FILE_SAVE_AS), "Ctrl + A", LoadBitmapFromResource("saveas.png")));
    m->AddItem(new os::MenuSeparator());

    m->AddItem(new ImageItem("Close", new os::Message(ID_FILE_CLOSE), "Ctrl + Z", LoadBitmapFromResource("close.png")));
    menu->AddItem(m);

    m=new os::Menu(os::Rect(), "Edit", os::ITEMS_IN_COLUMN);
    m->AddItem(new ImageItem("Cut", new os::Message(ID_EDIT_CUT), "Ctrl + X", LoadBitmapFromResource("cut.png")) );
    m->AddItem(new ImageItem("Copy", new os::Message(ID_EDIT_COPY), "Ctrl + C", LoadBitmapFromResource("copy.png")));
    m->AddItem(new ImageItem("Paste", new os::Message(ID_EDIT_PASTE), "Ctrl + V", LoadBitmapFromResource("paste.png")));
    m->AddItem(new os::MenuSeparator());

    m->AddItem(new ImageItem("Delete", new os::Message(ID_EDIT_DELETE), "Ctrl + D", LoadBitmapFromResource("delete.png")));
    m->AddItem(new os::MenuSeparator());

    m->AddItem(new ImageItem("Undo", new os::Message(ID_EDIT_UNDO), "Ctrl + U", LoadBitmapFromResource("undo.png")));
    m->AddItem(new ImageItem("Redo", new os::Message(ID_EDIT_REDO), "Ctrl + R", LoadBitmapFromResource("redo.png")));
    menu->AddItem(m);

    m=new os::Menu(os::Rect(), "View", os::ITEMS_IN_COLUMN);
    mTool = new os::Menu(os::Rect(), "ToolBars",os::ITEMS_IN_COLUMN);
    pcToolCheck = new os::CheckMenu("Main ToolBar", new os::Message(ID_CHECK_MAIN_TOOL),true);
    mTool->AddItem(pcToolCheck);
    m->AddItem(new ImageItem(mTool,NULL,LoadBitmapFromResource("toolbar.png"),4.3));
    m->AddItem(new os::MenuSeparator());
    m->AddItem(new ImageItem("FullScreen", NULL, "",LoadBitmapFromResource("fullscreen.png")));
    menu->AddItem(m);


    m=new os::Menu(os::Rect(), "Search", os::ITEMS_IN_COLUMN);
    mFind = new os::Menu(os::Rect(), "Find", os::ITEMS_IN_COLUMN);
    mFind->AddItem(new ImageItem("Find...", new os::Message(ID_FIND),"",LoadBitmapFromResource("find.png")));
    mFind->AddItem(new ImageItem("Find next", new os::Message(ID_FIND_NEXT), "", LoadBitmapFromResource("next.png")));
    mFind->AddItem(new ImageItem("Find previous", new os::Message(ID_FIND_PREVIOUS), "", LoadBitmapFromResource("previous.png")));
    m->AddItem(new ImageItem(mFind,NULL,LoadBitmapFromResource("find.png")));
    m->AddItem(new os::MenuSeparator());

    mReplace = new Menu(os::Rect(),"Replace", os::ITEMS_IN_COLUMN);
    mReplace->AddItem(new ImageItem("Replace", new os::Message(ID_REPLACE),"",LoadBitmapFromResource("find.png")));
    mReplace->AddItem(new ImageItem("Replace next", new os::Message(ID_REPLACE_NEXT), "",LoadBitmapFromResource("next.png")));
    mReplace->AddItem(new ImageItem("Replace all", new os::Message(ID_REPLACE_ALL),"",LoadBitmapFromResource("previous.png")));

    m->AddItem(new ImageItem(mReplace, NULL,LoadBitmapFromResource("find.png"),2));
    m->AddItem(new os::MenuSeparator());

    m->AddItem(new ImageItem("Go to line...", new os::Message(ID_GOTO_LINE), "Ctrl + E",LoadBitmapFromResource("goto.png")) );
    menu->AddItem(m);

    m = new os::Menu(os::Rect(), "Help",os::ITEMS_IN_COLUMN);
    m->AddItem(new ImageItem("About CodeFolding...", new os::Message(ID_HELP_CODE_FOLDING), "",LoadBitmapFromResource("about.png")));
    menu->AddItem(m);

    menu->SetTargetForItems(this);

    os::Point p=menu->GetPreferredSize(false);
    menu->SetFrame(os::Rect(r.left, r.top, r.right, r.top+p.y));
    r.top+=p.y+1;
    AddChild(menu);
}
void EditWin::setupToolbar()
{
	Resources res(get_image_id());

    pcToolBar = new ToolBar(os::Rect(0,20,GetBounds().Width(),46) ,"");

    pcBreaker = new os::ImageButton(os::Rect(2,3,11,19), "M_BUT_BREAKER", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
    pcBreaker->SetImage(res.GetResourceStream("breaker.png"));
    pcToolBar->AddChild(pcBreaker);

    pcNew = new os::ImageButton(os::Rect(15,3,32,22), "M_BUT_NEW", "New", new Message(M_BUT_NEW),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcNew->SetImage(res.GetResourceStream("new.png"));

    pcOpen = new os::ImageButton(os::Rect(33,2,50,22), "M_BUT_OPEN", "Open", new Message(M_BUT_OPEN),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcOpen->SetImage(res.GetResourceStream("open.png"));

    pcSave = new os::ImageButton(os::Rect(55,3,72,22), "M_BUT_SAVE", "Save", new Message(M_BUT_SAVE),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcSave->SetImage(res.GetResourceStream("save.png"));

    pcSaveAs = new os::ImageButton(os::Rect(73,2,90,22), "M_BUT_SAVE_AS", "Save As...", new Message(M_BUT_SAVE_AS),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcSaveAs->SetImage(res.GetResourceStream("saveas.png"));

    pcPrint =   new os::ImageButton(os::Rect(93,2,110,22), "M_BUT_SAVE_AS", "Save As...", new Message(M_BUT_SAVE_AS),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcPrint->SetImage(res.GetResourceStream("print.png"));
    pcPrint->SetEnable(false);

    pcBreaker = new os::ImageButton(os::Rect(112,3,121,19), "M_BUT_BREAKER", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
    pcBreaker->SetImage(res.GetResourceStream("breaker.png"));
    pcToolBar->AddChild(pcBreaker);

    pcSearch = new os::ImageButton(os::Rect(123,2,140,22), "M_BUT_FIND", "Find", new Message(M_BUT_FIND),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcSearch->SetImage(res.GetResourceStream("find.png"));

    pcUndo = new os::ImageButton(os::Rect(141,2,158,22), "M_BUT_UNDO", "undo", new Message(M_BUT_UNDO),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcUndo->SetImage(res.GetResourceStream("undo.png"));

    pcRedo = new os::ImageButton(os::Rect(159,2,176,22), "M_BUT_REDO", "undo", new Message(M_BUT_REDO),NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
    pcRedo->SetImage(res.GetResourceStream("redo.png"));

    pcToolBar->AddChild(pcNew);
    pcToolBar->AddChild(pcOpen);
    pcToolBar->AddChild(pcSave);
    pcToolBar->AddChild(pcSaveAs);
    pcToolBar->AddChild(pcPrint);
    pcToolBar->AddChild(pcSearch);
    pcToolBar->AddChild(pcUndo);
    pcToolBar->AddChild(pcRedo);
    AddChild(pcToolBar);
}

void EditWin::fontChanged(os::Font* fp)
{
    cEdit->FontChanged(fp);
}

void EditWin::appAbout()
{
    const std::string About = (const std::string)APP_NAME +  " is a programmers text editor for Syllable.\n\nCopyright(c) 2001 - 2002 Andreas Engh-Halstvedt\nCopyright(c) 2003            Syllable Desktop Team\n\nCodeEdit is released under the GNU General\nPublic License. Please go to www.gnu.org\nmore information.\n";
    os::Alert* md = new os::Alert((const std::string)APP_NAME,About,os::Alert::ALERT_INFO,0,"OK",NULL);
    md->Go(new Invoker());
}

void EditWin::appQuit()
{
    uint j = pcTabView->GetTabCount();


    for (uint i=0; i<j; i++)
    {
        PostMessage(ID_FILE_CLOSE, this);
    }

}

void EditWin::appOptions()
{
    app->PostMessage(App::ID_SHOW_OPTIONS);
}

void EditWin::fileNew()
{
    openTab();
}

void EditWin::fileNewWin()
{
    app->PostMessage(App::ID_NEW_FILE);
}

void EditWin::fileOpen(bool bOpenWin)
{
    os::Message* pcMsg = new os::Message(App::ID_OPEN_FILE);
    app->PostMessage(pcMsg, app);
}

bool EditWin::fileSave(const string &p)
{
    if(saveRequesterVisible)
    {
        saveRequester->MakeFocus();
        return false;
    }

    if(!named[pcTabView->GetSelection()] && p.empty())
    {
        fileSaveAs();
        return false;
    }
    else
    {
        string path;
        if(p.empty())
            path=title[pcTabView->GetSelection()];
        else
            path=p;

        try
        {
            ofstream out(path.c_str());

            for(uint a=0;a<cEdit->GetLineCount();++a)
            {
                out << cEdit->GetLine(a).const_str() << endl;
            }

            title[pcTabView->GetSelection()]=path;
            changed[pcTabView->GetSelection()]=false;
            named[pcTabView->GetSelection()]=true;
            updateTitle();
            formatChanged();
            MakeFocus();
            return true;
        }
        catch(...)
        {
            string s="Couldn't save to file '";
            s+=path;
            s+="'\n";
            updateStatus(s);
            return false;
        }
    }
}

void EditWin::fileSaveAs(os::Message *parent)
{
    if(saveRequesterVisible)
    {
        saveRequester->MakeFocus();
    }
    else
    {
        os::Message *m=new os::Message(ID_DO_FILE_SAVE_AS);
        if(parent)
            m->AddPointer("Parent", parent);

        saveRequester=new FileReq(os::FileRequester::SAVE_REQ,
                                  new os::Messenger(this), app->zCWD,
                                  os::FileRequester::NODE_FILE,
                                  false, m, NULL, true);

        //TODO: set path
        saveRequester->CenterInWindow(this);
        saveRequester->Show();
        saveRequesterVisible=true;
        saveRequester->MakeFocus();
    }
}

void EditWin::fileClose()
{
    uint i = pcTabView->GetSelection();

    if(changed[i])
    {
        string s="The file '";
        s+=title[pcTabView->GetSelection()];
        s+="' has changed. Do you want to save it?";

        os::Message *m=new os::Message(ID_DO_FILE_SAVE);
        m->AddPointer("Parent", new os::Message(ID_FILE_CLOSE));

        MsgDialog *md=new MsgDialog(APP_NAME, s, 0, "Yes", "No", "Cancel", NULL);
        md->center(this);
        md->addMapping("0y", 0);
        md->addMapping("0\n", 0);
        md->addMapping("0n", 1);
        md->addMapping("0c", 2);
        md->addMapping("0\e", 2);
        md->Go(new os::Invoker(m, this));
    }
    else
    {
        if (i > 0)
        {
            pcTabView->DeleteTab(pcTabView->GetSelection());
            nTabNum--;
            named.erase(named.begin()+i);
            changed.erase(changed.begin()+i);
            title.erase(title.begin()+i);
        }

        else
        {
            if (DEBUG_ENABLED)
            {
                std::string zDebug = "\nDestroying EditWin  ";
                Debug<std::string> debug("/var/log/CodeEdit",zDebug);
            }

            Application::GetInstance()->PostMessage(M_QUIT);
        }
    }
}

void EditWin::editUndo()
{
    cEdit->Undo();
}

void EditWin::UpdateFavorites()
{
    char path[1024];
    sprintf(path, "%s/Settings/CodeEdit/opened",getenv("HOME"));
    FILE* fin = fopen(path, "r");
    if (fin)
    {
        pcFavoritesMenu->ClearHistory();
        pcFavoriteMessage->MakeEmpty();
        AddFavorites();
        pcFavoritesMenu->AddItems();
        pcFavoritesMenu->SetTarget(this);
        fclose(fin);
    }

    else
    {
        pcFavoritesMenu->ClearHistory();
        pcFavoriteMessage->MakeEmpty();
        AddFavorites();
        pcFavoritesMenu->AddItems();
        pcFavoritesMenu->SetTarget(this);
    }
}

void EditWin::editRedo()
{
    cEdit->Redo();
}

void EditWin::editCut()
{
    cEdit->Cut();
}

void EditWin::editCopy()
{
    cEdit->Copy();
}

void EditWin::editPaste()
{
    cEdit->Paste();
}

void EditWin::editDelete()
{
    cEdit->Delete();
}

void EditWin::updateTitle()
{
    std::string tmp=title[pcTabView->GetSelection()];

    if(changed[pcTabView->GetSelection()])
        tmp+="*";

    pcTabView->SetTabTitle(pcTabView->GetSelection(),tmp);
}

t_List EditWin::getFavorites()
{

    t_List t_list;
    char zTemp[1024];
    sprintf(zTemp,"%s/Settings/CodeEdit/opened", getenv("HOME"));
    FILE* fin = fopen(zTemp, "r");

    if (fin)
    {
        while (!feof(fin))
        {
            fscanf(fin, "%s", zTemp);
            if (zTemp != NULL)
                t_list.push_back(zTemp);
        }
        fclose(fin);
    }


    return (t_list);
}

void EditWin::AddFavorites()
{
    t_List t_FavoriteList = getFavorites();


    for (uint32 n = 0; n < t_FavoriteList.size(); n++)
    {
        pcFavoriteMessage->AddString(t_FavoriteList[n].c_str(), t_FavoriteList[n]);
    }
}

void EditWin::setFavorites(const char* path)
{
    t_List tlist = getFavorites();
    if (path !=  NULL)
    {
        char zTemp[1024];
        sprintf(zTemp,"%s/Settings/CodeEdit/opened", getenv("HOME"));
        FILE* fin = fopen(zTemp, "w");
        if (!fin)
            ;
        else
        {

            for (uint n=0; n<tlist.size(); n++)
                fprintf(fin, "%s\n",tlist[n].c_str());
            fprintf(fin, "%s\n",	path);
            fclose(fin);
        }
    }
}

void EditWin::updateStatus(const string &s, bigtime_t timeout)
{
    char tmp[20];
    os::IPoint p=cEdit->GetCursor();

    sprintf(tmp, "Line %i", p.y+1);
    statusbar->setText(tmp, 0);

    sprintf(tmp, "Col %i", p.x);
    statusbar->setText(tmp, 1);

    statusbar->setText(s, 2, timeout);
}



void EditWin::find(const string &str, bool down, bool caseS, bool extended)
{
    if(str.empty())
    {
        updateStatus("Search string empty!");
        return;
    }

    os::RegExp reg;
    status_t status=reg.Compile(str, !caseS, extended);
    switch(status)
    {
    case os::RegExp::ERR_BADREPEAT:
        updateStatus("Error in regular expression: Bad repeat!");
        return;
    case os::RegExp::ERR_BADBACKREF:
        updateStatus("Error in regular expression: Bad back reference!");
        return;
    case os::RegExp::ERR_BADBRACE:
        updateStatus("Error in regular expression: Bad brace!");
        return;
    case os::RegExp::ERR_BADBRACK:
        updateStatus("Error in regular expression: Bad bracket!");
        return;
    case os::RegExp::ERR_BADPARENTHESIS:
        updateStatus("Error in regular expression: Bad parenthesis!");
        return;
    case os::RegExp::ERR_BADRANGE:
        updateStatus("Error in regular expression: Bad range!");
        return;
    case os::RegExp::ERR_BADSUBREG:
        updateStatus("Error in regular expression: Bad number in \\digit construct!");
        return;
    case os::RegExp::ERR_BADCHARCLASS:
        updateStatus("Error in regular expression: Bad character class name!");
        return;
    case os::RegExp::ERR_BADESCAPE:
        updateStatus("Error in regular expression: Bad escape!");
        return;
    case os::RegExp::ERR_BADPATTERN:
        updateStatus("Error in regular expression: Syntax error!");
        return;
    case os::RegExp::ERR_TOLARGE:
        updateStatus("Error in regular expression: Expression to large!");
        return;
    case os::RegExp::ERR_NOMEM:
        updateStatus("Error in regular expression: Out of memory!");
        return;
    case os::RegExp::ERR_GENERIC:
        updateStatus("Error in regular expression: Unknown error!");
        return;
    }

    os::IPoint start=cEdit->GetCursor();

    if(down)
    {
        //test current line
        if(reg.Search(cEdit->GetLine(start.y), start.x))
        {//found!
            cEdit->SetCursor(start.x+reg.GetEnd(), start.y, false, false);
            cEdit->Select(os::IPoint(start.x+reg.GetStart(), start.y), os::IPoint(start.x+reg.GetEnd(), start.y));
            Flush();
            return;
        }

        //test to end of file
        for(uint a=start.y+1;a<cEdit->GetLineCount();++a)
        {
            if(reg.Search(cEdit->GetLine(a)))
            {//found!
                cEdit->SetCursor(reg.GetEnd(), a, false, false);
                cEdit->Select(os::IPoint(reg.GetStart(), a), os::IPoint(reg.GetEnd(), a));
                Flush();
                return;
            }
        }

        //test from start of file
        for(int a=0;a<=start.y;++a)
        {
            if(reg.Search(cEdit->GetLine(a)))
            {//found!
                cEdit->SetCursor(reg.GetEnd(), a, false, false);
                cEdit->Select(os::IPoint(reg.GetStart(), a), os::IPoint(reg.GetEnd(), a));
                Flush();
                return;
            }
        }

        //not found
        updateStatus("Search string not found!");

    }
    else
    {//search up
        //this isn't nice, but emulates searching from the end of each line

        //test current line
        if(reg.Search(cEdit->GetLine(start.y), 0, start.x))
        {//found!
            int begin=reg.GetStart();
            int end=reg.GetEnd();

            while(reg.Search(cEdit->GetLine(start.y), begin+1, start.x-begin-1))
            {
                end=begin+reg.GetEnd()+1;
                begin+=reg.GetStart()+1;
            };

            cEdit->SetCursor(begin, start.y, false, false);
            cEdit->Select(os::IPoint(begin, start.y), os::IPoint(end, start.y));
            Flush();
            return;
        }

        //test to start of file
        for(int a=start.y-1;a>=0;--a)
        {
            if(reg.Search(cEdit->GetLine(a)))
            {//found!
                int begin=reg.GetStart();
                int end=reg.GetEnd();

                while(reg.Search(cEdit->GetLine(a), begin+1))
                {
                    end=begin+reg.GetEnd()+1;
                    begin+=reg.GetStart()+1;
                };

                cEdit->SetCursor(begin, a, false, false);
                cEdit->Select(os::IPoint(begin, a), os::IPoint(end, a));
                Flush();
                return;
            }
        }

        //test from end of file
        for(int a=cEdit->GetLineCount()-1;a>=start.y;--a)
        {
            if(reg.Search(cEdit->GetLine(a)))
            {//found!
                int begin=reg.GetStart();
                int end=reg.GetEnd();

                while(reg.Search(cEdit->GetLine(a), begin+1))
                {
                    end=begin+reg.GetEnd()+1;
                    begin+=reg.GetStart()+1;
                };

                cEdit->SetCursor(begin, a, false, false);
                cEdit->Select(os::IPoint(begin, a), os::IPoint(end, a));
                Flush();
                return;
            }
        }

        //not found
        updateStatus("Search string not found!");
    }
}


void EditWin::formatChanged()
{
    const vector<FormatSet> &list=app->getFormatList();

    if(list.size()==0)
        return;

    if(named[pcTabView->GetSelection()])
    {
        for(uint a=1;a<list.size();++a)
        {
            int i0=0;
            int i1=list[a].filter.find(';', 0);
            if(i1<0)
                i1=list[a].filter.size();

            while(i0<i1)
            {
                string filter=list[a].filter.substr(i0, i1-i0);

                if(title[pcTabView->GetSelection()].size()>filter.size())
                {
                    if(filter==title[pcTabView->GetSelection()].c_str()+title[pcTabView->GetSelection()].size()-filter.size())
                    {
                        //match, load format
                        cEdit->SetFormat(list[a].format);
                        cEdit->SetTabSize(list[a].tabSize);
                        cEdit->SetUseTab(list[a].useTab);
                        cEdit->SetTextColor(list[a].fgColor);
                        cEdit->SetBackground(list[a].bgColor);
                        updateStatus(string("Using format ")+list[a].name);
                        return;
                    }
                }


                i0=i1+1;
                i1=list[a].filter.find(';', i0);
                if(i1<0)
                    i1=list[a].filter.size();
            }

        }
    }

    //no match, load defaults
    cEdit->SetFormat(list[0].format);
    cEdit->SetTabSize(list[0].tabSize);
    cEdit->SetUseTab(list[0].useTab);
    cEdit->SetTextColor(list[0].fgColor);
    cEdit->SetBackground(list[0].bgColor);
    updateStatus("Using default format");
}

bool EditWin::OkToQuit()
{
    PostMessage(ID_FILE_CLOSE, this);
    return false;
}


void EditWin::DispatchMessage(os::Message *m, os::Handler *h)
{
    if(m->GetCode()==os::M_KEY_DOWN)
    {
        string str=KeyMap::encodeKey(m);
        int msg;
        if(!str.empty() && app->getKeyMap().findKey(str, &msg))
        {
            PostMessage(msg, this);
            return;
        }
    }
    Window::DispatchMessage(m, h);
}


void EditWin::HandleMessage(os::Message* msg)
{
    switch(msg->GetCode())
    {
    case ID_ABOUT:
        appAbout();
        break;
    case ID_QUIT:
        appQuit();
        break;
    case ID_OPTIONS:
        appOptions();
        break;

    case M_BUT_NEW:
    case ID_FILE_NEW:
        fileNew();
        break;

    case ID_FILE_NEW_WIN:
        fileNewWin();
        break;

    case M_BUT_OPEN:
    case ID_FILE_OPEN:
        fileOpen(false);
        break;

    case M_BUT_SAVE:
    case ID_FILE_SAVE:
        fileSave();
        break;

    case M_BUT_SAVE_AS:
    case ID_FILE_SAVE_AS:
        fileSaveAs();
        break;
    case ID_FILE_CLOSE:
        {
            fileClose();
        }
        break;
    case ID_DO_FILE_SAVE_AS:
        {
            saveRequesterVisible=false;
            saveRequester->Quit();
            saveRequester=NULL;

            os::Message *parent=NULL;
            msg->FindPointer("Parent", (void**)&parent);

            string s;
            assert(msg->FindString("file/path", &s)==0);

            fileSave(s);
            if(parent)
                PostMessage(parent, this);
            break;
        }
    case os::M_FILE_REQUESTER_CANCELED:
        saveRequesterVisible=false;
        saveRequester->Quit();
        saveRequester=NULL;

        break;
    case ID_DO_FILE_SAVE:
        {
            int32 which;
            os::Message* parent;
            assert(msg->FindInt32("which", &which)==0);
            assert(msg->FindPointer("Parent", (void**)&parent)==0);

            switch(which)
            {
            case 0://yes, do save
                if(named[pcTabView->GetSelection()])
                {
                    if(fileSave(title[pcTabView->GetSelection()]))
                    {
                        PostMessage(parent, this);
                    }
                }
                else
                    fileSaveAs(parent);
                break;
            case 1://no, don't save
                changed[pcTabView->GetSelection()]=false;
                PostMessage(parent, this);
                break;
            case 2://cancel, do nothing
                break;
            default:
                assert(false);
            }
            break;
        }
    case M_BUT_UNDO:
    case ID_EDIT_UNDO:
        editUndo();
        break;
    case M_BUT_REDO:
    case ID_EDIT_REDO:
        editRedo();
        break;

    case ID_EDIT_CUT:
        editCut();
        break;
    case ID_EDIT_COPY:
        editCopy();
        break;
    case ID_EDIT_PASTE:
        editPaste();
        break;
    case ID_EDIT_DELETE:
        editDelete();
        break;
    case ID_TEXT_INVOKED:
        {
            int32 i;
            if(msg->FindInt32("events", &i)==0)
            {
                if(!changed[pcTabView->GetSelection()] && (i&CodeView::EI_CONTENT_CHANGED))
                {
                    changed[pcTabView->GetSelection()]=true;
                    updateTitle();
                }
                if(i&CodeView::EI_CURSOR_MOVED)
                {
                    updateStatus();
                }
            }
            break;
        }
    case M_BUT_FIND:
    case ID_FIND:
        if(findDialog)
        {
            findDialog->MakeFocus();
        }
        else
        {
            //load selection
            String tmp;
            cEdit->GetRegion(&tmp);
            if(!tmp.empty())
                searchText=tmp;

            findDialog=new FindDialog(searchText, searchDown, searchCase, searchExtended);
            findDialog->center(this);
            findDialog->Go(new os::Invoker(new os::Message(ID_DO_FIND), this));
        }
        break;
    case ID_FIND_NEXT:
        if(searchText.empty())
            PostMessage(ID_FIND, this);
        else
            find(searchText, true, searchCase, searchExtended);
        break;
    case ID_FIND_PREVIOUS:
        if(searchText.empty())
            PostMessage(ID_FIND, this);
        else
            find(searchText, false, searchCase, searchExtended);
        break;
    case ID_GOTO_LINE:
        if(gotoLineDialog)
        {
            gotoLineDialog->MakeFocus();
        }
        else
        {
            gotoLineDialog=new GotoLineDialog();
            gotoLineDialog->center(this);
            gotoLineDialog->Go(new os::Invoker(new os::Message(ID_DO_GOTO_LINE), this));
        }
        break;
    case ID_DO_FIND:
        {
            bool b=false;
            if(msg->FindBool("closed", &b)==0 && b)
            {
                findDialog=NULL;
            }
            else
            {
                msg->FindString("text", &searchText);
                msg->FindBool("down", &searchDown);
                msg->FindBool("case", &searchCase);
                msg->FindBool("extended", &searchExtended);
                find(searchText, searchDown, searchCase, searchExtended);
            }
            break;
        }
    case ID_DO_GOTO_LINE:
        {
            bool b=false;
            if(msg->FindBool("closed", &b)==0 && b)
            {
                gotoLineDialog=NULL;
            }
            else
            {
                int32 which;
                assert(msg->FindInt32("which", &which)==0);
                cEdit->SetCursor(0, which-1);
                cEdit->MakeFocus();
            }
            break;
        }
    case ID_FORMAT_CHANGED:
        formatChanged();
        break;

    case ID_HELP_CODE_FOLDING:
        {
            os::Alert* pcAlert = new os::Alert("About CodeFolding","Folded pieces of code are marked (...)\nTo fold or unfold, use CTRL + SPACE.\n\n    *If the cursor is on a folded line, it will be unfolded.\n\n    *If you have something selected, the selection will be folded.\n\n    *If the cursor is on, or just above a bracket ({),\n     the code will be folded to the next matching bracket (}).\n",os::Alert::ALERT_INFO,0,"OK",NULL);
            pcAlert->CenterInWindow(this);
            pcAlert->Go(new os::Invoker());
        }
        break;

    case ID_REPLACE:

        replaceDialog=new ReplaceDialog(Rect(200,200,450,310),this);
        replaceDialog->CenterInWindow(this);
        replaceDialog->Show();
        replaceDialog->MakeFocus();
        break;

    case M_BUT_REPLACE_DO:
        {
            if(!msg->FindString("replace_text",&zReplaceText,0))
            {
                cEdit->Delete();
                cEdit->Insert(zReplaceText.c_str(),false);
                HandleMessage(new os::Message(ID_FIND_NEXT));
            }
        }
        break;

    case M_BUT_REPLACE_CLOSE:
        {
            // Reset the variables used to indicate the last positions
            // we searched from / found text at.
            //bHasFind=false;
            //nTextPosition=0;
            //nCurrentLine=0;

            // Close the Window
            replaceDialog->Close();
            //edit->MakeFocus();
        }
        break;

    case ID_RECENT_ITEM:
        {
            char pzOpen[1024];
            sprintf(pzOpen, "%s",pcFavoritesMenu->GetClicked(msg));
            os::Message* pcMsg = new Message(App::ID_DO_OPEN_FILE);
            pcMsg->AddString("file/path", pzOpen);
            app->PostMessage(pcMsg, app);
        }
        break;

    case ID_ADD_TO_FAVORITES:
        char zSet[1024];
        sprintf(zSet,"rm -rf %s/Settings/CodeEdit/opened",getenv("HOME"));
        system(zSet);
        UpdateFavorites();
        break;

    case ID_CHECK_MAIN_TOOL:
        if (!pcToolCheck->IsChecked())
        {
            RemoveChild(pcToolBar);
            pcTabView->ResizeTo(GetBounds().Width(), GetBounds().Height()-22);
        }
        else
        {
            AddChild(pcToolBar);
        }
        break;

    case ID_TAB_CHANGED:
        {
            switchTab();
        }
        break;

    case M_OPEN_FILE_IN_TAB:
        {
            const char *pzPath;

            msg->FindString( "path", &pzPath );
            openTab( pzPath );
        }
        break;

    default:
        {
            char tmp[1024];
            sprintf(tmp, "\nEditWin::Unhandled Message: %i", msg->GetCode());
            if (DEBUG_ENABLED)
            {
                Debug<std::string> debug("/var/log/CodeEdit",tmp);
            }

            if (DEBUG_ENABLED)
            {
                for(int a=0;a<msg->CountNames();++a)
                {
                    sprintf(tmp,"\t%s",msg->GetName(a).c_str());
                    Debug<const char*> debug("/var/log/CodeEdit",tmp);
                }
            }
        }

    }
}











































































