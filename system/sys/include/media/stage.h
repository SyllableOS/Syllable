/*  Media Stage class
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
 
#ifndef __F_MEDIA_STAGE_H_
#define __F_MEDIA_STAGE_H_

#include <atheos/types.h>
#include <atheos/time.h>
#include <gui/view.h>
#include <util/string.h>
#include <util/resource.h>
#include <media/packet.h>
#include <media/format.h>
#include <media/input.h>
#include <media/codec.h>
#include <media/output.h>
#include <vector>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media stage typs.
 * \ingroup media
 * \par Description:
 * Types of stages.
 * \author	Arno Klenke
 *****************************************************************************/

enum media_stages
{
	MEDIA_STAGE_OTHER,
	MEDIA_STAGE_INPUT,
	MEDIA_STAGE_CODEC,
	MEDIA_STAGE_OUTPUT,
	MEDIA_STAGE_SYNC
};

#define MEDIA_MAX_INPUT_OUTPUT 8


/** Media Stage.
 * \ingroup media
 * \par Description:
 * The Media Stage class provides a high level api on top of the low level
 * media objects (MediaInput, MediaOutput, MediaCodec). It provides an easy
 * way to create powerful media applications. The basic concept is that the
 * Media stage objects are connected to each other and create a pipeline. One
 * example would be to play an audio file. In this case the pipeline would look
 * like this: Input stage -> Decoder stage -> Sync stage -> Output stage.
 * The Sync stage is a special object that controls the packet flow from the input
 * through the decoder stages to the output. It can also synchronize one audio and
 * one video stream.
 *
 * To create your pipeline just create the stage objects like MediaInputStage or
 * MediaDecoderStage and connect them to each other using their Connect() method.
 * Then call the Initialize() method of every object. If no errors occured then you
 * can now call the Run() method of the sync object in a thread to start the playback.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaStage : public Resource
{
protected:
	virtual ~MediaStage();
public:
	MediaStage();
	virtual uint32			GetPhysicalType();
	virtual String			GetIdentifier();
	virtual uint32			GetInputCount();
	virtual uint32			GetInputFormatCount( uint32 nInput );
	virtual MediaFormat_s	GetInputFormat( uint32 nInput, uint32 nNum );
	virtual uint32			GetOutputCount();
	virtual MediaFormat_s	GetOutputFormat( uint32 nOutput );
	virtual status_t		Connected( uint32 nOutput, MediaStage* pcNextStage, uint32 nInput );
	virtual void			Disconnected( bool bInput, uint32 nInOutput );
	virtual status_t		Connect( MediaStage* pcPrevStage, uint32 nOutput, uint32 nInput );
	virtual status_t		Disconnect( bool bInput, uint32 nInOutput );
	virtual	void			GetPrevStage( uint32 nInput, MediaStage** pcPrev, uint32* pnOutput );
	virtual	void	 		GetNextStage( uint32 nOutput, MediaStage** pcNext, uint32* pnInput );
	virtual	status_t		Initialize();
	virtual bool			HasInternalBuffer();
	virtual status_t		GetPacket( uint32 nOutput, MediaPacket_s* psPacket );
	virtual void			FreePacket( MediaPacket_s* psPacket );
	virtual uint64			GetDelay( bool bNonSharedOnly = false );
	virtual uint64			GetBufferSize( bool bNonSharedOnly = false );
private:
	int m_nRefCount;
	MediaStage*	m_pcPrevStage[MEDIA_MAX_INPUT_OUTPUT];
	uint32		m_nPrevOutput[MEDIA_MAX_INPUT_OUTPUT];
	MediaStage*	m_pcNextStage[MEDIA_MAX_INPUT_OUTPUT];
	uint32		m_nNextInput[MEDIA_MAX_INPUT_OUTPUT];
};

/** Media input stage.
 * \ingroup media
 * \par Description:
 * This media stage object represents an input.
 * \author	Arno Klenke
 *****************************************************************************/

class MediaInputStage : public MediaStage
{
public:
	MediaInputStage();
	~MediaInputStage();
	uint32			GetPhysicalType();
	String			GetIdentifier();
	bool			IsStream();
	uint32			GetTrackCount();
	uint32			SelectTrack( uint32 nTrack );
	String			GetTitle();
	String			GetAuthor();
	String			GetAlbum();	
	String			GetComment();	
	
