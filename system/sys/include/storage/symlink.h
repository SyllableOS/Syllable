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

#ifndef __F_STORAGE_SYMLINK_H__
#define __F_STORAGE_SYMLINK_H__

#include <storage/fsnode.h>
#include <util/string.h>

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
    SymLink( const String& cPath, int nOpenMode = O_RDONLY );
    SymLink( const Directory& cDir, const String& cName, int nOpenMode = O_RDONLY );
    SymLink( const FileReference& cRef, int nOpenMode = O_RDONLY );
    SymLink( const FSNode& cNode );
    SymLink( const SymLink& cNode );
    virtual ~SymLink();

    virtual status_t SetTo( const String& cPath, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const Directory& cDir, const String& cPath, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const FileReference& cRef, int nOpenMode = O_RDONLY );
    virtual status_t SetTo( const FSNode& cNode );
    virtual status_t SetTo( const SymLink& cLink );

    status_t 	ReadLink( String* pcBuffer );
    String ReadLink();
    status_t ConstructPath( const Directory& cParent, Path* pcBuffer );
    status_t ConstructPath( const String& cParent, Path* pcBuffer );
private:
};


} // end of namespace

#endif // __F_STORAGE_SYMLINK_H__
