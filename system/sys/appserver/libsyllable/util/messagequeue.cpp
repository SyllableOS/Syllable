
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <util/messagequeue.h>
#include <util/message.h>

#include <atheos/kernel.h>
#include <atheos/semaphore.h>

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MessageQueue::MessageQueue():m_cMutex( "msgqueue" )
{
	m_pcFirstMessage = NULL;
	m_pcLastMessage = NULL;
	m_nMsgCount = 0;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MessageQueue::~MessageQueue()
{
	Message *pcMsg;

	while( ( pcMsg = NextMessage() ) )
	{
		delete pcMsg;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MessageQueue::Lock( void )
{
	m_cMutex.Lock();
	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MessageQueue::Unlock( void )
{
	m_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MessageQueue::AddMessage( Message * pcMsg )
{
	if( NULL == m_pcLastMessage )
	{
		m_pcFirstMessage = pcMsg;
		m_pcLastMessage = pcMsg;
	}
	else
	{
		m_pcLastMessage->m_pcNext = pcMsg;
		m_pcLastMessage = pcMsg;
	}
	pcMsg->m_pcNext = NULL;
	m_nMsgCount++;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Message *MessageQueue::NextMessage( void )
{
	Message *pcMsg = m_pcFirstMessage;

	if( NULL != pcMsg )
	{
		m_pcFirstMessage = pcMsg->m_pcNext;
		if( NULL == m_pcFirstMessage )
		{
			m_pcLastMessage = NULL;
		}
		m_nMsgCount--;
	}
	return ( pcMsg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MessageQueue::RemoveMessage( Message * pcMsg )
{
	Message **ppcPtr;

	for( ppcPtr = &m_pcFirstMessage; NULL != *ppcPtr; ppcPtr = &( ( *ppcPtr )->m_pcNext ) )
	{
		if( *ppcPtr == pcMsg )
		{
			if( pcMsg == m_pcLastMessage )
			{
				m_pcLastMessage = *ppcPtr;
			}

			*ppcPtr = pcMsg->m_pcNext;
			m_nMsgCount--;
			delete pcMsg;

			return ( true );
		}
	}
	return ( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Message *MessageQueue::FindMessage( int nIndex ) const
{
	Message *pcMsg = m_pcFirstMessage;

	if( nIndex >= 0 )
	{
		for( pcMsg = m_pcFirstMessage; NULL != pcMsg; pcMsg = pcMsg->m_pcNext )
		{
			if( 0 == nIndex )
			{
				return ( pcMsg );
			}
			nIndex--;
		}
	}
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Message *MessageQueue::FindMessage( int nCode, int nIndex ) const
{
	Message *pcMsg = m_pcFirstMessage;

	if( nIndex >= 0 )
	{
		for( pcMsg = m_pcFirstMessage; NULL != pcMsg; pcMsg = pcMsg->m_pcNext )
		{
			if( pcMsg->GetCode() == nCode )
			{
				if( 0 == nIndex )
				{
					return ( pcMsg );
				}
				nIndex--;

				if( nIndex < 0 )
				{
					break;
				}
			}
		}
	}
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int MessageQueue::GetMessageCount( void ) const
{
	return ( m_nMsgCount );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MessageQueue::IsEmpty( void ) const
{
	return ( m_nMsgCount == 0 );
}
