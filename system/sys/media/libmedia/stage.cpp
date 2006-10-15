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

#include <media/stage.h>
#include <media/manager.h>
#include <cassert>

using namespace os;

MediaStage::MediaStage()
{
	m_nRefCount = 1;
	for( uint i = 0; i < MEDIA_MAX_INPUT_OUTPUT; i++ )
	{		
		m_pcPrevStage[i] = m_pcNextStage[i] = NULL;
	}
}

MediaStage::~MediaStage()
{
	for( uint i = 0; i < MEDIA_MAX_INPUT_OUTPUT; i++ )
	{	
		if( m_pcPrevStage[i] != NULL )
		{
			m_pcPrevStage[i]->Disconnected( false, m_nPrevOutput[i] );
		}
		if( m_pcNextStage[i] != NULL )
		{
			m_pcNextStage[i]->Disconnected( true, m_nNextInput[i] );
		}
	}
}


/** Physical type.
 * \par Description:
 * Returns the physical of the stage.
 * \author	Arno Klenke
 ******************************************************************************/
uint32 MediaStage::GetPhysicalType()
{
	return( MEDIA_PHY_UNKNOWN );
}

/** Identifier.
 * \par Description:
 * Returns the identifier of the object.
 * \author	Arno Klenke
 *****************************************************************************/
String MediaStage::GetIdentifier()
{
	return( "Unknown" );
}


/** Number of inputs.
 * \par Description:
 * Returns the number of inputs.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaStage::GetInputCount()
{
	return( 0 );
}

/** Number of input formats.
 * \par Description:
 * Returns the number of formats of one input.
 * \param nInput - The input.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaStage::GetInputFormatCount( uint32 nInput )
{
	return( 0 );
}

/** Input format.
 * \par Description:
 * Returns one input format.
 * \param nInput - The input.
 * \param nNum - Input format number.
 * \author	Arno Klenke
 *****************************************************************************/
MediaFormat_s MediaStage::GetInputFormat( uint32 nInput, uint32 nNum )
{
	MediaFormat_s sDummy;
	MEDIA_CLEAR_FORMAT( sDummy );
	return( sDummy );
}

/** Number of outputs.
 * \par Description:
 * Returns the number of outputs.
 * \author	Arno Klenke
 *****************************************************************************/
uint32 MediaStage::GetOutputCount()
{
	return( 0 );
}

/** Output format.
 * \par Description:
 * Returns the format of one output.
 * \param nOutput - The output.
 * \author	Arno Klenke
 *****************************************************************************/
MediaFormat_s MediaStage::GetOutputFormat( uint32 nOutput )
{
	MediaFormat_s sDummy;
	MEDIA_CLEAR_FORMAT( sDummy );
	return( sDummy );
}

/** Called when the media stage is connected to the next one
 * \par Description:
 * Called when the media stage is connected to the next one.
 * \param nOutput - Output of the stage.
 * \param pcNextStage - The stage that has been connected.
 * \param nInput - Input of the connected stage.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaStage::Connected( uint32 nOutput, MediaStage* pcNextStage, uint32 nInput )
{
	printf( "Stage %s now connected to %s\n", GetIdentifier().c_str(), pcNextStage->GetIdentifier().c_str() );
	m_pcNextStage[nOutput] = pcNextStage;
	m_nNextInput[nOutput] = nInput;
	return( 0 );
}

/** Called when the media stage is disconnected from the next or previous one.
 * \par Description:
 * Called when the media stage is disconnected from the next or previous one.
 * \param bInput - If true then the stage was disconnected from the previous one.
 *					Else from the next one.
 * \param nInOutput - The input (bInput=true) or output (bInput=false)
 * \author	Arno Klenke
 *****************************************************************************/
void MediaStage::Disconnected( bool bInput, uint32 nInOutput )
{
	if( bInput ) {
		printf( "Stage %s Input %i disconnected\n", GetIdentifier().c_str(), nInOutput );
		m_pcPrevStage[nInOutput] = NULL;
		m_nPrevOutput[nInOutput] = 0;
	} else {
		printf( "Stage %s Output %i disconnected\n", GetIdentifier().c_str(), nInOutput );
		m_pcNextStage[nInOutput] = NULL;
		m_nNextInput[nInOutput] = 0;
	}
}

