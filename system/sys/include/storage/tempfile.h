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

#ifndef __STORAGE_TEMPFILE_H__
#define __STORAGE_TEMPFILE_H__

#include <storage/file.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

/** 
 * \ingroup storage
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class TempFile : public File
{
public:
    TempFile( const char* pzPrefix = NULL, const char* pzPath = NULL, int nAccess = S_IRUSR | S_IWUSR );
    ~TempFile();

    void     Detatch();
    status_t Unlink();

    std::string GetPath() const;
    
private:
    class Private;
    Private* m;
};

} // end of namespace


#endif // __STORAGE_TEMPFILE_H__
