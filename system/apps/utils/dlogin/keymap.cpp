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
	m_pcRoot->AddChild(m_pcString = new os::StringView(os::Rect(),"keymap_string","Keymap:"));
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
	
	os::View::SetTabOrder( os::NO_TAB_ORDER );
}

os::Point KeymapSelector::GetPreferredSize(bool) const
{
	return m_pcRoot->GetPreferredSize(false);
}

void KeymapSelector::SetTabOrder( int nOrder )
{
	m_pcMenu->SetTabOrder( nOrder );
}

void KeymapSelector::Activated( bool bIsActive )
{
	if( bIsActive ) m_pcMenu->MakeFocus( true );
}


void KeymapSelector::SelectKeymap( const os::String& zKeymap )
{
	int nCount = m_pcMenu->GetItemCount();
	bool bFound = false;
	
	for( int i = 0; i < nCount; i++ )
	{
		if( m_pcMenu->GetItem( i ) == zKeymap )
		{
			m_pcMenu->SetSelection( i, true );	/* bNotify == true, so the DropdownMenu will notify the window and the keymap will be activated as usual */
			bFound = true;
			break;
		}
	}
	/* Didn't find it - well, we'll just stick with the current keymap then. */
}

