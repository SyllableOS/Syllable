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

#ifndef __F_STORAGE_VOLUMES_H__
#define __F_STORAGE_VOLUMES_H__

#include <atheos/filesystem.h>

#include <util/string.h>


namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class Messenger;
class Message;

/** Information about the mounted volumes
 * \ingroup storage
 * \par Description:
 *	This class makes it possible to get a list of the mounted volumes
 *  and also offers a way to mount and unmount volumes.
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/

class Volumes 
{
public:
	Volumes();
	~Volumes();
	
	uint GetMountPointCount();
	status_t GetMountPoint( uint nIndex, String* cMountPoint );
	status_t GetFSInfo( uint nIndex, fs_info* psInfo );

	Window* MountDialog( const Messenger & cMsgTarget, Message* pcMountMsg );
	void Unmount( String cMountPoint, const Messenger & cMsgTarget, Message* pcFinishMsg );
};

} // end of namespace

#endif // __F_STORAGE_VOLUMES_H__
