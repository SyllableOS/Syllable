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

#include <media/codec.h>

using namespace os;

MediaCodec::MediaCodec()
{
}

MediaCodec::~MediaCodec()
{
}

/** Identifier.
 * \par Description:
 * Returns the identifier of the object.
 * \author	Arno Klenke
 *****************************************************************************/
String MediaCodec::GetIdentifier()
{
	return( "Unknown" );
}

/** Physical type.
 * \par Description:
 * Returns the physical type of the device.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaCodec::GetPhysicalType()
{
	return( MEDIA_PHY_UNKNOWN );
}

/** Configuration view.
 * \par Description:
 * Returns a view which includes configuration controls or NULL if this is 
 * not supported.
 * \author	Arno Klenke
 *****************************************************************************/
View* MediaCodec::GetConfigurationView()
{
	return( NULL );
}

/** Open codec.
 * \par Description:
 * Opens the codec with the formats supplied by the Media Input / Output classes.
 * \param sInternalFormat - Internal format of the codec.
 * \param sExternalFormat - Format to which the data will be decoded or from which
 *							they will be encoded. The most common will be a raw format.
 * \param bEncode		  - If true then the codec will be opened for encoding, otherwise
 **							for decoding.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaCodec::Open( MediaFormat_s sInternalFormat, MediaFormat_s sExternalFormat, bool bEncode )
{
	return( -1 );
}

/** Close the codec.
 * \par Description:
 * Closes the codec. Needs to be called when the codec is no longer used.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaCodec::Close()
{
}


/** Return internal format.
 * \par Description:
 * Returns the internal format of the codec. Should be called after Open() to get
 * the _real_ format.
 * \author	Arno Klenke
 *****************************************************************************/
MediaFormat_s MediaCodec::GetInternalFormat()
{
	MediaFormat_s sFormat;
	return( sFormat );
}


/** Return External format.
 * \par Description:
 * Returns the external format of the codec. Should be called after Open() to get
 * the _real_ format.
 * \author	Arno Klenke
 *****************************************************************************/
MediaFormat_s MediaCodec::GetExternalFormat()
{
	MediaFormat_s sFormat;
	return( sFormat );
}

/** Create a media output packet for audio data.
 * \par Description:
 * Creates a media output packet which can be used with the DecodePacket() 
 * / EncodePacket() method.
 * \par Note:
 * You can use this packet as often as you want.
 * \param psOutput - Pointer to the output packet.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaCodec::CreateAudioOutputPacket( MediaPacket_s* psOutput )
{
	return( 0 );
}


/** Delete a media output packet for audio data.
 * \par Description:
 * Deletes a media output packet which has been created with the
 * CreateAudioOutputPacket() method.
 * \param psOutput - Pointer to the output packet.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaCodec::DeleteAudioOutputPacket( MediaPacket_s* psOutput )
{
}



/** Create a media output packet for video data.
 * \par Description:
 * Creates a media output packet which can be used with the 
 * DecodePacket() / EncodePacket() method.
 * \par Note:
 * You can use this packet as often as you want.
 * \param psOutput - Pointer to the output packet.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaCodec::CreateVideoOutputPacket( MediaPacket_s* psOutput )
{
	return( 0 );
}


/** Delete a media output packet for video data.
 * \par Description:
 * Deletes a media output packet which has been created with the
 * CreateVideoOutputPacket() method.
 * \param psOutput - Pointer to the output packet.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaCodec::DeleteVideoOutputPacket( MediaPacket_s* psOutput )
{
}

/** Decode media data.
 * \par Description:
 * Decodes one packet of media data. The packet can be retrieved by calling 
 * the ReadPacket() method of a media input object. Returns 0 if successful.
 * \param psPacket - packet of media data.
 * \param psOutput - Output packet which has been allocated with the Create - Video / Audio - OutputPacket()
 *					method.
 *
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaCodec::DecodePacket( MediaPacket_s* psPacket, MediaPacket_s* psOutput )
{
	return( -1 );
}

/** Parse media data.
 * \par Description:
 * Call this function if you want the codec to parse the media data but not
 * to decode it. This can be useful if you want to decode video data but the
 * computer is to slow to do it in realtime and you want to drop frames.
 * \param psPacket - packet of media data.
 * \par Note:
 * If the codec does not implement this method then DecodePacket() will be called.
 * \param psPacket - packet of media data.
 * \param psOutput - Output packet which has been allocated with the Create - Video / Audio - OutputPacket()
 *					method.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaCodec::ParsePacket( MediaPacket_s* psPacket, os::MediaPacket_s * psOutput )
{
	return( DecodePacket( psPacket, psOutput ) );
}


/** Encode media data.
 * \par Description:
 * Encodes one packet of media data. The packet can be retrieved by calling 
 * the ReadPacket() method of a media input object. Returns 0 if successful.
 * \param psPacket - packet of media data.
 * \param psOutput - Output packet which has been allocated with the Create - Video / Audio - OutputPacket()
 *					method.
 *
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaCodec::EncodePacket( MediaPacket_s* psPacket, MediaPacket_s* psOutput )
{
	return( -1 );
}


































