/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001 Kurt Skauen
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

#ifndef __F_UTIL_CIRCULARBUFFER_H__
#define __F_UTIL_CIRCULARBUFFER_H__

#include <sys/types.h>
#include <atheos/types.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Circular (FIFO) data container.
 * \ingroup util
 * \par Description:
 *
 * \since 0.3.7
 * \sa os::Window, os::View, os::Looper
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class CircularBuffer
{
public:
    CircularBuffer( int nMaxReserved = 3, int nBlockSize = 8192 );
    ~CircularBuffer();
    status_t Write( const void* pBuffer, int nSize );
    ssize_t  Read( void* pBuffer, int nSize );
    void     Clear();
    ssize_t  Size() const;
private:
    struct Block
    {
	Block* m_psNext;
	int    m_nStart;
	int    m_nEnd;
    };

    Block* _AllocBlock();
    
    int	   m_nBlockSize;
    int	   m_nMaxReserved;
    int    m_nNumReserved;
    Block* m_psFirstBlock;
    Block* m_psLastBlock;
    Block* m_psFirstReserved;
    int	   m_nTotalSize;
};

} // end of namespace

#endif // __F_UTIL_CIRCULARBUFFER_H__
