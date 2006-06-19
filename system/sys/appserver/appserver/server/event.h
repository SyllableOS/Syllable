/*
 *  The Syllable application server
 *	Copyright (C) 2005 - Syllable Team
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

struct SrvEvent_s
{
	int64 m_nProcess;
	os::String m_zID;
	int m_nTargetPort;
	uint32 m_nHandlerToken;
	int m_nMessageCode;
	os::String m_zDescription;
	os::Message m_cLastMessage;
	std::vector<SrvEventMonitor_s*> m_cMonitors;
};

class SrvEvents
{
public:
	SrvEvent_s* RegisterEvent( os::String zID, int64 nProcess, os::String zDescription, 
						int64 nTargetPort, int64 nToken, int64 nMessageCode );
	void UnregisterEvent( os::String zID, int64 nProcess, int64 nTargetPort );
	void GetEventInfo( os::Message* pcMessage );
	void AddMonitor( os::Message* pcMessage );
	void RemoveMonitor( os::Message* pcMessage );
	void GetLastEventMessage( os::Message* pcMessage );
	void PostEvent( SrvEvent_s* pcEvent, os::Message* pcData );
	void PostEvent( os::Message* pcMessage );
	void DispatchMessage( os::Message * pcMessage );
	void ProcessKilled( proc_id hProc );
private:
	std::vector<SrvEvent_s*> m_cEvents;
	std::vector<SrvEventMonitor_s*> m_cMonitors;
};


#endif


















