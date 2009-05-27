#ifndef __APPSETTINGS_H__
#define __APPSETTINGS_H__

#include <util/settings.h>

/* AppSettings - an os::Settings subclass that will store our saved state, eg last selected user */

class AppSettings
{
public:
	AppSettings();
	~AppSettings();
	
	bool IsAutoLoginEnabled();
	os::String GetAutoLoginUser();
	
	os::String GetActiveUser();
	status_t SetActiveUser( const os::String& zUser );
	
	os::String FindKeymapForUser( const os::String& zUserName );
	status_t SetKeymapForUser( const os::String& zUser, const os::String& zKeymap );
	
	status_t Save();

private:
	bool m_bAutoLogin;
	os::String m_zAutoLoginUser;

	os::Settings* m_pcSettings;
	os::File* m_pcSettingsFile;
};



#endif	/* __APPSETTINGS_H__ */

