/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#ifndef __F_STORAGE_STREAMABLEIO_H__
#define __F_STORAGE_STREAMABLEIO_H__

#include <sys/types.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

/** 
 * \ingroup storage
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class StreamableIO
{
public:
    virtual ~StreamableIO();

    virtual ssize_t Read( void* pBuffer, ssize_t nSize ) = 0;
    virtual ssize_t Write( const void* pBuffer, ssize_t nSize ) = 0;
    
private:
};

} // end of namespace

#endif // __F_STORAGE_STREAMABLEIO_H__
