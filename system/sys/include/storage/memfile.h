/*  MemFile - Virtual file class for Syllable
 *  Copyright (C) 2002 Henrik Isaksson
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

#ifndef __F_STORAGE_MEMFILE_H__
#define __F_STORAGE_MEMFILE_H__

#include <storage/seekableio.h>
#include <util/locker.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

/** Virtual stream object.
 * \ingroup storage
 * \par Description:
 *	This class simulates a SeekableIO object in memory. It may be used to
 *	capture data that would normally be sent to a file, or it may be used
 *	to supply compiled-in data, or data retrieved elsewhere to an object
 *	that expects a stream or file.
 *
 *	One possible application would be to decode data received from an
 *	HTTP link, with the BitmapImage class' Load() method.
 *
 * \sa os::StreamableIO, os::SeekableIO, os::BitmapImage
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/

class MemFile : public SeekableIO {
    public:
    MemFile();
    MemFile( const void *pData, uint32 nLen );
    ~MemFile();

      // From StreamableIO
    virtual ssize_t Read( void* pBuffer, ssize_t nSize );
    virtual ssize_t Write( const void* pBuffer, ssize_t nSize );
    
      // From SeekableIO
    virtual ssize_t ReadPos( off_t nPos, void* pBuffer, ssize_t nSize );
    virtual ssize_t WritePos( off_t nPos, const void* pBuffer, ssize_t nSize );

    virtual off_t Seek( off_t nPos, int nMode );

      // From MemFile
    virtual void SetSize( uint32 nSize );
   
    private:
    class Private;
    Private *m;
};

} // end of namespace

#endif // __F_STORAGE_MEMFILE_H__
