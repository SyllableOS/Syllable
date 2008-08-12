#include <util/keymap.h>

using namespace os;

Keymap::Keymap(const os::String& cKeymap)
{
	_Init(cKeymap);
}

Keymap::~Keymap()
{
}

os::String Keymap::GetKeymapDirectory()
{
	return "/system/keymaps/";
}

std::vector<os::String> Keymap::GetKeymapNames()
{
	std::vector<os::String> names;
	os::String entry;
	os::Directory cDir(GetKeymapDirectory());
	
	while (cDir.GetNextEntry(&entry) != 0)
	{
		if (entry[0] != '.')
		{
			names.push_back(entry);
		}
	}
	
	return names;
}

void Keymap::_Init(const os::String& cKeymap)
{
	os::String cPath;
	m_pcMap = NULL;
	
	if (cKeymap == "")
	{
		os::Application::GetInstance()->GetKeyboardConfig(&cPath,NULL,NULL);
	}
	else
	{
		cPath = GetKeymapDirectory() + cKeymap;
	}
	
	FILE* fin = fopen(cPath.c_str(),"r");
	
	if (fin != NULL)
	{
		keymap_header sHeader;

		if( fread( &sHeader, sizeof( sHeader ), 1, fin ) != 1 )
		{
			throw GeneralFailure("Error: Failed to read keymap header\n",-1 );
		}
		if( sHeader.m_nMagic != KEYMAP_MAGIC )
		{
			throw GeneralFailure( os::String().Format("Error: Keymap have bad magic number (%08x) should have been %08x\n", sHeader.m_nMagic, KEYMAP_MAGIC).c_str(),-1);
		}
		
		if( sHeader.m_nVersion != CURRENT_KEYMAP_VERSION )
		{
			throw GeneralFailure( os::String().Format("Error: Unknown keymap version %d\n", sHeader.m_nVersion).c_str(),-1);
		}

		m_pcMap = ( keymap* ) malloc( sHeader.m_nSize );

		if( !m_pcMap )
		{
			throw GeneralFailure( os::String().Format("Error: Could not allocate memory for keymap (Size: %d)\n", sHeader.m_nSize).c_str(),-1);
		}

		if( fread( m_pcMap, sHeader.m_nSize, 1, fin ) != 1 )
		{
			throw GeneralFailure("Error: Failed to read keymap\n",-1 );
		}
	}
	else
	{
		throw GeneralFailure("Error: Invalid file",-1);
	}
}

int32_t Keymap::Find(char zFind)
{
	int32_t find=-1;
	
	for (uint32_t i=0; i<128; i++)
	{
		if (m_pcMap->m_anMap[i][0] == zFind)
		{
			find = i;
			break;
		}
	}
	return find;
}

std::vector< std::pair<int32_t, int32_t> > Keymap::FindAllOccurances(char zFind)
{
	std::vector< std::pair<int32_t, int32_t> > map;
	
	for (uint32_t i=0; i<128; i++)
	{
		for (uint32_t j=0; j<9; j++)
		{
			if (m_pcMap->m_anMap[i][j] == zFind)
			{
				map.push_back(std::make_pair(i,j));
			}
		}
	}
	return map;
}

std::vector<int32_t> Keymap::GetKeys()
{
	std::vector<int32_t> cKeys;
	
	for (uint32_t i=0; i<128; i++)
	{
		cKeys.push_back(m_pcMap->m_anMap[i][0]);
	}
	return cKeys;
}

keymap* Keymap::GetKeymapData() const
{
	return m_pcMap;
}



