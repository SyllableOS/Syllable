#include "keymap.h"

#include <util/application.h>

KeymapSelector::KeymapSelector(const os::Messenger& parent) : os::LayoutView(os::Rect(),"",NULL)
{
	m_cParent = parent;
	map = new os::Keymap();
	Layout();
}


void KeymapSelector::AttachedToWindow()
{
	m_pcMenu->SetTarget(m_cParent);
	m_pcMenu->SetSelectionMessage(new os::Message(M_SELECT));
}

void KeymapSelector::Layout()
{
	m_pcRoot = new os::HLayoutNode("keymap_root");
	m_pcRoot->AddChild(m_pcString = new os::StringView(os::Rect(),"keyamp_string","Keymap:"));
	m_pcRoot->AddChild(new os::HLayoutSpacer("",5,5));
	
	m_pcRoot->AddChild(m_pcMenu = new os::DropdownMenu(os::Rect(),""));
	
	
	os::String cName;
	os::Application::GetInstance()->GetKeyboardConfig(&cName,NULL,NULL);
	std::vector<os::String> names = map->GetKeymapNames();
	for (int i=0; i<names.size(); i++)
	{
		m_pcMenu->AppendItem(names[i]);
		
		if (cName == os::String().Format("%s%s",map->GetKeymapDirectory().c_str(),names[i].c_str()))
			m_pcMenu->SetSelection(i);
	}
	m_pcMenu->SetMinPreferredSize(8);
	m_pcMenu->SetMaxPreferredSize(8);
	m_pcMenu->SetReadOnly(true);



	SetRoot(m_pcRoot);
}

os::Point KeymapSelector::GetPreferredSize(bool) const
{
	return m_pcRoot->GetPreferredSize(false);
}







