/*  Media Server definitions
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
 
#ifndef __F_MEDIA_SERVER_H_
#define __F_MEDIA_SERVER_H_

#include <atheos/types.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

#define MEDIA_SERVER_AUDIO_BUFFER 32 * 1024 // 32k shared area

enum {
	MEDIA_SERVER_OK,
	MEDIA_SERVER_ERROR,
	MEDIA_SERVER_PING,
	MEDIA_SERVER_GET_DEFAULT_INPUT,
	MEDIA_SERVER_GET_DEFAULT_AUDIO_OUTPUT,
	MEDIA_SERVER_GET_DEFAULT_VIDEO_OUTPUT,
	MEDIA_SERVER_SET_DEFAULT_INPUT,
	MEDIA_SERVER_SET_DEFAULT_AUDIO_OUTPUT,
	MEDIA_SERVER_SET_DEFAULT_VIDEO_OUTPUT,
	MEDIA_SERVER_GET_STARTUP_SOUND,
	MEDIA_SERVER_SET_STARTUP_SOUND,
	
	MEDIA_SERVER_SHOW_CONTROLS = 50,
	
	MEDIA_SERVER_CREATE_AUDIO_STREAM = 100,
	MEDIA_SERVER_DELETE_AUDIO_STREAM,
	MEDIA_SERVER_FREE_1,
	MEDIA_SERVER_FREE_2,
	MEDIA_SERVER_FREE_3,
	MEDIA_SERVER_CLEAR_AUDIO_STREAM,
	MEDIA_SERVER_START_AUDIO_STREAM,

	MEDIA_SERVER_GET_DSP_COUNT = 150,
	MEDIA_SERVER_GET_DSP_INFO,
	MEDIA_SERVER_GET_DEFAULT_DSP,
	MEDIA_SERVER_SET_DEFAULT_DSP,
	MEDIA_SERVER_GET_DSP_FORMATS,
	
	MEDIA_SERVER_GET_MASTER_VOLUME = 200,
	MEDIA_SERVER_SET_MASTER_VOLUME
};

}

#endif





