/** Connect the stage to the previous one.
 * \par Description:
 * Connect the stage to a previous one.
 * \param pcPrevStage - The previous stage.
 * \param nOutput - Output of the previous stage.
 * \param nInput - Input to which the previous stage should be connected.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaStage::Connect( MediaStage* pcPrevStage, uint32 nOutput, uint32 nInput )
{
	assert( pcPrevStage != NULL );
	printf( "Connect %s:%i to %s:%i\n", GetIdentifier().c_str(), (int)nInput, pcPrevStage->GetIdentifier().c_str(), (int)nOutput );
	m_pcPrevStage[nInput] = pcPrevStage;
	m_nPrevOutput[nInput] = nOutput;
	return( pcPrevStage->Connected( nOutput, this, nInput ) );
}

/** Disconnect the stage from the previous/next stage.
 * \par Description:
 * Disconnect the stage from the previous/next stage.
 * \param bInput - If true then the stage will be disconnected from the previous one.
 *					Else from the next one.
 * \param nInOutput - The input (bInput=true) or output (bInput=false)
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaStage::Disconnect( bool bInput, uint32 nInOutput )
{
	if( bInput ) {
		printf( "Stage %s Input %i disconnect\n", GetIdentifier().c_str(), nInOutput );
		m_pcPrevStage[nInOutput]->Disconnected( false, m_nPrevOutput[nInOutput] );
		m_pcPrevStage[nInOutput] = NULL;
		m_nPrevOutput[nInOutput] = 0;
	} else {
		printf( "Stage %s Output %i disconnect\n", GetIdentifier().c_str(), nInOutput );
		m_pcNextStage[nInOutput]->Disconnected( true, m_nNextInput[nInOutput] );
		m_pcNextStage[nInOutput] = NULL;
		m_nNextInput[nInOutput] = 0;
	}
	return( 0 );
}

/** Get the previous stage.
 * \par Description:
 * Get the previous stage.
 * \param nInput - The input.
 * \param pcPrev - Contains a pointer to the previous stage after the call.
 * \param pnOutput - Contains the output of the previous stage after the call.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaStage::GetPrevStage( uint32 nInput, MediaStage** pcPrev, uint32* pnOutput )
{
	*pcPrev = m_pcPrevStage[nInput];
	*pnOutput = m_nPrevOutput[nInput];
}

/** Get the next stage.
 * \par Description:
 * Get the next stage.
 * \param nOutput - The output.
 * \param pcNext - Contains a pointer to the next stage after the call.
 * \param pnInput - Contains the input of the next stage after the call.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaStage::GetNextStage(uint32 nOutput, MediaStage** pcNext, uint32* pnInput )
{
	*pcNext = m_pcNextStage[nOutput];
	*pnInput = m_nNextInput[nOutput];
}

/** Initializes the stage.
 * \par Description:
 * This method has to be called for every stage object after you have 
 * connected all the stages.
 * \author	Arno Klenke
 ******************************************************************************/
status_t MediaStage::Initialize()
{
	return( 0 );
}

/** Internal buffer.
 * \par Description:
 * Return whether the data pointer of the packets read with GetPacket() point 
 * to internal memory of the stage object.
 * \author	Arno Klenke
 ******************************************************************************/
bool MediaStage::HasInternalBuffer()
{
	return( false );
}

/** Read one packet.
 * \par Description:
 * Reads one packet from the stage. The stage object will read data from the
 * previous stages if necessary. You normally do not have to call this method directly
 * because the MediaSyncStage takes care of this.
 * \param nOutput - The output.
 * \param psPacket - Pointer to a packet.
 * \author	Arno Klenke
 ******************************************************************************/
status_t MediaStage::GetPacket( uint32 nOutput, MediaPacket_s* psPacket )
{
	return( -ENODEV );
}

/** Frees one packet.
 * \par Description:
 * Frees one packet. You normally do not have to call this method directly
 * because the MediaSyncStage takes care of this.
 * \param psPacket - Pointer to a packet.
 * \author	Arno Klenke
 ******************************************************************************/
void MediaStage::FreePacket( MediaPacket_s* psPacket )
{
}

