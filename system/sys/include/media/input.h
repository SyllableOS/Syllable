/*  Media Input class
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
 
#ifndef __F_MEDIA_DEMUXER_H_
#define __F_MEDIA_DEMUXER_H_

#include <util/resource.h>
#include <gui/view.h>
#include <gui/image.h>
#include <atheos/types.h>
#include <util/string.h>
#include <media/packet.h>
#include <media/format.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Media Input.
 * \ingroup media
 * \par Description:
 * The Media Input class can be used standalone for non packet based files / devices. 
 * For packet based files it is used to split the media file into various streams
 * which can be transcoded by the Media Codec class.  
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaInput : public Resource
{
protected:
	virtual ~MediaInput();
public:
	MediaInput();
	
	virtual String 			GetIdentifier();
	virtual uint32			GetPhysicalType();
	virtual View*			GetConfigurationView();
	
	virtual bool			FileNameRequired();
	virtual status_t		Open( String zFileName );
	virtual void			Close();
	virtual void			Clear();
	
	virtual bool			PacketBased();
	virtual bool			StreamBased();
	
	virtual uint32			GetTrackCount();
	virtual uint32			SelectTrack( uint32 nTrack );
	
	virtual String			GetTitle();
	virtual String			GetAuthor();
	virtual String			GetAlbum();	
	virtual String			GetComment();	
	
	virtual uint32			GetStreamCount();
	virtual MediaFormat_s	GetStreamFormat( uint32 nIndex );
	virtual status_t		ReadPacket( MediaPacket_s* psPacket );
	virtual void			FreePacket( MediaPacket_s* psPacket );
	
	virtual status_t		StartTrack();
	virtual void			StopTrack();
	
	virtual uint64			GetLength();
	virtual uint64			GetCurrentPosition();
	virtual uint64			Seek( uint64 nPosition );
	
private:
};

typedef MediaInput* init_media_input();

}

#endif
























