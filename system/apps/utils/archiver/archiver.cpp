/*  Archiver 0.3.0 -:-  (C)opyright 2000-2001 Rick Caudill

   This is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



//gui headers for Archiver
#include <gui/window.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/desktop.h>
#include <gui/view.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/textview.h>
#include <gui/filerequester.h>
#include <gui/listview.h>
#include <gui/layoutview.h>
#include <storage/directory.h>
#include <storage/file.h>
#include <storage/fsnode.h>
#include <util/application.h>
#include <util/message.h>
#include <fstream.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "messages.h"
#include "buttonbar.cpp"
#include "extract.cpp"
#include "exe.cpp"
#include "options.cpp"
#include "new.cpp"
#include "crect.h"
using namespace os;




class ArcWindow : public Window
{
public:

    ArcWindow();
    ~ArcWindow();
    void ListFiles(char* cFileName);
    void LoadAtStart(char* cFileName);

private:

    void SetupMenus();
    virtual bool OkToQuit();
    virtual void HandleMessage(Message* pcMessage);


    Menu*  m_pcMenu;  //main menu
    Menu* aMenu;
    Menu* fMenu;
    Menu* actMenu;
    Menu* hMenu;
    ButtonBar* pcButtonBar;

    const char* pzFPath;
    char pzStorePath[0x900];
    //char gzip[605000];
    bool Lst;
    bool Read();
    char* tmp[8][1024],trash[2048];

    Window* m_pcExeWindow;
    Window* m_pcExtractWindow;
    Window* m_pcNewWindow;
    FileRequester* pcOpenRequest;
    Window* m_pcOptions;
    os::ListView* AListView;

    ifstream ConfigFile;
    char junk[1024];
    char lineOpen[1024];
    char zConfigFile[128];
    bool bCloseN, bCloseE;
    string zOpen, zExtract;
    

};



ArcWindow::ArcWindow() : Window(CRect(500,270),"main_wnd","Archiver")
{

    Rect cWindow = GetBounds();

    // makes sure that the listview is not blocking the menu and buttonbar
    cWindow.left = 0;
    cWindow.top = 61;

    AListView = new ListView(cWindow,"arc_listview",ListView::F_MULTI_SELECT,CF_FOLLOW_ALL);
    AListView->InsertColumn( "Name", (GetBounds().Width() /5)+55);
    AListView->InsertColumn( "Owner", (GetBounds().Width()/5)-15);
    AListView->InsertColumn( "Size", GetBounds().Width() / 5-10 );
    AListView->InsertColumn( "Date", GetBounds().Width() / 5-10);
    AListView->InsertColumn( "Perm",(GetBounds().Width() / 5)-20);
    AddChild(AListView);

    //MoveTo(dDesktop.GetResolution().x / 2 - GetBounds().Width() / 2, dDesktop.GetResolution().y / 2 - GetBounds().Height() /2 ); //moves the main window to the center of the desktop

    SetupMenus(); // where we will create the menus

    sprintf( zConfigFile, "%s/config/Archiver/Archiver.cfg", getenv("HOME") );

    Read(); //reads the config file
}

ArcWindow::~ArcWindow()
{

}

void ArcWindow::SetupMenus()

{

    Rect cMenuFrame = GetBounds();
    Rect cMainFrame = cMenuFrame;
    cMenuFrame.bottom = 15;
    m_pcMenu = new Menu(cMenuFrame,"main_menu",ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);

    aMenu = new Menu(Rect(0,0,0,0),"App",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_TOP);
    aMenu->AddItem("Settings",new Message(ID_APP_SET)) ;
    aMenu->AddItem(new MenuSeparator()  );
    aMenu->AddItem("Exit",new Message(ID_QUIT) );
    m_pcMenu->AddItem(aMenu);

    fMenu = new Menu( Rect(0,0,1,1),"File",ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP);
    fMenu->AddItem("Open", new Message(ID_OPEN) );
    fMenu->AddItem(new MenuSeparator()  );
    fMenu->AddItem("New",new Message(ID_NEW));
    m_pcMenu->AddItem(fMenu);

    actMenu = new Menu(Rect(0,0,1,4),"Actions",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_TOP);
    actMenu->AddItem("Extract",new Message(ID_EXTRACT) );
    actMenu->AddItem(new MenuSeparator());
    actMenu->AddItem("Make Self-Extracting File",new Message(ID_EXE));
    m_pcMenu->AddItem(actMenu);

    hMenu = new Menu(Rect(0,0,1,2),"Help",ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP);
    Menu* helpMenu = new Menu(Rect(0,0,1,5),"Help Topics->",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_TOP);

    helpMenu->AddItem("Keyboard",new Message(ID_HELP_KEY));
    helpMenu->AddItem("Making Self-Extracting Files",new Message(ID_HELP_EXE));

    hMenu->AddItem(helpMenu);
    hMenu->AddItem("About",new Message(ID_HELP_ABOUT) );
    m_pcMenu->AddItem(hMenu);

    cMenuFrame.bottom = m_pcMenu->GetPreferredSize( true).y +  -3.5f;  //Sizes the menu.  The higher the negative, the smaller the menu.
    cMainFrame.top = cMenuFrame.bottom + 1;
    m_pcMenu->SetFrame(cMenuFrame);
    m_pcMenu->SetTargetForItems(this);
    AddChild(m_pcMenu);

    pcButtonBar = new ButtonBar(Rect(0,0,cMenuFrame.right, BUTTON_HEIGHT),"buttonbar");
    pcButtonBar->MoveBy(0,MENU_OFFSET+3);
    AddChild(pcButtonBar);

}



void ArcWindow::LoadAtStart(char* cFileName)
{
    strcpy(pzStorePath,cFileName);
    ListFiles(cFileName);
}

bool ArcWindow::Read()
{
  	FSNode *pcNode = new FSNode();
    string pzConfigFile = (string)getenv("HOME") + (string) "/config/Archiver/Archiver.cfg";
    
    if( pcNode->SetTo(pzConfigFile.c_str()) == 0 )
    {
        File *pcConfig = new File( *pcNode );
        uint32 nSize = pcConfig->GetSize( );
        void *pBuffer = malloc( nSize );
        pcConfig->Read( pBuffer, nSize );
        Message *pcPrefs = new Message( );
        pcPrefs->Unflatten( (uint8 *)pBuffer );

        pcPrefs->FindString( "Open", &zOpen );
        pcPrefs->FindString ( "Extract",  &zExtract  );
        pcPrefs->FindBool   ( "Close_Extract",    &bCloseE   );
        pcPrefs->FindBool   ( "Close_New",   &bCloseN);
    }
}

void ArcWindow::HandleMessage(Message* pcMessage)

{

    switch (pcMessage->GetCode() )
    {

    case ID_RECENT:
        {

            Alert* RecentAlert = new Alert("Recent File List","The Recent File List function \nis not implemented yet.\n",0x00,"OK",NULL);
            RecentAlert->Go(new Invoker());
        }
        break;


    case ID_NEW:
        {
	   m_pcNewWindow = new NewWindow();
	   m_pcNewWindow->Show();
	   m_pcNewWindow->MakeFocus();
        }

        break;


    case ID_HELP_EXE:
        {
            Alert* exeAlert = new Alert("How to Create Them?","First you must have makeself installed.
                                        \nThen click on 'Actions' Menu and select 'Make Self-Extracting File'. \n\nIn the 'Make Self-Extracting File' Window:
                                        \nThe 'Archive's Name' field sets the name of the file that will be created. \nUsually ends with '.gz.sh'.  \n\nThe 'Directory to Archive' sets the directory that you want to archive. \n\nThe 'Label When Extracting' field sets what is displayed when you run the file.  \n\nUnfortuantely all files will be created in archiver's current directory. \nI will have that fixed for the next release.\n",0x00,"OK",NULL);
            exeAlert->Go(new Invoker());
        }

        break;

    case ID_EXE:
        {

            m_pcExeWindow = new ExeWindow();
            m_pcExeWindow->Show();
            m_pcExeWindow->MakeFocus();
        }

        break;


    case ID_DELETE:
        {
            Alert* delAlert = new Alert("Delete","The Delete function is not \nimplemented yet",0x00,"OK",NULL);
            delAlert->Go(new Invoker());
        }

        break;


    case ID_APP_SET:
        {

            Rect r(0,0,300,220);
            m_pcOptions = new OptionsWindow(r, zOpen,zExtract,bCloseN,bCloseE);
            m_pcOptions->Show();
            m_pcOptions->MakeFocus();
        }
        break;


    case ID_HELP_ABOUT:
        {

            Alert* aAbout = new Alert("About Archiver 0.4.0","Gui Archiver for AtheOS.\n Written by Rick Caudill.\n",0x00,"OK",NULL);
            aAbout->Go(new Invoker());
            aAbout->MakeFocus();
        }
        break;


    case ID_HELP_KEY:
        {

            Alert* aKey = new Alert("Keyboard Shortcuts","There isn't any Keyboard Shortcuts in this \nversion. They will definately be a part of \nthe next release.\n" ,0x00,"OK",NULL);
            aKey->Go(new Invoker());
        }
        break;


        //must be in this order

    case ID_EXTRACT:
        {

            if (Lst != false)
            {

                m_pcExtractWindow = new ExtractWindow(pzStorePath);
                m_pcExtractWindow->Show();
                m_pcExtractWindow->MakeFocus();
                /*Message* newMSG;
                newMSG = new Message(ID_GOING_TO_EXTRACT);
                newMSG->AddString("pzPath",pzStorePath);
                m_pcExtractWindow->PostMessage(newMSG,m_pcExtractWindow);*/
            }

            else
            {

                Alert* aFile = new Alert("File not found!","You must select a file first.",0x00,"OK",NULL);
                aFile->Go(new Invoker());
            }

        }
        break;


    case ID_OPEN:
        {
            pcOpenRequest = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),zOpen.c_str(),NODE_FILE,false,NULL,NULL,true,true,"Open","Cancel");
            pcOpenRequest->Show();
            pcOpenRequest->MakeFocus();
        }

    case M_LOAD_REQUESTED:
        {

            if (pcMessage->FindString("file/path",&pzFPath) == 0)
            {

                strcpy(pzStorePath,pzFPath); // Copies pzFPath to pzStorePath

                ListFiles((char*)pzStorePath); //opens file contents in the listview

            }
            break;

        }


    case ID_QUIT:
        {
            OkToQuit();
        }
        break;


    default:

        Window::HandleMessage(pcMessage);

        break;

    }

}


