/*  Media Output class
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

#include <media/output.h>

using namespace os;

MediaOutput::MediaOutput()
{
}

MediaOutput::~MediaOutput()
{
}

/** Identifier.
 * \par Description:
 * Returns the identifier of the object.
 * \author	Arno Klenke
 *****************************************************************************/
String MediaOutput::GetIdentifier()
{
	return( "Unknown" );
}

/** Physical type.
 * \par Description:
 * Returns the physical type of the device.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaOutput::GetPhysicalType()
{
	return( MEDIA_PHY_UNKNOWN );
}

/** Configuration view.
 * \par Description:
 * Returns a view which includes configuration controls or NULL if this is 
 * not supported.
 * \author	Arno Klenke
 *****************************************************************************/
View* MediaOutput::GetConfigurationView()
{
	return( NULL );
}

/** Filename required.
 * \par Description:
 * Returns true if a filename for the Open() method is required.
 * \author	Arno Klenke
 *****************************************************************************/
bool MediaOutput::FileNameRequired()
{
	return( false );
}

/** Open output.
 * \par Description:
 * Opens the output with the provided filename.
 * \param zFileName - Filename or identifier of the output if FileNameRequired()
 * returns true.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaOutput::Open( String zFileName )
{
	return( -1 );
}

/** Close the output.
 * \par Description:
 * Closes the output. Should be called when the output is no longer used.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaOutput::Close()
{
}

/** Clear the output.
 * \par Description:
 * Clears all caches of the output.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaOutput::Clear()
{
}

/** Format Count.
 * \par Description:
 * Returns the number of supported output formats. The Open() method needs to be
 * called before.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaOutput::GetOutputFormatCount()
{
	return( 0 );
}


/** Return one supported format.
 * \par Description:
 * Returns one supported media format.
 * \param nIndex - Index of the format.
 * \author	Arno Klenke
 *****************************************************************************/
MediaFormat_s MediaOutput::GetOutputFormat( uint32 nIndex )
{
	MediaFormat_s sDummy;
	return( sDummy );
}


/** Supported stream count.
 * \par Description:
 * Returns the number of supported streams.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaOutput::GetSupportedStreamCount()
{
	return( 0 );
}


/** Add one stream.
 * \par Description:
 * Adds one stream to the output. The stream index is 0 for the first stream.
 * Returns 0 if successful.
 * \param zName - Name of the stream.
 * \param sFormat - Requested format.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaOutput::AddStream( String zName, MediaFormat_s sFormat )
{
	return( -1 );
}


/** Return a view with the video.
 * \par Description:
 * Returns a view with the displayed video or NULL if this feature is not 
 * supported.
 * \par Note:
 * The view will have the size of the input video stream first.
 * \param nStream - Stream index.
 * \author	Arno Klenke
 *****************************************************************************/
View* MediaOutput::GetVideoView( uint32 nStream )
{
	return( NULL );
}


/** Update video view.
 * \par Description:
 * Updates the video view. Needs to be called when the view position on
 * the screen changes ( e.g. when the window with the view is moved ).
 * \param nStream - Stream index.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaOutput::UpdateView( uint32 nStream )
{
} 

/** Write media data.
 * \par Description:
 * Writes one packet of media data.
 * \par Note:
 * Use GetDelay() to look how long it will take until the data is played.
 * \param nStream - Stream index.
 * \param psPacket - Media packet with the data.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaOutput::WritePacket( uint32 nStream, MediaPacket_s* psPacket )
{
	return( -1 );
}


/** Delay.
 * \par Description:
 * Returns the time it takes till the last queued packet has been played.
 * \param bNonSharedOnly - If the output has shared and non-shared buffers then
 *					  true will return the delay of the non-shared buffer only.
 *					  Otherwise the complete delay will be returned.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 MediaOutput::GetDelay( bool bNonSharedOnly )
{
	return( 0 );
}

/** Returns the size of the buffer.
 * \par Description:
 * Returns the size of the output buffer in msecs.
 * \param bNonSharedOnly - If the output has shared and non-shared buffers then
 *					  true will return the buffer size of the non-shared buffer only.
 *					  Otherwise the complete buffer size will be returned.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 MediaOutput::GetBufferSize( bool bNonSharedOnly )
{
	return( 0 );
}





































