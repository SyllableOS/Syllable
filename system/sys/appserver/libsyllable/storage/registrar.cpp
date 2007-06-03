/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <gui/stringview.h>
#include <gui/treeview.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/imageview.h>
#include <util/exceptions.h>
#include <util/messenger.h>
#include <util/resources.h>
#include <util/application.h>
#include <storage/file.h>
#include <storage/registrar.h>
#include <appserver/protocol.h>
//#include <iostream>
#include <unistd.h>

using namespace os;

/** Returns the mimetype.
 * \par Description:
 *  Returns the mimetype.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarFileType::GetMimeType()
{
	os::String cString;
	FindString( "mimetype", &cString );
	return( cString );
}

/** Returns the identifier.
 * \par Description:
 *  Returns the identifier.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarFileType::GetIdentifier()
{
	os::String cString;
	FindString( "identifier", &cString );
	return( cString );
}

/** Returns the icon path.
 * \par Description:
 *  Returns the icon path.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarFileType::GetIcon()
{
	os::String cString;
	FindString( "icon", &cString );
	return( cString );
}

/** Returns the default handler.
 * \par Description:
 *  Returns the default handler.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarFileType::GetDefaultHandler()
{
	os::String cString;
	FindString( "default_handler", &cString );
	return( cString );
}

/** Returns the extension count.
 * \par Description:
 *  Returns the extension count.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int RegistrarFileType::GetExtensionCount()
{
	int nCount = 0;
	FindInt32( "extension_count", &nCount );
	return( nCount );
}

/** Returns an extension.
 * \par Description:
 *  Returns an extension.
 * \par nIndex - Index.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarFileType::GetExtension( int nIndex )
{
	os::String cString;
	FindString( "extension", &cString, nIndex );
	return( cString );
}

/** Returns the handler count.
 * \par Description:
 *  Returns the handler count.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int RegistrarFileType::GetHandlerCount()
{
	int nCount = 0;
	FindInt32( "handler_count", &nCount );
	return( nCount );
}


/** Returns a handler path.
 * \par Description:
 *  Returns a handler path.
 * \par nIndex - Index.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarFileType::GetHandler( int nIndex )
{
	os::String cString;
	FindString( "handler", &cString, nIndex );
	return( cString );
}


/** Returns the application count.
 * \par Description:
 *  Returns the application count.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int RegistrarAppList::GetCount()
{
	int nCount = 0;
	FindInt32( "count", &nCount );
	return( nCount );
}


/** Returns the name.
 * \par Description:
 *  Returns the name.
 * \par nIndex - Index.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarAppList::GetName( int nIndex )
{
	os::String cString;
	FindString( "name", &cString, nIndex );
	return( cString );
}

/** Returns the path.
 * \par Description:
 *  Returns the path.
 * \par nIndex - Index.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarAppList::GetPath( int nIndex )
{
	os::String cString;
	FindString( "path", &cString, nIndex );
	return( cString );
}

/** Returns the category.
 * \par Description:
 *  Returns the category.
 * \par nIndex - Index.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String RegistrarAppList::GetCategory( int nIndex )
{
	os::String cString;
	FindString( "category", &cString, nIndex );
	return( cString );
}


class RegistrarManager::Private
{
public:
	String GetAppPath()
	{
		char zPath[PATH_MAX];
	
		/* Get path of the application */
		int nFd = open_image_file( get_image_id() );
		if( nFd < 0 )
			return( "" );
	
		get_directory_path( nFd, zPath, PATH_MAX );
		close( nFd );
		return( zPath );
	}
	
	int m_nRefCount;
	const char* m_pzUser;
	Messenger m_cServerLink;
	bool m_bSync;
	bool m_bCloseAllOnQuit;
};

RegistrarManager* RegistrarManager::s_pcInstance = NULL;


