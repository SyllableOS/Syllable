/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#ifndef __F_GUI_PATH_H__
#define __F_GUI_PATH_H__

#include <sys/types.h>
#include <atheos/types.h>

#include <string>

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
    Path( const char* pzPath );
    Path( const char* pzPath, int nLen );
    ~Path();
    void operator =(const Path& cPath );
    void operator =(const char* pzPath );
    bool operator==(const Path& cPath) const;
    void SetTo( const char* pzPath );
    void SetTo( const char* pzPath, int nLen );
    void Append( const Path& cPath );
    void Append( const char* pzName );
  
    const char* GetLeaf() const;
    const char* GetPath() const;
    Path	GetDir() const;

    operator std::string() const;
private:
    void _Normalize();

//    std::string m_cBuffer;
    char*	m_pzPath;
    uint	m_nMaxSize;
};

} // End of namespace

#endif // __F_GUI_PATH_H__
