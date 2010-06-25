/*  libsyllable.so - the highlevel API library for Syllable
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

#include <storage/attribute.h>
#include <storage/fsnode.h>
#include <util/locker.h>
#include <util/string.h>

using namespace os;

class Attribute::Private
{
public:
	Private() : m_cMutex( "os::Attribute" )
	{
		m_pcFSNode = NULL;
		m_cAttrName = "";
		m_nType = ATTR_TYPE_NONE;
		m_nOffset = 0;
		m_nFlags = 0;
	}
	~Private() {}
	
    mutable Locker m_cMutex;
	FSNode* m_pcFSNode;
	String  m_cAttrName;
	fsattr_type     m_nType;
	off_t   m_nOffset;
	int     m_nFlags;
};


/** Construct an Attribute object from a filename and attribute name.
 * \par Description:
 *  Creates an Attribute object for reading and writing to the specified attribute of the specified file.
 *
 * \param cPath
 *	The file possessing the attribute to open. The path can either be absolute (starting
 *	with "/") or relative to the current working directory.  If the file cannot be opened (via os::FSNode),
 *  an exception will be thrown.
 *
 * \prarm cAttrName
 *  The name of the attribute to open.
 *
 * \param nType
 *  The type of the attribute named in cAttrName. This must be one of the ATTR_TYPE_ constants defined in atheos/fs_attribs.h.
 *
 * \param nFlags
 *	Flags controlling how the attribute should be opened. Currently only the following flag is implemented:
 *	- O_TRUNC	trunate the size of the attribute to 0.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

Attribute::Attribute( const String& cPath, const String& cAttrName, fsattr_type nType, int nFlags )
{
	_Init();
	m->m_pcFSNode = new FSNode( cPath );
	m->m_cAttrName = cAttrName;
	m->m_nType = nType;
	m->m_nOffset = 0;
	m->m_nFlags = nFlags;
}


/** Construct an Attribute object from a file given as an FSNode and an attribute name.
 * \par Description:
 *  Creates an Attribute object for reading and writing to the specified attribute of the specified file.
 *
 * \param cFSNode
 *	An FSNode object representing the file with the attribute to open.  The FSNode will be copied, so the
 *  caller should delete cFSNode when done.
 *
 * \prarm cAttrName
 *  The name of the attribute to open.
 *
 * \param nType
 *  The type of the attribute named in cAttrName. This must be one of the ATTR_TYPE_ constants defined in atheos/fs_attribs.h.
 *
 * \param nFlags
 *	Flags controlling how the attribute should be opened. Currently only the following flag is implemented:
 *	- O_TRUNC	trunate the size of the attribute to 0.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

Attribute::Attribute( const FSNode& cFSNode, const String& cAttrName, fsattr_type nType, int nFlags )
{
	_Init();
	m->m_pcFSNode = new FSNode( cFSNode );
	m->m_cAttrName = cAttrName;
	m->m_nType = nType;
	m->m_nOffset = 0;
	m->m_nFlags = nFlags;
}

/** Copy constructor.
 * \par Description:
 *	This constructor will make an independent copy of \p cAttribute.
 *	The flags, attribute type and current seek position of the old object will be copied.
 *
 * \param cAttribute
 *	The Attribute object to copy.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

Attribute::Attribute( const Attribute& cAttribute )
{
	_Init();
	if( cAttribute.m->m_pcFSNode ) m->m_pcFSNode = new FSNode( *cAttribute.m->m_pcFSNode );
	m->m_cAttrName = cAttribute.m->m_cAttrName;
	m->m_nType = cAttribute.m->m_nType;
	m->m_nOffset = cAttribute.m->m_nOffset;
	m->m_nFlags = cAttribute.m->m_nFlags;
}

Attribute::~Attribute()
{
	if( m->m_pcFSNode ) { delete( m->m_pcFSNode ); }
	delete( m );
}

void Attribute::_Init()
{
	m = new Private();
}

/** Read data from a specified location from the attribute.
 * \par Description:
 *  Reads nSize bytes from the attribute, starting at offset nPos, and stores the data in the buffer pBuffer.
 *
 * \param nPos
 *  The offset (from the beginning of the attribute) at which to read.
 *
 * \param pBuffer
 *  The buffer in which to store the data.
 *
 * \param nSize
 *  The number of bytes to read.
 * 
 * \return
 *  On success, returns the number of bytes read. On error, returns a negative error code.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

ssize_t Attribute::ReadPos( off_t nPos, void* pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );
	if( !m->m_pcFSNode ) {
//		printf( "Attribute: ReadPos() with invalid File!\n" );
		return( -EINVAL );
	}
	
	return( m->m_pcFSNode->ReadAttr( m->m_cAttrName, m->m_nType, pBuffer, nPos, nSize ) );
}

/** Write data to a specified location in the attribute.
 * \par Description:
 *  Writes nSize bytes from the buffer pBuffer into the attribute, starting at offset nPos.
 *
 * \param nPos
 *  The offset (from the beginning of the attribute) at which to write.
 *
 * \param pBuffer
 *  The buffer containing the data to be written.
 *
 * \param nSize
 *  The number of bytes to write.
 * 
 * \return
 *  On success, returns the number of bytes written. On error, returns a negative error code.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

ssize_t Attribute::WritePos( off_t nPos, const void* pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );
	if( !m->m_pcFSNode ) {
//		printf( "Attribute: WritePos() with invalid File!\n" );
		return( -EINVAL );
	}

	return( m->m_pcFSNode->WriteAttr( m->m_cAttrName, m->m_nFlags, m->m_nType, pBuffer, nPos, nSize ) );
}

/** Seek to a specified location in the attribute.
 * \par Description:
 *  Seeks to the specified location in the attribute. The next read or write will occur from
 * the offset specified by nPos and nMode.
 *
 * \param nPos
 *  The offset by which to seek, from the position specified by nMode.
 *
 * \param nMode
 *  A constant specifying how the parameter nPos will be interpreted.  It must be one of the SEEK_ constants:
 *  \par SEEK_START: Seek to nPos bytes from the start of the attribute.
 *  \par SEEK_CUR: Seek to nPos bytes from the current seek position in the attribute.
 *  \par SEEK_END: Seek to nPos bytes from the end of the attribute.
 *
 * \return
 *  Returns the new seek position, in bytes from the start of the attribute.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

off_t Attribute::Seek( off_t nPos, int nMode )
{
	AutoLocker cLock( &m->m_cMutex );
	
//	printf( "Attribute::Seek( %i, %i %s)\n", (int)nPos, nMode, (nMode == SEEK_SET ? "SEEK_SET" : (nMode == SEEK_END ? "SEEK_END" : (nMode == SEEK_CUR ? "SEEK_CUR" : "UNKNOWN mode!"))) );
	if( nMode == SEEK_SET ) { m->m_nOffset = nPos; }
	if( nMode == SEEK_CUR ) { m->m_nOffset += nPos; }
	if( nMode == SEEK_END ) { m->m_nOffset = GetSize() + nPos; }
	return( m->m_nOffset );
}

ssize_t Attribute::Read( void *pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );
	
	ssize_t nTemp;
	nTemp = ReadPos( m->m_nOffset, pBuffer, nSize );
	m->m_nOffset += nTemp;
	return( nTemp );
}

ssize_t Attribute::Write( const void *pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );
	
	ssize_t nTemp;
	nTemp = WritePos( m->m_nOffset, pBuffer, nSize );
	m->m_nOffset += nTemp;
	return( nTemp );
}

/** Returns the type of the attribute.
 * 
 * \return
 *  One of the ATTR_TYPE_ constants (defined in atheos/fs_attribs.h) specifying
 *  the type of the attribute.
 *
 * \sa fsattr_type
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

fsattr_type Attribute::GetType() const
{
	return( m->m_nType );
}

/** Returns the size of the attribute.
 *
 * \return
 *  Returns the number of bytes currently stored in the attribute, or a negative error code on error.
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

off_t Attribute::GetSize()
{
	AutoLocker cLock( &m->m_cMutex );
	if( !m->m_pcFSNode )
	{
		return( -EINVAL );
	}

	struct attr_info sInfo;
	m->m_pcFSNode->StatAttr( m->m_cAttrName, &sInfo );
	return( sInfo.ai_size );
}

