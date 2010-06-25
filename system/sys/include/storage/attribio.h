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

#ifndef __F_STORAGE_ATTRIBIO_H_
#define __F_STORAGE_ATTRIBIO_H_

#include <atheos/fs_attribs.h>

#include <storage/seekableio.h>

namespace os {

class FSNode;
class String;

/** 
 * \ingroup storage
 * \par Description: Class for reading and writing to filesystem attributes.
 *  All IO to the attribute is unbuffered.
 *
 * \sa os::SeekableIO, os::File
 *
 * \since 0.6.7
 *
 * \author	Anthony Morphett (anthony@syllable.org)
 *****************************************************************************/

class AttribIO : public SeekableIO
{
public:
	AttribIO();
	AttribIO( const String& cPath, const String& cAttrName, fsattr_type nType, int nFlags );
	AttribIO( const FSNode& cFSNode, const String& cAttrName, fsattr_type nType, int nFlags );
	AttribIO( const AttribIO& cAttribIO );
	virtual ~AttribIO();
	
	/* From StreamableIO */
	virtual ssize_t Read( void *pBuffer, ssize_t nSize );
	virtual ssize_t Write( const void *pBuffer, ssize_t nSize );

	/* From SeekableIO */
	virtual ssize_t ReadPos( off_t nPos, void* pBuffer, ssize_t nSize );
	virtual ssize_t WritePos( off_t nPos, const void* pBuffer, ssize_t nSize );
	virtual off_t   Seek( off_t nPos, int nMode );
	
	/* From AttribIO */
	off_t GetSize();
	fsattr_type GetType() const;
	
private:
	void _Init();
	class Private;
	Private* m;
};


}	/* namespace os */

#endif /* __F_STORAGE_ATTRIBIO_H_ */
