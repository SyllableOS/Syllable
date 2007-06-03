/*  Registrar
 *  Copyright (C) 2004 Arno Klenke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */
 
#ifndef _REGISTRAR_H_
#define _REGISTRAR_H_
 
#include <gui/window.h>
#include <gui/tabview.h>
#include <gui/stringview.h>
#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>
#include <util/looper.h>
#include <util/string.h>
#include <util/settings.h>
#include <util/resources.h>
#include <util/event.h>
#include <atheos/msgport.h>
#include <atheos/threads.h>
#include <atheos/image.h>
#include <atheos/areas.h>
#include <atheos/time.h>
#include <storage/directory.h>
#include <storage/file.h>
#include <storage/symlink.h>
#include <storage/registrar.h>
#include <storage/nodemonitor.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cassert>

namespace os
{
	
struct FileType
{
	os::String m_zMimeType;
	os::String m_zIdentifier;
	os::String m_zIcon;
	std::vector<os::String> m_cExtensions;
	std::vector<os::String> m_cHandlers;
	os::String m_zDefaultHandler;
};

struct RegistrarClient
{
	port_id m_hPort;
	int64 m_nProcess;
};

struct RegistrarApp
{
	os::String zPath;
	os::String zName;
	os::String zCategory;
};

	
struct RegistrarUser
{
	os::String m_zUser;
	std::vector<RegistrarClient> m_cClients;
	std::vector<FileType> m_cTypes;
	std::vector<RegistrarApp> m_cApps;
	bool m_bAppListValid;
};

class RegistrarMonitor : public os::NodeMonitor
{
public:
	RegistrarMonitor( const os::String& cPath, uint32 nFlags, const os::Handler* pcHandler )
	: os::NodeMonitor( cPath, nFlags, pcHandler )
	{
		m_cPath = cPath;
	}
	os::NodeMonitor* m_pcMonitor;
	os::String m_cPath;
};

class Registrar : public Application
{
public:
	Registrar();
	~Registrar();
	
	bool OkToQuit();
	
	void HandleMessage( Message* pcMessage );
	thread_id Start();
	
	
private:
	void SaveDatabase( os::String zUser );
	void LoadTypes( os::String zUser, port_id hPort, int64 nProcess );
	RegistrarUser* GetUser( const os::String& zUser );
	FileType* GetType( const os::String& zUser, const os::String& zMimeType );
	void Login( Message* pcMessage );
	void Logout( Message* pcMessage );
	void GetTypeCount( Message* pcMessage );
	void GetType( Message* pcMessage );
	void RegisterType( Message* pcMessage );
	void UnregisterType( Message* pcMessage );
	void RegisterTypeIcon( Message* pcMessage );
	void RegisterTypeExtension( Message* pcMessage );
	void ClearTypeExtensions( Message* pcMessage );
	void RegisterTypeHandler( Message* pcMessage );
	void ClearTypeHandlers( Message* pcMessage );
	void SetDefaultHandler( Message* pcMessage );
	os::String GetAttribute( os::FSNode* pcNode, os::String zAttribute );
	void GetTypeAndIcon( Message* pcMessage );
	void GetAppList( Message* pcMessage );
	void ScanAppPath( RegistrarUser* psUser, int nLevel, os::Path cPath, os::String cPrimaryLanguage );
	void UpdateAppList( RegistrarUser* psUser, bool bForce );
	void ProcessKilled( Message* pcMessage );
	
	std::vector<RegistrarUser> m_cUsers;
	std::vector<RegistrarMonitor*> m_cMonitors;
	os::Event* m_pcAppListEvent;
};
	

};

#endif



