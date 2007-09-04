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
#include <media/addon.h>
#include <storage/directory.h>
#include <util/message.h>
#include <util/resources.h>
#include <atheos/msgport.h>
#include <atheos/device.h>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <sys/ioctl.h>

using namespace os;

typedef struct MediaPlugin
{
	MediaPlugin( image_id hImage, MediaAddon* pcAddon )
	{
		mp_hImage = hImage;
		mp_pcAddon = pcAddon;
	}
	image_id mp_hImage;
	MediaAddon* mp_pcAddon;
};

class MediaManager::Private
{
public:
	void LoadPlugins()
	{
		if( m_bPluginsLoaded )
			return;
			
		
		String zFileName;
		String zPath = String( "/system/extensions/media" );

		/* Open all media plugins in /system/media */
		Directory *pcDirectory = new Directory();
		if( pcDirectory->SetTo( zPath ) != 0 )
			return;

		std::cout<<"Start plugin scan.."<<std::endl;
		m_nPlugins.clear();

		while( pcDirectory->GetNextEntry( &zFileName ) )
		{
			/* Load image */
			if( zFileName == "." || zFileName == ".." )
				continue;
			zFileName = zPath + String( "/" ) + zFileName;
			
			/* Check category attribute. We load audio drivers later */
			char zCategory[256];
			memset( zCategory, 0, 256 );
			os::FSNode cNode( zFileName );
			if( cNode.ReadAttr( "os::Category", ATTR_TYPE_STRING, zCategory, 0, 255 ) > 0 )
			{
				if( os::String( zCategory ) == "AudioDriver" )
					continue;
			}

			image_id nID = load_library( zFileName.c_str(), 0 );
			if( nID >= 0 ) {
				init_media_addon *pInit;
				/* Call init_media_addon() */
				if( get_symbol_address( nID, "init_media_addon",
				-1, (void**)&pInit ) == 0 ) {
					MediaAddon* pcAddon = pInit( "" );
					if( pcAddon ) {
						if( pcAddon->GetAPIVersion() != MEDIA_ADDON_API_VERSION )
						{
							std::cout<<zFileName.c_str()<<" has wrong api version"<<std::endl;
						}
						else if( pcAddon->Initialize() != 0 )
						{
							std::cout<<pcAddon->GetIdentifier().c_str()<<" failed to initialize"<<std::endl;
						} else {
							std::cout<<pcAddon->GetIdentifier().c_str()<<" initialized"<<std::endl;
							m_nPlugins.push_back( MediaPlugin( nID, pcAddon ) );
						}
					}
				} else {
					std::cout<<zFileName.c_str()<<" does not export init_media_addon()"<<std::endl;
				}
			}
		}
		m_bPluginsLoaded = true;
		
		/* Load audio drivers */
		
		zPath = String( "/dev/audio" );
		if( pcDirectory->SetTo( zPath ) != 0 )
			return;
		while( pcDirectory->GetNextEntry( &zFileName ) )
		{
			if( zFileName == "." || zFileName == ".." )
				continue;
			zPath = String( "/dev/audio" );
			os::String zDevFileName = zPath + String( "/" ) + zFileName;
			
			/* Get userspace driver name */
			char zDriverPath[PATH_MAX];
			memset( zDriverPath, 0, PATH_MAX );
			int nFd = open( zDevFileName.c_str(), O_RDONLY );
			if( nFd < 0 )
				continue;
			if( ioctl( nFd, IOCTL_GET_USERSPACE_DRIVER, zDriverPath ) != 0 )
			{
				close( nFd );
				continue;
			}
			close( nFd );
			
		
			/* Construct plugin path */
			zPath = String( "/system/extensions/media" );
			zFileName = zPath + String( "/" ) + zDriverPath;
			
			image_id nID = load_library( zFileName.c_str(), 0 );
			if( nID >= 0 ) {
				init_media_addon *pInit;
				/* Call init_media_addon() */
				if( get_symbol_address( nID, "init_media_addon",
				-1, (void**)&pInit ) == 0 ) {
					MediaAddon* pcAddon = pInit( zDevFileName );
					if( pcAddon ) {
						if( pcAddon->GetAPIVersion() != MEDIA_ADDON_API_VERSION )
						{
							std::cout<<zFileName.c_str()<<" has wrong api version"<<std::endl;
						}
						else if( pcAddon->Initialize() != 0 )
						{
							std::cout<<pcAddon->GetIdentifier().c_str()<<" failed to initialize"<<std::endl;
						} else {
							std::cout<<pcAddon->GetIdentifier().c_str()<<" initialized"<<std::endl;
							m_nPlugins.push_back( MediaPlugin( nID, pcAddon ) );
						}
					}
				} else {
					std::cout<<zFileName.c_str()<<" does not export init_media_addon()"<<std::endl;
				}
			}
		}		
		

		std::cout<<"Plugin scan finished"<<std::endl;
	}
	
