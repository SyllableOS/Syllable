/*  Registrar
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
 
#include "registrar.h" 


using namespace os;

Registrar::Registrar()
			: Application( "registrar" )
{
	m_cUsers.clear();
	m_cCalls.clear();
	
	/* Load the database of root */
	LoadTypes( "root", GetMsgPort(), -1 );
}

Registrar::~Registrar()
{
}

/* Save the database of one user */
void Registrar::SaveDatabase( os::String zUser )
{
	
	String zPath = "/home/";
	zPath += zUser + String( "/Settings" );
	struct stat sStat;
	if( lstat( zPath.c_str(), &sStat ) < 0 )
		mkdir( zPath.c_str(), 700 );
	zPath += "/registrar";
	File* pcFile = new File();
	if( pcFile->SetTo( zPath, O_RDWR | O_CREAT ) < 0 )
	{
		dbprintf( "Error: Could not write to %s\n", zPath.c_str() );
		return;
	}
	Settings* pcSettings = new Settings( pcFile );
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{
			pcSettings->SetInt32( "type_count", m_cUsers[i].m_cTypes.size() );
			for( uint j = 0; j < m_cUsers[i].m_cTypes.size(); j++ )
			{
				os::Message cType;
			
				cType.AddString( "mimetype", m_cUsers[i].m_cTypes[j].m_zMimeType );
				cType.AddString( "identifier", m_cUsers[i].m_cTypes[j].m_zIdentifier );
				cType.AddString( "icon", m_cUsers[i].m_cTypes[j].m_zIcon );
				
				cType.AddString( "default_handler", m_cUsers[i].m_cTypes[j].m_zDefaultHandler );
				cType.AddInt32( "extension_count", m_cUsers[i].m_cTypes[j].m_cExtensions.size() );
				for( uint k = 0; k < m_cUsers[i].m_cTypes[j].m_cExtensions.size(); k++ )
					cType.AddString( "extension", m_cUsers[i].m_cTypes[j].m_cExtensions[k] );
				cType.AddInt32( "handler_count", m_cUsers[i].m_cTypes[j].m_cHandlers.size() );
				for( uint k = 0; k < m_cUsers[i].m_cTypes[j].m_cHandlers.size(); k++ )
					cType.AddString( "handler", m_cUsers[i].m_cTypes[j].m_cHandlers[k] );
				pcSettings->SetMessage( "type", cType, j );
			}

			break;
		}
	}
	pcSettings->Save();
	delete( pcSettings );
}

/* Load filetype database for one user */
void Registrar::LoadTypes( os::String zUser, port_id hPort, int64 nProcess )
{
	/* Check if the database is already loaded */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{
			RegistrarClient sClient;
			sClient.m_hPort = hPort;
			sClient.m_nProcess = nProcess;
			//dbprintf( "Database for user %s now has %i users (New user %i)\n", zUser.c_str(), 
				//m_cUsers[i].m_cClients.size() + 1, (int)nProcess );
			m_cUsers[i].m_cClients.push_back( sClient );
			return;
		}
	}
	
	/* Create a new user */
	RegistrarUser cUser;
	RegistrarClient sClient;
	
	cUser.m_zUser = zUser;
	cUser.m_cClients.clear();
	
	sClient.m_hPort = hPort;
	sClient.m_nProcess = nProcess;
	cUser.m_cClients.push_back( sClient );
	
	// TODO: Load
	cUser.m_cTypes.clear();
	
	m_cUsers.push_back( cUser );
	
	String zPath = "/home/";
	zPath += zUser + String( "/Settings/registrar" );
	File* pcFile = new File();
	if( pcFile->SetTo( zPath ) < 0 )
	{
		dbprintf( "Database for user %s not present\n", zUser.c_str() );
		return;
	}
	Settings* pcSettings = new Settings( pcFile );
	if( pcSettings->Load() < 0 )
	{
		dbprintf( "Database for user %s invalid\n", zUser.c_str() );
		return;
	}
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{
			int32 nTypeCount = 	pcSettings->GetInt32( "type_count", 0 );
			//std::cout<<nTypeCount<<" types"<<std::endl;
			for( int j = 0; j < nTypeCount; j++ )
			{
				FileType cFileType;
				os::Message cType;
				
				if( pcSettings->FindMessage( "type", &cType, j ) < 0 )
					break;
					
				cType.FindString( "mimetype", &cFileType.m_zMimeType );
				cType.FindString( "identifier", &cFileType.m_zIdentifier );
				cType.FindString( "icon", &cFileType.m_zIcon );
				cType.FindString( "default_handler", &cFileType.m_zDefaultHandler );
			
				int32 nExtensionCount;
				cType.FindInt32( "extension_count", &nExtensionCount );
				for( int k = 0; k < nExtensionCount; k++ )
				{
					os::String zExt;
					cType.FindString( "extension", &zExt, k );
					cFileType.m_cExtensions.push_back( zExt );
				}
				int32 nHandlerCount;
				cType.FindInt32( "handler_count", &nHandlerCount );
				for( int k = 0; k < nHandlerCount; k++ )
				{
					os::String zHandler;
					cType.FindString( "handler", &zHandler, k );
					cFileType.m_cHandlers.push_back( zHandler );
				}
				m_cUsers[i].m_cTypes.push_back( cFileType );
				
				//std::cout<<"Loaded type "<<cFileType.m_zMimeType.c_str()<<" "<<cFileType.m_zIdentifier.c_str()<<std::endl;
			}

			break;
		}
	}
	delete( pcSettings );
	
	
	//dbprintf( "Database for user %s loaded\n", zUser.c_str() );
	
}

