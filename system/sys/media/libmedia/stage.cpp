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

#define RETURN_IF_NULL( x, y ) if( ( x ) == NULL ) return( y )

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
 * \param bNonSharedOnly - If the output has shared and non-shared buffers then
 *					  true will return the buffer size of the non-shared buffer only.
 *					  Otherwise the complete buffer size will be returned.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaStage::GetDelay( bool bNonSharedOnly )
{
	return( 0 );
}

/** Buffer size.
 * \par Description:
 * Returns the buffer size of the stage.
 * \param bNonSharedOnly - If the output has shared and non-shared buffers then
 *					  true will return the buffer size of the non-shared buffer only.
 *					  Otherwise the complete buffer size will be returned.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaStage::GetBufferSize( bool bNonSharedOnly )
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
	RETURN_IF_NULL( m_pcInput, "None" );
	return( m_pcInput->GetIdentifier() );
}

/** Stream.
 * \par Description:
 * Returns whether the input is a stream and does not support seeking.
 * \author	Arno Klenke
 ******************************************************************************/
bool MediaInputStage::IsStream()
{
	RETURN_IF_NULL( m_pcInput, false );
	return( m_pcInput->StreamBased() );
}

/** Number of tracks.
 * \par Description:
 * Returns the number of tracks.
 * \author	Arno Klenke
 ******************************************************************************/
uint32 MediaInputStage::GetTrackCount()
{
	RETURN_IF_NULL( m_pcInput, 0 );
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
	RETURN_IF_NULL( m_pcInput, 0 );
	ClearCache();
	return( m_pcInput->SelectTrack( nTrack ) );
}

/** Author of the file.
 * \par Description:
 * Returns the author of the file or "".
 * \author	Arno Klenke
 ******************************************************************************/
String MediaInputStage::GetAuthor()
{
	RETURN_IF_NULL( m_pcInput, "" );
	return( m_pcInput->GetAuthor() );
}

/** Title of the file.
 * \par Description:
 * Returns the title of the file or "".
 * \author	Arno Klenke
 ******************************************************************************/
String MediaInputStage::GetTitle()
{
	RETURN_IF_NULL( m_pcInput, "" );
	return( m_pcInput->GetTitle() );
}

/** Album.
 * \par Description:
 * Returns the album the file belongs to or "".
 * \author	Arno Klenke
 ******************************************************************************/
String MediaInputStage::GetAlbum()
{
	RETURN_IF_NULL( m_pcInput, "" );
	return( m_pcInput->GetAlbum() );
}

/** Comment.
 * \par Description:
 * Returns a comment or "".
 * \author	Arno Klenke
 ******************************************************************************/
String MediaInputStage::GetComment()
{
	RETURN_IF_NULL( m_pcInput, "" );
	return( m_pcInput->GetComment() );
}


/** Length of the file.
 * \par Description:
 * Returns the length of the file in seconds.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaInputStage::GetLength()
{
	RETURN_IF_NULL( m_pcInput, 0 );
	return( m_pcInput->GetLength() );
}

/** Current position.
 * \par Description:
 * Returns the current position.
 * \author	Arno Klenke
 ******************************************************************************/
uint64 MediaInputStage::GetCurrentPosition()
{
	RETURN_IF_NULL( m_pcInput, 0 );
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
	RETURN_IF_NULL( m_pcInput, 0 );
	ClearCache();
	return( m_pcInput->Seek( nPosition ) );
}

uint32 MediaInputStage::GetOutputCount()
{
	RETURN_IF_NULL( m_pcInput, 0 );
	return( m_pcInput->GetStreamCount() );
}
MediaFormat_s MediaInputStage::GetOutputFormat( uint32 nOutput )
{
	MediaFormat_s sDummy;
	MEDIA_CLEAR_FORMAT( sDummy );
	RETURN_IF_NULL( m_pcInput, sDummy );
	return( m_pcInput->GetStreamFormat( nOutput ) );
}

bool MediaInputStage::HasInternalBuffer()
{
	return( false );
}