/** Delay.
 * \par Description:
 * Returns the delay of the stage.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaStage::GetDelay()
{
	return( 0 );
}

/** Buffer size.
 * \par Description:
 * Returns the buffer size of the stage.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaStage::GetBufferSize()
{
	return( 0 );
}




MediaInputStage::MediaInputStage()
{
	m_pcInput = NULL;
}

MediaInputStage::~MediaInputStage()
{
	ClearCache();
	if( m_pcInput )
		m_pcInput->Release();
}

/** Clear cache.
 * \par Description:
 * Clears the cache of the input.
 * \author	Arno Klenke
 ******************************************************************************/
void MediaInputStage::ClearCache()
{
	for( uint i = 0; i < m_acPackets.size(); i++ )
	{
		m_pcInput->FreePacket( &m_acPackets[i] );
	}
	m_acPackets.clear();
}

uint32 MediaInputStage::GetPhysicalType()
{
	if( m_pcInput )
		return( m_pcInput->GetPhysicalType() );
	return( MEDIA_PHY_UNKNOWN );
}

String MediaInputStage::GetIdentifier()
{
	assert( m_pcInput );
	return( m_pcInput->GetIdentifier() );
}

/** Stream.
 * \par Description:
 * Returns whether the input is a stream and does not support seeking.
 * \author	Arno Klenke
 ******************************************************************************/
bool MediaInputStage::IsStream()
{
	assert( m_pcInput );
	return( m_pcInput->StreamBased() );
}

/** Number of tracks.
 * \par Description:
 * Returns the number of tracks.
 * \author	Arno Klenke
 ******************************************************************************/
uint32 MediaInputStage::GetTrackCount()
{
	assert( m_pcInput );
	return( m_pcInput->GetTrackCount() );
}

/** Select one track.
 * \par Description:
 * Selects one track.
 * \param nTrack - The track.
 * \author	Arno Klenke
 ******************************************************************************/
uint32 MediaInputStage::SelectTrack( uint32 nTrack )
{
	assert( m_pcInput );
	ClearCache();
	return( m_pcInput->SelectTrack( nTrack ) );
}

/** Length of the file.
 * \par Description:
 * Returns the length of the file in seconds.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaInputStage::GetLength()
{
	assert( m_pcInput );
	return( m_pcInput->GetLength() );
}

/** Current position.
 * \par Description:
 * Returns the current position.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaInputStage::GetCurrentPosition()
{
	assert( m_pcInput );
	return( m_pcInput->GetCurrentPosition() );
}

/** Seek.
 * \par Description:
 * Seeks to a position in the file.
 * \param nPosition - Position.
 * \author	Arno Klenke
 ******************************************************************************/

uint64 MediaInputStage::Seek( uint64 nPosition )
{
	assert( m_pcInput );
	ClearCache();
	return( m_pcInput->Seek( nPosition ) );
}

uint32 MediaInputStage::GetOutputCount()
{
	assert( m_pcInput );
	return( m_pcInput->GetStreamCount() );
}
MediaFormat_s MediaInputStage::GetOutputFormat( uint32 nOutput )
{
	assert( m_pcInput );
	return( m_pcInput->GetStreamFormat( nOutput ) );
}

bool MediaInputStage::HasInternalBuffer()
{
	return( false );
}

status_t MediaInputStage::GetPacket( uint32 nOutput, MediaPacket_s* psPacket )
{
	// FIXME
	assert( m_pcInput );
	MediaPacket_s sPacket;
	
	/* Fill queue */
	while( m_acPackets.size() < 20 )
	{
		if( m_pcInput->ReadPacket( &sPacket ) == 0 )
		{
			//printf( "Fill Read Packet at %i:%i:%i\n", (int)m_acPackets.size(), sPacket.nStream, (int)sPacket.nSize[0] );
			m_acPackets.push_back( sPacket );
		}
		else
			break;
	}	
	
	
	/* Find packet */
	for( uint i = 0; i < m_acPackets.size(); i++ )
	{
		if( m_acPackets[i].nStream == nOutput )
		{
			*psPacket = m_acPackets[i];
			m_acPackets.erase( m_acPackets.begin() + i );
			//printf("Found packet at position %i\n", i );
			return( 0 );
		}
	}
	
	/* Read more packets */
	while( m_acPackets.size() < 50 )
	{
		if( m_pcInput->ReadPacket( &sPacket ) == 0 )
		{
			//printf( "Fill Read Packet at %i:%i:%i\n", (int)m_acPackets.size(), sPacket.nStream, (int)sPacket.nSize[0] );
			if( sPacket.nStream == nOutput )
			{
				*psPacket = sPacket;
				return( 0 );
			}
			m_acPackets.push_back( sPacket );
		}
		else
			break;
	}	
	
	return( -EIO );
}

