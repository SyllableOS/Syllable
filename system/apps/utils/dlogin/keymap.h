#ifndef _KEYMAP_H
#define _KEYMAP_H

#include <util/keymap.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>
#include <util/messenger.h>
#include <util/message.h>

class KeymapSelector : public os::LayoutView
{
public:
	KeymapSelector(const os::Messenger&);
	
	void Layout();
	void AttachedToWindow();
	os::Point GetPreferredSize(bool) const;
	os::String GetKeymap()
	{
		return m_pcMenu->GetCurrentString();
	}
public:
	enum
	{
		M_SELECT = 1000000001
	};
private:
	os::HLayoutNode* m_pcRoot;
	os::DropdownMenu* m_pcMenu;
	os::StringView* m_pcString;
	os::Keymap* map;
	os::Messenger m_cParent;
};

#endif




