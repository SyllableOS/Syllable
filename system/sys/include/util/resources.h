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

#ifndef __F_UTIL_RESOURCES_H__
#define __F_UTIL_RESOURCES_H__

#include <storage/file.h>
#include <atheos/image.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif


/** Data stream helper class for os::Resources.
 * \ingroup util
 * \par Description:
 *	Instances of this class is returned by various members of the
 *	os::Resources class to give access to data inside the resource
 *	archive.
 * \par
 *	As the private constructor indicate you should never create
 *	instances of this class yourself.
 * \since 0.3.7
 * \sa os::Resources
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ResStream : public SeekableIO
{
public:
    ~ResStream();
    
    String GetName() const;
    String GetType() const;
    ssize_t	GetSize() const;

      // From StreamableIO
    virtual ssize_t Read( void* pBuffer, ssize_t nSize );
    virtual ssize_t Write( const void* pBuffer, ssize_t nSize );
    
      // From seekableIO
    virtual ssize_t ReadPos( off_t nPos, void* pBuffer, ssize_t nSize );
    virtual ssize_t WritePos( off_t nPos, const void* pBuffer, ssize_t nSize );
    virtual off_t   Seek( off_t nPos, int nMode );
private:
    friend class Resources;
    ResStream( SeekableIO* pcFile, off_t nOffset, const String& cName, const String& cType, ssize_t nSize, bool bReadOnly );

    class Private;
    Private* m;
};

/** Helper class for reading/writing resources embedded in executables and DLL's.
 * \ingroup util
 * \par Description:
 *	Many applications will need external resources like bitmaps, icons, text,
 *	or other data that is not includeded in the executable by the compiler
 *	or linker. Such resources can be stored in separate files inside the
 *	application directory but it is often convinient to collect them inside
 *	the application executable iteself to avoid having a large amount of
 *	small files floating around. Also if a DLL need external resources
 *	it can be hard for the DLL to locate external resource files.
 * \par
 *	It is therefor often more convinient to add the resources to the
 *	executable or DLL with the "rescopy" utility and then use this
 *	class to load them at runtime. A os::Resources object can be initialized from
 *	a regular data-stream or from a image-ID. The executable and all
 *	the DLL's loaded by a process is assigned individual image-ID's. This ID's can be
 *	obtained by a global function named get_image_id(). The get_image_id()
 *	function will return different ID's depending on what DLL or executable
 *	the calling function lives in and can therefor be used by a DLL
 *	or executable to learn what image it should use when loading it's resources.
 * \par
 *	The os::Resources class can be used to read resources directly from
 *	the resource section of an executable or from a dedicated resource
 *	archive. It can also be used to write resources into a resource
 *	archive but it can not embed the archive into an executable.
 * \par
 *	Resources within a archive is identified by uniqueue names and
 *	each resource have a mime-type that can be used by to specify
 *	what kind of data is contained by the resource. The mime-type is
 *	not used by os::Resources itself but can be useful when manipulating
 *	the resources with external applications.
 * \since 0.3.7
 * \sa ResStream
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Resources
{
public:
    enum { RES_MAGIC = 0x3020c49b, RES_VERSION = 1 };
    Resources( SeekableIO* pcStream );
    Resources( int nImageID );
    Resources( SeekableIO* pcStream, off_t nResOffset, bool bCreate = false );
    ~Resources();

    void	DetachStream();
    int		GetResourceCount() const;
    String GetResourceName( uint nIndex ) const;
    String GetResourceType( uint nIndex ) const;
    ssize_t 	GetResourceSize( uint nIndex ) const;
    
    ssize_t	ReadResource( const String& cResName, void* pBuffer, String* pzResType, ssize_t nSize );
    ResStream*	GetResourceStream( const String& cName );
    ResStream*	GetResourceStream( uint nIndex );
    ResStream*	CreateResource( const String& cName, const String& cType, ssize_t nSize );

    status_t	FindExecutableResource( SeekableIO* pcStream, off_t* pnOffset, ssize_t* pnSize, const char* pzSectionName = NULL );
private:
    void _Init( SeekableIO* pcStream, off_t nResOffset, bool bCreate );
    status_t _LoadResourceList();
    class Private;
    Private* m;
};

} // end of namespace

#endif // __F_UTIL_RESOURCES_H__

