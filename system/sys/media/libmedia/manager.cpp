/*  Media Manager class
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

#include <media/manager.h>
#include <media/server.h>
#include <storage/directory.h>
#include <util/message.h>
#include <util/resources.h>
#include <atheos/msgport.h>
#include <iostream.h>

using namespace os;

class MediaManager::Private
{
public:
	bool			m_bValid;
	Messenger		m_cMediaServerLink;
	std::vector<int> m_nPlugins;
};


MediaManager* MediaManager::s_pcInstance = NULL;


/** Construct one media manager.
 * \par Description:
 * Constructs a new media manager and loads all plugins.
 * \par Note:
 * Never create two media managers, use the GetInstance() method instead.
 * \author	Arno Klenke
 *****************************************************************************/
MediaManager::MediaManager()
{
	/* Look for media manager port */
	port_id nPort;
	Message cReply;
	assert( NULL == s_pcInstance );
	
	m = new Private;
	
	cout<<"Trying to connect to media server..."<<endl;
	if( ( nPort = find_port( "l:media_server" ) ) < 0 ) {
		m->m_bValid = false;
		return;
	} else {
		/* Connect */
		m->m_cMediaServerLink = Messenger( nPort );
		m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_PING, &cReply );
		if( cReply.GetCode() == MEDIA_SERVER_OK )
			m->m_bValid = true;
		else {
			m->m_bValid = false;
			return;
		}
	}
	cout<<"Connected to media server at port "<<nPort<<endl;
	
	/* Open all media plugins in /system/media */
	Directory *pcDirectory = new Directory();
	if( pcDirectory->SetTo( "/system/drivers/media" ) != 0 )
		return;
	
	String zFileName;
	String zPath;
	pcDirectory->GetPath( &zPath.str() );
	
	cout<<"Start plugin scan.."<<endl;
	m->m_nPlugins.clear();
	
	while( pcDirectory->GetNextEntry( &zFileName.str() ) )
	{
		if( zFileName == "." || zFileName == ".." )
			continue;
		zFileName = zPath + String( "/" ) + zFileName;
		
		int nID = load_library( zFileName.c_str(), 0 );
		if( nID >= 0 ) {
			cout<<zFileName.c_str()<<" loaded @ "<<nID<<endl;
			m->m_nPlugins.push_back( nID );
		}
	}
	s_pcInstance = this;
}


/** Destruct one media manager.
 * \par Description:
 * Destructs a media manager and unloads all plugins.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaManager::~MediaManager()
{
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ ) {
		unload_library( m->m_nPlugins[i] );
	}
	s_pcInstance = NULL;
	delete( m );
}


/** Return instance.
 * \par Description:
 * Returns the current instance of the media manager.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaManager* MediaManager::GetInstance()
{
	return( s_pcInstance );
}

/** Valid.
* \par Description:
* Returns whether the created media manager is valid.
*
* \author	Arno Klenke
*****************************************************************************/
bool MediaManager::IsValid() 
{ 
	return( m->m_bValid ); 
}
	
/** Server link
* \par Description:
* Returns a link to the media server.
* \par Note:
* Normal applications should not use this!
*
* \author	Arno Klenke
*****************************************************************************/
Messenger MediaManager::GetServerLink() 
{
	return( m->m_cMediaServerLink ); 
}


/** Return a matching input object.
 * \par Description:
 * Returns a matching input object for the requested device / file. Returns NULL if
 * no input object could be found.
 * \par Note:
 * This only works for inputs which require a filename! To get other inputs
 * you have to use the GetInput() or GetDefaultInput() method.
 * \param zFileName - Filename of the device / file.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaInput* MediaManager::GetBestInput( String zFileName )
{
	/* Look through all plugins for the init_media_demuxer symbol */
	init_media_input *pInit;
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		if( get_symbol_address( m->m_nPlugins[i], "init_media_input",
			-1, (void**)&pInit ) == 0 ) {
			MediaInput* pcInput = pInit();
			if( pcInput ) {
				if( pcInput->FileNameRequired() && pcInput->Open( zFileName ) == 0 ) {
					pcInput->Close();
					return( pcInput );
				}
				delete( pcInput );
			}
				
		}
	}
	return( NULL );
}


