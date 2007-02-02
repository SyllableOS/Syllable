/*
 *  The Syllable application server
 *	Copyright (C) 2005 - 2007 Syllable Team
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__F_EVENT_H__
#define	__F_EVENT_H__

#include <atheos/types.h>
#include <atheos/semaphore.h>
#include <util/string.h>
#include <util/message.h>
#include <vector>

struct SrvEventMonitor_s
{
	int64 m_nProcess;
	os::String m_zID;
	int m_nTargetPort;
	uint32 m_nHandlerToken;
	int m_nMessageCode;
};

struct SrvEventNode_s;

struct SrvEvent_s
{
	SrvEventNode_s* m_psNode;
	int64 m_nProcess;
	int m_nTargetPort;
	uint32 m_nHandlerToken;
	int m_nMessageCode;
	os::String m_zDescription;
	os::Message m_cLastMessage;
};

class SrvEventNode_s
{
public:
	SrvEventNode_s( os::String zID, os::String zName, SrvEventNode_s* psParent )
	{
		m_psParent = psParent;
		m_zID = zID;
		m_zName = zName;
		m_nRefCount = 0;
	}
	~SrvEventNode_s()
	{
		if( m_psParent == NULL )
			return;
		std::vector<SrvEventNode_s*>::iterator i = m_psParent->m_cChildren.begin();
		for( ; i != m_psParent->m_cChildren.end(); i++ )
			if( (*i) == this )
			{
				i = m_psParent->m_cChildren.erase( i );
				m_psParent->Release();
				return;
			}
		
		dbprintf( "Error: Event %s not found in the child list of the parent\n", m_zID.c_str() );
	}
	
	void AddRef()
	{
		m_nRefCount++;
		//dbprintf("++Ref count of %s now %i\n", m_zID.c_str(), m_nRefCount );
	}
	void Release()
	{
		m_nRefCount--;
		//dbprintf("--Ref count of %s now %i\n", m_zID.c_str(), m_nRefCount );		
		if( m_nRefCount <= 0 )
			delete( this );
	}
	SrvEventNode_s* m_psParent;
	os::String m_zID;
	os::String m_zName;
	int m_nRefCount;
	std::vector<SrvEvent_s*> m_cEvents;	
	std::vector<SrvEventMonitor_s*> m_cMonitors;
	std::vector<SrvEventNode_s*> m_cChildren;
};

class SrvEvents
{
public:
	SrvEvents();
	SrvEvent_s* RegisterEvent( os::String zID, int64 nProcess, os::String zDescription, 
						int64 nTargetPort, int64 nToken, int64 nMessageCode );
	void UnregisterEvent( os::String zID, int64 nProcess, int64 nTargetPort );
	void GetEventInfo( os::Message* pcMessage );
	void AddMonitor( os::Message* pcMessage );
	void RemoveMonitor( os::Message* pcMessage );
	void GetLastEventMessage( os::Message* pcMessage );
	void PostEvent( SrvEvent_s* pcEvent, os::Message* pcData );
	void PostEvent( os::Message* pcMessage );
	void GetChildren( os::Message* pcMessage );
	void DispatchMessage( os::Message * pcMessage );
	void ProcessKilled( proc_id hProc );
private:
	const char* ParseEventStringForward( const char* pzString, uint& nLength );	
	SrvEventNode_s* GetNode( const SrvEventNode_s* psParent, const os::String& cName );
	SrvEventNode_s* CheckNode( SrvEventNode_s* psParent, const os::String& cName );	
	SrvEventNode_s* GetParentNode( const os::String& zID, os::String& zName, bool bCreateDirs = false );
	void RemoveFromParent( SrvEventNode_s* psEvent );	
	SrvEventNode_s* GetEvent( const os::String& zID );
	void SendEventList( const os::Messenger& cLink, const int nMessageCode, SrvEventNode_s* psNode );
	void DeleteObjectsForProcess( proc_id hProc, SrvEventNode_s* psNode );
	SrvEventNode_s* m_psRoot;
	os::Locker m_cLock;
};


#endif


