void MediaInputStage::FreePacket( MediaPacket_s* psPacket )
{
	assert( m_pcInput );
	return( m_pcInput->FreePacket( psPacket ) );
}
	
void MediaInputStage::SetInput( MediaInput* pcInput )
{
	m_pcInput = pcInput;
}

/** Creates a stage object for a file.
 * \par Description:
 * Creates a stage object for a file.
 * \param zFileName - The filename.
 * \author	Arno Klenke
 ******************************************************************************/
MediaInputStage* MediaInputStage::CreateStageForFile( String zFileName )
{
	MediaManager* pcManager = MediaManager::Get();
	if( pcManager == NULL )
		return( NULL );
	MediaInput* pcInput = pcManager->GetBestInput( zFileName );
	pcManager->Put();
	if( pcInput == NULL || pcInput->Open( zFileName ) != 0 )
		return( NULL );
	if( !pcInput->PacketBased() )
	{
		pcInput->Release();
		return( NULL );
	}
	/* FIXME */

	pcInput->SelectTrack( 0 );
	MediaInputStage* pcStage = new MediaInputStage();
	pcStage->SetInput( pcInput );
	return( pcStage );
}

MediaDecoderStage::MediaDecoderStage()
{
	m_pcCodec = NULL;
	m_bAudio = m_bVideo = false;
}


MediaDecoderStage::~MediaDecoderStage()
{
	if( m_bAudio )
		m_pcCodec->DeleteAudioOutputPacket( &m_sPacket );
	if( m_bVideo )
		m_pcCodec->DeleteVideoOutputPacket( &m_sPacket );
	if( m_pcCodec )
		m_pcCodec->Release();
}

uint32 MediaDecoderStage::GetPhysicalType()
{
	if( m_pcCodec )
		return( m_pcCodec->GetPhysicalType() );
	return( MEDIA_PHY_UNKNOWN );
}

String MediaDecoderStage::GetIdentifier()
{
	if( m_pcCodec )
		return( m_pcCodec->GetIdentifier() );
	return( "Decoder Stage" );
}

uint32 MediaDecoderStage::GetInputCount()
{
	return( 1 );
}

uint32 MediaDecoderStage::GetInputFormatCount( uint32 nInput )
{
	return( 1 );
}

MediaFormat_s MediaDecoderStage::GetInputFormat( uint32 nInput, uint32 nNum )
{
	// FIXME
	MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	return( sFormat );
}

uint32 MediaDecoderStage::GetOutputCount()
{
	return( 1 );
}

MediaFormat_s MediaDecoderStage::GetOutputFormat( uint32 nOutput )
{
	if( m_pcCodec == NULL )
	{
		MediaFormat_s sFormat;
		MEDIA_CLEAR_FORMAT( sFormat );
		return( sFormat );
	}
	return( m_pcCodec->GetExternalFormat() );
}

static void DumpFormat( os::MediaFormat_s sFormat )
{
	printf( "Type: %s\n", sFormat.nType == MEDIA_TYPE_AUDIO ? "Audio" : "Video" );
	printf( "Type: %s\n", sFormat.zName.c_str() );
	printf( "Bitrate: %i\n", sFormat.nBitRate );
	if( sFormat.nType == MEDIA_TYPE_AUDIO )
	{
		printf( "SampleRate: %i\n", sFormat.nSampleRate );
		printf( "Channels: %i\n", sFormat.nChannels );
	}
	if( sFormat.nType == MEDIA_TYPE_VIDEO )
	{
		printf( "Size: %ix%i\n", sFormat.nWidth, sFormat.nHeight );
		printf( "Framerate: %f\n", sFormat.vFrameRate );
	}
}

