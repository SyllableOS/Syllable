#include "mainapp.h"

ImageApp::ImageApp(char *fileName) : Application( "application/x-wnd-RGC-"APP_NAME)
{
    //settings = new Settings();
    //loadSettings();

    m_pcMainWindow = new AppWindow(CRect(400,400),sFileRequester);

    if (fileName!=NULL)
        m_pcMainWindow->Load(fileName);

    m_pcMainWindow->Show();
    m_pcMainWindow->MakeFocus();

}

ImageApp::~ImageApp()
{
    //storeSettings();
    //delete settings;
}

/*bool ImageApp::loadSettings()
{
	
	bool bFlag = false;
    if(settings->Load() != 0)
    	loadDefaults();
      
    else{
    		if(settings->FindString("dirname",&sFileRequester)==0)
    			bFlag = true;
    	}
    return bFlag;
}
 
bool ImageApp::storeSettings()
{
	settings->RemoveName("dirname");
	settings->AddString("dirname","me");
	settings->Save();
	return false;
}
 
void ImageApp::loadDefaults()
{
	sFileRequester = getenv("HOME");
}*/

