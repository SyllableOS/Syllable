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

#ifndef __F_STORAGE_FILEREFERENCE_H__
#define __F_STORAGE_FILEREFERENCE_H__

#include <sys/stat.h>
#include <storage/directory.h>
#include <storage/path.h>

#include <util/string.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif


class Window;
class Messenger;


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
    FileReference( const String& cPath, bool bFollowLinks = false );
    FileReference( const Directory& cDir, const String& cName, bool bFollowLinks = false );
    FileReference( const FileReference& cRef );
    virtual ~FileReference();

    
    status_t SetTo( const String& cPath, bool bFollowLinks = false );
    status_t SetTo( const Directory& cDir, const String& cName, bool bFollowLinks = false );
    status_t SetTo( const FileReference& cRef );
    void Unset();
    
    bool IsValid() const;
    
    String GetName() const;
    status_t 	GetPath( String* pcPath ) const;

    int	     Rename( const String& cNewName );
    status_t Delete();
    Window*  RenameDialog( const Messenger & cMsgTarget, Message* pcChangeMsg );
    Window*  InfoDialog( const Messenger & cMsgTarget, Message* pcChangeMsg );

    status_t GetStat( struct stat* psStat ) const;

    const Directory& GetDirectory() const;
private:
	class Private;
	Private *m;
};


} // end of namespace

#endif // __STORAGE_FILEREFERENCE_H__