status_t MediaDecoderStage::Initialize()
{
	printf( "Looking for best codec...\n" );
	if( m_pcCodec == NULL )
	{
		MediaStage* pcNext;
		MediaStage* pcPrev;
		uint32 nNextInput;
		uint32 nPrevOutput;
		GetNextStage( 0, &pcNext, &nNextInput );
		if( pcNext == NULL )
			return( -EINVAL );
		
		GetPrevStage( 0, &pcPrev, &nPrevOutput );
		if( pcPrev == NULL )
			return( -EINVAL );
		MediaManager* pcManager = MediaManager::Get();
		if( pcManager == NULL )
			return( -EINVAL );
		MediaCodec* pcCodec = NULL;
		uint j;
		for( j = 0; j < pcNext->GetInputFormatCount( nNextInput ); j++ )
		{
			pcCodec = pcManager->GetBestCodec( pcPrev->GetOutputFormat( nPrevOutput ), pcNext->GetInputFormat( nNextInput, j ), false );
			if( pcCodec != NULL )
			{
				printf( "Found matching codec for %s ( %s ) -> %s ( %s )\n", pcPrev->GetIdentifier().c_str(), pcPrev->GetOutputFormat( nPrevOutput ).zName.c_str(),
						pcNext->GetIdentifier().c_str(), pcNext->GetInputFormat( nNextInput, j ).zName.c_str() );
				break;
			}
		}
		if( pcCodec == NULL )
		{
			printf( "Could not find codec!" );
			return( -EINVAL );
		}
		
		
		pcCodec->Open( pcPrev->GetOutputFormat( nPrevOutput ), pcNext->GetInputFormat( nNextInput, j ), false );
		if( pcCodec->GetExternalFormat().nType == MEDIA_TYPE_AUDIO )
		{
			printf( "Creating audio output packet!\n" );
			if( pcCodec->CreateAudioOutputPacket( &m_sPacket ) != 0 )
			{
				pcCodec->Release();
				return( -EINVAL );
			}
			m_bAudio = true;
		}
		else if( pcCodec->GetExternalFormat().nType == MEDIA_TYPE_VIDEO )
		{
			printf( "Creating video output packet!\n" );
			if( pcCodec->CreateVideoOutputPacket( &m_sPacket ) != 0 )
			{
				pcCodec->Release();
				return( -EINVAL );
			}
			m_bVideo = true;
		}
		m_pcCodec = pcCodec;
		printf( "Found codec %s\n", pcCodec->GetIdentifier().c_str() );
		printf( "*****Input format*****\n" );
		DumpFormat( pcCodec->GetInternalFormat() );
		printf( "*****Output format******\n" );
		DumpFormat( pcCodec->GetExternalFormat() );		
		pcManager->Put();
	}
	return( 0 );
}

bool MediaDecoderStage::HasInternalBuffer()
{
	return( true );
}

status_t MediaDecoderStage::GetPacket( uint32 nOutput, MediaPacket_s* psPacket )
{
	/* Get previous packet */
	MediaStage* pcPrev;
	uint32 nPrevOutput;
	GetPrevStage( nOutput, &pcPrev, &nPrevOutput );
	assert( pcPrev );
	assert( m_pcCodec );
	
	MediaPacket_s sPacket;
	if( pcPrev->GetPacket( nPrevOutput, &sPacket ) != 0 )
	{
		printf( "ReadPacket error!\n" );
		return( -EIO );
	}
	if( m_pcCodec->DecodePacket( &sPacket, &m_sPacket) != 0 )
	{
		pcPrev->FreePacket( &sPacket );
		printf( "Decode error!\n" );
		return( -EIO );
	}
	pcPrev->FreePacket( &sPacket );
	//printf( "Input size %i Output size %i Timestamp %i\n", sPacket.nSize[0], m_sPacket.nSize[0], sPacket.nTimeStamp );
	*psPacket = m_sPacket;
	return( 0 );
}
	

void MediaDecoderStage::SetCodec( MediaCodec* pcCodec )
{
	m_pcCodec = pcCodec;
}

/** Creates a decoder stage object.
 * \par Description:
 * Creates a decoder stage object. The object will choose the right codec to
 * convert the data between the input and the output.
 * \author	Arno Klenke
 ******************************************************************************/
MediaDecoderStage* MediaDecoderStage::CreateStage()
{
	return( new MediaDecoderStage() );
}