/** Return default input.
 * \par Description:
 * Returns the default input or NULL if there is no default output.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaInput* MediaManager::GetDefaultInput()
{
	Message cReply;
	String zPlugin;
	init_media_input *pInit;
	if( !m->m_bValid )
		return( NULL );
	/* Ask media server */
	m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_GET_DEFAULT_INPUT, &cReply );
	if( cReply.GetCode() == MEDIA_SERVER_OK && cReply.FindString( "input", &zPlugin.str() ) == 0 ) {
		for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
		{
			if( get_symbol_address( m->m_nPlugins[i], "init_media_input",
			-1, (void**)&pInit ) == 0 ) {
				MediaInput* pcInput = pInit();
				if( pcInput ) {
					if( pcInput->GetIdentifier() == zPlugin ) {
						return( pcInput );
					}
					delete( pcInput );
				}
				
			}
		}
	}
	return( NULL );
}


/** Set default input.
 * \par Description:
 * Sets the default input.
 * \par Note:
 * Applications will probably never call this.
 *
 * \param zIdentifier - Identifier of the input.
 *
 * \author	Arno Klenke
 *****************************************************************************/
void MediaManager::SetDefaultInput( os::String zIdentifier )
{
	if( !m->m_bValid )
		return;
	Message cMsg( MEDIA_SERVER_SET_DEFAULT_INPUT );
	cMsg.AddString( "input", zIdentifier.str() );
	m->m_cMediaServerLink.SendMessage( &cMsg );
}

/** Return one input.
 * \par Description:
 * Returns the input specified by the nIndex parameter or NULL if this index
 * is invalid.
 *
 * \param nIndex - Index of the input.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaInput* MediaManager::GetInput( uint32 nIndex )
{
	init_media_input *pInit;
	uint32 nCount = 0;
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		if( get_symbol_address( m->m_nPlugins[i], "init_media_input",
			-1, (void**)&pInit ) == 0 ) {
			MediaInput* pcInput = pInit();
			if( pcInput ) {
				if( nCount == nIndex )
					return( pcInput );
				nCount++;
				delete( pcInput );
			}
		}
	}
	return( NULL );
}


/** Return a matching codec.
 * \par Description:
 * Returns a matching codec for the requested codec format. Returns NULL if
 * no codec could be found.
 * \param sInternal - Requested internal format of the codec.
 * \param sExternal - Requested external format of the codec.
 * \param bEncode - If the codec should be used for encoding.
 * \author	Arno Klenke
 *****************************************************************************/
MediaCodec* MediaManager::GetBestCodec( MediaFormat_s sInternal, MediaFormat_s sExternal, bool bEncode )
{
	/* Look through all plugins for the init_media_codec symbol */
	init_media_codec *pInit;
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		if( get_symbol_address( m->m_nPlugins[i], "init_media_codec",
			-1, (void**)&pInit ) == 0 ) {
			MediaCodec* pcCodec = pInit();
			if( pcCodec ) {
				if( pcCodec->Open( sInternal, sExternal, bEncode ) == 0 ) {
					pcCodec->Close();
					return( pcCodec );
				}
				else
					delete( pcCodec );
			}
		}
	}
	return( NULL );
}



/** Return one codec.
 * \par Description:
 * Returns the codec specified by the nIndex parameter or NULL if this index
 * is invalid.
 *
 * \param nIndex - Index of the input.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaCodec* MediaManager::GetCodec( uint32 nIndex )
{
	init_media_codec *pInit;
	uint32 nCount = 0;
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		if( get_symbol_address( m->m_nPlugins[i], "init_media_codec",
			-1, (void**)&pInit ) == 0 ) {
			MediaCodec* pcCodec = pInit();
			if( pcCodec ) {
				if( nCount == nIndex )
					return( pcCodec );
				nCount++;
				delete( pcCodec );
			}
		}
	}
	return( NULL );
}



/** Return a matching output.
 * \par Description:
 * Returns a matching output for the identifier. Returns NULL if
 * no output could be found.
 * \param zFileName - File or device to where the data should be written.
 * \param zIdentifier - Identifier of the output
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaOutput* MediaManager::GetBestOutput( String zFileName, String zIdentifier )
{
	/* Look through all plugins for the init_media_output symbol */
	init_media_output *pInit;
	MediaFormat_s sSpFmt;
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		if( get_symbol_address( m->m_nPlugins[i], "init_media_output",
			-1, (void**)&pInit ) == 0 ) {
			MediaOutput* pcOutput = pInit();
			if( pcOutput != NULL ) {
				if( pcOutput->GetIdentifier() == zIdentifier ) {
					if( pcOutput->Open( zFileName ) == 0 ) {
						pcOutput->Close();
						return( pcOutput );
					}
				}
				delete( pcOutput );
			}
			
		}
	}
	return( NULL );
}


