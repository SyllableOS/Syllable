/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

#ifndef _F_STORAGE_OPERATIONS_H_
#define _F_STORAGE_OPERATIONS_H_

#include <vector>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class Messenger;
class Message;

void copy_files( const std::vector < os::String > &cDstPaths, const std::vector < os::String > &cSrcPaths, const Messenger & cMsgTarget, Message* pcFinishMsg, 
					bool bReplace = false, bool bDontOverwrite = false );
void move_files( const std::vector < os::String > &cDstPaths, const std::vector < os::String > &cSrcPaths, const Messenger & cMsgTarget, Message* pcFinishMsg, 
					bool bReplace = false, bool bDontOverwrite = false );
void delete_files( const std::vector < os::String > &cPaths, const Messenger & cMsgTarget, Message* pcFinishMsg );


}; // end of namespace

#endif // __STORAGE_OPERATIONS_H__










