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

#include <media/input.h>

using namespace os;

MediaInput::MediaInput()
{
}

MediaInput::~MediaInput()
{
}



/** Identifier.
 * \par Description:
 * Returns the identifier of the object.
 * \author	Arno Klenke
 *****************************************************************************/
String MediaInput::GetIdentifier()
{
	return( "Unknown" );
}


/** Physical type.
 * \par Description:
 * Returns the physical type of the device.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaInput::GetPhysicalType()
{
	return( MEDIA_PHY_UNKNOWN );
}

/** Configuration view.
 * \par Description:
 * Returns a view which includes configuration controls or NULL if this is 
 * not supported.
 * \author	Arno Klenke
 *****************************************************************************/
View* MediaInput::GetConfigurationView()
{
	return( NULL );
}


/** Filename required.
 * \par Description:
 * Returns true if a filename for the Open() method is required.
 * \author	Arno Klenke
 *****************************************************************************/
bool MediaInput::FileNameRequired()
{
	return( false );
}

/** Open one device / file.
 * \par Description:
 * Opens one device or file. Returns true if successful.
 * \param zFileName - Filename of the device / file if FileNameRequired() returns true.
 *					  This parameter might also be used to pass parameters to the input.
 *
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaInput::Open( String zFileName )
{
	return( -1 );
}


/** Close the device / file.
 * \par Description:
 * Closes the opened device or file. Needs to be called when the input is no longer used.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaInput::Close()
{
}

/** Clear the input.
 * \par Description:
 * Clears all caches of the input.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaInput::Clear()
{
}

/** Whether the device / file is packet based.
 * \par Description:
 * When this method returns true then the ReadPacket() / FreePacket() methods
 * are valid. Otherwise you have to use the *Track() methods. 
 * \author	Arno Klenke
 *****************************************************************************/
bool MediaInput::PacketBased()
{
	return( false );
}


/** Whether the device / file is stream based.
 * \par Description:
 * When this method returns true then the GetLength(), GetCurrentPosition()
 * and Seek() members are not valid.
 * \author	Arno Klenke
 *****************************************************************************/
bool MediaInput::StreamBased()
{
	return( false );
}



/** Number of tracks.
 * \par Description:
 * Returns the number of tracks.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaInput::GetTrackCount()
{
	return( 0 );
}


/** Select one track
 * \par Description:
 * Selects one track of the device / file ( The first track is Track 0 ). 
 * Returns the now selected track.
 * \par Note:
 * The input will decide if it seeks to the start.
 * \param nTrack - Selected track.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaInput::SelectTrack( uint32 nTrack )
{
	return( 0 );
}

/** Title.
 * \par Description:
 * Returns the title or ""
 * \author	Arno Klenke
 *****************************************************************************/
String MediaInput::GetTitle()
{
	return( "" );
}

/** Author.
 * \par Description:
 * Returns the author or ""
 * \author	Arno Klenke
 *****************************************************************************/
String MediaInput::GetAuthor()
{
	return( "" );
}

/** Album.
 * \par Description:
 * Returns the album or ""
 * \author	Arno Klenke
 *****************************************************************************/
String MediaInput::GetAlbum()
{
	return( "" );
}

/** Comment.
 * \par Description:
 * Returns a comment or ""
 * \author	Arno Klenke
 *****************************************************************************/
String MediaInput::GetComment()
{
	return( "" );
}

/** Stream number.
 * \par Description:
 * Returns the number of streams inside packet based devices / files.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaInput::GetStreamCount()
{
	return( 0 );
}


/** Return the format of one stream.
 * \par Description:
 * Returns the format of one stream. 
 * \param nIndex - Index of the stream.
 * \author	Arno Klenke
 *****************************************************************************/
MediaFormat_s MediaInput::GetStreamFormat( uint32 nIndex )
{
	MediaFormat_s sDummy;
	return( sDummy );
}


/** Read one packet.
 * \par Description:
 * Reads one packet of media data. Returns 0 if successful.
 * \param psPacket - pointer to the packet which is filled wit the data.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaInput::ReadPacket( MediaPacket_s* psPacket )
{
	return( 1 );
}


/** Free one packet.
 * \par Description:
 * Frees one packet of media data. Call it if the packet is no longer used.
 * \par Note:
 * Not calling this method can cause memory leaks!
 * \author	Arno Klenke
 *****************************************************************************/
void MediaInput::FreePacket( MediaPacket_s* psPacket )
{
}


/** Start track.
 * \par Description:
 * Starts the selected track of non packet based devices / files.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaInput::StartTrack()
{
	return( -1 );
}


/** Stop track.
 * \par Description:
 * Stops the selected track of non packet based devices / files.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaInput::StopTrack()
{
}



/** Length of the device / file.
 * \par Description:
 * Returns the length of the device / file in seconds.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 MediaInput::GetLength()
{
	return( 0 );
}


/** Current position of the device / file.
 * \par Description:
 * Returns the current position of the device / files.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 MediaInput::GetCurrentPosition()
{
	return( 0 );
}


/** Seek.
 * \par Description:
 * Seeks inside one device / file. Returns the position the device / file was
 * seeked to.
 * \par Note:
 * Non packet based devices will start playing after seeking!
 * \param nPosition - Position the device / file should seek to.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 MediaInput::Seek( uint64 nPosition )
{
	return( 0 );
}
























