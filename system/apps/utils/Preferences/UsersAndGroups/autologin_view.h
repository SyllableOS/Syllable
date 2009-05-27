#ifndef __AUTOLOGIN_VIEW_H__
#define __AUTOLOGIN_VIEW_H__

#include <gui/view.h>
#include <gui/checkbox.h>
#include <gui/dropdownmenu.h>
#include <util/settings.h>
#include <storage/file.h>

enum autologin_messages
{
	ID_AUTOLOGIN_ENABLE,
	ID_AUTOLOGIN_SEL_CHANGE
};


class AutoLoginView : public os::View
{
public:
	AutoLoginView( const os::Rect& cRect );
	~AutoLoginView();
	
	virtual void AllAttached( void );
	
	virtual void HandleMessage( os::Message* pcMessage );

	void PopulateMenu();
	
	void LoadSettings();
	status_t SaveChanges();

private:
	os::CheckBox* m_pcCheckbox;
	
	os::DropdownMenu* m_pcUserList;
	
	os::Settings* m_pcSettings;
	os::File* m_pcFile;

	bool m_bModified;
};



#endif	/* __AUTOLOGIN_VIEW_H__ */

