#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "include.h"
#include <util/message.h>
#include <gui/gfxtypes.h>

using namespace os;

class DeskSettings
{
public:
	DeskSettings();
	void SaveSettings(Message* pcMessage);
	void ReadSettings();
	Message* GetSettings();
	const char* GetIconDir();
	const char* GetSetDir();
	const char* GetConfigFile();
	const char* GetImageDir();
	Color32_s GetBgColor();
	Color32_s GetFgColor();
	int32 GetImageSize();
	
private:
	string zDeskImage;
	bool   bShowVer;
	bool   bAlpha;
	Color32_s cBgColor;
	Color32_s cFgColor;
	Message* pcReturnMessage;
	int32 nSizeImage;
	char pzConfigFile[1024];
	char pzConfigDir[1024];
	char pzImageDir[1024];
	char pzIconDir[1024];
	
		
};
	
#endif




