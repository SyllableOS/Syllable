/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org

    Modified in Oct 2004 by Hannu Savolainen 
    hannu@opensound.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Allow access to a raw mixing buffer */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <atheos/kernel.h>
#ifdef OSS_USE_SOUNDCARD_H
/* This is installed on some systems */
#include <soundcard.h>
#else
/* This is recommended by OSS */
#include <atheos/soundcard.h>
#endif

extern "C"
{
	#include "SDL_audio.h"
	#include "SDL_error.h"
	#include "SDL_audiomem.h"
	#include "SDL_audio_c.h"
	#include "SDL_timer.h"
	#include "SDL_audiodev_c.h"
};
#include "SDL_dspaudio.h"

/* The tag name used by DSP audio */
#define DSP_DRIVER_NAME         "syllable"

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS	(O_WRONLY|O_NONBLOCK)

//#define DEBUG_AUDIO

/* Audio driver functions */
static int DSP_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void DSP_WaitAudio(_THIS);
static void DSP_PlayAudio(_THIS);
static Uint8 *DSP_GetAudioBuf(_THIS);
static void DSP_CloseAudio(_THIS);

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{

	os::MediaManager* pcManager = os::MediaManager::Get();
	if( pcManager == NULL )
		return( 0 );
	os::MediaOutput* pcOutput = pcManager->GetDefaultAudioOutput();
	if( pcOutput == NULL )
		return( 0 );
	delete( pcOutput );
	return( 1 );
}

static void Audio_DeleteDevice(_THIS)
{
	delete( output );
	manager->Put();
	free(_this->hidden);
	free(_this);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *_this;

	/* Initialize all variables that we clean on shutdown */
	_this = (SDL_AudioDevice *)malloc(sizeof(SDL_AudioDevice));
	if ( _this ) {
		memset(_this, 0, (sizeof *_this));
		_this->hidden = (struct SDL_PrivateAudioData *)
				malloc((sizeof *_this->hidden));
	}
	if ( (_this == NULL) || (_this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( _this ) {
			free(_this);
		}
		return(0);
	}
	memset(_this->hidden, 0, (sizeof *_this->hidden));
	//audio_fd = -1;
	manager = os::MediaManager::Get();

	/* Set the function pointers */
	_this->OpenAudio = DSP_OpenAudio;
	_this->WaitAudio = DSP_WaitAudio;
	_this->PlayAudio = DSP_PlayAudio;
	_this->GetAudioBuf = DSP_GetAudioBuf;
	_this->CloseAudio = DSP_CloseAudio;

	_this->free = Audio_DeleteDevice;

	return _this;
}

AudioBootStrap SYLLABLE_Audio_bootstrap = {
	DSP_DRIVER_NAME, "Syllable audio",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void DSP_WaitAudio(_THIS)
{
	output->Flush();
	if( output )
	{
		while( output->GetUsedBufferPercentage() > 20 || output->GetDelay() > 100 )
		{
			output->Flush();
			snooze( 1000 );
		}
	}
}

static void DSP_PlayAudio(_THIS)
{
	if( output == NULL )
		return;
	os::MediaPacket_s packet;
	//printf("Write packet %i %i\n", mixlen, (int)output->GetDelay() );
	packet.nStream = 0;
	packet.nTimeStamp = ~0;
	packet.pBuffer[0] = mixbuf;
	packet.nSize[0] = mixlen;
	output->WritePacket( 0, &packet );
	output->Flush();
#if 0
	if (write(audio_fd, mixbuf, mixlen)==-1)
	{
		perror("Audio write");
		this->enabled = 0;
	}
#endif
#ifdef DEBUG_AUDIO
	fprintf(stderr, "Wrote %d bytes of audio data\n", mixlen);
#endif
}

static Uint8 *DSP_GetAudioBuf(_THIS)
{
	return(mixbuf);
}

static void DSP_CloseAudio(_THIS)
{
	if ( mixbuf != NULL ) {
		SDL_FreeAudioMem(mixbuf);
		mixbuf = NULL;
	}

	if( output )
	{
		output->Close();
		delete( output );
		output = NULL;
	}
}

static int DSP_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	os::MediaFormat_s format;
	MEDIA_CLEAR_FORMAT( format );

	output = manager->GetDefaultAudioOutput();
	if( output == NULL )
		return( -1 );

	spec->format = AUDIO_S16LSB;
	
	format.nType = os::MEDIA_TYPE_AUDIO;
	format.zName = "Raw Audio";
	format.nChannels = spec->channels;
	format.nSampleRate = spec->freq;
	if( output->Open("") != 0 ) {
		delete( output );
		output = NULL;
		return( -1 );
	}
	if( output->AddStream( "SDL", format ) != 0 ) {
		output->Close();
		delete( output );
		output = NULL;
		return( -1 );
	}

	/* Allocate mixing buffer */
	mixlen = spec->size;
	mixbuf = (Uint8 *)SDL_AllocAudioMem(mixlen);
	if ( mixbuf == NULL ) {
		return(-1);
	}
	memset(mixbuf, spec->silence, spec->size);

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* We're ready to rock and roll. :-) */
	return(0);
}
