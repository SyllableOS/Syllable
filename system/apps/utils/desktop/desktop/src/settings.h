#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "include.h"
#include <util/message.h>
#include <gui/gfxtypes.h>

using namespace os;
using namespace std;

class DeskSettings
{
public:
    DeskSettings();
    void SaveSettings(Message* pcMessage);
    void ReadSettings();
    void ResetSettings();
    Message* GetSettings();
    const char* GetIconDir();
    const char* GetExtDir();
    const char* GetSetDir();
    const char* GetConfigFile();
    const char* GetImageDir();
    bool GetVersion();
    Color32_s GetBgColor();
    Color32_s GetFgColor();
    int32 GetImageSize();
    bool GetTrans();
    string GetImageName();

private:
    string zDeskImage;
    bool   bShowVer;
    bool   bAlpha;
    bool   bTrans;
    Color32_s cBgColor;
    Color32_s cFgColor;
    Message* pcReturnMessage;
    int32 nSizeImage;
    char pzConfigFile[1024];
    char pzConfigDir[1024];
    char pzImageDir[1024];
    char pzIconDir[1024];
    char pzExtDir[1024];
    void DefaultSettings();


};

#endif











