#ifndef __F_GUI_KEYEVENT_H__
#define __F_GUI_KEYEVENT_H__


#include <util/shortcutkey.h>
#include <util/string.h>

namespace os
{
class KeyboardEvent
{
public:
	KeyboardEvent()
	{
		m_cKey = os::ShortcutKey();
		m_cEventName = m_cApp = "";
	}
	
	KeyboardEvent(const os::ShortcutKey& cKey,const os::String& cEvent)
	{
		m_cEventName = cEvent;
		m_cKey = cKey;
	}

	void SetShortcutKey(const os::ShortcutKey& cKey)
	{
		m_cKey = cKey;
	}
	
	void SetEventName(const os::String& cName)
	{
		m_cEventName = cName;
	}
	
	os::String GetEventName() const
	{
		return m_cEventName;
	}
	
	os::ShortcutKey GetShortcutKey() const
	{
		return m_cKey;
	}
	
	os::String GetApplicationName() const
	{
		return m_cApp;
	}
	
protected:
	void SetApplicationName(const os::String& cApp)
	{
		m_cApp = cApp;
	}
private:
	os::ShortcutKey m_cKey;
	os::String m_cEventName;
	os::String m_cApp;
	
	friend class Application;
};
}

#endif