/* Return a type identified with its user and mimetype */
FileType* Registrar::GetType( os::String zUser, os::String zMimeType )
{
	/* Search user */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{	
			/* Search the mimetype */
			for( uint j = 0; j < m_cUsers[i].m_cTypes.size(); j++ )
			{
				if( m_cUsers[i].m_cTypes[j].m_zMimeType == zMimeType )
				{
					return( &m_cUsers[i].m_cTypes[j] );
				}
			}
			break;
		}
	}
	return( NULL );
}

void Registrar::Login( Message* pcMessage )
{
	/* Extract parameters */
	os::String zUser;
	int64 nPort;
	port_id hPort;
	int64 nProcess = -1;
	
	if( pcMessage->FindString( "user", &zUser ) != 0 ||
		pcMessage->FindInt64( "port", &nPort ) != 0 )
	{
		pcMessage->SendReply( REGISTRAR_ERROR );
	}
	
	pcMessage->FindInt64( "process", &nProcess );
	
	hPort = nPort;
	
	//std::cout<<"User "<<zUser.c_str()<<" Port "<<hPort<<std::endl;
	
	/* Load the database of the user */
	LoadTypes( zUser, hPort, nProcess );
	
	pcMessage->SendReply( REGISTRAR_OK );
}

void Registrar::Logout( Message* pcMessage )
{
	/* Extract parameters */
	os::String zUser;
	int64 nPort;
	port_id hPort;
	bool bAll = false;
	
	
	if( pcMessage->FindString( "user", &zUser ) != 0 ||
		pcMessage->FindInt64( "port", &nPort ) != 0 )
	{
		return;
	}
	
	pcMessage->FindBool( "all", &bAll );
	
	hPort = nPort;
	
	
	//std::cout<<"Logout user "<<zUser.c_str()<<" port "<<hPort<<std::endl;
	
	/* Search for this user and port */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{
			for( uint j = 0; j < m_cUsers[i].m_cClients.size(); j++ )
			{
				if( m_cUsers[i].m_cClients[j].m_hPort == hPort )
				{
					/* Found it */
					//std::cout<<"Logout"<<std::endl;
					SaveDatabase( zUser );
					//if( bAll )
					//	dbprintf( "Closing all clients of the database\n" );
					m_cUsers[i].m_cClients.erase( m_cUsers[i].m_cClients.begin() + j );
					//dbprintf( "Database for user %s now has %i users\n", zUser.c_str(), 
					//m_cUsers[i].m_cClients.size() );
					
					if( m_cUsers[i].m_cClients.size() == 0 || bAll )
					{
						/* Remove user database */
						m_cUsers.erase( m_cUsers.begin() + i );
						//dbprintf( "Database for user %s unloaded\n", zUser.c_str() );
					}
					
					return;
				}
			}
			return;
		}
	}
}

/* Return the number of registered file types */
void Registrar::GetTypeCount( Message* pcMessage )
{
	int32 nCount = 0;
	os::String zUser;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		return;
	}
	
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{
			nCount += (int32)m_cUsers[i].m_cTypes.size();
			break;
		}
	}
	
	//std::cout<<nCount<<" registered types"<<std::endl;
	
	
	os::Message cReply( REGISTRAR_OK );
	cReply.AddInt32( "count", nCount );
	pcMessage->SendReply( &cReply );
	
}