/** Constructor. Should never be called! Use the Get() method instead.
 * \par Description:
 * Constructor. Should never be called! Use the Get() method instead.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
RegistrarManager::RegistrarManager() : Looper( "registrar_manager" )
{
	m = new Private;
	
	m->m_nRefCount = 0;
	m->m_pzUser = getenv( "USER" );
	m->m_bSync = false;
	m->m_bCloseAllOnQuit = false;
	
	/* Connect to the registrar */
	int nPort;
	if( ( nPort = find_port( "l:registrar" ) ) < 0 )
	{
		dbprintf( "Failed to connect to the registrar\n" );
		throw os::errno_exception( "Failed to connect to the registrar" );
		return;
	}
	
	m->m_cServerLink = Messenger( nPort );
	
	/* Login */
	os::Message cReply;
	os::Message cMessage( REGISTRAR_LOGIN );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddInt64( "process", get_process_id( NULL ) );
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	if( cReply.GetCode() != REGISTRAR_OK )
	{
		dbprintf( "Registrar rejected our login\n" );
		throw os::errno_exception( "The registar rejected our login" );
		return;
	}
	
	s_pcInstance = this;
	
	//std::cout<<"Login finished"<<std::endl;
}


/** Destructor. Never delete a RegistrarManager object! Use the Put() method instead.
 * \par Description:
 * Destructor. Never delete a RegistrarManager object! Use the Put() method instead.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
RegistrarManager::~RegistrarManager()
{
	//printf("Logging out!\n" );
	
	/* Logout */
	os::Message cMessage( REGISTRAR_LOGOUT );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddInt64( "process", get_process_id( NULL ) );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddBool( "all", m->m_bCloseAllOnQuit );
	m->m_cServerLink.SendMessage( &cMessage );
	delete( m );
	s_pcInstance = NULL;
}

void RegistrarManager::AddRef()
{
	m->m_nRefCount++;
}


/** Provides access to the RegistrarManager class.
 * \par Description:
 *  Creates a new registrar manager if it does not exist. Otherwise return a pointer
 * to the current one.
 * \par Note:
 * Make sure you call Put() afterwards. If the creation fails, an exception will
 * be thrown.
 * \sa Put()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
RegistrarManager* RegistrarManager::Get()
{
	RegistrarManager* pcManager = s_pcInstance;
	if( pcManager == NULL ) {
		pcManager = new RegistrarManager();
		pcManager->Run();
	
	}
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
void RegistrarManager::Put( bool bAll )
{
	if( bAll )
		m->m_bCloseAllOnQuit = true;
	m->m_nRefCount--;
	if( m->m_nRefCount <= 0 ) {
		PostMessage( os::M_QUIT );
	}
}

void RegistrarManager::HandleMessage( Message* pcMessage )
{
	return( Looper::HandleMessage( pcMessage ) );
}


/** Sets whether the registrar manager class is in synchronous mode.
 * \par Description:
 * Sets whether the registrar manager class is in synchronous mode. If this mode
 * is activated, then all methods of this class will wait until the registrar server
 * has updated its database.
 * \par Note:
 * The default value is false. This is ok in 99% of all cases.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void RegistrarManager::SetSynchronousMode( bool bSync )
{
	m->m_bSync = bSync;
}


/** Registers a new filetype.
 * \par Description:
 * Registers a new filetype.
 * \param zMimeType - Mimetype of the new type.
 * \param zIdentifier - Name of the type.
 * \param bOverwrite - Whether the name should be overwritten if a type
 *                     with the same mimetype already exists. false is almost always right.
 * \return 0 if the call was successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::RegisterType( String zMimeType, String zIdentifier, bool bOverwrite )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_REGISTER_TYPE );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	cMessage.AddString( "identifier", zIdentifier );
	cMessage.AddBool( "overwrite", bOverwrite );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Tried to register invalid file type\n" );
			return( -1 );
		}
	} else {
		m->m_cServerLink.SendMessage( &cMessage );
	}
	return( 0 );
}


/** Unregisters a filetype.
 * \par Description:
 * Unregisters a filetype.
 * \param zMimeType - Mimetype of the type.
 * \return 0 if the call was successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::UnregisterType( String zMimeType )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_UNREGISTER_TYPE );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Tried to unrgister invalid file type\n" );
			return( -1 );
		}
	} else {
		m->m_cServerLink.SendMessage( &cMessage );
	}
	return( 0 );
}


/** Sets a new icon for a filetype.
 * \par Description:
 * Sets a new icon for a filetype.
 * \param zMimeType - Mimetype of the type.
 * \param zIcon - Path to the new icon. The icon should be in /system/icons/filetypes/.
 * \param bOverwrite - Whether the icon should be overwritten if a type
 *                     with the same mimetype already exists. false is almost always right.
 * \return 0 if the call was successful.
 * \sa RegisterTypeIconFromRes()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::RegisterTypeIcon( String zMimeType, Path cIcon, bool bOverwrite )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_REGISTER_TYPE_ICON );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	cMessage.AddString( "icon", cIcon.GetPath() );
	cMessage.AddBool( "overwrite", bOverwrite );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Could not register icon\n" );
			return( -1 );
		}
	} else
		m->m_cServerLink.SendMessage( &cMessage );
	return( 0 );
}


/** Sets a new icon for a filetype which will be extracted from the application resources.
 * \par Description:
 * Sets a new icon for a filetype. The icon will be extracted from the application resources.
 * \param zMimeType - Mimetype of the type.
 * \param zIconRes - Resource name of the icon.
 * \param bOverwrite - Whether the icon should be overwritten if a type
 *                     with the same mimetype already exists. false is almost always right.
 * \return 0 if the call was successful.
 * \sa RegisterTypeIcon()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::RegisterTypeIconFromRes( String zMimeType, String zIconRes, bool bOverwrite )
{
	/* Build path */
	os::String zPath = os::String( "/system/icons/filetypes/" ) + zIconRes;
	
	/* Lets see if the file already exists */
	struct stat sStat;
	if( lstat( zPath.c_str(), &sStat ) >= 0 && !bOverwrite )
		return( RegisterTypeIcon( zMimeType, zPath, bOverwrite ) );
	
	/* Access the resource stream */
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( zIconRes );
	if( pcStream == NULL )
	{
		return( -ENOENT );
	}
	
	/* Copy the stream into the destination file */
	os::File cFile;
	if( cFile.SetTo( zPath, O_RDWR | O_CREAT | O_TRUNC ) == 0 )
	{
		char zBuffer[8192];
		ssize_t nSize;
		
		while( ( nSize = pcStream->Read( zBuffer, 8192 ) ) > 0 )
		{
			cFile.Write( zBuffer, nSize );
		}
		
		cFile.Flush();
		cFile.Unset();
	} else {
		delete( pcStream );
		return( -EIO );
	}
	
	delete( pcStream );
	
	RegisterTypeIcon( zMimeType, zPath, bOverwrite );
	
	return( 0 );
}

