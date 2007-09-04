/*  Media Addon class
 *  Copyright (C) 2003 Arno Klenke
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
 
#ifndef __F_MEDIA_ADDON_H_
#define __F_MEDIA_ADDON_H_

#include <gui/view.h>
#include <atheos/types.h>
#include <util/string.h>
#include <media/packet.h>
#include <media/format.h>
#include <media/codec.h>
#include <media/input.h>
#include <media/output.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

#define MEDIA_ADDON_API_VERSION 0x1

/** Media Addon.
 * \ingroup media
 * \par Description:
 * The Media addon class describes one loadable media module and gives
 * the system information about the various classes included in this module.
 * \par Note:
 * This class is only important for developers of media addons.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaAddon
{
public:
	MediaAddon() {};
	virtual ~MediaAddon() {};
	virtual uint32 GetAPIVersion()
	{
		return( MEDIA_ADDON_API_VERSION );
	}
	virtual status_t		Initialize() = 0;
	virtual String 			GetIdentifier() = 0;
	virtual uint32			GetCodecCount() { return( 0 ); }
	virtual MediaCodec*		GetCodec( uint32 nIndex ) { return( NULL ); }
	virtual uint32			GetInputCount() { return( 0 ); }
	virtual MediaInput*		GetInput( uint32 nIndex ) { return( NULL ); }
	virtual uint32			GetOutputCount() { return( 0 ); }
	virtual MediaOutput*	GetOutput( uint32 nIndex ) { return( NULL ); }
private:
};

typedef MediaAddon* init_media_addon( os::String zDevice );

}

#endif


































