/*  FFMPEG plugin
 *  Copyright (C) 2004 Arno Klenke
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

#include <media/addon.h>

extern os::MediaCodec* init_ffmpeg_codec();
extern os::MediaInput* init_ffmpeg_input();
extern os::MediaOutput* init_ffmpeg_output();

class FFMpegAddon : public os::MediaAddon
{
public:
	status_t Initialize() { return( 0 ); }
	os::String GetIdentifier() { return( "FFMpeg" ); }
	uint32			GetCodecCount() { return( 1 ); }
	os::MediaCodec*		GetCodec( uint32 nIndex ) { return( init_ffmpeg_codec() ); }
	uint32			GetInputCount() { return( 1 ); }
	os::MediaInput*		GetInput( uint32 nIndex ) { return( init_ffmpeg_input() ); }
	uint32			GetOutputCount() { return( 1 ); }
	os::MediaOutput*	GetOutput( uint32 nIndex ) { return( init_ffmpeg_output() ); }
};

extern "C"
{
	os::MediaAddon* init_media_addon()
	{
		return( new FFMpegAddon() );
	}

}
