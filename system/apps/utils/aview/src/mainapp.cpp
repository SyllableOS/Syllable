#include "mainapp.h"
#include "appwindow.h"
#include "messages.h"

ImageApp::ImageApp(char *fileName) : Application( "application/x-wnd-RGC-"APP_NAME)
{
    bSaveSize = false;
    settings = new Settings();
    loadSettings();
    
    AppWindow* pcAppWindow =  new AppWindow(this,Rect(0,0,400,400),sFileRequester);
	pcAppWindow->CenterInScreen();

    if (fileName!=NULL)
        pcAppWindow->Load(fileName);

    pcAppWindow->Show();
    pcAppWindow->MakeFocus();
    pcWin = pcAppWindow;
}

bool ImageApp::OkToQuit()
{
    storeSettings();
    return true;
}

bool ImageApp::getSize()
{
    return bSaveSize;
}

String ImageApp::getFilePath()
{
    return sFileRequester;
}

ImageApp::~ImageApp()
{
}

void ImageApp::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    case M_MESSAGE_PASSED:
        pcMessage->FindBool("savesize",&bSaveSize);
        pcMessage->FindString("dirname",&sFileRequester);
        break;
    }
}

bool ImageApp::loadSettings()
{

    bool bFlag = false;
    if(settings->Load() != 0)
        loadDefaults();

    else
    {
        if(settings->FindString("dirname",&sFileRequester)!=0)
            sFileRequester = (String)getenv("HOME") + (String)"/";

        if(settings->FindBool("savesize",&bSaveSize)!=0)
            bSaveSize = false;
        else
            settings->FindRect("rectsize",&r);

        bFlag = true;
    }

    return bFlag;
}

bool ImageApp::storeSettings()
{
    settings->RemoveName("dirname");
    settings->RemoveName("savesize");
    settings->RemoveName("rectsize");
    settings->AddString("dirname",sFileRequester);
    settings->AddBool("savesize",bSaveSize);
    if (bSaveSize)
        settings->AddRect("rectsize",pcWin->GetBounds());
    settings->Save();
    return true;
}

void ImageApp::loadDefaults()
{
    sFileRequester = (String)getenv("HOME") + String("/");
    bSaveSize = false;
}










