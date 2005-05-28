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

#ifndef __F_UTIL_LOCALE_H__
#define __F_UTIL_LOCALE_H__

#include <atheos/types.h>
#include <util/string.h>
#include <storage/streamableio.h>
#include <locale.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


class Catalog;

/** Locale class
 * \ingroup util
 * \par Description:
 *
 * \since 0.5.3
 * \sa os::String
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/

class Locale
{
public:
	Locale( );
	Locale( int nCategory );
	Locale( const Locale& cLocale );
    ~Locale();

	const String& GetName() const;
	void SetName( const String& cName );

	StreamableIO* GetLocalizedResourceStream( const String& cName );
	StreamableIO* GetResourceStream( const String& cName );

	Catalog* GetLocalizedCatalog( const String& cName );

	// For resources embedded in libsyllable or stored in /system/resources
	StreamableIO* GetSystemResourceStream( const String& cName );
	StreamableIO* GetLocalizedSystemResourceStream( const String& cName );

	Catalog* GetLocalizedSystemCatalog( const String& cName );

	const Locale& operator=( const Locale& cLocale );

public:
	static const Locale& GetSystemDefaultLocale();
private:
	class Private;
	Private *m;
};

}

#endif /* __F_UTIL_LOCALE_H__ */
