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

#ifndef __F_STORAGE_DIRECTORY_H__
#define __F_STORAGE_DIRECTORY_H__

#include <storage/fsnode.h>
#include <storage/diriterator.h>
#include <storage/path.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class FileReference;
class File;
class SymLink;
class Window;
class Messenger;
class Message;

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
    Directory( const String& cPath, int nOpenMode = O_RDONLY );
    Directory( const Directory& cDir, const String& cName, int nOpenMode = O_RDONLY );
    Directory( const FileReference& cRef, int nOpenMode = O_RDONLY );
    Directory( const FSNode& cNode );
    Directory( int nFD );
    Directory( const Directory& cDir );
    virtual ~Directory();

    virtual status_t FDChanged( int nNewFD, const struct stat& sStat );
    
    virtual status_t GetNextEntry( String* pcName );
    virtual status_t GetNextEntry( FileReference* pcRef );
    virtual status_t Rewind();
    virtual status_t Delete();

    status_t CreateFile( const String& cName, os::File* pcFile, int nAccessMode = S_IRWXU);
    status_t CreateDirectory( const String& cName, os::Directory* pcDir, int nAccessMode = S_IRWXU );
    status_t CreateSymlink( const String& cName, const String& cDestination, os::SymLink* pcLink );
    Window*  CreateDirectoryDialog( const Messenger & cMsgTarget, Message* pcCreateMsg, String cDefaultName );
    status_t GetPath( String* pcPath ) const;
    status_t SetPath( const String& cPath, int nOpenMode = O_RDONLY );
    status_t SetPath( const Path& cPath, int nOpenMode = O_RDONLY );
    
private:
	class Private;
	Private *m;
};


} // end of namespace

#endif // __F_STORAGE_DIRECTORY_H__