/** Stream of the next available packet.
 * \par Description:
 * Returns the stream of the next available packet or -1.
 * \author	Arno Klenke
 ******************************************************************************/
 
int MediaInputStage::NextPacketStream()
{
	RETURN_IF_NULL( m_pcInput, -1 );
	if( m_acPackets.size() == 0 )
		return( -1 );
	return( m_acPackets[0].nStream );
}

status_t MediaInputStage::GetPacket( uint32 nOutput, MediaPacket_s* psPacket )
{
	RETURN_IF_NULL( m_pcInput, -ENODEV );
	MediaPacket_s sPacket;
	
	/* Fill queue */
	while( m_acPackets.size() < 20 )
	{
		if( m_pcInput->ReadPacket( &sPacket ) == 0 )
		{
			//printf( "Fill Read Packet at %i:%i:%i\n", (int)m_acPackets.size(), sPacket.nStream, (int)sPacket.nSize[0] );
			MediaStage* pcNext = NULL;
			uint32 nNextInput = 0;
			GetNextStage( sPacket.nStream, &pcNext, &nNextInput );
			if( pcNext != NULL )
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
			MediaStage* pcNext = NULL;
			uint32 nNextInput = 0;
			GetNextStage( sPacket.nStream, &pcNext, &nNextInput );
			if( pcNext != NULL )
				m_acPackets.push_back( sPacket );
		}
		else
			break;
	}	
	
	return( -EIO );
}

void MediaInputStage::FreePacket( MediaPacket_s* psPacket )
{
	if( m_pcInput == NULL )
		return;
	return( m_pcInput->FreePacket( psPacket ) );
}
	

/** Assigns a media input object to the stage.
 * \par Description:
 * Assigns a media input object to the stage.
 * \par Note:
 * The reference counter of the object is increased.
 * \param pcInput - The object.
 * \author	Arno Klenke
 ******************************************************************************/