MediaOutputStage::MediaOutputStage()
{
	m_pcOutput = NULL;
	
}

MediaOutputStage::~MediaOutputStage()
{
	if( m_pcOutput );
		m_pcOutput->Release();
}

uint32 MediaOutputStage::GetPhysicalType()
{
	if( m_pcOutput )
		return( m_pcOutput->GetPhysicalType() );
	return( MEDIA_PHY_UNKNOWN );
}

String MediaOutputStage::GetIdentifier()
{
	assert( m_pcOutput );
	return( m_pcOutput->GetIdentifier() );
}

uint32 MediaOutputStage::GetInputCount()
{
	assert( m_pcOutput );
	return( m_pcOutput->GetSupportedStreamCount() );
}

uint32 MediaOutputStage::GetInputFormatCount( uint32 nInput )
{
	assert( m_pcOutput );
	return( m_pcOutput->GetOutputFormatCount() );
}

MediaFormat_s MediaOutputStage::GetInputFormat( uint32 nInput, uint32 nNum )
{
	// FIXME
	assert( m_pcOutput );
	return( m_pcOutput->GetOutputFormat( nNum ) );
}

status_t MediaOutputStage::Initialize()
{
	assert( m_pcOutput );
	for( uint i = 0; i < m_pcOutput->GetSupportedStreamCount(); i++ )
	{
		MediaStage* pcPrev;
		uint32 nPrevOutput;
		GetPrevStage( i, &pcPrev, &nPrevOutput );
		if( pcPrev != NULL )
		{
			m_pcOutput->AddStream( "Output", pcPrev->GetOutputFormat( nPrevOutput ) );
		}
	}
	return( 0 );
}

status_t MediaOutputStage::GetPacket( uint32 nOutput, MediaPacket_s* psPacket )
{
	/* Get packet from previous stage */
	MediaStage* pcPrev;
	uint32 nPrevOutput;
	GetPrevStage( nOutput, &pcPrev, &nPrevOutput );
	assert( pcPrev );
	assert( m_pcOutput );
	
	MediaPacket_s sPacket;
	if( pcPrev->GetPacket( nPrevOutput, &sPacket ) != 0 )
	{
		printf( "Output:: ReadPacket error!\n" );
		return( -EIO );
	}
	
	if( sPacket.nSize[0] > 0 )
	{
		//printf( "Write %i Timestamp %i\n", sPacket.nSize[0], sPacket.nTimeStamp );
		m_pcOutput->WritePacket( nOutput, &sPacket );
	}
	
	pcPrev->FreePacket( &sPacket );
	
	return( 0 );
}

uint64 MediaOutputStage::GetDelay()
{
	assert( m_pcOutput );
	return( m_pcOutput->GetDelay() );
}

uint64 MediaOutputStage::GetBufferSize()
{
	assert( m_pcOutput );
	return( m_pcOutput->GetBufferSize() );
}

os::View* MediaOutputStage::GetView()
{
	assert( m_pcOutput );
	return( m_pcOutput->GetVideoView( 0 ) );
}
	
void MediaOutputStage::SetOutput( MediaOutput* pcOutput )
{
	m_pcOutput = pcOutput;
}

/** Creates a output stage for the default audio output.
 * \par Description:
 * Creates a output stage for the default audio output. 
 * \author	Arno Klenke
 ******************************************************************************/
MediaOutputStage* MediaOutputStage::CreateDefaultAudioOutputStage()
{
	MediaManager* pcManager = MediaManager::Get();
	if( pcManager == NULL )
		return( NULL );
	MediaOutput* pcOutput = pcManager->GetDefaultAudioOutput();
	pcManager->Put();
	if( pcOutput == NULL || pcOutput->Open( "" ) != 0 )
		return( NULL );

	MediaOutputStage* pcStage = new MediaOutputStage();
	pcStage->SetOutput( pcOutput );
	return( pcStage );
}

/** Creates a output stage for the default video output.
 * \par Description:
 * Creates a output stage for the default video output. 
 * \author	Arno Klenke
 ******************************************************************************/
