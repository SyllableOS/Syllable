/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001  Kurt Skauen
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

#include <unistd.h>
#include <fcntl.h>

#include <storage/nodemonitor.h>
#include <storage/filereference.h>
#include <storage/fsnode.h>
#include <storage/directory.h>
#include <util/looper.h>
#include <util/messenger.h>

using namespace os;

class NodeMonitor::Private
{
	public:
	Private() {
		m_nMonitor = -1;
	}

    int	m_nMonitor;
};


int NodeMonitor::_CreateMonitor( const String & cPath, uint32 nFlags, const Messenger & cTarget )
{
	int nFile = open( cPath.c_str(), O_RDONLY );

	if( nFile < 0 )
	{
		return ( -1 );
	}
	int nMonitor = watch_node( nFile, cTarget.m_hPort, nFlags, ( void * )cTarget.m_hHandlerID );

	close( nFile );
	return ( nMonitor );
}

int NodeMonitor::_CreateMonitor( const Directory & cDir, const String & cPath, uint32 nFlags, const Messenger & cTarget )
{
	if( cDir.IsValid() == false )
	{
		return ( -1 );
	}
	int nFile = based_open( cDir.GetFileDescriptor(), cPath.c_str(  ), O_RDONLY, nFlags );

	if( nFile < 0 )
	{
		return ( -1 );
	}
	int nMonitor = watch_node( nFile, cTarget.m_hPort, nFlags, ( void * )cTarget.m_hHandlerID );

	close( nFile );
	return ( nMonitor );
}

int NodeMonitor::_CreateMonitor( const FileReference & cRef, uint32 nFlags, const Messenger & cTarget )
{
	if( cRef.IsValid() == false )
	{
		return ( -1 );
	}
	int nFile = based_open( cRef.GetDirectory().GetFileDescriptor(  ), cRef.GetName(  ).c_str(  ), O_RDONLY );

	if( nFile < 0 )
	{
		return ( -1 );
	}
	int nMonitor = watch_node( nFile, cTarget.m_hPort, nFlags, ( void * )cTarget.m_hHandlerID );

	close( nFile );
	return ( nMonitor );
}

int NodeMonitor::_CreateMonitor( const FSNode * pcNode, uint32 nFlags, const Messenger & cTarget )
{
	if( pcNode->IsValid() == false )
	{
		errno = EINVAL;
		return ( -1 );
	}
	int nMonitor = watch_node( pcNode->GetFileDescriptor(), cTarget.m_hPort, nFlags, ( void * )cTarget.m_hHandlerID );

	return ( nMonitor );
}



NodeMonitor::NodeMonitor()
{
	m = new Private;
}

