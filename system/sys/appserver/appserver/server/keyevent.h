#ifndef __KEYEVEMT_H__
#define __KEYEVEMT_H__

#include <util/string.h>
#include <util/shortcutkey.h>
#include "event.h"

struct KeyEvent_s
{
	os::ShortcutKey		cKey;
	os::String			cApplication;
	os::String			cEventName;
	SrvEvent_s*			pcEvent; 	
};

#endif	


