/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2005 Syllable Team
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

#ifndef	__F_UTIL_FLATTENABLE_H__
#define	__F_UTIL_FLATTENABLE_H__

#include "typecodes.h"
#include <sys/types.h>
#include <atheos/types.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** 
 * \ingroup util
 * \par Description:
 *	The os::Flattenable class defines an interface for "flattening"
 *	objects. Flattening means that all the data in the object is stored
 *	in a flat buffer.
 * \par
 * \par
 * \sa os::Message
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/

class Flattenable
{
public:
	virtual int			GetType( void )	const = 0;
	virtual bool		TypeIsCompatible( int nType ) const { return ( nType == GetType() ); }
	virtual size_t		GetFlattenedSize( void ) const = 0;
	virtual status_t	Flatten( uint8* pBuffer, size_t nSize ) const = 0;
	virtual status_t	Unflatten( const uint8* pBuffer ) = 0;
};

}

#endif	// __F_UTIL_FLATTENABLE_H__