NodeMonitor::NodeMonitor( const String & cPath, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	m = new Private;

	m->m_nMonitor = _CreateMonitor( cPath, nFlags, Messenger( pcHandler, pcLooper ) );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const String & cPath, uint32 nFlags, const Messenger & cTarget )
{
	m = new Private;

	m->m_nMonitor = _CreateMonitor( cPath, nFlags, cTarget );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const Directory & cDir, const String & cPath, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	m = new Private;

	if( cDir.IsValid() == false )
	{
		throw errno_exception( "Invalid directory", EINVAL );
	}
	m->m_nMonitor = _CreateMonitor( cDir, cPath, nFlags, Messenger( pcHandler, pcLooper ) );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const Directory & cDir, const String & cPath, uint32 nFlags, const Messenger & cTarget )
{
	m = new Private;

	if( cDir.IsValid() == false )
	{
		throw errno_exception( "Invalid directory", EINVAL );
	}
	m->m_nMonitor = _CreateMonitor( cDir, cPath, nFlags, cTarget );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const FileReference & cRef, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	m = new Private;

	if( cRef.IsValid() == false )
	{
		throw errno_exception( "Invalid directory", EINVAL );
	}
	m->m_nMonitor = _CreateMonitor( cRef, nFlags, Messenger( pcHandler, pcLooper ) );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const FileReference & cRef, uint32 nFlags, const Messenger & cTarget )
{
	m = new Private;

	if( cRef.IsValid() == false )
	{
		throw errno_exception( "Invalid directory", EINVAL );
	}
	m->m_nMonitor = _CreateMonitor( cRef, nFlags, cTarget );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const FSNode * pcNode, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	m = new Private;

	if( pcNode->IsValid() == false )
	{
		throw errno_exception( "Invalid node", EINVAL );
	}
	m->m_nMonitor = _CreateMonitor( pcNode, nFlags, Messenger( pcHandler, pcLooper ) );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::NodeMonitor( const FSNode * pcNode, uint32 nFlags, const Messenger & cTarget )
{
	m = new Private;

	if( pcNode->IsValid() == false )
	{
		throw errno_exception( "Invalid node", EINVAL );
	}
	m->m_nMonitor = _CreateMonitor( pcNode, nFlags, cTarget );
	if( m->m_nMonitor < 0 )
	{
		throw errno_exception( "Failed to create monitor" );
	}
}

NodeMonitor::~NodeMonitor()
{
	if( m->m_nMonitor >= 0 )
	{
		delete_node_monitor( m->m_nMonitor );
	}
	delete m;
}

status_t NodeMonitor::Unset()
{
	if( m->m_nMonitor >= 0 )
	{
		return ( delete_node_monitor( m->m_nMonitor ) );
	}
	else
	{
		errno = EINVAL;
		return ( -1 );
	}

}

status_t NodeMonitor::SetTo( const String & cPath, uint32 nFlags, const Messenger & cTarget )
{
	int nNewMonitor = _CreateMonitor( cPath, nFlags, cTarget );

	if( nNewMonitor < 0 )
	{
		return ( -1 );
	}
	if( m->m_nMonitor >= 0 )
	{
		delete_node_monitor( m->m_nMonitor );
	}
	m->m_nMonitor = nNewMonitor;
	return ( 0 );
}

status_t NodeMonitor::SetTo( const String & cPath, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	return ( SetTo( cPath, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

status_t NodeMonitor::SetTo( const Directory & cDir, const String & cPath, uint32 nFlags, const Messenger & cTarget )
{
	if( cDir.IsValid() == false )
	{
		return ( -1 );
	}
	int nNewMonitor = _CreateMonitor( cDir, cPath, nFlags, cTarget );

	if( nNewMonitor < 0 )
	{
		return ( -1 );
	}
	if( m->m_nMonitor >= 0 )
	{
		delete_node_monitor( m->m_nMonitor );
	}
	m->m_nMonitor = nNewMonitor;
	return ( 0 );
}

status_t NodeMonitor::SetTo( const Directory & cDir, const String & cPath, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	return ( SetTo( cDir, cPath, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

status_t NodeMonitor::SetTo( const FileReference & cRef, uint32 nFlags, const Messenger & cTarget )
{
	if( cRef.IsValid() == false )
	{
		return ( -1 );
	}
	int nNewMonitor = _CreateMonitor( cRef, nFlags, cTarget );

	if( nNewMonitor < 0 )
	{
		return ( -1 );
	}
	if( m->m_nMonitor >= 0 )
	{
		delete_node_monitor( m->m_nMonitor );
	}
	m->m_nMonitor = nNewMonitor;
	return ( 0 );
}

status_t NodeMonitor::SetTo( const FileReference & cRef, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	return ( SetTo( cRef, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

status_t NodeMonitor::SetTo( const FSNode * pcNode, uint32 nFlags, const Messenger & cTarget )
{
	if( pcNode->IsValid() == false )
	{
		return ( -1 );
	}
	int nNewMonitor = _CreateMonitor( pcNode, nFlags, cTarget );

	if( nNewMonitor < 0 )
	{
		return ( -1 );
	}
	if( m->m_nMonitor >= 0 )
	{
		delete_node_monitor( m->m_nMonitor );
	}
	m->m_nMonitor = nNewMonitor;
	return ( 0 );
}

status_t NodeMonitor::SetTo( const FSNode * pcNode, uint32 nFlags, const Handler * pcHandler, const Looper * pcLooper )
{
	return ( SetTo( pcNode, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

int NodeMonitor::GetMonitor() const
{
	return ( m->m_nMonitor );
}
