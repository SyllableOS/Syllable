#include "settings.h"
using namespace os;

AppSettings::AppSettings() : Settings()
{
	_Init();
}

void AppSettings::_Init()
{
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Archiver";
	mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	zPath += "/Settings";	
	File* pcFile = new File(zPath,O_RDWR | O_CREAT);	
	SetFile(pcFile);
	Load();

	if (FindBool("new_window",&bCloseNewWindow) != 0)
		bCloseNewWindow = true;

	if (FindBool("extract_window",&bCloseExtractWindow) != 0)
		bCloseExtractWindow = true;

	if (FindString("open_path",&cOpenPath) != 0)
		cOpenPath = String(getenv( "HOME")) + String ("/");

	if (FindString("open_path",&cExtractPath) != 0)
		cExtractPath = String(getenv( "HOME")) + String ("/");

	if (FindRect("position",&cRect) != 0)
		cRect = Rect(-1,-1,-1,-1);
}

void AppSettings::SaveSettings()
{
	AddBool("extract_window",&bCloseExtractWindow);
	AddBool("new_window",&bCloseNewWindow);
	AddString("open_path",cOpenPath);
	AddString("extract_path",cExtractPath);
	SetRect("position",cRect);
	Save();
}

Rect AppSettings::GetLastRect()
{
	return cRect;
}

void AppSettings::SetLastRect(const Rect& r)
{
	cRect = r;
}

String AppSettings::GetExtractPath()
{
	return cExtractPath;
}

void AppSettings::SetExtractPath(const String& cPath)
{
	cExtractPath = cPath;
}

String AppSettings::GetOpenPath()
{
	return cOpenPath;
}

void AppSettings::SetOpenPath(const String& cPath)
{
	cOpenPath = cPath;
}

bool AppSettings::GetCloseNewWindow()
{
	return bCloseNewWindow;
}

void AppSettings::SetCloseNewWindow(bool bClose)
{
	bCloseNewWindow = bClose;
}

bool AppSettings::GetCloseExtractWindow()
{
	return bCloseExtractWindow;
}

void AppSettings::SetCloseExtractWindow(bool bClose)
{
	bCloseExtractWindow = bClose;
}