MediaOutputStage* MediaOutputStage::CreateDefaultVideoOutputStage()
{
	MediaManager* pcManager = MediaManager::Get();
	if( pcManager == NULL )
		return( NULL );
	MediaOutput* pcOutput = pcManager->GetDefaultVideoOutput();
	pcManager->Put();
	if( pcOutput == NULL || pcOutput->Open( "" ) != 0 )
		return( NULL );

	MediaOutputStage* pcStage = new MediaOutputStage();
	pcStage->SetOutput( pcOutput );
	return( pcStage );
}


MediaSyncStage::MediaSyncStage()
{
}


MediaSyncStage::~MediaSyncStage()
{
}

uint32 MediaSyncStage::GetPhysicalType()
{
	return( MEDIA_PHY_SOFT_SYNC );
}

String MediaSyncStage::GetIdentifier()
{
	return( "Sync Stage" );
}

uint32 MediaSyncStage::GetInputCount()
{
	return( 2 );
}

uint32 MediaSyncStage::GetInputFormatCount( uint32 nInput )
{
	uint32 nNextInput;
	MediaStage* pcStage = NULL;
	GetNextStage( nInput, &pcStage, &nNextInput );
	if( pcStage == NULL )
		return( 0 );
		
	return( pcStage->GetInputFormatCount( nNextInput ) );
}

MediaFormat_s MediaSyncStage::GetInputFormat( uint32 nInput, uint32 nNum )
{
	MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	uint32 nNextInput;
	MediaStage* pcStage = NULL;
	GetNextStage( nInput, &pcStage, &nNextInput );
	if( pcStage == NULL )
	{
		sFormat.nType = ( nInput == 1 ? MEDIA_TYPE_VIDEO : MEDIA_TYPE_AUDIO ); 
		return( sFormat );
	}
		
	printf( "GetInputFormat() passthrough %i -> %s:%i\n", nInput, pcStage->GetIdentifier().c_str(), nNextInput );
		
	return( pcStage->GetInputFormat( nNextInput, nNum ) );
}

uint32 MediaSyncStage::GetOutputCount()
{
	return( 2 );
}

MediaFormat_s MediaSyncStage::GetOutputFormat( uint32 nOutput )
{
	MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	uint32 nPrevOutput;
	MediaStage* pcStage = NULL;
	GetPrevStage( nOutput, &pcStage, &nPrevOutput );
	if( pcStage == NULL )
		return( sFormat );
		
	printf( "GetOutputFormat() passthrough %i -> %s:%i\n", nOutput, pcStage->GetIdentifier().c_str(), nPrevOutput );
		
	return( pcStage->GetOutputFormat( nPrevOutput ) );
}


status_t MediaSyncStage::GetPacket( uint32 nOutput, MediaPacket_s* psPacket )
{
	switch( nOutput )
	{
		case 0: // Audio data:
			if( m_bAudio )
			{
				assert( m_bAudioValid );
				*psPacket = m_sAudioPacket;			
				m_bAudioValid = false;
				return( 0 );
			}
		break;
		case 1: // Video data:
			if( m_bVideo )
			{
				assert( m_bVideoValid );
				*psPacket = m_sVideoPacket;			
				m_bVideoValid = false;
				return( 0 );
			}
		break;
	}
	
	return( -EIO );
	
}

status_t MediaSyncStage::Initialize()
{
	m_bAudio = m_bVideo = m_bAudioValid = m_bVideoValid = false;
	m_nPlayTime = 0;
	
	/* Check connected stages */
	uint32 nPrevOutput;
	MediaStage* pcStage = NULL;
	GetPrevStage( 0, &pcStage, &nPrevOutput );
	
	if( pcStage != NULL )
		m_bAudio = true;
	
	GetPrevStage( 1, &pcStage, &nPrevOutput );
	
	if( pcStage != NULL )
		m_bVideo = true;
	
	return( 0 );
}

/** Run.
 * \par Description:
 * This method will fetch the audio and/or video packets from the connected inputs
 * and codecs and pass them to the connected outputs at the right time. This method
 * should be called from a separate thread and might block.
 * \author	Arno Klenke
 ******************************************************************************/
