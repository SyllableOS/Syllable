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

#ifndef __F_STORAGE_FSNODE_H__
#define __F_STORAGE_FSNODE_H__

#include <atheos/types.h>
#include <sys/types.h>
#include <atheos/filesystem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <util/string.h>

#include <util/exceptions.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class FileReference;
class Directory;

/** Lowlevel filesystem node class.
 * \ingroup storage
 * \par Description:
 *	This class give access to the lowest level of a filesystem node.
 *	A node can be a directory, regular file, symlink or named pipe.
 *
 *	It give you access to stats that is common to all nodes in the
 *	filesystem like time-stamps, access-rights, inode and device
 *	numbers, and most imporant the file-attributes.
 *
 *	The native AtheOS filesystem (AFS) support "attributes" wich is
 *	extra data-streams associated with filesystem nodes. An attribute
 *	can have a specific type like int, float, string, etc etc, or it
 *	can be a untyped stream of data. Attributes can be used to store
 *	information associated by the file but that don't belong to file
 *	content itself (for example the file's icon-image and mime-type).
 *
 * \sa os::FileReference, os::File, os::Directory
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


class FSNode
{
public:
    FSNode();
    FSNode( const String& cPath, int nOpenMode = O_RDONLY );
    FSNode( const Directory& cDir, const String& cName, int nOpenMode = O_RDONLY );
    FSNode( const FileReference& cRef, int nOpenMode = O_RDONLY );
    FSNode( int nFD );
    FSNode( const FSNode& cNode );
    virtual ~FSNode();

    virtual status_t FDChanged( int nNewFD, const struct stat& sStat );
    
    virtual status_t SetTo( const String& cPath, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const Directory& cDir, const String& cPath, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const FileReference& cRef, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( int nFD );
    virtual status_t SetTo( const FSNode& cNode );
    virtual void     Unset();
    
    virtual bool     IsValid() const;
    
    virtual status_t GetStat( struct stat* psStat, bool bUpdateCache = true ) const;
    virtual ino_t    GetInode() const;
    virtual dev_t    GetDev() const;
    virtual int	     GetMode( bool bUpdateCache = false ) const;
    virtual off_t    GetSize( bool bUpdateCache = true ) const;
    virtual time_t   GetCTime( bool bUpdateCache = true ) const;
    virtual time_t   GetMTime( bool bUpdateCache = true ) const;
    virtual time_t   GetATime( bool bUpdateCache = true ) const;

    bool	     IsDir() const  { return( S_ISDIR( GetMode() ) != false ); }
    bool	     IsLink() const { return( S_ISLNK( GetMode() ) != false ); }
    bool	     IsFile() const { return( S_ISREG( GetMode() ) != false ); }
    bool	     IsCharDev() const { return( S_ISCHR( GetMode() ) != false ); }
    bool	     IsBlockDev() const { return( S_ISBLK( GetMode() ) != false ); }
    bool	     IsFIFO() const { return( S_ISFIFO( GetMode() ) != false ); }
    
    virtual status_t GetNextAttrName( String* pcName );
    virtual status_t RewindAttrdir();

    virtual ssize_t  WriteAttr( const String& cAttrName, int nFlags, int nType,
				const void* pBuffer, off_t nPos, size_t nLen );
    virtual ssize_t  ReadAttr( const String& cAttrName, int nType,
			       void* pBuffer, off_t nPos, size_t nLen );

    virtual status_t RemoveAttr( const String& cName );
    virtual status_t StatAttr( const String& cName, struct ::attr_info* psBuffer );

    virtual int GetFileDescriptor() const;
    
private:
friend class Directory;
    
    void _SetFD( int nFD );

    mutable struct stat m_sStatCache;
    
    class Private;
    Private *m;
};

} // end of namespace

#endif // __F_STORAGE_FSNODE_H__
