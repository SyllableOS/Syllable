#include "extract.h"

ExtractWindow::ExtractWindow(Window* pcParent, const String& cExtract, const String& cPath): Window(Rect(0,0,200,75),"extract_win","Extract To:", WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE | WND_NOT_RESIZABLE | WID_TRANSPARENT)
{
	pcParentWindow = pcParent;

	cFilePath = cPath;
	cExtractPath = cExtract;
	
    Rect dBounds = GetBounds();
    Rect cButFrame(0,0,50,20);
    Rect cViewFrame(0,0,140,20);


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
}

void ExtractWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode()) 
	{

    case ID_BROWSE:
        {
            m_pcBrowse = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),cExtractPath.c_str(),FileRequester::NODE_DIR,false,NULL,NULL,true,true,"Select","Cancel");
            m_pcBrowse->CenterInWindow(this);
            m_pcBrowse->Show();
            m_pcBrowse->MakeFocus();

        }

    case M_LOAD_REQUESTED:
    {
		String cGetPath;
    	if ( pcMessage->FindString("file/path",&cGetPath) == 0) 
		{
			if (cGetPath[cGetPath.size()-1] != '/')
				cGetPath+="/";	

			struct stat my_stat;
          	stat(cGetPath.c_str(),&my_stat);
			if ( S_ISDIR( my_stat.st_mode ) == true ) 
			{
                m_pcExtractView->Set(cGetPath.c_str());
                ExtractWindow::MakeFocus();
            }
        }
		break;
	}

        
      case M_FILE_REQUESTER_CANCELED:
      {
      		MakeFocus();
        	break;
	 }


    case ID_TO_EXTRACT: 
	{
		Extract();
        break;
    }

	case ID_QUIT:
    {
        Close();
        break;
	}

    default:
        Window::HandleMessage(pcMessage);
        break;
    }
}




void ExtractWindow::Extract()
{
    cBuf = m_pcExtractView->GetBuffer() [0];

    if((strstr(cFilePath.c_str(),".tar")) && (!strstr(cFilePath.c_str(),".gz")) && (!strstr(cFilePath.c_str(),".bz2")))
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
            execlp("tar","tar", "-xf",cFilePath.c_str(),"-C",cBuf.c_str(),NULL);
        }

        else
        {
			close (anPipe[2]);
        }

        sleep(1);
        PostMessage(M_QUIT);
        return;
    }


    if(strstr(cFilePath.c_str(),".bz2")){

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
            execlp("tar","tar","--use-compress-program=bunzip2 -xf",cFilePath.c_str(),"-C",cBuf.c_str(),NULL);
		}
		else
		{
			close (anPipe[2]);
        }
        sleep(1);
        PostMessage(M_QUIT);
        return;
    }


    if(strstr(cFilePath.c_str(),".gz")) {
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
            execlp("tar","tar", "-xvpzf",cFilePath.c_str(),"-C",cBuf.c_str(),NULL);
		}
		else
        {
			close (anPipe[2]);
        }

        sleep(1);
        PostMessage(M_QUIT);
        return;
    }

	if (strstr(cFilePath.c_str(),".tgz"))
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
            execlp("tar","tar", "-xvpzf",cFilePath.c_str(),"-C",cBuf.c_str(),NULL);
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
	Invoker *pcParentInvoker = new Invoker(new Message(ID_EXTRACT_CLOSE),pcParentWindow);
    pcParentInvoker->Invoke();
}