void MediaInputStage::SetInput( MediaInput* pcInput )
{
	if( m_pcInput )
		m_pcInput->Release();
	m_pcInput = pcInput;
	m_pcInput->AddRef();
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
	
	pcInput->SelectTrack( 0 );
	MediaInputStage* pcStage = new MediaInputStage();
	pcStage->SetInput( pcInput );
	pcInput->Release();
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
	if( m_pcCodec == NULL )
	{
		MediaFormat_s sFormat;
		MEDIA_CLEAR_FORMAT( sFormat );
		return( sFormat );
	}
	return( m_pcCodec->GetInternalFormat() );
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
	
/** Assigns a media codec object to the stage.
 * \par Description:
 * Assigns a media codec object to the stage.
 * \par Note:
 * The reference counter of the object is increased.
 * \param pcCodec - The object.
 * \author	Arno Klenke
 ******************************************************************************/
void MediaDecoderStage::SetCodec( MediaCodec* pcCodec )
{
	if( m_pcCodec )
		m_pcCodec->Release();
	m_pcCodec = pcCodec;
	m_pcCodec->AddRef();
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
	if( m_pcOutput )
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
	RETURN_IF_NULL( m_pcOutput, "None" );
	return( m_pcOutput->GetIdentifier() );
}

uint32 MediaOutputStage::GetInputCount()
{
	RETURN_IF_NULL( m_pcOutput, 0 );
	return( m_pcOutput->GetSupportedStreamCount() );
}

uint32 MediaOutputStage::GetInputFormatCount( uint32 nInput )
{
	RETURN_IF_NULL( m_pcOutput, 0 );
	return( m_pcOutput->GetOutputFormatCount() );
}

MediaFormat_s MediaOutputStage::GetInputFormat( uint32 nInput, uint32 nNum )
{
	MediaFormat_s sDummy;
	MEDIA_CLEAR_FORMAT( sDummy );
	RETURN_IF_NULL( m_pcOutput, sDummy );
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

uint64 MediaOutputStage::GetDelay( bool bS )
{
	RETURN_IF_NULL( m_pcOutput, 0 );
	return( m_pcOutput->GetDelay( bS ) );
}

uint64 MediaOutputStage::GetBufferSize( bool bS )
{
	RETURN_IF_NULL( m_pcOutput, 0 );
	return( m_pcOutput->GetBufferSize( bS ) );
}

os::View* MediaOutputStage::GetView()
{
	RETURN_IF_NULL( m_pcOutput, NULL );
	return( m_pcOutput->GetVideoView( 0 ) );
}

/** Assigns a media output object to the stage.
 * \par Description:
 * Assigns a media output object to the stage.
 * \par Note:
 * The reference counter of the object is increased.
 * \param pcOutput - The object.
 * \author	Arno Klenke
 ******************************************************************************/
void MediaOutputStage::SetOutput( MediaOutput* pcOutput )
{
	if( m_pcOutput )
		m_pcOutput->Release();
	m_pcOutput = pcOutput;
	m_pcOutput->AddRef();
}

/** Creates a output stage for the default audio output.
 * \par Description:
 * Creates a output stage for the default audio output. 
 * \author	Arno Klenke
 ******************************************************************************/
MediaOutputStage* MediaOutputStage::CreateDefaultAudioOutputStage( os::String zParameters )
{
	MediaManager* pcManager = MediaManager::Get();
	if( pcManager == NULL )
		return( NULL );
	MediaOutput* pcOutput = pcManager->GetDefaultAudioOutput();
	pcManager->Put();
	if( pcOutput == NULL || pcOutput->Open( zParameters ) != 0 )
		return( NULL );

	MediaOutputStage* pcStage = new MediaOutputStage();
	pcStage->SetOutput( pcOutput );
	pcOutput->Release();
	return( pcStage );
}

/** Creates a output stage for the default video output.
 * \par Description:
 * Creates a output stage for the default video output. 
 * \author	Arno Klenke
 ******************************************************************************/
MediaOutputStage* MediaOutputStage::CreateDefaultVideoOutputStage( os::String zParameters )
{
	MediaManager* pcManager = MediaManager::Get();
	if( pcManager == NULL )
		return( NULL );
	MediaOutput* pcOutput = pcManager->GetDefaultVideoOutput();
	pcManager->Put();
	if( pcOutput == NULL || pcOutput->Open( zParameters ) != 0 )
		return( NULL );

	MediaOutputStage* pcStage = new MediaOutputStage();
	pcStage->SetOutput( pcOutput );
	pcOutput->Release();
	return( pcStage );
}

class MediaSyncStage::Private
{
public:
	bool		m_bAudio;
	bool		m_bVideo;
	bool		m_bAudioValid;
	bool		m_bVideoValid;
	MediaPacket_s m_sAudioPacket;
	MediaPacket_s m_sDispatchAudioPacket;
	uint32		m_nAudioPacketPos;
	MediaPacket_s m_sVideoPacket;
	MediaFormat_s m_sAudioFormat;
	bigtime_t	m_nPlayTime;
	bigtime_t	m_nStartTime;
};

MediaSyncStage::MediaSyncStage()
{
	m = new Private;
}


MediaSyncStage::~MediaSyncStage()
{
	delete( m );
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
	//printf("MediaStage::GetPacket() %i %i %i\n", nOutput, m->m_bAudioValid, m->m_bVideoValid );
	switch( nOutput )
	{
		case 0: // Audio data:
			if( m->m_bAudio )
			{
				assert( m->m_bAudioValid );
				*psPacket = m->m_sAudioPacket;			
				return( 0 );
			}
		break;
		case 1: // Video data:
			if( m->m_bVideo )
			{
				assert( m->m_bVideoValid );
				*psPacket = m->m_sVideoPacket;			
				return( 0 );
			}
		break;
	}
	
	return( -EIO );
	
}

status_t MediaSyncStage::Initialize()
{
	m->m_bAudio = m->m_bVideo = m->m_bAudioValid = m->m_bVideoValid = false;
	m->m_nPlayTime = m->m_nStartTime = 0;
	
	
	/* Check connected stages */
	uint32 nPrevOutput;
	MediaStage* pcStage = NULL;
	GetPrevStage( 0, &pcStage, &nPrevOutput );
	
	if( pcStage != NULL )
	{
		m->m_bAudio = true;
		m->m_sAudioFormat = pcStage->GetOutputFormat( nPrevOutput );
	}
	
	GetPrevStage( 1, &pcStage, &nPrevOutput );
	
	if( pcStage != NULL )
		m->m_bVideo = true;
	
	return( 0 );
}

/** Run.
 * \par Description:
 * This method will fetch the audio and/or video packets from the connected inputs
 * and codecs and pass them to the connected outputs at the right time. This method
 * should be called from a separate thread and might block. It returns a value < 0
 * if there was an error. Otherwise it returns a value in msecs that tells the application
 * when it has to call this function again.
 * \author	Arno Klenke
 ******************************************************************************/
int MediaSyncStage::Run()
{
	int nNextCallTime = 100000;
	uint32 nVidPrevOutput;
	MediaStage* pcVidPrevStage = NULL;
	uint32 nVidNextInput;
	MediaStage* pcVidNextStage = NULL;
	
	uint32 nAudPrevOutput;
	MediaStage* pcAudPrevStage = NULL;
	uint32 nAudNextInput;
	MediaStage* pcAudNextStage = NULL;
	
	if( m->m_bVideo )
	{
		GetPrevStage( 1, &pcVidPrevStage, &nVidPrevOutput );
		GetNextStage( 1, &pcVidNextStage, &nVidNextInput );
		assert( pcVidPrevStage );
		assert( pcVidNextStage );
	}
	
	
	if( m->m_bAudio )
	{
		GetPrevStage( 0, &pcAudPrevStage, &nAudPrevOutput );
		GetNextStage( 0, &pcAudNextStage, &nAudNextInput );
		assert( pcAudPrevStage );
		assert( pcAudNextStage );
	}
	
	
	/* Read Video frame */
	if( m->m_bVideo && !m->m_bVideoValid )
	{
		video_again:
		if( pcVidPrevStage->GetPacket( nVidPrevOutput, &m->m_sVideoPacket ) != 0 )
		{
			printf( "Sync:: GetPacket() error!\n" );
			return( -EIO );
		}
		
		if ( m->m_sVideoPacket.nSize[0] > 0 )
		{
			m->m_bVideoValid = true;
		}
		else
			goto video_again;
		
	}
			
	/* Show videoframe */
	if( m->m_bVideo && m->m_bVideoValid )
	{
		if( !m->m_bAudio )
		{
			if( m->m_nStartTime == 0 )
				m->m_nStartTime = get_system_time();
			m->m_nPlayTime = ( get_system_time() - m->m_nStartTime ) / 1000;
		}
		/* Calculate current time */
		bigtime_t nCurrentTime = m->m_nPlayTime;
		if( m->m_bAudio )
		 	nCurrentTime -= pcAudNextStage->GetDelay();
		
		uint64 nVideoFrameLength = 1000 / (uint64)pcVidPrevStage->GetOutputFormat( nVidPrevOutput ).vFrameRate;
			 
		if( (uint64)nCurrentTime > m->m_sVideoPacket.nTimeStamp + nVideoFrameLength * 2 )
		{
			printf( "Dropping Frame %i %i!\n", (int)m->m_sVideoPacket.nTimeStamp, (int)nCurrentTime );
			pcVidPrevStage->FreePacket( &m->m_sVideoPacket );
			m->m_bVideoValid = false;
			nNextCallTime = 0;
		}
		else if( !( (uint64)nCurrentTime < m->m_sVideoPacket.nTimeStamp ) )
		{
			/* Write video frame */
			//printf( "Write video frame %i %i\n", (int)m->m_sVideoPacket.nTimeStamp, (int)nCurrentTime );
			pcVidNextStage->GetPacket( nVidNextInput, NULL );
			pcVidPrevStage->FreePacket( &m->m_sVideoPacket );
			m->m_bVideoValid = false;
			nNextCallTime = 0;
		}
		else
		{
			/* Calculate time when we want to write the frame */
			int nRemainingTime = (int)( m->m_sVideoPacket.nTimeStamp - (uint64)nCurrentTime ) - 2;
			if( nRemainingTime < 0 )
				nRemainingTime = 0;
			nNextCallTime = std::min( nNextCallTime, nRemainingTime );
		}
	}
	
	
	/* Read audio packet */
	if( m->m_bAudio && !m->m_bAudioValid )
	{
audio_again:		
		if( pcAudPrevStage->GetPacket( nAudPrevOutput, &m->m_sAudioPacket ) != 0 )
		{
			printf( "Sync:: GetPacket() error!\n" );
			return( -EIO );
		}
		
		if ( m->m_sAudioPacket.nSize[0] > 0 )
		{
			m->m_bAudioValid = true;
			m->m_nAudioPacketPos = 0;
		}
		else
			goto audio_again;
	}
	
		
	/* Write audio */
	
	if( m->m_bAudio && m->m_bAudioValid )
	{
		uint64 nAudioBufferSize = pcAudNextStage->GetBufferSize( true );
		uint32 nFreeAudioBufSize = nAudioBufferSize - pcAudNextStage->GetDelay( true );
		
		/* We only write as much bytes as the free buffer size because we 
		do not want to block when GetPacket() is called */
		if( nAudioBufferSize <= 80 || ( nFreeAudioBufSize > 40 ) )
		{
			uint32 nFreeAudioBufBytes = (uint32)( nFreeAudioBufSize * bigtime_t( 2 * m->m_sAudioFormat.nChannels * m->m_sAudioFormat.nSampleRate ) / 1000 );
			nFreeAudioBufBytes -= nFreeAudioBufBytes % ( 2 * m->m_sAudioFormat.nChannels );
			uint32 nWriteBytes = std::min( nFreeAudioBufBytes, m->m_sAudioPacket.nSize[0] - m->m_nAudioPacketPos );

			bigtime_t nAudioLength = (bigtime_t)nWriteBytes * 1000;
			nAudioLength /= bigtime_t( 2 * m->m_sAudioFormat.nChannels * m->m_sAudioFormat.nSampleRate );
					
			nFreeAudioBufSize -= nAudioLength;
			
			if( m->m_nAudioPacketPos == 0 )
				if( m->m_sAudioPacket.nTimeStamp != ~0 ) {
					m->m_nPlayTime = m->m_sAudioPacket.nTimeStamp;
					//printf("Audio time stamp %i\n", (int)nLastAudio );
				}
				
			m->m_nPlayTime += nAudioLength;
				
			m->m_sDispatchAudioPacket = m->m_sAudioPacket;
			m->m_sDispatchAudioPacket.pBuffer[0] += m->m_nAudioPacketPos;
			m->m_sDispatchAudioPacket.nSize[0] = nWriteBytes;
					
			m->m_nAudioPacketPos += nWriteBytes;
			
			pcAudNextStage->GetPacket( nAudNextInput, NULL );
					
			if( m->m_nAudioPacketPos == m->m_sAudioPacket.nSize[0] )
			{
				//printf( "Packet complete!\n" );
				m->m_bAudioValid = false;
				pcAudPrevStage->FreePacket( &m->m_sAudioPacket );
			}
					
		}
		
		/* Calculate when we want to get called again */
		int nRemainingTime = nAudioBufferSize > 200 ? ( 80 - nFreeAudioBufSize ) : ( 40 - nFreeAudioBufSize );
		if( nRemainingTime < 0 )
			nRemainingTime = 0;
		nNextCallTime = std::min( nNextCallTime, nRemainingTime );
		//printf( "%i %i %i %i\n", (int)nAudioBufferSize, (int)nFreeAudioBufSize, (int)nRemainingTime, (int)nNextCallTime );
	}
	return( nNextCallTime );	
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