/** Registers a new extension for a type.
 * \par Description:
 * Registers a new extension for a type. This is especially important for files on non native Syllable
 * partitions.
 * \param zMimeType - Mimetype of the type.
 * \param zExtension - Extension that should be registeread.
 * \return 0 if the call was successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::RegisterTypeExtension( String zMimeType, String zExtension )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_REGISTER_TYPE_EXTENSION );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	cMessage.AddString( "extension", zExtension.Lower() );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Could not register extension\n" );
			return( -1 );
		}
	} else
		m->m_cServerLink.SendMessage( &cMessage );
	return( 0 );
}


/** Clears the extension list for a type.
 * \par Description:
 * Clears the extension list for a type. Not often needed.
 * \param zMimeType - Mimetype of the type.
 * \return 0 if the call was successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::ClearTypeExtensions( String zMimeType )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_CLEAR_TYPE_EXTENSIONS );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Could not clear extensions\n" );
			return( -1 );
		}
	} else
		m->m_cServerLink.SendMessage( &cMessage );
	return( 0 );
}


/** Registers a new handler for a type.
 * \par Description:
 * Registers a new handler for a type. 
 * \par Note:
 * If this is the first handler that is registered for the type then it 
 * will become the default handler.
 * \param zMimeType - Mimetype of the type.
 * \param cHandler - Absolute path to the handler.
 * \return 0 if the call was successful.
 * \sa RegisterAsTypeHandler()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::RegisterTypeHandler( String zMimeType, Path cHandler )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_REGISTER_TYPE_HANDLER );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	cMessage.AddString( "handler", cHandler.GetPath() );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Could not register handler\n" );
			return( -1 );
		}
	} else
		m->m_cServerLink.SendMessage( &cMessage );
	return( 0 );
}


/** Registers the current application as a handler for a type.
 * \par Description:
 * Registers the current application as a handler for a type.
 * \param zMimeType - Mimetype of the type.
 * \return 0 if the call was successful.
 * \sa RegisteTypeHandler()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::RegisterAsTypeHandler( String zMimeType )
{
	status_t nError;
	char zPath[PATH_MAX];
	
	int nFd = open_image_file( get_image_id() );
	if( nFd < 0 )
		return( -ENODEV );
	
	get_directory_path( nFd, zPath, PATH_MAX );
	
	nError = RegisterTypeHandler( zMimeType, os::Path( zPath ) );
	
	close( nFd );
	
	return( nError );
}


/** Clears the handler list for a type.
 * \par Description:
 * Clears the handler list for a type. Not often needed.
 * \param zMimeType - Mimetype of the type.
 * \return 0 if the call was successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::ClearTypeHandlers( String zMimeType )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_CLEAR_TYPE_HANDLERS );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Could not clear handlers\n" );
			return( -1 );
		}
	} else
		m->m_cServerLink.SendMessage( &cMessage );
	return( 0 );
}



/** Returns the number of types that are registered for the calling user.
 * \par Description:
 * Returns the number of types that are registered for the calling user.
 * \return The number of types.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int32 RegistrarManager::GetTypeCount()
{
	int32 nCount;
	os::Message cReply;
	os::Message cMessage( REGISTRAR_GET_TYPE_COUNT );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	if( ( cReply.GetCode() != REGISTRAR_OK ) || ( cReply.FindInt32( "count", &nCount ) != 0 ) )
		return( -1 );
	return( nCount );
}

/** Returns a RegistrarFileType object that describes a specific type.
 * \par Description:
 * Returns a RegistrarFileType object that describes a specific type.
 * \param nIndex - Index of the type.
 * \return The data.
 * \sa GetTypeAndIcon()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
RegistrarFileType RegistrarManager::GetType( int32 nIndex )
{
	RegistrarFileType cReply;
	os::Message cMessage( REGISTRAR_GET_TYPE );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddInt32( "index", nIndex );
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	if( cReply.GetCode() != REGISTRAR_OK ) 
	{
		dbprintf( "Error: Invalid file type requested\n" );
		return( cReply );
	}
	return( cReply );
}


/** Sets the default handler for a type.
 * \par Description:
 * Sets the default handler for a type. Make sure the handler is already
 * registered.
 * \param zMimeType - Mimetype of the type.
 * \param zHandler - Absoulte path to the handler.
 * \return 0 if successful.
 * \sa RegisterTypeHandler()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::SetDefaultHandler( String zMimeType, Path zHandler )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_SET_DEFAULT_HANDLER );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "mimetype", zMimeType );
	cMessage.AddString( "handler", zHandler.GetPath() );
	
	if( m->m_bSync ) {
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
		if( cReply.GetCode() != REGISTRAR_OK ) 
		{
			dbprintf( "Error: Could not set default handler\n" );
			return( -1 );
		}
	} else
		m->m_cServerLink.SendMessage( &cMessage );
	return( 0 );
}


/** Returns a description and an image for a file.
 * \par Description:
 * Returns a description and an image for a file.
 * \param zFile - Path to the file.
 * \param cIconSize - Requested size of the icon.
 * \param zTypeName - Contains the type description after the call.
 * \param pcIcon - Contains the icon after the call.
 * \param pcMessage - Contains the full description of the type. In the same
 *                    format as the returned message by GetType().
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::GetTypeAndIcon( String zFile, Point cIconSize, String* zTypeName, Image** pcIcon, Message* pcMessage )
{
	os::Message cReply;
	os::Message cMessage( REGISTRAR_GET_TYPE_AND_ICON );
	cMessage.AddInt64( "port", GetMsgPort() );
	cMessage.AddString( "user", m->m_pzUser );
	cMessage.AddString( "path", zFile );
	cMessage.AddPoint( "icon_size", cIconSize );
	
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	if( cReply.GetCode() != REGISTRAR_OK ) 
	{
		dbprintf( "Error: Could not get type\n" );
			return( -1 );
	}
	
	os::String zMimeType;
	os::String zIdentifier;
	os::String zIcon;
	cReply.FindString( "mimetype", &zMimeType );
	cReply.FindString( "identifier", &zIdentifier );
	cReply.FindString( "icon", &zIcon );
	
	//std::cout<<zFile.c_str()<<" Type "<<zMimeType.c_str()<<" Name "<<zIdentifier.c_str()<<" Icon "<<zIcon.c_str()<<std::endl;
	
	*zTypeName = zIdentifier;
	
	if( pcIcon )
	{
		os::BitmapImage* pcImage = new os::BitmapImage();
		*pcIcon = pcImage;
		/* Load icon */
		if( zIcon.CountChars() > 11 && zIcon.substr( 0, 11 ) == "resource://" )
		{
			/* Load icon from resources */
			os::File cFile;
			os::Resources* pcCol = NULL;
			bool bError = false;
			try
			{
				if( cFile.SetTo( zFile ) < 0 )
					bError = true;
				else
					pcCol = new os::Resources( &cFile );
			} catch( ... ) {
				bError = true;
			}
			
			if( !bError ) {
				os::ResStream *pcStream = pcCol->GetResourceStream( zIcon.substr( 11, zIcon.Length() - 11 ) );
				if( pcStream ) {
					pcImage->Load( pcStream );
					//std::cout<<zIcon.substr( 11, zIcon.Length() - 11 ).c_str()<<" loaded"<<std::endl;
				}
				delete( pcCol );	
			}
		} else {
			//std::cout<<get_thread_id( NULL )<<" Load "<<zIcon.c_str()<<std::endl;
			/* Load from file */
			try
			{
				os::File cFile( zIcon );
				//std::cout<<get_thread_id( NULL )<<" Got file"<<std::endl;
				pcImage->Load( &cFile );
				//std::cout<<get_thread_id( NULL )<<" "<<zIcon.c_str()<<" loaded"<<std::endl;
			} catch( ... )
			{
				//std::cout<<"EXCEPTION!"<<std::endl;
			}
		}
		if( pcImage->GetSize() != cIconSize );
			pcImage->SetSize( cIconSize );
	}
	
	if( pcMessage != NULL )
		*pcMessage = cReply;
	
	return( 0 );
}