status_t MediaSyncStage::Run()
{
	
	uint32 nVidPrevOutput;
	MediaStage* pcVidPrevStage = NULL;
	uint32 nVidNextInput;
	MediaStage* pcVidNextStage = NULL;
	
	uint32 nAudPrevOutput;
	MediaStage* pcAudPrevStage = NULL;
	uint32 nAudNextInput;
	MediaStage* pcAudNextStage = NULL;
	
	if( m_bVideo )
	{
		GetPrevStage( 1, &pcVidPrevStage, &nVidPrevOutput );
		GetNextStage( 1, &pcVidNextStage, &nVidNextInput );
		assert( pcVidPrevStage );
		assert( pcVidNextStage );
	}
	
	
	if( m_bAudio )
	{
		GetPrevStage( 0, &pcAudPrevStage, &nAudPrevOutput );
		GetNextStage( 0, &pcAudNextStage, &nAudNextInput );
		assert( pcAudPrevStage );
		assert( pcAudNextStage );
	}
	
	
	/* Read Video frame */
	if( m_bVideo && !m_bVideoValid )
	{
		video_again:		
		if( pcVidPrevStage->GetPacket( nVidPrevOutput, &m_sVideoPacket ) != 0 )
		{
			printf( "Sync:: GetPacket() error!\n" );
			return( -EIO );
		}
		
		if ( m_sVideoPacket.nSize[0] > 0 )
		{
			m_bVideoValid = true;
		}
		else
			goto video_again;
		
	}
			
	/* Show videoframe */
	if( m_bVideo && m_bVideoValid )
	{
		/* Calculate current time */
		bigtime_t nCurrentTime = m_nPlayTime;
		if( m_bAudio )
		 	nCurrentTime -= pcAudNextStage->GetDelay();
	 
		uint64 nVideoFrameLength = 1000 / (uint64)pcVidPrevStage->GetOutputFormat( nVidPrevOutput ).vFrameRate;
			 
		if( (uint64)nCurrentTime > m_sVideoPacket.nTimeStamp + nVideoFrameLength * 2 )
		{
			printf( "Droping Frame %i %i!\n", (int)m_sVideoPacket.nTimeStamp, (int)nCurrentTime );
			pcVidPrevStage->FreePacket( &m_sVideoPacket );
			m_bVideoValid = false;
		}
		else if( !( (uint64)nCurrentTime < m_sVideoPacket.nTimeStamp ) )
		{
			/* Write video frame */
			//printf( "Write video frame %i %i\n", (int)m_sVideoPacket.nTimeStamp, (int)nCurrentTime );
			pcVidNextStage->GetPacket( nVidNextInput, NULL );
			m_bVideoValid = false;						
		}
	}
	
	
	/* Read audio packet */
	if( m_bAudio && !m_bAudioValid )
	{
audio_again:		
		if( pcAudPrevStage->GetPacket( nAudPrevOutput, &m_sAudioPacket ) != 0 )
		{
			printf( "Sync:: GetPacket() error!\n" );
			return( -EIO );
		}
		
		if ( m_sAudioPacket.nSize[0] > 0 )
		{
			m_bAudioValid = true;
		}
		else
			goto audio_again;
	}
	
		
	/* Write audio */
	
	uint64 nDelay = pcAudNextStage->GetDelay();
	uint64 nBufferSize = pcAudNextStage->GetBufferSize();
	if( m_bAudio && m_bAudioValid && ( nDelay < ( nBufferSize / 2 ) ) )
	{
		pcAudNextStage->GetPacket( nAudNextInput, NULL );
		MediaFormat_s sAudioFormat = pcAudPrevStage->GetOutputFormat( nAudPrevOutput );
	
		bigtime_t nAudioLength = (bigtime_t)m_sAudioPacket.nSize[0] * 1000;
		nAudioLength /= bigtime_t( 2 * sAudioFormat.nChannels * sAudioFormat.nSampleRate );
		
		if( m_sAudioPacket.nTimeStamp != (uint64)( ~0 ) )
			m_nPlayTime = m_sAudioPacket.nTimeStamp;
						
		m_bAudioValid = false;
		m_nPlayTime += nAudioLength;
	}
	return( 0 );	
}

/** Creates a sync statge.
 * \par Description:
 * Creates a sync stage.
 * \author	Arno Klenke
 ******************************************************************************/
MediaSyncStage* MediaSyncStage::CreateStage()
{
	return( new MediaSyncStage() );
}





















