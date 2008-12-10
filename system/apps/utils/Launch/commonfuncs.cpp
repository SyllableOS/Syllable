#include "commonfuncs.h"
#include "application.h"

#include <storage/registrar.h>

bool IsDirectory(const String& cDir)
{
	try
	{
		FSNode cFile(cDir);
		if (cFile.IsValid() && cFile.IsDir())
			return true;
	}
	catch (...)
	{
	}
	return false;
}

bool IsWebsite(const String& cWebsite)
{
	if (cWebsite.substr(0,7) == "http://" || cWebsite.substr(0,4) == "www.")
	{
		return true;
	}
	return false;
}

bool IsExecutable(const String& cString)
{
	struct stat sStat;
	
	try
	{
		File cFile(cString);
		
		if (cFile.IsValid())
		{
			cFile.GetStat(&sStat);
			
			if (sStat.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH))
			{
				return true;
			} 
		}
	}
	catch(...)
	{
	}
	return false;
}

bool Launch(const String& cFile)
{
	try
	{
		RunApplication* pcApp = (RunApplication*)RunApplication::GetInstance();
		RegistrarManager* pcManager = RegistrarManager::Get();
		
		status_t nStatus = pcManager->Launch(pcApp->GetWindow(),cFile);
		
		if (nStatus >=0)
			return true;
	}
	catch(...)
	{
	}
	return false;
}

bool LaunchFile(const String& cFile)
{	
	Execute cExecute;
	
	if (IsDirectory(cFile))
	{
		cExecute.SetCommand("/system/bin/FileBrowser");
		cExecute.SetArguments(cFile);
		cExecute.Run();
		return true;
	}

	else if (IsWebsite(cFile))
	{
		cExecute.SetCommand("/Applications/Webster/Webster");
		cExecute.SetArguments(cFile);
		cExecute.Run();
		return true;
	}

	
	else if (IsExecutable(cFile))
	{
		cExecute.SetCommand(cFile);
		cExecute.Run();
		return true;
	}

	else if(IsExecutable( String("/bin/") + cFile.c_str())) 
	{
		cExecute.SetCommand(String("/bin/") + cFile.c_str());
		cExecute.Run();
		return true;
	}
	
	else if (IsExecutable( String("/usr/bin/") + cFile.c_str()))
	{
		cExecute.SetCommand(String("/usr/bin/") + cFile.c_str());
		cExecute.Run();
		return true;		
	}
	else
	{
		os::Path cPath = os::Path(cFile);
		return Launch(cPath);
	}
}


