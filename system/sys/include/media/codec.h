/*  Media Codec class
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
 
#ifndef __F_MEDIA_CODEC_H_
#define __F_MEDIA_CODEC_H_

#include <gui/view.h>
#include <util/resource.h>
#include <atheos/types.h>
#include <util/string.h>
#include <media/packet.h>
#include <media/format.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Codec.
 * \ingroup media
 * \par Description:
 * The Media Codec class is used together with the Media Input
 * class to decode / encode audio and video data of packet based streams. 
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaCodec : public Resource
{
protected:
	virtual ~MediaCodec();
public:
	MediaCodec();
	
	virtual String 			GetIdentifier();
	virtual uint32			GetPhysicalType();
	virtual View*			GetConfigurationView();
	
	virtual status_t		Open( MediaFormat_s sInternalFormat, MediaFormat_s sExternalFormat, bool bEncode );
	virtual void			Close();
	
	virtual MediaFormat_s	GetInternalFormat();
	virtual MediaFormat_s	GetExternalFormat();
	
	virtual status_t		CreateAudioOutputPacket( MediaPacket_s* psOutput );
	virtual void			DeleteAudioOutputPacket( MediaPacket_s* psOutput );
	virtual status_t		CreateVideoOutputPacket( MediaPacket_s* psOutput );
	virtual void			DeleteVideoOutputPacket( MediaPacket_s* psOutput );
	
	virtual status_t 		DecodePacket( MediaPacket_s* psPacket, MediaPacket_s* psOutput );
	virtual status_t		ParsePacket( MediaPacket_s* psPacket, os::MediaPacket_s * psOutput );
	virtual status_t		EncodePacket( MediaPacket_s* psPacket, MediaPacket_s* psOutput );
private:
};

typedef MediaCodec* init_media_codec();

}

#endif

