	int 			m_nRefCount;
	bool			m_bValid;
	bool			m_bPluginsLoaded;
	Messenger		m_cMediaServerLink;
	std::vector<MediaPlugin> m_nPlugins;
};


MediaManager* MediaManager::s_pcInstance = NULL;


/** Construct one media manager.
 * \par Description:
 * Constructs a new media manager and loads all plugins.
 * \par Note:
 * Should never be called! Use the Get() method instead.
 * \author	Arno Klenke
 *****************************************************************************/
MediaManager::MediaManager()
{
	/* Look for media manager port */
	port_id nPort;
	Message cReply;
	assert( NULL == s_pcInstance );
	
	m = new Private;
	m->m_nRefCount = 1;
	m->m_bPluginsLoaded = false;
	
	std::cout<<"Trying to connect to media server..."<<std::endl;
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
	std::cout<<"Connected to media server at port "<<nPort<<std::endl;
	
	s_pcInstance = this;
}


/** Destruct one media manager.
 * \par Description:
 * Destructs a media manager and unloads all plugins.
 * WARNING: This call is deprecated. Always use the Get() and Put() methods!
 * \author	Arno Klenke
 *****************************************************************************/
MediaManager::~MediaManager()
{
	if( m->m_bPluginsLoaded )
	{
		for( uint32 i = 0; i < m->m_nPlugins.size(); i++ ) {
			delete( m->m_nPlugins[i].mp_pcAddon );
			unload_library( m->m_nPlugins[i].mp_hImage );
		}
		std::cout<<"All plugins unloaded"<<std::endl;
	}
	s_pcInstance = NULL;
	delete( m );
}


void MediaManager::AddRef()
{
	m->m_nRefCount++;
}

