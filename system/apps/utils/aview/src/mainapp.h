#ifndef MAINAPP_H
#define MAINAPP_H

#include <util/application.h>
#include <util/settings.h>
#include "crect.h"


#define APP_NAME "AView"
using namespace os;

class ImageApp : public Application
{
public:
    ImageApp(char *fileName);
    ~ImageApp();
    virtual void HandleMessage(Message*);
    virtual bool OkToQuit();
    bool getSize();
    std::string getFilePath();
private:
    Window* m_pcMainWindow;
    Settings* settings;
    bool loadSettings();
    void loadDefaults();
    bool storeSettings();
    std::string sFileRequester;
    bool bSaveSize;
    Rect r;
    Window* pcWin;
};

#endif









