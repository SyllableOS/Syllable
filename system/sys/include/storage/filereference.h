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

#ifndef __STORAGE_FILEREFERENCE_H__
#define __STORAGE_FILEREFERENCE_H__

#include <sys/stat.h>
#include <storage/directory.h>
#include <storage/path.h>

#include <string>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif


/** Semi persistent reference to a file
 * \ingroup storage
 * \par Description:
 *	FileReference's serve much the same functionality as a plain
 *	path.  It uniquely identifies a file within the
 *	filesystem. The main advantage of a FileReference is that it
 *	can partly "track" the file it reference. While moving a file
 *	to another directory or renaming the directory it lives in or
 *	any of it's subdirectories whould break a path, it will not
 *	affect a FileReference. Renaming the file itself however from
 *	outside the FileReference (ie. not using the
 *	FileReference::Rename() member) will break a FileReference
 *	aswell.
 *
 *  \sa
 *  \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


class FileReference
{
public:
    FileReference();
    FileReference( const std::string& cPath, bool bFollowLinks = false );
    FileReference( const Directory& cDir, const std::string& cName, bool bFollowLinks = false );
    FileReference( const FileReference& cRef );
    virtual ~FileReference();

    
    status_t SetTo( const std::string& cPath, bool bFollowLinks = false );
    status_t SetTo( const Directory& cDir, const std::string& cName, bool bFollowLinks = false );
    status_t SetTo( const FileReference& cRef );
    void Unset();
    
    bool IsValid() const;
    
    std::string GetName() const;
    status_t 	GetPath( std::string* pcPath ) const;

    int	     Rename( const std::string& cNewName );
    status_t Delete();

    status_t GetStat( struct ::stat* psStat ) const;

    const Directory& GetDirectory() const;
private:
    Directory	m_cDirectory;
    std::string m_cName;
};


} // end of namespace

#endif // __STORAGE_FILEREFERENCE_H__
