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

#ifndef __STORAGE_DIRECTORY_H__
#define __STORAGE_DIRECTORY_H__

#include <storage/fsnode.h>
#include <storage/diriterator.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class FileReference;
class File;
class SymLink;

/** Filesystem directory class
 * \ingroup storage
 * \par Description:
 *	This class let you iterate over the content of a directory.
 *
 *	Unlik other FSNode derivated classes it is possible to ask a
 *	os::Directory to retrieve it's own path.
 *
 * \sa FSNode
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Directory : public FSNode, public DirIterator
{
public:
    Directory();
    Directory( const std::string& cPath, int nOpenMode = O_RDONLY );
    Directory( const Directory& cDir, const std::string& cName, int nOpenMode = O_RDONLY );
    Directory( const FileReference& cRef, int nOpenMode = O_RDONLY );
    Directory( const FSNode& cNode );
    Directory( int nFD );
    Directory( const Directory& cDir );
    virtual ~Directory();

    virtual status_t FDChanged( int nNewFD, const struct ::stat& sStat );
    
    virtual status_t GetNextEntry( std::string* pcName );
    virtual status_t GetNextEntry( FileReference* pcRef );
    virtual status_t Rewind();

    status_t CreateFile( const std::string& cName, os::File* pcFile, int nAccessMode = S_IRWXU);
    status_t CreateDirectory( const std::string& cName, os::Directory* pcDir, int nAccessMode = S_IRWXU );
    status_t CreateSymlink( const std::string& cName, const std::string& cDestination, os::SymLink* pcLink );
    status_t GetPath( std::string* pcPath ) const;
    
private:
    std::string	   m_cPathCache;
    DIR*	   m_hDirIterator;
};


} // end of namespace

#endif // __STORAGE_DIRECTORY_H__
