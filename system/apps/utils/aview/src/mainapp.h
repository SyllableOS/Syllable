#ifndef MAINAPP_H
#define MAINAPP_H

#include <util/application.h>
#include <util/settings.h>
#include "crect.h"
#include "appwindow.h"

#define APP_NAME "AView"
using namespace os;

class ImageApp : public Application
{
public:
    ImageApp(char *fileName);
    ~ImageApp();

private:
    AppWindow* m_pcMainWindow;
    Settings* settings;
    bool loadSettings();
    void loadDefaults();
    bool storeSettings();
    std::string sFileRequester;
};

#endif



