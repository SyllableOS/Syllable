#include <gui/window.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/desktop.h>
#include <util/message.h>
#include <fstream.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <gui/requesters.h>
#include <iostream.h>
#include <gui/filerequester.h>
#include <storage/filereference.h>
#include <sys/stat.h>
#include "crect.h"

using namespace os;


class ExtractWindow : public Window
{

public:

    ExtractWindow(const char* pzPath);
    ~ExtractWindow();
    Button* m_pcExtractButton;
    TextView* m_pcExtractView;
    Button*  m_pcBrowseButton;
    Button*  m_pcCloseButton;
    void mExtract();
    void HandleMessage(Message* pcMessage);

private:
    String cBuf;
    FileRequester* m_pcBrowse;
    const char* pzFilePath;
    const char* pzAPath;
    const char* pzDPath;
    const char* pzDestPath;
    char junk[1024];
    char lineExtract[1024];
    ifstream ConfigFile;
    char zConfigFile[128];
};



ExtractWindow::ExtractWindow(const char* pzPath): Window(CRect(200,75),"extract_win","Extract To:", WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE | WND_NOT_RESIZABLE | WID_TRANSPARENT)
{
    Rect dBounds = GetBounds();
    Rect cButFrame(0,0,50,20);
    Rect cViewFrame(0,0,140,20);

	pzFilePath = pzPath;
    sprintf( zConfigFile, "%s/config/Archiver/Archiver.cfg", getenv("HOME") );
    
	m_pcExtractButton = new Button(cButFrame,"Extract","Extract",new Message(ID_TO_EXTRACT));
    m_pcExtractView   = new TextView(cViewFrame,"ext_view",NULL);
    m_pcBrowseButton = new Button(Rect(0,0,0,0),"brows_but","Browse",new Message(ID_BROWSE));
    m_pcCloseButton = new Button(Rect(0,0,0,0),"close_but","Cancel", new Message(ID_QUIT));

    m_pcExtractButton->SetFrame(cButFrame + Point(32,50)); //was 72,50
    m_pcCloseButton->SetFrame(cButFrame + Point(100,50));
    m_pcBrowseButton->SetFrame(cButFrame + Point(147,10));
    m_pcExtractView->SetFrame(cViewFrame + Point(2,10));
    
	AddChild(m_pcExtractView);
    AddChild(m_pcBrowseButton);
    AddChild(m_pcExtractButton);
    AddChild(m_pcCloseButton);
    
	m_pcExtractView->SetTabOrder(0);
    m_pcBrowseButton->SetTabOrder(1);
    m_pcExtractButton->SetTabOrder(2);
    m_pcCloseButton->SetTabOrder(3);
    SetDefaultButton(m_pcExtractButton);
    SetFocusChild(m_pcExtractView);
    
	//Read();
}


/*bool ExtractWindow::Read()
{

   
    ConfigFile.open(zConfigFile);

    if(!ConfigFile.eof())
    {
        ConfigFile.getline(junk,1024);
        ConfigFile.getline(junk,1023);
        ConfigFile.getline(junk,1024);
        ConfigFile.getline(lineExtract,1024);
       

    }

    m_pcExtractView->Set(lineExtract);
    ConfigFile.close();

}*/

void ExtractWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode()) {

    case ID_BROWSE:
        {
            m_pcBrowse = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),getenv("$HOME"),NODE_DIR,false,NULL,NULL,true,true,"Select","Cancel");
            m_pcBrowse->Show();
            m_pcBrowse->MakeFocus();

        }

    case M_LOAD_REQUESTED:
        {
            if ( pcMessage->FindString("file/path",&pzAPath) == 0) {

                struct stat my_stat;
                stat(pzAPath,&my_stat);
                if(my_stat.st_mode & S_IFDIR) {
                m_pcExtractView->Set(pzAPath);
                ExtractWindow::MakeFocus();
            }

			  else{
            Alert* m_pcError = new Alert("Must be a directory","You must select a directory.",0x00,"OK",NULL);
            m_pcError->Go(new Invoker());
        }
}
}
        break;


        /* doesn't work yet.  Kurt is working on it
            case M_FILE_REQUESTER_CANCELED:
              {
        	 //ExtractWindow::MakeFocus();
                 printf("HELLO");
              }
        break;


    case ID_GOING_TO_EXTRACT:  {


            pcMessage->FindString("pzPath",&pzFilePath);
            strcpy(pzPath,pzExtractPath);

            break;
        }*/

    case ID_TO_EXTRACT: {

            mExtract();
        }
        break;
    case ID_QUIT:
        {
            PostMessage(M_QUIT);
            Window::MakeFocus();
        }
        break;
    default:
        Window::HandleMessage(pcMessage);
        break;
    }
}




void ExtractWindow::mExtract()
{
    cBuf = m_pcExtractView->GetBuffer() [0];

    if((strstr(pzFilePath,".tar")) && (!strstr(pzFilePath,".gz")) && (!strstr(pzFilePath,".bz2")))
    {

        int anPipe[2];
        pipe (anPipe);
        pid_t hPid = fork();
        if (hPid==0)

        {

            close(0);
            close(1);
            close(2);
            dup2(anPipe[2],1);
            dup2(anPipe[2],2);
            close (anPipe[0] );
            execlp("tar","tar", "-xf",pzFilePath,"-C",cBuf.c_str(),NULL);


        }

        else

        {

            close (anPipe[2]);
        }

        sleep(1);
        PostMessage(M_QUIT);
        return;
    }


    if(strstr(pzFilePath,".bz2")){

        int anPipe[2];
        pipe (anPipe);
        pid_t hPid = fork();
        if (hPid==0)

        {

            close(0);
            close(1);
            close(2);
            dup2(anPipe[2],1);
            dup2(anPipe[2],2);
            close (anPipe[0] );
            execlp("tar","tar", "-xvpjf",pzFilePath,"-C",cBuf.c_str(),NULL);


        }

        else

        {

            close (anPipe[2]);
        }
        sleep(1);
        PostMessage(M_QUIT);
        return;
    }


    if(strstr(pzFilePath,".gz")) {
        int anPipe[2];
        pipe (anPipe);
        pid_t hPid = fork();
        if (hPid==0)

        {

            close(0);
            close(1);
            close(2);
            dup2(anPipe[2],1);
            dup2(anPipe[2],2);
            close (anPipe[0] );
            execlp("tar","tar", "-xvpzf",pzFilePath,"-C",cBuf.c_str(),NULL);


        }

        else

        {

            close (anPipe[2]);
        }

        sleep(1);
        PostMessage(M_QUIT);
        return;
    }


    if (strstr(pzFilePath,".tgz"))

    {

        int anPipe[2];
        pipe (anPipe);
        pid_t hPid = fork();
        if (hPid==0)

        {

            close(0);
            close(1);
            close(2);
            dup2(anPipe[2],1);
            dup2(anPipe[2],2);
            close (anPipe[0] );
            execlp("tar","tar", "-xvpzf",pzFilePath,"-C",cBuf.c_str(),NULL);


        }

        else

        {

            close (anPipe[2]);
        }
        sleep(1);
        PostMessage(M_QUIT);
        return;
    }


    else{
        Alert* m_pcExtractErrorAlert = new Alert("You cannot extract this file!","This file is not supported \nby Archiver.",0x00,"OK",NULL);
        m_pcExtractErrorAlert->Go(new Invoker());
    }

}




ExtractWindow::~ExtractWindow()
{


}