/* Return a requested type */
void Registrar::GetType( Message* pcMessage )
{
	int64 nCount = 0;
	os::String zUser;
	bool bUseMimeType = false;
	os::String zMimeType;
	int32 nIndex;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) == 0 )
	{
		bUseMimeType = true;
		//std::cout<<"Using mimetype "<<zMimeType.c_str()<<std::endl;
	} else
	{
		if( pcMessage->FindInt32( "index", &nIndex ) != 0 )
		{
			return;
		}
		//std::cout<<"Using index "<<nIndex<<std::endl;
	}
	
	if( bUseMimeType )
	{
		/* Use mimetype */
		FileType* pcType = GetType( zUser, zMimeType );
		if( !pcType )
		{
			pcMessage->SendReply( REGISTRAR_ERROR );
			return;
		}
		
		os::Message cMsg( REGISTRAR_OK );
		cMsg.AddString( "mimetype", pcType->m_zMimeType );
		cMsg.AddString( "identifier", pcType->m_zIdentifier );
		cMsg.AddString( "icon", pcType->m_zIcon );
		cMsg.AddInt32( "extension_count", pcType->m_cExtensions.size() );
		for( uint j = 0; j < pcType->m_cExtensions.size(); j++ )
			cMsg.AddString( "extension", pcType->m_cExtensions[j] );
		cMsg.AddInt32( "handler_count", pcType->m_cHandlers.size() );
		for( uint j = 0; j < pcType->m_cHandlers.size(); j++ )
			cMsg.AddString( "handler", pcType->m_cHandlers[j] );
		cMsg.AddString( "default_handler", pcType->m_zDefaultHandler );
		pcMessage->SendReply( &cMsg );	
	}
	else 
	{
		/* Use index */
		for( uint i = 0; i < m_cUsers.size(); i++ )
		{
			/* Calculate index */
			if( ( m_cUsers[i].m_zUser == zUser ) && ( nIndex >= 0 ) 
					&& ( nIndex < nCount + m_cUsers[i].m_cTypes.size() ) )
			{
				nIndex -= nCount;
				os::Message cMsg( REGISTRAR_OK );
				cMsg.AddString( "mimetype", m_cUsers[i].m_cTypes[nIndex].m_zMimeType );
				cMsg.AddString( "identifier", m_cUsers[i].m_cTypes[nIndex].m_zIdentifier );
				cMsg.AddString( "icon", m_cUsers[i].m_cTypes[nIndex].m_zIcon );
				//cMsg.AddBool( "usertype", m_cUsers[i].m_zUser == zUser );
				cMsg.AddInt32( "extension_count", m_cUsers[i].m_cTypes[nIndex].m_cExtensions.size() );
				for( uint j = 0; j < m_cUsers[i].m_cTypes[nIndex].m_cExtensions.size(); j++ )
					cMsg.AddString( "extension", m_cUsers[i].m_cTypes[nIndex].m_cExtensions[j] );
				cMsg.AddInt32( "handler_count", m_cUsers[i].m_cTypes[nIndex].m_cHandlers.size() );
				for( uint j = 0; j < m_cUsers[i].m_cTypes[nIndex].m_cHandlers.size(); j++ )
					cMsg.AddString( "handler", m_cUsers[i].m_cTypes[nIndex].m_cHandlers[j] );
				cMsg.AddString( "default_handler", m_cUsers[i].m_cTypes[nIndex].m_zDefaultHandler );
				pcMessage->SendReply( &cMsg );	
				return;
			}
		}
		//std::cout<<"Could not find type!"<<std::endl;
	}
	pcMessage->SendReply( REGISTRAR_ERROR );
	
}


/* Register a type */
void Registrar::RegisterType( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	os::String zIdentifier;
	bool bOverwrite;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::RegisterType() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 ||
		pcMessage->FindString( "identifier", &zIdentifier ) != 0 ||
		pcMessage->FindBool( "overwrite", &bOverwrite ) != 0 )
	{
		dbprintf( "Registrar::RegisterType() Parameters missing\n" );
	}
	
	//std::cout<<"Register "<<zMimeType.c_str()<<std::endl;
	
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
		/* Update present type */
		if( bOverwrite )
		{
			pcType->m_zIdentifier = zIdentifier;
			//std::cout<<"New identifier "<<zIdentifier.c_str()<<std::endl;
		}
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_OK );
		return;
	}
	else
	{
		for( uint i = 0; i < m_cUsers.size(); i++ )
		{
			if( m_cUsers[i].m_zUser == zUser )
			{
				/* Create a new type */
				FileType cType;
				cType.m_zMimeType = zMimeType;
				cType.m_zIdentifier = zIdentifier;
				cType.m_zIcon = "";
				cType.m_cExtensions.clear();
				cType.m_cHandlers.clear();
				cType.m_zDefaultHandler = "";
			
				m_cUsers[i].m_cTypes.push_back( cType );
				//std::cout<<"Type registered"<<std::endl;
				if( pcMessage->IsSourceWaiting() )
					pcMessage->SendReply( REGISTRAR_OK );
				return;
			}
		}
	}
	
	/* Should never happen */
	
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
}


/* Unregister a type */
void Registrar::UnregisterType( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::UnregisterType() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 )
	{
		dbprintf( "Registrar::UnregisterType() Parameters missing\n" );
	}
	
	//std::cout<<"Unregister "<<zMimeType.c_str()<<std::endl;
	
	/* Get type */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{	
			/* Search the mimetype */
			for( uint j = 0; j < m_cUsers[i].m_cTypes.size(); j++ )
			{
				if( m_cUsers[i].m_cTypes[j].m_zMimeType == zMimeType )
				{
					/* Delete it */
					m_cUsers[i].m_cTypes.erase( m_cUsers[i].m_cTypes.begin() + j );
					if( pcMessage->IsSourceWaiting() )
						pcMessage->SendReply( REGISTRAR_OK );
					return;
				}
			}
			break;
		}
	}
	
	//std::cout<<"Could not find type"<<std::endl;
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
}