bool ArcWindow::OkToQuit(void)
{
    Application::GetInstance()->PostMessage(M_QUIT );
    return (true);
}



void ArcWindow::ListFiles(char* cFileName)

{

       // probably the crapiest implementation in the world, but it works for now :)

       if((strstr(pzStorePath,".tar")) && (!strstr(pzStorePath,".gz")) && (!strstr(pzStorePath,".bz2")) )
       {
        int j; 
        char zip_n_args[300] = "";
        FILE *f;
        strcpy(zip_n_args,"");
        strcat(zip_n_args,"tar -tvf ");
        strcat(zip_n_args,pzStorePath);
        f = NULL;
        f = popen(zip_n_args,"r");
        AListView->Clear();
        j= 0;
        while (!feof(f))  {

            fscanf(f,"%s %s %s %s %s %s",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
            ListViewStringRow *row;
            row = new ListViewStringRow();

            row->AppendString((String)(const char*)tmp[5]);  // i love casting :)
            row->AppendString((String)(const char*)tmp[1]);
            row->AppendString((String)(const char*)tmp[2]);
            row->AppendString((String)(const char*)tmp[3]);
            row->AppendString((String)(const char*)tmp[0]);
            AListView->InsertRow(row);

            j++;
        }
		
        pclose(f);
        Lst = true;
        ArcWindow::MakeFocus();
        return;
       
}
    

    if(strstr(pzStorePath,".bz2")){
       
        char bzip_n_args[300] = "";
        FILE *f;
        int j;
        strcpy(bzip_n_args,"");
        strcat(bzip_n_args,"tar -jtvf ");
        strcat(bzip_n_args,pzStorePath);
        f = NULL;
        f = popen(bzip_n_args,"r");
         AListView->Clear();
        j= 0;
        while (!feof(f))  {

            fscanf(f,"%s %s %s %s %s %s",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
            ListViewStringRow *row;
            row = new ListViewStringRow();

            row->AppendString((String)(const char*)tmp[5]);
            row->AppendString((String)(const char*)tmp[1]);
            row->AppendString((String)(const char*)tmp[2]);
            row->AppendString((String)(const char*)tmp[3]);
            row->AppendString((String)(const char*)tmp[0]);
            AListView->InsertRow(row);

            j++;
        }

        pclose(f);
        Lst = true;
        ArcWindow::MakeFocus();
        return;
}
       

 


    if(strstr(pzStorePath,".gz"))
    {

        char tgzip_n_args[300] = "";
        FILE *f;
        int  j;
        //char  trash[1024];
        //String label1,label2,label3,label4;
        //char* l;
        strcpy(tgzip_n_args,"");
        strcat(tgzip_n_args,"tar -ztvf ");
        strcat(tgzip_n_args,pzStorePath);
        f = NULL;
        f = popen(tgzip_n_args,"r");
        AListView->Clear();
        j= 0;
        while (!feof(f))  {

            fscanf(f,"%s %s %s %s %s %s",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
            ListViewStringRow *row;
            row = new ListViewStringRow();

            row->AppendString((String)(const char*)tmp[5]);
            row->AppendString((String)(const char*)tmp[1]);
            row->AppendString((String)(const char*)tmp[2]);
            row->AppendString((String)(const char*)tmp[3]);
            row->AppendString((String)(const char*)tmp[0]);
            AListView->InsertRow(row);

            j++;
        }
		AListView->RemoveRow(j-1);
        pclose(f);
        Lst = true;
        ArcWindow::MakeFocus();
        return;
    }


    if(strstr(pzStorePath,".tgz"))
    {
        char tgzip_n_args[300] = "";
        FILE *f;
        int  j;
        strcpy(tgzip_n_args,"");
        strcat(tgzip_n_args,"tar -ztvf ");
        strcat(tgzip_n_args,pzStorePath);
        f = NULL;
        f = popen(tgzip_n_args,"r");
        AListView->Clear();
        j= 0;
        while (!feof(f))  {

            fscanf(f,"%s %s %s %s %s %s",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
            ListViewStringRow *row;
            row = new ListViewStringRow();

            row->AppendString((String)(const char*)tmp[5]);
            row->AppendString((String)(const char*)tmp[1]);
            row->AppendString((String)(const char*)tmp[2]);
            row->AppendString((String)(const char*)tmp[3]);
            row->AppendString((String)(const char*)tmp[0]);
            AListView->InsertRow(row);
            j++;
        }
        pclose(f);
        Lst = true;
        ArcWindow::MakeFocus();
        return;
    }



    else{

        Alert* m_pcErrorViewAlert = new Alert("File Not Supported","The file that you selected is \nnot supported in Archiver.",0x00,"OK",NULL);
        m_pcErrorViewAlert->Go(new Invoker());
        AListView->Clear();
        Lst = false;


    }
}














