#include "settingswindow.h"
#include <sys/stat.h>

SettingsWindow::SettingsWindow(ImageApp* pcApp,Window* pcAppWindow) : Window(CRect(310,90), "","AView Settings")
{
    m_pcLoadRequester = NULL;
    pcApplication = pcApp;
    pcParent = pcAppWindow;
    pcOk = new ImageButton(Rect(GetBounds().Width()/2-100, GetBounds().Height()-25,GetBounds().Width()/2-40, GetBounds().Height()), "ib_1","Ok",new Message(ID_APPLY),NULL,ImageButton::IB_TEXT_RIGHT,true,true);
    pcOk->SetImageFromResource("ok.png");
    AddChild(pcOk);

    pcCancel = new ImageButton(Rect(GetBounds().Width()/2+40, GetBounds().Height()-25,GetBounds().Width()/2+115, GetBounds().Height()), "ib2","Cancel",new Message(ID_CANCEL),NULL,ImageButton::IB_TEXT_RIGHT,true,true);
    pcCancel->SetImageFromResource("cancel.png");
    AddChild(pcCancel);

    ImageButton* pcSearch = new ImageButton(Rect(GetBounds().Width()-16,3,GetBounds().Width(), 19), "ib2","",new Message(ID_SEARCH),NULL);
    pcSearch->SetImageFromResource("open.png");
    AddChild(pcSearch);

    pcOpenFieldSView = new StringView(Rect(5,5,75,25),"","Open Path:");
    AddChild(pcOpenFieldSView);

    pcOpenFieldTView = new TextView(Rect(80,5,GetBounds().Width()-21,25),"","");
    AddChild(pcOpenFieldTView);

    pcSaveSizeCBView = new CheckBox(Rect(5,30,GetBounds().Width(),60),"","Save window size on exit",NULL);
    AddChild(pcSaveSizeCBView);
    _Init();
}

void SettingsWindow::_Init()
{
    pcOpenFieldTView->Set(pcApplication->getFilePath().c_str());
    pcSaveSizeCBView->SetValue(pcApplication->getSize());
}

void SettingsWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    case ID_CANCEL:
        Close();
        break;

    case ID_SEARCH:
        if (m_pcLoadRequester==NULL)
            m_pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),sOpenTo.c_str());
        m_pcLoadRequester->CenterInWindow(this);
        m_pcLoadRequester->Show();
        m_pcLoadRequester->MakeFocus();
        break;

    case M_LOAD_REQUESTED:
        {
            std::string temp;
            pcMessage->FindString("file/path",&temp);
            temp += "/";
            struct stat my_stat;
            stat(temp.c_str(),&my_stat);
            if ( S_ISDIR( my_stat.st_mode ) == true )
            {
                pcOpenFieldTView->Set(temp.c_str());
            }

            else
            {
                Alert* pcError =  new Alert("Error","You need to select a directory and not a file.",Alert::ALERT_WARNING,0x00,"OK",NULL);
                pcError->Go(new Invoker());
                pcOpenFieldTView->Clear();
            }
        }
        break;

    case ID_APPLY:
        Message* m = new Message(M_MESSAGE_PASSED);
        bSaveSize = pcSaveSizeCBView->GetValue();
        sOpenTo = pcOpenFieldTView->GetBuffer()[0].c_str();
        m->AddBool("savesize",bSaveSize);
        m->AddString("dirname",sOpenTo);
        pcApplication->PostMessage(m,pcApplication);
        //m->RemoveName("dirname");
        //m->AddString("dirname",  pcOpenFieldTView->GetBuffer()[0].c_str());
        pcParent->PostMessage(m,pcParent);
        Close();
        break;
    }
}