/* Set the default handler for a type */
void Registrar::RegisterTypeIcon( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	os::String zIcon;
	bool bOverwrite;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::RegisterTypeIcon() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 ||
		pcMessage->FindString( "icon", &zIcon ) != 0 ||
		pcMessage->FindBool( "overwrite", &bOverwrite ) != 0 )
	{
		dbprintf( "Registrar::RegisterTypeIcon() Parameters missing\n" );
	}
	
	//std::cout<<"Set icon "<<zIcon.c_str()<<" for "<<zMimeType.c_str()<<std::endl;
	
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
		if( ( pcType->m_zIcon == "" ) || bOverwrite )
		{
			pcType->m_zIcon = zIcon;
			//std::cout<<"Icon registered"<<std::endl;
		}					
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_OK );
		return;
	}
	else
	{
		//std::cout<<"Could not find type "<<std::endl;
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_ERROR );
		return;
	}
	/* Should never happen */
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
}

/* Register a handler for a type */
void Registrar::RegisterTypeHandler( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	os::String zHandler;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::RegisterTypeHandler() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 ||
		pcMessage->FindString( "handler", &zHandler ) != 0 )
	{
		dbprintf( "Registrar::RegisterTypeHandler() Parameters missing\n" );
	}
	
	//std::cout<<"Register "<<zHandler.c_str()<<" for "<<zMimeType.c_str()<<std::endl;
	
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
		/* Go through the handler list */
		for( uint h = 0; h < pcType->m_cHandlers.size(); h++ )
		{
			if( pcType->m_cHandlers[h] == zHandler )
			{
				/* Already present */
				if( pcMessage->IsSourceWaiting() )
					pcMessage->SendReply( REGISTRAR_OK );
				return;
			}
		}
		/* Add the handler to the list */
		pcType->m_cHandlers.push_back( zHandler );
		//std::cout<<"New handler registered"<<std::endl;
					
		/* Set it as the default one if no other one is registered */
		if( pcType->m_cHandlers.size() == 1 )
			pcType->m_zDefaultHandler = zHandler;
					
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_OK );
		return;
	}
			
	//std::cout<<"Could not find type "<<std::endl;
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
}


/* Clear the handler list for a type */
void Registrar::ClearTypeHandlers( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::ClearTypeHandlers() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 )
	{
		dbprintf( "Registrar::ClearTypeHandlers() Parameters missing\n" );
	}
	
	//std::cout<<"Clear handlers for "<<zMimeType.c_str()<<std::endl;
	
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
		pcType->m_cHandlers.clear();
		//std::cout<<"Handlers cleared"<<std::endl;
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_OK );
		return;
	}
			
	//std::cout<<"Could not find type "<<std::endl;
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );		
}

/* Register an extension for a type */
void Registrar::RegisterTypeExtension( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	os::String zExtension;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::RegisterTypeExtension() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 ||
		pcMessage->FindString( "extension", &zExtension ) != 0 )
	{
		dbprintf( "Registrar::RegisterTypeExtension() Parameters missing\n" );
	}
	
	//std::cout<<"Register extension "<<zExtension.c_str()<<" for "<<zMimeType.c_str()<<std::endl;
	
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
		/* Go through the extension list */
		for( uint h = 0; h < pcType->m_cExtensions.size(); h++ )
		{
			if( pcType->m_cExtensions[h] == zExtension )
			{
				/* Already present */
				if( pcMessage->IsSourceWaiting() )
					pcMessage->SendReply( REGISTRAR_OK );
				return;
			}
		}
		/* Add the extension to the list */
		pcType->m_cExtensions.push_back( zExtension );
		//std::cout<<"New extension registered"<<std::endl;
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_OK );
		return;
	}
	//std::cout<<"Could not find type "<<std::endl;
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
}

/* Clear the extension list for a type */
void Registrar::ClearTypeExtensions( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::ClearTypeExtensions() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 )
	{
		dbprintf( "Registrar: Parameters missing\n" );
	}
	
	//std::cout<<"Clear extensions for "<<zMimeType.c_str()<<std::endl;
	
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
		pcType->m_cExtensions.clear();
		//std::cout<<"Extensions cleared"<<std::endl;
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_OK );
		return;
	}
			
	//std::cout<<"Could not find type "<<std::endl;
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );		
}