	uint64			GetLength();
	uint64			GetCurrentPosition();
	uint64			Seek( uint64 nPosition );
	uint32			GetOutputCount();
	MediaFormat_s	GetOutputFormat( uint32 nOutput );
	bool			HasInternalBuffer();
	int				NextPacketStream();
	status_t		GetPacket( uint32 nOutput, MediaPacket_s* psPacket );
	void			FreePacket( MediaPacket_s* psPacket );
	
	void SetInput( MediaInput* pcInput );
	static MediaInputStage* CreateStageForFile( String zFileName );
private:
	void			ClearCache();
	MediaInput* m_pcInput;
	std::vector<MediaPacket_s> m_acPackets;
};

/** Media decoder stage.
 * \ingroup media
 * \par Description:
 * This media stage object represents a decoder.
 * \author	Arno Klenke
 *****************************************************************************/

class MediaDecoderStage : public MediaStage
{
public:
	MediaDecoderStage();
	~MediaDecoderStage();
	uint32			GetPhysicalType();
	String			GetIdentifier();
	uint32			GetInputCount();
	uint32			GetInputFormatCount( uint32 nInput );
	MediaFormat_s	GetInputFormat( uint32 nInput, uint32 nNum );
	uint32			GetOutputCount();
	uint32			GetOutputFormatCount( uint32 nOutput );
	MediaFormat_s	GetOutputFormat( uint32 nOutput );
	status_t		Initialize();
	bool			HasInternalBuffer();	
	status_t		GetPacket( uint32 nOutput, MediaPacket_s* psPacket );
	
	void SetCodec( MediaCodec* pcCodec );
	static MediaDecoderStage* CreateStage();
private:
	MediaCodec* m_pcCodec;
	MediaPacket_s m_sPacket;
	bool m_bAudio;
	bool m_bVideo;
};
/** Media output stage.
 * \ingroup media
 * \par Description:
 * This media stage object represents an output
 * \author	Arno Klenke
 *****************************************************************************/

class MediaOutputStage : public MediaStage
{
public:
	MediaOutputStage();
	~MediaOutputStage();
	uint32			GetPhysicalType();
	String			GetIdentifier();
	uint32			GetInputCount();
	uint32			GetInputFormatCount( uint32 nInput );
	MediaFormat_s	GetInputFormat( uint32 nInput, uint32 nNum );
	status_t		Initialize();
	status_t		GetPacket( uint32 nOutput, MediaPacket_s* psPacket );
	uint64			GetDelay( bool bNonSharedOnly = false );
	uint64			GetBufferSize( bool bNonSharedOnly = false );
	
	os::View*		GetView();
	
	void SetOutput( MediaOutput* pcOutput );
	static MediaOutputStage* CreateDefaultAudioOutputStage( os::String zParameters );
	static MediaOutputStage* CreateDefaultVideoOutputStage( os::String zParameters );
private:
	MediaOutput* m_pcOutput;
};

/** Media sync stage.
 * \ingroup media
 * \par Description:
 * This media stage object manages the packet flow. It has two inputs and two outputs.
 * The first one is for audio and the second one for video. The calls to GetOutputFormat()
 * and GetIntputFormat() are always redirected to the previous/next stage.
 * \author	Arno Klenke
 *****************************************************************************/


class MediaSyncStage : public MediaStage
{
public:
	MediaSyncStage();
	~MediaSyncStage();
	uint32			GetPhysicalType();
	String			GetIdentifier();
	uint32			GetInputCount();
	uint32			GetInputFormatCount( uint32 nInput );
	MediaFormat_s	GetInputFormat( uint32 nInput, uint32 nNum );
	uint32			GetOutputCount();
	uint32			GetOutputFormatCount( uint32 nOutput );
	MediaFormat_s	GetOutputFormat( uint32 nOutput );
	status_t		GetPacket( uint32 nOutput, MediaPacket_s* psPacket );
	
	status_t		Initialize();
	int				Run();

	static MediaSyncStage* CreateStage();
private:
	class Private;
	Private* m;
};

}

#endif




















































