#ifndef SETTINGS_H
#define SETTINGS_H

#include <util/settings.h>
#include <util/string.h>
#include <gui/rect.h>

using namespace os;

class AppSettings : public os::Settings
{
public:
	AppSettings();
	
	bool 	GetCloseNewWindow();
	void 	SetCloseNewWindow(bool);

	bool 	GetCloseExtractWindow();
	void 	SetCloseExtractWindow(bool);
	
	String 	GetOpenPath();
	void   	SetOpenPath(const String&);

	String 	GetExtractPath();
	void	SetExtractPath(const String&);

	Rect	GetLastRect();
	void	SetLastRect(const Rect&);

	void	SaveSettings();
private:
	void 	_Init();

	bool 	bCloseNewWindow;
	bool 	bCloseExtractWindow;
	
	String 	cOpenPath;
	String 	cExtractPath;

	Rect 	cRect;
};

#endif