/* Set the default handler for a type */
void Registrar::SetDefaultHandler( Message* pcMessage )
{
	os::String zUser;
	os::String zMimeType;
	os::String zHandler;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::SetDefaultHandler() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "mimetype", &zMimeType ) != 0 ||
		pcMessage->FindString( "handler", &zHandler ) != 0 )
	{
		dbprintf( "Registrar::SetDefaultHandler() Parameters missing\n" );
	}
	
	//std::cout<<"Set default handler "<<zHandler.c_str()<<" for "<<zMimeType.c_str()<<std::endl;
	
	/* Get type */
	FileType* pcType = GetType( zUser, zMimeType );
	if( pcType )
	{
	
		/* Go through the handler list */
		for( uint h = 0; h < pcType->m_cHandlers.size(); h++ )
		{
			if( pcType->m_cHandlers[h] == zHandler )
			{
				/* Set it as the default one */
				pcType->m_zDefaultHandler = zHandler;
				if( pcMessage->IsSourceWaiting() )
					pcMessage->SendReply( REGISTRAR_OK );
				return;
			}
		}
		//std::cout<<"Invalid handler"<<std::endl;
		if( pcMessage->IsSourceWaiting() )
			pcMessage->SendReply( REGISTRAR_ERROR );
		return;
	}
			
	//std::cout<<"Could not find type "<<std::endl;
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
	return;
}

/* Read an icon attribute */
os::String Registrar::GetAttribute( os::FSNode* pcNode, os::String zAttribute )
{
	pcNode->RewindAttrdir();
	os::String zAttrib;
	while( pcNode->GetNextAttrName( &zAttrib ) == 1 )
	{
		if( zAttrib == zAttribute )
		{
			char zBuffer[PATH_MAX];
			memset( zBuffer, 0, PATH_MAX );
			if( pcNode->ReadAttr( zAttribute, ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX ) > 0 )
			{
				return( os::String( zBuffer ) );
			}
			break;
		}
	}
	return( "" );
}