/** Provides access to the MediaManager class.
 * \par Description:
 *  Creates a new media manager if it does not exist. Otherwise return a pointer
 * to the current one.
 * \par Note:
 * Make sure you call Put() afterwards. If the creation fails, an exception will
 * be thrown.
 * \sa Put()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
MediaManager* MediaManager::Get()
{
	MediaManager* pcManager = s_pcInstance;
	if( pcManager == NULL ) {
		pcManager = new MediaManager();
	} else
		pcManager->AddRef();
	return( pcManager );
}


/** Decrements the reference counter of the instance.
 * \par Description:
 * Decrements the reference counter of the instance.
 * \par Note:
 * Make sure that you do one call to this method for every Get() call in your
 * appliction.
 * \sa Get()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void MediaManager::Put()
{
	m->m_nRefCount--;
	if( m->m_nRefCount <= 0 ) {
		delete( this );
	}
}

/** Return instance.
 * \par Description:
 * Returns the current instance of the media manager.
 * WARNING: This call is deprecated. Always use the Get() and Put() methods!
 * \sa Get(), Put()
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
	m->LoadPlugins();
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetInputCount(); j++ )
		{
			MediaInput* pcInput = m->m_nPlugins[i].mp_pcAddon->GetInput( j );
			if( pcInput ) {
				if( pcInput->FileNameRequired() && pcInput->Open( zFileName ) == 0 ) {
					pcInput->Close();
					return( pcInput );
				}
				pcInput->Release();
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
	if( !m->m_bValid )
		return( NULL );
	/* Ask media server */
	m->LoadPlugins();
	m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_GET_DEFAULT_INPUT, &cReply );
	if( !( cReply.GetCode() == MEDIA_SERVER_OK && cReply.FindString( "input", &zPlugin.str() ) == 0 ) )
		return( NULL );
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetInputCount(); j++ )
		{
			MediaInput* pcInput = m->m_nPlugins[i].mp_pcAddon->GetInput( j );
			if( pcInput ) {
				if( pcInput ) {
					if( pcInput->GetIdentifier() == zPlugin ) {
						return( pcInput );
					}
					pcInput->Release();
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
	uint32 nCount = 0;
	m->LoadPlugins();
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetInputCount(); j++ )
		{
			MediaInput* pcInput = m->m_nPlugins[i].mp_pcAddon->GetInput( j );
			if( pcInput ) {
				if( nCount == nIndex )
					return( pcInput );
				nCount++;
				pcInput->Release();
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
	m->LoadPlugins();
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetCodecCount(); j++ )
		{
			MediaCodec* pcCodec = m->m_nPlugins[i].mp_pcAddon->GetCodec( j );
			if( pcCodec ) {
				if( pcCodec->Open( sInternal, sExternal, bEncode ) == 0 ) {
					pcCodec->Close();
					return( pcCodec );
				}
				else
					pcCodec->Release();
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
	uint32 nCount = 0;
	m->LoadPlugins();
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetCodecCount(); j++ )
		{
			MediaCodec* pcCodec = m->m_nPlugins[i].mp_pcAddon->GetCodec( j );
			if( pcCodec ) {
				if( nCount == nIndex )
					return( pcCodec );
				nCount++;
				pcCodec->Release();
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
	m->LoadPlugins();
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetOutputCount(); j++ )
		{
			MediaOutput* pcOutput = m->m_nPlugins[i].mp_pcAddon->GetOutput( j );
			if( pcOutput ) {
				if( pcOutput->GetIdentifier() == zIdentifier ) {
					if( pcOutput->Open( zFileName ) == 0 ) {
						pcOutput->Close();
						return( pcOutput );
					}
				}
				pcOutput->Release();
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
	if( !m->m_bValid )
		return( NULL );
	/* Ask media server */
	m->LoadPlugins();
	m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_GET_DEFAULT_AUDIO_OUTPUT, &cReply );
	if( !( cReply.GetCode() == MEDIA_SERVER_OK && cReply.FindString( "output", &zPlugin.str() ) == 0 ) )
		return( NULL );
		
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetOutputCount(); j++ )
		{
			MediaOutput* pcOutput = m->m_nPlugins[i].mp_pcAddon->GetOutput( j );
			if( pcOutput ) {
				if( pcOutput->GetIdentifier() == zPlugin ) {
					return( pcOutput );
				}
				pcOutput->Release();
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
	if( !m->m_bValid )
		return( NULL );
	/* Ask media server */
	m->LoadPlugins();
	m->m_cMediaServerLink.SendMessage( MEDIA_SERVER_GET_DEFAULT_VIDEO_OUTPUT, &cReply );
	if( !( cReply.GetCode() == MEDIA_SERVER_OK && cReply.FindString( "output", &zPlugin.str() ) == 0 ) )
		return( NULL );
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetOutputCount(); j++ )
		{
			MediaOutput* pcOutput = m->m_nPlugins[i].mp_pcAddon->GetOutput( j );
			if( pcOutput ) {
				if( pcOutput->GetIdentifier() == zPlugin ) {
					return( pcOutput );
				}
				pcOutput->Release();
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
	uint32 nCount = 0;
	m->LoadPlugins();
	for( uint32 i = 0; i < m->m_nPlugins.size(); i++ )
	{
		for( uint32 j = 0; j < m->m_nPlugins[i].mp_pcAddon->GetOutputCount(); j++ )
		{
			MediaOutput* pcOutput = m->m_nPlugins[i].mp_pcAddon->GetOutput( j );
			if( pcOutput ) {
				if( nCount == nIndex )
					return( pcOutput );
				nCount++;
				pcOutput->Release();
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

