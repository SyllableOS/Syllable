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

#ifndef __F_STORAGE_PATH_H__
#define __F_STORAGE_PATH_H__

#include <sys/types.h>
#include <atheos/types.h>

#include <util/string.h>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

/** 
 * \ingroup storage
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Path
{
public:
    Path();
    Path( const Path& cPath );
    Path( const String& cPath );
    ~Path();
    void operator =(const Path& cPath );
    void operator =(const String& cPath );
    bool operator==(const Path& cPath) const;
    void SetTo( const String& cPath );
    void Append( const Path& cPath );
    void Append( const String& cName );
  
    String GetLeaf() const;
    String GetPath() const;
    Path	GetDir() const;

    operator String() const;
private:
    void _Normalize();
	void _SetTo( const char *pzPath, int nLen );

	class Private;
	Private *m;
};

} // End of namespace

#endif // __F_STORAGE_PATH_H__