/* Get type and icon for a type */
void Registrar::GetTypeAndIcon( Message* pcMessage )
{
	os::String zUser;
	os::String zPath;
	os::Point cIconSize;
	os::FSNode cNode;
	os::String zName = "";
	os::String zMimeType = "";
	os::String zIcon = "";
	os::String zExtension = "";
	bool bLink = false;
	bool bDirectory = false;
	bool bExecutable = false;
	bool bBrokenLink = false;
	bool bUseMimeType = false;
	os::Message cReply( REGISTRAR_OK );
	
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::GetTypeAndIcon() called without user\n" );
		return;
	}
	
	if( pcMessage->FindString( "path", &zPath ) != 0 ||
		pcMessage->FindPoint( "icon_size", &cIconSize ) != 0 )
	{
		dbprintf( "Registrar: Parameters missing\n" );
	}
	
	//std::cout<<"Path "<<zPath.c_str()<<std::endl;
	
	/* Open with O_NOTRAVERSE first to check for a link */
	if( cNode.SetTo( zPath, O_RDONLY | O_NOTRAVERSE ) < 0 )
	{
		//std::cout<<"Could not set node!"<<std::endl;
		pcMessage->SendReply( REGISTRAR_ERROR );
		return;
	}
	
	if( cNode.IsLink() )
	{
		bBrokenLink = true;
		bUseMimeType = true;
		os::Path cTarget;
		
		/* Defaults */
		zName = "Invalid link";
		zMimeType = "application/x-broken-link";
		zIcon = "/system/icons/file.png";
		
		try
		{
			/* Try to construct the path of the target of the link */
			os::SymLink cLink( zPath, O_RDONLY );
			if( cLink.ConstructPath( os::Path( zPath ).GetDir().GetPath(), &cTarget ) >= 0 )
			{
				bLink = true; /* Avoid that the name gets overwritten */
				zName = os::String( "Broken link to " ) + cTarget.GetPath();
				if( cNode.SetTo( cTarget.GetPath(), O_RDONLY ) >= 0 )
				{
					zName = os::String( "Link to " ) + cTarget.GetPath();
					
					if( cTarget.GetPath() == "/" )
					{
						/* Link to the root directory */
						zName = "Mounted disks";
						zIcon = "/system/icons/disk.png";
						zMimeType = "application/x-disks";
						cReply.AddString( "mimetype", zMimeType );
						cReply.AddString( "identifier", zName );
						cReply.AddString( "icon", zIcon );
	
						pcMessage->SendReply( &cReply );
						return;
					}
					
					bUseMimeType = false; /* Extensions are possible */
					bBrokenLink = false;
				}
			}
		} catch( ... )
		{
		}	
	} 
	
	/* Open without O_NOTRAVERSE */
	if( !bBrokenLink && cNode.SetTo( zPath, O_RDONLY ) < 0 )
	{
		pcMessage->SendReply( REGISTRAR_ERROR );
		return;
	}
	
	
	/* Try to read the os::MimeType attribute */
	os::String zAttributeMimeType = "";
	if( !bBrokenLink )
		zAttributeMimeType = GetAttribute( &cNode, "os::MimeType" );

	
	/* Set defaults */
	if( !bBrokenLink && cNode.IsDir() )
	{
		bDirectory = true;
		bUseMimeType = true;
		if( !bLink )
			zName = "Directory";
		zMimeType = "application/x-directory";
		zIcon = "/system/icons/folder.png";
		goto no_database;
	} else if( !bBrokenLink && ( ( ( cNode.GetMode() & ( S_IXUSR|S_IXGRP|S_IXOTH ) && zAttributeMimeType == "" ) ) 
		|| zAttributeMimeType == "application/x-executable" ) )
	{
		bExecutable = true;
		bUseMimeType = true;
		if( !bLink )
			zName = "Executable";
		zMimeType = "application/x-executable";
		zIcon = "/system/icons/executable.png";
		goto no_database;
	} else if( !bBrokenLink )
	{
		if( !bLink )
			zName = "Unknown file";
		zMimeType = "application/x-file";
		zIcon = "/system/icons/file.png";
		if( !( zAttributeMimeType == "" ) )
		{
			/* Use attribute */
			zMimeType = zAttributeMimeType;
			bUseMimeType = true;
		}
		/* We do not set bUseMimeType here, because we want to be able to use extensions */
	}
	
	/* Try to get extension if we have no mimetype attribute */
	
	if( !bUseMimeType )
	{
		os::String zLeaf = os::Path( zPath ).GetLeaf();
		
		for( uint e = zLeaf.Length(); e >= 0; e-- )
		{
			//std::cout<<zPath.c_str()<<" "<<zPath.Length()<<" "<<e<<" "<<zPath[e]<<std::endl;
			if( zLeaf[e] == '.' )
			{
				zExtension = zLeaf.substr( e + 1, zLeaf.Length() - e - 1 );
				zExtension.Lower();
				//std::cout<<"Extension "<<zExtension.c_str()<<" "<<zExtension.Length()<<std::endl;
				break;
			}
		}
	}
	
	/* Use default mimetype if no extension could be found */
	if( zExtension == "" )
		bUseMimeType = true;
		
	/* If we have the default mimetype then overjump the database lookup */
	if( bUseMimeType && zAttributeMimeType == "" )
		goto no_database;
		
	/* Check for filetype */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{	
			/* Search the mimetype */
			for( uint j = 0; j < m_cUsers[i].m_cTypes.size(); j++ )
			{
				
				if( bUseMimeType && m_cUsers[i].m_cTypes[j].m_zMimeType == zMimeType )
				{
					/* Check for a already known mimetype */
					if( !bLink )
						zName = m_cUsers[i].m_cTypes[j].m_zIdentifier;
					struct stat sStat;
					
					/* Add handlers */
					cReply.AddInt32( "handler_count", m_cUsers[i].m_cTypes[j].m_cHandlers.size() );
					for( uint h = 0; h < m_cUsers[i].m_cTypes[j].m_cHandlers.size(); h++ )
						cReply.AddString( "handler", m_cUsers[i].m_cTypes[j].m_cHandlers[h] );
					cReply.AddString( "default_handler", m_cUsers[i].m_cTypes[j].m_zDefaultHandler );
					
					/* Check if the icon exist */
					if( !( m_cUsers[i].m_cTypes[j].m_zIcon == "" ) &&
						lstat( m_cUsers[i].m_cTypes[j].m_zIcon.c_str(), &sStat ) >= 0 )
							zIcon = m_cUsers[i].m_cTypes[j].m_zIcon;
					break;
				} 
				else 
				{
					/* Check file extension */
					for( uint ext = 0; ext < m_cUsers[i].m_cTypes[j].m_cExtensions.size(); ext++ )
					{
						//std::cout<<m_cUsers[i].m_cTypes[j].m_cExtensions[ext].c_str()<<" "<<
						//m_cUsers[i].m_cTypes[j].m_cExtensions[ext].Length()<<" "<<zExtension.c_str()<<std::endl;
						if( m_cUsers[i].m_cTypes[j].m_cExtensions[ext] == zExtension )
						{
							zMimeType = m_cUsers[i].m_cTypes[j].m_zMimeType;
							if( !bLink )
								zName = m_cUsers[i].m_cTypes[j].m_zIdentifier;
							struct stat sStat;
							
							/* Add handlers */
							cReply.AddInt32( "handler_count", m_cUsers[i].m_cTypes[j].m_cHandlers.size() );
							for( uint h = 0; h < m_cUsers[i].m_cTypes[j].m_cHandlers.size(); h++ )
								cReply.AddString( "handler", m_cUsers[i].m_cTypes[j].m_cHandlers[h] );
							cReply.AddString( "default_handler", m_cUsers[i].m_cTypes[j].m_zDefaultHandler );
							
							/* Check if the icon exist */
							if( !( m_cUsers[i].m_cTypes[j].m_zIcon == "" ) &&
								lstat( m_cUsers[i].m_cTypes[j].m_zIcon.c_str(), &sStat ) >= 0 )
								zIcon = m_cUsers[i].m_cTypes[j].m_zIcon;
							break;
						}
					}
				}
			}
			break;
		}
	} 
	
