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

#ifndef __STORAGE_SYMLINK_H__
#define __STORAGE_SYMLINK_H__

#include <storage/fsnode.h>

#include <string>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class Path;

/** Symbolic link handling class.
 * \ingroup storage
 * \par Description:
 *
 *  \sa FSNode, FileReference
 *  \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


class SymLink : public FSNode
{
public:
    SymLink();
    SymLink( const std::string& cPath, int nOpenMode = O_RDONLY );
    SymLink( const Directory& cDir, const std::string& cName, int nOpenMode = O_RDONLY );
    SymLink( const FileReference& cRef, int nOpenMode = O_RDONLY );
    SymLink( const FSNode& cNode );
    SymLink( const SymLink& cNode );
    virtual ~SymLink();

    virtual status_t SetTo( const std::string& cPath, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const Directory& cDir, const std::string& cPath, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const FileReference& cRef, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const FSNode& cNode );
    virtual status_t SetTo( const SymLink& cLink );

    status_t 	ReadLink( std::string* pcBuffer );
    std::string ReadLink();
    status_t ConstructPath( const Directory& cParent, Path* pcBuffer );
    status_t ConstructPath( const std::string& cParent, Path* pcBuffer );
    
    
private:
};


} // end of namespace

#endif // __STORAGE_SYMLINK_H__
