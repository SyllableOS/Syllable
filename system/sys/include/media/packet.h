/*  Media Packet class
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
 
#ifndef __F_MEDIA_PACKET_H_
#define __F_MEDIA_PACKET_H_

#include <atheos/types.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Packet.
 * \ingroup media
 * \par Description:
 * The Media Packet structure describes one packet of media data. It is used
 * for communication between the inputs/outputs and the codecs. A packet
 * is for example created by calling the ReadPacket() method of the MediaInput
 * class.
 *
 * \author	Arno Klenke
 *****************************************************************************/

typedef struct {
	/** \par Description: 
	* Stream the packet belongs to.
	*****************************************************************************/
	uint32 nStream;
	
	/** \par Description: 
	* Timestamp of the packet.
	*****************************************************************************/
	uint64 nTimeStamp;
	
	/** \par Description: 
	* Buffer which contains the media data. Audio data will only use one buffer,
	* but at least raw video data will probably need more.
	*****************************************************************************/
	uint8* pBuffer[4];
	/** \par Description: 
	* Size of the buffers. For raw video data this will be the linesize.
	*****************************************************************************/
	uint32 nSize[4];
	/** \par Description: 
	* Private data allocated by the MediaInput class.
	*****************************************************************************/
	void*  pPrivate;
} MediaPacket_s;

}

#endif