no_database:
	
	/* Get Icon using attributes or resources */
	
	/* Try the os::Icon attribute */
	os::String zAttributeIcon;
	
	if( bBrokenLink )
		goto end; /* Should we try to read the os::Icon attribute from broken links */
	
	if( !( ( zAttributeIcon = GetAttribute( &cNode, "os::Icon" ) ) == "" ) )
	{
		/* If we have a directory then check if we have a relative icon path */
		if( bDirectory && zAttributeIcon[0] != '/' )
		{
			zAttributeIcon = zPath + "/" + zAttributeIcon;
		}
		
		struct stat sStat;
		if( lstat( zAttributeIcon.c_str(), &sStat ) >= 0 )
			zIcon = zAttributeIcon;
	} else if( bExecutable ) {
		/* Try resources if we have an executable */
		bool bError = false;
		os::File cFile( zPath );
		os::Resources* pcCol = NULL;
		
		/* Use best icon */
		os::String zFirstResource = ( cIconSize.x < 32 && cIconSize.y < 32 ) ? "icon24x24.png" : "icon48x48.png";
		os::String zSecondResource = ( cIconSize.x < 32 && cIconSize.y < 32 ) ? "icon48x48.png" : "icon24x24.png";
		
		try
		{
			pcCol = new os::Resources( &cFile );
		} catch( ... ) {
			bError = true;
		}
			
		if( !bError ) {
			os::ResStream *pcStream = pcCol->GetResourceStream( zFirstResource );
			if( pcStream ) {
				zIcon = os::String( "resource://" ) + zFirstResource;
				delete( pcStream );
			} else {
				pcStream = pcCol->GetResourceStream( zSecondResource );
				if( pcStream ) {
					zIcon = os::String( "resource://" ) + zSecondResource;
					delete( pcStream );
				}
			}
			delete( pcCol );	
		}
	}

end:
	cReply.AddString( "mimetype", zMimeType );
	cReply.AddString( "identifier", zName );
	cReply.AddString( "icon", zIcon );
	
	
	pcMessage->SendReply( &cReply );
}

/* Register a call */
void Registrar::RegisterCall( Message* pcMessage )
{
	os::String zID;
	os::String zDescription;
	int64 nTargetPort;
	int64 nMessageCode;
	int64 nProcess;
	RegistrarCall_s sCall;
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "Registrar::RegisterCall() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "Registrar::RegisterCall() called without process\n" );
		return;
	}
	if( pcMessage->FindString( "description", &zDescription ) != 0 ) {
		dbprintf( "Registrar::RegisterCall() called without description\n" );
		return;
	}
	if( pcMessage->FindInt64( "target", &nTargetPort ) != 0 ) {
		dbprintf( "Registrar::RegisterCall() called without targer port\n" );
		return;
	}
	if( pcMessage->FindInt64( "message_code", &nMessageCode ) != 0 ) {
		dbprintf( "Registrar::RegisterCall() called without message code\n" );
		return;
	}
	
	/* Create new call object */
	sCall.m_nProcess = nProcess;
	sCall.m_zID = zID;
	sCall.m_nTargetPort = nTargetPort;
	sCall.m_nMessageCode = nMessageCode;
	sCall.m_zDescription = zDescription;
	
	m_cCalls.push_back( sCall );
	
	//dbprintf( "Call %s registered by %i:%i\n", zID.c_str(), (int)nProcess, (int)nTargetPort );
	
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_OK );
}

/* Unregister a call */
void Registrar::UnregisterCall( Message* pcMessage )
{
	os::String zID;
	int64 nProcess;
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "Registrar::UnregisterCall() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "Registrar::RegisterCall() called without process\n" );
		return;
	}
	
	/* Find call */
	for( uint i = 0; i < m_cCalls.size(); i++ )
	{
		if( m_cCalls[i].m_zID == zID && m_cCalls[i].m_nProcess == nProcess )
		{
			m_cCalls.erase( m_cCalls.begin() + i );
			
			//dbprintf( "Call %s unregistered by %i\n", zID.c_str(), (int)nProcess );
	
			if( pcMessage->IsSourceWaiting() )
				pcMessage->SendReply( REGISTRAR_OK );
				
			return;
		}
	}
	
	dbprintf( "Registrar::UnregisterCall(): Call %s by %i not present\n", zID.c_str(), (int)nProcess );
	
	
	if( pcMessage->IsSourceWaiting() )
		pcMessage->SendReply( REGISTRAR_ERROR );
}

