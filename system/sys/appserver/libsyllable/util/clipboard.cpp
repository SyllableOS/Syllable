
/*  libgui.so - the GUI API/appserver interface for AtheOS
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include <atheos/kernel.h>
#include <atheos/msgport.h>
#include <util/clipboard.h>
#include <appserver/protocol.h>

using namespace os;

Clipboard::Clipboard():m_cMutex( "clipboard_lock" ), m_cName( "__system_clipboard__" ), m_cBuffer( 0 )
{
	m_hReplyPort = create_port( "app_reply", DEFAULT_PORT_SIZE );
	m_hServerPort = get_app_server_port();
	m_bCleared = false;
}

Clipboard::Clipboard( const String& cName ):m_cMutex( "clipboard_lock" ), m_cName( cName ), m_cBuffer( 0 )
{
	m_hReplyPort = create_port( "app_reply", DEFAULT_PORT_SIZE );
	m_hServerPort = get_app_server_port();
	m_bCleared = false;
}

Clipboard::~Clipboard()
{
	delete_port( m_hReplyPort );
}

bool Clipboard::Lock()
{
	m_cMutex.Lock();
	m_bCleared = false;
	return ( true );
}

void Clipboard::Unlock()
{
	m_cBuffer.MakeEmpty();
	m_cMutex.Unlock();
}

void Clipboard::Clear()
{
	m_bCleared = true;
}

Message *Clipboard::GetData()
{
	if( m_bCleared == true )
	{
		m_cBuffer.MakeEmpty();
		m_bCleared = false;
	}
	else
	{
		DR_GetClipboardData_s sReq( m_cName.c_str(), m_hReplyPort );
		DR_GetClipboardDataReply_s sReply;

		if( send_msg( m_hServerPort, DR_GET_CLIPBOARD_DATA, &sReq, sizeof( sReq ) ) != 0 )
		{
			dbprintf( "Error: Clipboard::GetData() failed to send request to server: %s\n", strerror( errno ) );
			goto error;
		}
		if( get_msg( m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
		{
			dbprintf( "Error: Clipboard::GetData() failed to read reply from server: %s\n", strerror( errno ) );
			goto error;
		}
		if( sReply.m_nTotalSize > 0 )
		{
			if( m_cBuffer.Unflatten( sReply.m_anBuffer ) != 0 )
			{
				dbprintf( "Error: Clipboard::GetData() failed to unpack the message\n" );
				m_cBuffer.MakeEmpty();
				goto error;
			}
		}
		else
		{
			m_cBuffer.MakeEmpty();
			m_bCleared = false;
		}
	}
	return ( &m_cBuffer );
      error:
	return ( NULL );
}

void Clipboard::Commit()
{
	DR_SetClipboardData_s sReq;
	int nSize = m_cBuffer.GetFlattenedSize();

	strcpy( sReq.m_zName, m_cName.c_str() );
	sReq.m_hReply = m_hReplyPort;	// Just used as an source ID so the server wont interleave multiple commits
	sReq.m_nTotalSize = nSize;

	if( nSize <= CLIPBOARD_FRAGMENT_SIZE )
	{
		sReq.m_nFragmentSize = nSize;
		m_cBuffer.Flatten( sReq.m_anBuffer, nSize );
		if( send_msg( m_hServerPort, DR_SET_CLIPBOARD_DATA, &sReq, sizeof( sReq ) - CLIPBOARD_FRAGMENT_SIZE + nSize ) < 0 )
		{
			dbprintf( "Error: Clipboard::Commit() failed to send DR_SET_CLIPBOARD_DATA to server!\n" );
		}
	}
	else
	{
		uint8 *pBuffer = new uint8[nSize];

		m_cBuffer.Flatten( pBuffer, nSize );

		int nOffset = 0;

		while( nSize > 0 )
		{
			int nCurSize = min( CLIPBOARD_FRAGMENT_SIZE, nSize );

			memcpy( sReq.m_anBuffer, pBuffer + nOffset, nCurSize );
			if( send_msg( m_hServerPort, DR_SET_CLIPBOARD_DATA, &sReq, sizeof( sReq ) - CLIPBOARD_FRAGMENT_SIZE + nCurSize ) != 0 )
			{
				dbprintf( "Error: Clipboard::Commit() failed to send buffer to server: %s\n", strerror( errno ) );
				delete[]pBuffer;
				return;
			}
			nSize -= nCurSize;
			nOffset += nCurSize;
		}
		delete[]pBuffer;
	}
}