/** Return default audio output.
 * \par Description:
 * Returns the default audio output or NULL if there is no default audio output.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaOutput* MediaManager::GetDefaultAudioOutput()
{
	Message cReply;
	String zPlugin;
	init_media_output *pInit;
	if( !m->m_bValid )
		return( NULL );
	/* Ask media server */
	m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_GET_DEFAULT_AUDIO_OUTPUT, &cReply );
	if( cReply.GetCode() == MEDIA_SERVER_OK && cReply.FindString( "output", &zPlugin.str() ) == 0 ) {
		for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
		{
			if( get_symbol_address( m->m_nPlugins[i], "init_media_output",
			-1, (void**)&pInit ) == 0 ) {
				MediaOutput* pcOutput = pInit();
				if( pcOutput ) {
					if( pcOutput->GetIdentifier() == zPlugin ) {
						return( pcOutput );
					}
					delete( pcOutput );
				}
				
			}
		}
	}
	return( NULL );
}


/** Set default audio output.
 * \par Description:
 * Sets the default audio output.
 * \par Note:
 * Applications will probably never call this.
 *
 * \param zIdentifier - Identifier of the output.
 *
 * \author	Arno Klenke
 *****************************************************************************/
void MediaManager::SetDefaultAudioOutput( os::String zIdentifier )
{
	if( !m->m_bValid )
		return;
	Message cMsg( MEDIA_SERVER_SET_DEFAULT_AUDIO_OUTPUT );
	cMsg.AddString( "output", zIdentifier.str() );
	m->m_cMediaServerLink.SendMessage( &cMsg );
}


/** Return default video output.
 * \par Description:
 * Returns the default video output or NULL if there is no default video output.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaOutput* MediaManager::GetDefaultVideoOutput()
{
	Message cReply;
	String zPlugin;
	init_media_output *pInit;
	if( !m->m_bValid )
		return( NULL );
	/* Ask media server */
	m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_GET_DEFAULT_VIDEO_OUTPUT, &cReply );
	if( cReply.GetCode() == MEDIA_SERVER_OK && cReply.FindString( "output", &zPlugin.str() ) == 0 ) {
		for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
		{
			if( get_symbol_address( m->m_nPlugins[i], "init_media_output",
			-1, (void**)&pInit ) == 0 ) {
				MediaOutput* pcOutput = pInit();
				if( pcOutput ) {
					if( pcOutput->GetIdentifier() == zPlugin ) {
						return( pcOutput );
					}
					delete( pcOutput );
				}
				
			}
		}
	}
	return( NULL );
}


/** Set default video output.
 * \par Description:
 * Sets the default video output.
 * \par Note:
 * Applications will probably never call this.
 *
 * \param zIdentifier - Identifier of the output.
 *
 * \author	Arno Klenke
 *****************************************************************************/
void MediaManager::SetDefaultVideoOutput( os::String zIdentifier )
{
	if( !m->m_bValid )
		return;
	Message cMsg( MEDIA_SERVER_SET_DEFAULT_VIDEO_OUTPUT );
	cMsg.AddString( "output", zIdentifier.str() );
	m->m_cMediaServerLink.SendMessage( &cMsg );
}


/** Return one output.
 * \par Description:
 * Returns the output specified by the nIndex parameter or NULL if this index
 * is invalid.
 *
 * \param nIndex - Index of the input.
 *
 * \author	Arno Klenke
 *****************************************************************************/
MediaOutput* MediaManager::GetOutput( uint32 nIndex )
{
	init_media_output *pInit;
	uint32 nCount = 0;
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		if( get_symbol_address( m->m_nPlugins[i], "init_media_output",
			-1, (void**)&pInit ) == 0 ) {
			MediaOutput* pcOutput = pInit();
			if( pcOutput ) {
				if( nCount == nIndex )
					return( pcOutput );
				nCount++;
				delete( pcOutput );
			}
		}
	}
	return( NULL );
}

void MediaManager::_reserved1() {}
void MediaManager::_reserved2() {}
void MediaManager::_reserved3() {}
void MediaManager::_reserved4() {}
void MediaManager::_reserved5() {}
void MediaManager::_reserved6() {}
void MediaManager::_reserved7() {}
void MediaManager::_reserved8() {}
void MediaManager::_reserved9() {}
void MediaManager::_reserved10() {}