/* Query a call */
void Registrar::QueryCall( Message* pcMessage )
{
	os::String zID;
	int64 nIndex;
	int64 nCounter = 0;
	Message cReply( REGISTRAR_OK );
	
	/* Unpack message */
	if( pcMessage->FindString( "id", &zID ) != 0 ) {
		dbprintf( "Registrar::QueryCall() called without id\n" );
		return;
	}
	if( pcMessage->FindInt64( "index", &nIndex ) != 0 ) {
		dbprintf( "Registrar::QueryCall() called without index\n" );
		return;
	}
	
	/* Find call */
	for( uint i = 0; i < m_cCalls.size(); i++ )
	{
		if( m_cCalls[i].m_zID == zID  )
		{
			if( nIndex == nCounter )
			{
				cReply.AddInt64( "target", m_cCalls[i].m_nTargetPort );
				cReply.AddInt64( "message_code", m_cCalls[i].m_nMessageCode );
				cReply.AddString( "description", m_cCalls[i].m_zDescription );
				
				pcMessage->SendReply( &cReply );
				return;
			}
			nCounter++;
		}
	}
	
	//dbprintf( "Registrar::QueryCall(): Call %s with index %i not present\n", zID.c_str(), (int)nIndex );
	
	pcMessage->SendReply( REGISTRAR_ERROR );
}

/* Called when a process is killed. Check the database users and calls */
void Registrar::ProcessKilled( Message* pcMessage )
{
	int64 nProcess;
	
	
	if( pcMessage->FindInt64( "process", &nProcess ) != 0 ) {
		dbprintf( "Registrar::ProcessKilled() called without process\n" );
		return;
	}
	
	/* Check if the process has used the database */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		for( uint j = 0; j < m_cUsers[i].m_cClients.size(); j++ )
		{
			RegistrarClient sClient = m_cUsers[i].m_cClients[j];
			if( sClient.m_nProcess == nProcess )
			{
				dbprintf( "Process %i forgot to close the database for user %s\n", (int)nProcess, m_cUsers[i].m_zUser.c_str() );
				
				
				os::Message cKillMsg( REGISTRAR_LOGOUT );
				cKillMsg.AddInt64( "port", sClient.m_hPort );
				cKillMsg.AddInt64( "process", nProcess );
				cKillMsg.AddString( "user", m_cUsers[i].m_zUser );
				Logout( &cKillMsg );
			}
		}
	}
	
	/* Delete calls owned by the process */
again:
	
	for( uint i = 0; i < m_cCalls.size(); i++ )
	{
		if( m_cCalls[i].m_nProcess == nProcess  )
		{
			dbprintf( "Process %i forgot to delete call %s\n", (int)nProcess, m_cCalls[i].m_zID.c_str() );
			m_cCalls.erase( m_cCalls.begin() + i );
			goto again; /* m_cCalls.size() has changed */
		}
	}
}

void Registrar::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case REGISTRAR_LOGIN:
			if( pcMessage->IsSourceWaiting() )
				Login( pcMessage );
		break;
		case REGISTRAR_LOGOUT:
			Logout( pcMessage );
		break;
		case REGISTRAR_GET_TYPE_COUNT:
			if( pcMessage->IsSourceWaiting() )
				GetTypeCount( pcMessage );
		break;
		case REGISTRAR_GET_TYPE:
			if( pcMessage->IsSourceWaiting() )
				GetType( pcMessage );
		break;
		case REGISTRAR_REGISTER_TYPE:
			RegisterType( pcMessage );
		break;
		case REGISTRAR_UNREGISTER_TYPE:
			UnregisterType( pcMessage );
		break;
		case REGISTRAR_REGISTER_TYPE_ICON:
			RegisterTypeIcon( pcMessage );
		break;
		case REGISTRAR_REGISTER_TYPE_HANDLER:
			RegisterTypeHandler( pcMessage );
		break;
		case REGISTRAR_CLEAR_TYPE_HANDLERS:
			ClearTypeHandlers( pcMessage );
		break;
		case REGISTRAR_REGISTER_TYPE_EXTENSION:
			RegisterTypeExtension( pcMessage );
		break;
		case REGISTRAR_CLEAR_TYPE_EXTENSIONS:
			ClearTypeExtensions( pcMessage );
		break;
		case REGISTRAR_SET_DEFAULT_HANDLER:
			SetDefaultHandler( pcMessage );
		break;
		case REGISTRAR_GET_TYPE_AND_ICON:
			if( pcMessage->IsSourceWaiting() )
				GetTypeAndIcon( pcMessage );
		break;
		case REGISTRAR_REGISTER_CALL:
			RegisterCall( pcMessage );
		break;
		case REGISTRAR_UNREGISTER_CALL:
			UnregisterCall( pcMessage );
		break;
		case REGISTRAR_QUERY_CALL:
			if( pcMessage->IsSourceWaiting() )
				QueryCall( pcMessage );
		break;
		case -1:
			ProcessKilled( pcMessage );
		break;
		default:
			Looper::HandleMessage( pcMessage );
		break;
	}
}


thread_id Registrar::Start()
{
	make_port_public( GetMsgPort() );
	dbprintf( "Registrar running at port %i\n", GetMsgPort() );
	
	return( Application::Run() );
}

bool Registrar::OkToQuit()
{
	return( false );
}

int main()
{
	while( 1 ) 
	{
		Registrar* pcServer = new Registrar();
		pcServer->Start();
	}
}



