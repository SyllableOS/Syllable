/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2004 Syllable Team
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

#ifndef __F_UTIL_CATALOG_H__
#define __F_UTIL_CATALOG_H__

#include <atheos/types.h>
#include <util/string.h>
#include <util/resource.h>
#include <util/locker.h>
#include <util/string.h>
#include <storage/streamableio.h>
#include <map>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Locale;

/** String Catalog
 * \ingroup util
 * \par Description:
 *
 * \since 0.5.3
 * \sa os::String
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/

class Catalog : public Resource
{
public:
	Catalog( );
	Catalog( StreamableIO* pcSource  );
	Catalog( String& cName, Locale *pcLocale = NULL );
    ~Catalog();

	const String& GetString( uint32 nID ) const;
	const String& GetString( uint32 nID, const String& cDefault ) const;
	void SetString( uint32 nID, const String& cStr ) const;

	status_t Load( StreamableIO* pcSource );
	status_t Save( StreamableIO* pcDest );

	typedef std::map<uint32, String> StringMap;
	typedef StringMap::const_iterator const_iterator;

      // STL iterator interface to the strings.
    const_iterator begin() const;
    const_iterator end() const;

	int Lock() const;
	int Unlock() const;

private:
	struct FileHeader;
	class Private;
	Private *m;
};

/** Localised String
 * \ingroup util
 * \par Description:
 *
 * \since 0.5.3
 * \sa os::String
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/

class LString : public String
{
	public:
	LString( uint32 nID );
};

} // end of namespace

#endif // __F_UTIL_CATALOG_H__
