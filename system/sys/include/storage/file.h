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

#ifndef __F_STORAGE_FILE_H__
#define __F_STORAGE_FILE_H__

#include <storage/seekableio.h>
#include <storage/fsnode.h>
#include <util/locker.h>

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

class File : public SeekableIO, public FSNode
{
public:
    enum { DEFAULT_BUFFER_SIZE=32768 };
    File();
    File( const String& cPath, int nOpenMode = O_RDONLY );
    File( const Directory& cDir, const String& cName, int nOpenMode = O_RDONLY );
    File( const FileReference& cRef, int nOpenMode = O_RDONLY );
    File( const FSNode& cNode );
    File( int nFD );
    File( const File& cFile );
    virtual ~File();
    
      // From FSNode
    virtual status_t FDChanged( int nNewFD, const struct stat& sStat );
    virtual off_t GetSize( bool bUpdateCache = true ) const;
    
      // From StreamableIO
    virtual ssize_t Read( void* pBuffer, ssize_t nSize );
    virtual ssize_t Write( const void* pBuffer, ssize_t nSize );
    
      // From seekableIO
    virtual ssize_t ReadPos( off_t nPos, void* pBuffer, ssize_t nSize );
    virtual ssize_t WritePos( off_t nPos, const void* pBuffer, ssize_t nSize );

    virtual off_t Seek( off_t nPos, int nMode );

      // From File
    status_t SetBufferSize( int nSize );
    int	     GetBufferSize() const;
    status_t Flush();
    
private:
    void _Init();
    status_t _FillBuffer( off_t nPos );

	class Private;
	Private *m;
};


} // end of namespace

#endif // __F_STORAGE_FILE_H__