/* Handler selector window  */

class HandlerSelector : public Window
{
public:
	HandlerSelector( os::Rect cFrame, os::Path cPath, os::Image* pcIcon, Message cMessage ) : os::Window( cFrame, "handler_selector", 
																		"Select handler" )

	{
		bool bCanOpen = false;
		m_cMessage = cMessage;
		m_cPath = cPath;
		
			
		/* Create main view */
		m_pcView = new os::LayoutView( GetBounds(), "main_view" );
	
		os::VLayoutNode* pcVRoot = new os::VLayoutNode( "v_root" );
		pcVRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
		
		
		os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node", 0.0f );
		
		/* Create icon */
		m_pcIconView = new os::ImageView( os::Rect(), "icon", pcIcon );
		
	
		/* Create text */
		os::String zType;
		cMessage.FindString( "identifier", &zType );
		m_pcText = new os::StringView( os::Rect(), "text", os::String( "Please select the handler to open \"" ) +	
														cPath.GetLeaf() + os::String( "\"\n\nType: " ) + zType );
	
		
		pcHNode->AddChild( m_pcIconView );
		pcHNode->AddChild( new os::HLayoutSpacer( "", 15.0f, 15.0f ) );
		pcHNode->AddChild( m_pcText );
		
		/* Create handler list */
		m_pcHandlerList = new os::TreeView( os::Rect(), "handler_list", os::ListView::F_RENDER_BORDER | os::ListView::F_NO_HEADER,
									os::CF_FOLLOW_ALL, os::WID_FULL_UPDATE_ON_RESIZE );
		m_pcHandlerList->InsertColumn( "Handler", 1000 );
		m_pcHandlerList->SetSelChangeMsg( new os::Message( 0 ) );
		m_pcHandlerList->SetInvokeMsg( new os::Message( 1 ) );
		m_pcHandlerList->SetTarget( this );
		
		int32 nHandlerCount;
		
		m_bHandlerValid.clear();
		
		RegistrarManager* pcManager = RegistrarManager::Get();
		
		if( cMessage.FindInt32( "handler_count", &nHandlerCount ) == 0 )
		{
			os::String zDefaultHandler;
			cMessage.FindString( "default_handler", &zDefaultHandler );
			for( int32 i = 0; i < nHandlerCount; i++ )
			{
				os::String zHandler;
				cMessage.FindString( "handler", &zHandler, i );
				os::String zListEntry = zHandler;
				struct stat sStat;
				
				/* Check if the handler is valid */
				bool bValid = ( lstat( zHandler.c_str(), &sStat ) >= 0 );
				if( !bValid )
					zListEntry += os::String( " (invalid)" );
				
				os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
				pcNode->AppendString( zListEntry );
				/* Try to get an icon for the handler */
				String zTemp;
				Image* pcImage = NULL;
				if( pcManager->GetTypeAndIcon( zHandler, os::Point( 24, 24 ), &zTemp, &pcImage ) == 0 
					&& pcImage )
					pcNode->SetIcon( pcImage );
				m_pcHandlerList->InsertNode( pcNode, false );
				
				
				if( ( zHandler == zDefaultHandler ) && bValid ) {
					m_pcHandlerList->Select( i );
					bCanOpen = true;
				}
				
				/* Check if the handler is valid */
				m_bHandlerValid.push_back( bValid );
			}
		}
		pcManager->Put();
		
		m_pcHandlerList->RefreshLayout();
	
	
		os::HLayoutNode* pcHButtons = new os::HLayoutNode( "h_buttons", 0.0f );
	
		/* create buttons */
		m_pcOpen = new os::Button( os::Rect(), "open", 
					"Open", new os::Message( 1 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
		m_pcOpen->SetEnable( bCanOpen );
			
		pcHButtons->AddChild( new os::HLayoutSpacer( "" ) );
		pcHButtons->AddChild( m_pcOpen, 0.0f );
		
		pcVRoot->AddChild( pcHNode );
		pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVRoot->AddChild( m_pcHandlerList );
		pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVRoot->AddChild( pcHButtons );
	
	
		m_pcView->SetRoot( pcVRoot );
	
		AddChild( m_pcView );
		
		m_pcOpen->SetTabOrder( 0 );
	
		SetDefaultButton( m_pcOpen );
		
		m_pcOpen->MakeFocus();
		
		/* Resize if necessary */
		if( 70 + m_pcText->GetPreferredSize( false ).x > GetBounds().Width() )
			ResizeTo( 70 + m_pcText->GetPreferredSize( false ).x, GetBounds().Height() );
		
	}
	
	bool OkToQuit()
	{
		os::Image* pcImage = m_pcIconView->GetImage();
		m_pcIconView->SetImage( NULL );
		delete( pcImage );
		m_pcHandlerList->Clear();
		return( true );
	}
	
	void HandleMessage( os::Message* pcMessage )
	{
		switch( pcMessage->GetCode() )
		{
			case 0:
			{
				/* Another handler has been selected */
				int nSelected = m_pcHandlerList->GetFirstSelected();
				if( nSelected >= 0 && nSelected < (int)m_pcHandlerList->GetRowCount() )
				{
					/* Check if the selected handler ist valid */
					m_pcOpen->SetEnable( m_bHandlerValid[nSelected] );
				}
				break;
			}
			case 1:
			{
				/* Open has been clicked */
				if( !m_pcOpen->IsEnabled() )
					break;
				int nSelected = m_pcHandlerList->GetFirstSelected();
				if( nSelected >= 0 && nSelected < (int)m_pcHandlerList->GetRowCount() )
				{
					if( m_bHandlerValid[nSelected] ) /* Should always be true */
					{
						os::String zHandler;
						m_cMessage.FindString( "handler", &zHandler, nSelected );
						os::String zFile = "";
						
						zFile = m_cPath.GetPath();
						
						//std::cout<<zFile.c_str()<<std::endl;
						
						/* Execute */
						if( fork() == 0 )
						{
							set_thread_priority( -1, 0 );
							execlp( zHandler.c_str(), zHandler.c_str(), zFile.c_str(), NULL );
						}
						PostMessage( os::M_QUIT );
					}
				}
				break;
			}
			default:
				break;
		}
		os::Window::HandleMessage( pcMessage );
	}

private:
	os::Path			m_cPath;
	os::LayoutView*		m_pcView;
	os::ImageView*		m_pcIconView;	
	os::StringView*		m_pcText;
	os::TreeView*		m_pcHandlerList;
	os::Button*			m_pcOpen;
	Message				m_cMessage;
	std::vector<bool>	m_bHandlerValid;
};



/** Launches a file.
 * \par Description:
 * Launches a file. Executables will be started and other files will be run
 * with their associated application. In verbose mode a dialog is presented
 * to the user that lets him select the right filetype if no default handler
 * is present.
 * \param pcParentWindow - Parent window. Can be NULL. The handler selector will
 *                         be centered in this window.
 * \param zFile - Absolute path to the file.
 * \param bVerbose - Activates verbose mode.
 * \param zTitle - Title of the handler selector.
 * \param bDefaultHandler - If set to false, the handler selector will always be shown.
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t RegistrarManager::Launch( Window* pcParentWindow, String zFile, bool bVerbose, os::String zTitle, bool bDefaultHandler )
{
	//std::cout<<"Launch "<<zFile.c_str()<<std::endl;
	
	os::String zIdentifier;
	os::Image* pcImage = NULL;
	os::Message cMessage;
	
	/* Get the filetype and icon first */
	if( GetTypeAndIcon( zFile, os::Point( 48, 48 ), &zIdentifier, &pcImage, &cMessage ) < 0 )
		return( -EIO );
		
	os::Path cPath( zFile );
	if( bDefaultHandler )
	{
		/* Try to launch an executable */
		os::String zMimeType;
		cMessage.FindString( "mimetype", &zMimeType );
		if( zMimeType == "application/x-executable" )
		{
			if( fork() == 0 )
			{
				set_thread_priority( -1, 0 );
				execlp( zFile.c_str(), zFile.c_str(), NULL, NULL );
			}
			delete( pcImage );
			return( 0 );
		}
		
		/* Try the default handler first */
		os::String zDefaultHandler;
		struct stat sStat;
		cMessage.FindString( "default_handler", &zDefaultHandler );
		
		if( !( zDefaultHandler == "" ) && lstat( zDefaultHandler.c_str(), &sStat ) >= 0 ) 
		{
			/* Execute */
			if( fork() == 0 )
			{
				set_thread_priority( -1, 0 );
				execlp( zDefaultHandler.c_str(), zDefaultHandler.c_str(), cPath.GetPath().c_str(), NULL );
			}
			delete( pcImage );
			return( 0 );
		}
	}
	
	/* If we haven't launched anything yet, then we stop if in non-verbose mode */
	if( !bVerbose )
	{
		delete( pcImage );
		return( -EIO );	
	}
	
	HandlerSelector* pcSelector = new HandlerSelector( os::Rect( 50, 50, 400, 250 ), cPath, pcImage, cMessage );
	pcSelector->SetTitle( zTitle );
	if( pcParentWindow )
		pcSelector->CenterInWindow( pcParentWindow );
	else
		pcSelector->CenterInScreen();
	pcSelector->Show();
	pcSelector->MakeFocus();
	return( 0 );
}

/** Update the application list.
 * \par Description:
 * The registrar manages a list of installed applications in the /Applications
 * folder. Using this method you can tell it to update this list.
 * \param bForce - Force the registrar to update the list.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void RegistrarManager::UpdateAppList( bool bForce )
{
	os::Message cMessage( REGISTRAR_UPDATE_APP_LIST );
	cMessage.AddString( "user", m->m_pzUser );	
	if( m->m_bSync ) {
		os::Message cReply;
		m->m_cServerLink.SendMessage( &cMessage, &cReply );
	}
	else
		m->m_cServerLink.SendMessage( &cMessage );
}

/** Returns the application list.
 * \par Description:
 * Returns the list of applications in /Applications.
 * \return The application list.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

RegistrarAppList RegistrarManager::GetAppList()
{
	os::Message cMessage( REGISTRAR_GET_APP_LIST );
	cMessage.AddString( "user", m->m_pzUser );
	RegistrarAppList cReply;
	
	m->m_cServerLink.SendMessage( &cMessage, &cReply );
	return( cReply );
}



