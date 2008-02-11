#include "common.h"


os::String ShortcutKeyToString(const os::ShortcutKey& key)
{
	os::String cFile, cReturn = "";
	uint32 qual;
	keymap map;
	
	os::Application::GetInstance()->GetKeyboardConfig(&cFile,NULL,NULL);

	FILE* fin = fopen(os::String().Format("/system/keymaps/%s",cFile.c_str()).c_str(),"r");
	fread(&map,sizeof(map),1,fin);
	fclose(fin);

	qual = key.GetQualifiers();
		

	if (qual & os::QUAL_CTRL)
	{
		cReturn += "Ctrl+";
	}
	if (qual & os::QUAL_ALT)
	{
		cReturn += "Alt+";
	}
	if (qual & os::QUAL_SHIFT)
	{
		cReturn += "Shift+";
	}
	
	
	cReturn += (char)map.m_anMap[key.GetKeyCode()+0x01][0];
	return cReturn;
}



