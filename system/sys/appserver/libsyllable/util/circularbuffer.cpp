
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

#include <util/circularbuffer.h>
#include <errno.h>
#include <memory.h>
#include <new>

using namespace os;

CircularBuffer::CircularBuffer( int nMaxReserved, int nBlockSize )
{
	m_nBlockSize = nBlockSize;
	m_nMaxReserved = nMaxReserved;
	m_nNumReserved = 0;
	m_nTotalSize = 0;
	m_psFirstBlock = NULL;
	m_psLastBlock = NULL;
	m_psFirstReserved = NULL;
}

CircularBuffer::~CircularBuffer()
{
	Clear();
}

void CircularBuffer::Clear()
{
	while( m_psFirstBlock != NULL )
	{
		Block *psBlock = m_psFirstBlock;

		m_psFirstBlock = psBlock->m_psNext;
		delete[]reinterpret_cast < uint8 *>( psBlock );
	}
	while( m_psFirstReserved != NULL )
	{
		Block *psBlock = m_psFirstReserved;

		m_psFirstReserved = psBlock->m_psNext;
		delete[]reinterpret_cast < uint8 *>( psBlock );
	}
	m_nNumReserved = 0;
	m_nTotalSize = 0;
}

ssize_t CircularBuffer::Size() const
{
	return ( m_nTotalSize );
}

CircularBuffer::Block * CircularBuffer::_AllocBlock()
{
	Block *psBlock;

	if( m_psFirstReserved != NULL )
	{
		psBlock = m_psFirstReserved;
		m_psFirstReserved = psBlock->m_psNext;
		m_nNumReserved--;
	}
	else
	{
		try
		{
			psBlock = reinterpret_cast < Block * >( new uint8[sizeof( Block ) + m_nBlockSize] );
		}
		catch( std::bad_alloc & )
		{
			errno = ENOMEM;
			return ( NULL );
		}
	}
	psBlock->m_nStart = 0;
	psBlock->m_nEnd = 0;
	psBlock->m_psNext = NULL;
	if( m_psFirstBlock == NULL )
	{
		m_psFirstBlock = psBlock;
		m_psLastBlock = psBlock;
	}
	else
	{
		m_psLastBlock->m_psNext = psBlock;
		m_psLastBlock = psBlock;
	}
	return ( psBlock );
}


status_t CircularBuffer::Write( const void *pBuffer, int nSize )
{
	while( nSize > 0 )
	{
		Block *psBlock;

		if( m_psLastBlock != NULL && m_psLastBlock->m_nEnd < m_nBlockSize )
		{
			psBlock = m_psLastBlock;
		}
		else
		{
			psBlock = _AllocBlock();
			if( psBlock == NULL )
			{
				errno = ENOMEM;
				return ( -1 );
			}
		}
		int nCurLen = m_nBlockSize - psBlock->m_nEnd;

		if( nCurLen > nSize )
		{
			nCurLen = nSize;
		}
		memcpy( reinterpret_cast < uint8 *>( psBlock + 1 ) + psBlock->m_nEnd, pBuffer, nCurLen );

		psBlock->m_nEnd += nCurLen;
		nSize -= nCurLen;
		m_nTotalSize += nCurLen;
		pBuffer = reinterpret_cast < const uint8 *>( pBuffer ) + nCurLen;
	}
	return ( 0 );
}

ssize_t CircularBuffer::Read( void *pBuffer, int nSize )
{
	int nBytesRead = 0;

	while( nSize > 0 && m_psFirstBlock != NULL )
	{
		Block *psBlock = m_psFirstBlock;
		int nCurLen = psBlock->m_nEnd - psBlock->m_nStart;

		if( nCurLen > nSize )
		{
			nCurLen = nSize;
			memcpy( pBuffer, reinterpret_cast < uint8 *>( psBlock + 1 ) + psBlock->m_nStart, nCurLen );

			psBlock->m_nStart += nCurLen;
		}
		else
		{
			memcpy( pBuffer, reinterpret_cast < uint8 *>( psBlock + 1 ) + psBlock->m_nStart, nCurLen );

			m_psFirstBlock = psBlock->m_psNext;
			if( m_nNumReserved < m_nMaxReserved )
			{
				psBlock->m_psNext = m_psFirstReserved;
				m_psFirstReserved = psBlock;
				m_nNumReserved++;
			}
			else
			{
				delete[]reinterpret_cast < uint8 *>( psBlock );
			}
			if( m_psFirstBlock == NULL )
			{
				m_psLastBlock = NULL;
			}
		}
		nSize -= nCurLen;
		m_nTotalSize -= nCurLen;
		nBytesRead += nCurLen;
		pBuffer = reinterpret_cast < uint8 *>( pBuffer ) + nCurLen;
	}
	return ( nBytesRead );
}

