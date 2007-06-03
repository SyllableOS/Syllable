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

class ProgressView : public os::View
{
public:
	ProgressView( os::Rect cFrame ) : View( cFrame, "progress_view" )
	{
		m_nProgress = 0;
		SetEraseColor( os::get_default_color( os::COL_SHINE ) );
	}
	~ProgressView()
	{
	}
	void Paint( const os::Rect& cUpdate )
	{
		float vX = m_nProgress - 6;
			
		SetFgColor( os::get_default_color( os::COL_SEL_WND_BORDER ) );
			
		os::Color32_s sDefaultColor = os::get_default_color( os::COL_SEL_WND_BORDER );
		while( vX < GetBounds().Width() + 13 )
		{
			for( int i = -6; i < 7; i++ )
			{
				os::Color32_s sColor;
				sColor.red = sDefaultColor.red + ( 255 - sDefaultColor.red ) * abs( i ) / 6;
				sColor.blue = sDefaultColor.blue + ( 255 - sDefaultColor.blue ) * abs( i ) / 6;
				sColor.green = sDefaultColor.green + ( 255 - sDefaultColor.green ) * abs( i ) / 6;
				
				SetFgColor( sColor );
				FillRect( os::Rect( vX + i, 0, vX + i, GetBounds().Height() ) );
			}
			//EraseRect( os::Rect( vX + 5, 0, vX + 9, GetBounds().Height() ) );
			vX += 13;
		}
	}
	void AttachedToWindow()
	{
		GetWindow()->AddTimer( this, 0, 100000, false );
	}
	void DetachedFromWindow()
	{
		GetWindow()->RemoveTimer( this, 0 );
	}
	void TimerTick( int nID )
	{
		m_nProgress += 1;
		m_nProgress %= 13;
		Paint( GetBounds() );
		Flush();
	}
private:
	int m_nProgress;
};

class ProgressWindow : public os::Window
{
public:
	ProgressWindow( os::String cMessage ) : os::Window( os::Rect( 0, 0, 200, 60 ), "window", "",os::WND_NO_BORDER )
	{
		os::StringView* pcStringView = new os::StringView( os::Rect( 10, 10, 190, 35 ), "string_view", cMessage, os::ALIGN_CENTER );
		ProgressView* pcView = new ProgressView( os::Rect( 20, 40, 180, 50 ) );
		AddChild( pcStringView );
		AddChild( pcView );
	}
};

Registrar::Registrar()
			: Application( "registrar" )
{
	m_cUsers.clear();
	
	/* Load the database of root */
	LoadTypes( "root", GetMsgPort(), -1 );
	
	/* Create application list event */
	m_pcAppListEvent = os::Event::Register( "os/Registrar/AppList", "Called when the application list has changed", this,
											os::M_REPLY );
}

Registrar::~Registrar()
{
	delete( m_pcAppListEvent );
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
	cUser.m_bAppListValid = false;
	cUser.m_cApps.clear();
	
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

RegistrarUser* Registrar::GetUser( const os::String& zUser )
{
	/* Search user */
	for( uint i = 0; i < m_cUsers.size(); i++ )
	{
		if( m_cUsers[i].m_zUser == zUser )
		{	
			return( &m_cUsers[i] );
		}
	}
	return( NULL );
}

/* Return a type identified with its user and mimetype */
FileType* Registrar::GetType( const os::String& zUser, const os::String& zMimeType )
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
		
		for( int e = zLeaf.Length() - 1; e >= 0; e-- )
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

/* Returns the application list */
void Registrar::GetAppList( Message* pcMessage )
{
	os::String zUser;
	if( pcMessage->FindString( "user", &zUser ) != 0 )
	{
		dbprintf( "Registrar::GetAppList() called without user\n" );
		pcMessage->SendReply( REGISTRAR_ERROR );		
		return;
	}
	

	RegistrarUser* psUser = GetUser( zUser );
	if( psUser == NULL )
		pcMessage->SendReply( REGISTRAR_ERROR );
	
	UpdateAppList( psUser, false );
	
	os::Message cReply( REGISTRAR_OK );
	cReply.AddInt32( "count", (int32)psUser->m_cApps.size() );
	for( uint i = 0; i < psUser->m_cApps.size(); i++ )
	{
		cReply.AddString( "name", psUser->m_cApps[i].zName );
		cReply.AddString( "path", psUser->m_cApps[i].zPath );
		cReply.AddString( "category", psUser->m_cApps[i].zCategory );
	}
	
	pcMessage->SendReply( &cReply );		
}


/* Scan one path for applications */
void Registrar::ScanAppPath( RegistrarUser* psUser, int nLevel, os::Path cPath, os::String cPrimaryLanguage )
{
	
	/* Check if the directory is valid */
	os::Directory* pcDir = NULL;
	try
	{
		pcDir = new os::Directory( cPath.GetPath() );
	} catch( ... ) {
		return;
	}
	if( !pcDir->IsValid() )
	{
		delete( pcDir );
		return;
	}
	
	/* Add node monitor if necessary */
	bool bFound = false;
	for( uint i = 0; i < m_cMonitors.size(); i++ )
	{
		if( m_cMonitors[i]->m_cPath == cPath )
		{
			bFound = true;
			break;
		}
	}
	if( !bFound )
	{
		RegistrarMonitor* pcMonitor = new RegistrarMonitor( cPath, NWATCH_ALL, this );
		m_cMonitors.push_back( pcMonitor );	
	}
	/* Iterate through the directory */
	os::String zFile;
	
	while( pcDir->GetNextEntry( &zFile ) == 1 )
	{
		if( zFile == os::String( "." ) ||
			zFile == os::String( ".." ) )
			continue;
		os::Path cFilePath = cPath;
		
		/* We do not want any plugins */
		if( os::String( zFile ).Lower() == "plugins" || os::String( zFile ).Lower() == "lib" )
			continue;
			
		
		cFilePath.Append( zFile.c_str() );
		
		os::Image* pcItemIcon = NULL;
		
		/* Get icon */
		os::FSNode cFileNode;
		if( cFileNode.SetTo( cFilePath ) != 0 )
		{
			continue;
		}
		
		if( cFileNode.IsDir() )
		{
			cFileNode.Unset();
			if( nLevel < 5 )
				ScanAppPath( psUser, nLevel + 1, cFilePath, cPrimaryLanguage );
			continue;
		}
		
		/* Set default category */
		os::String zCategory = "Other";
		if( cPath.GetLeaf() == "Preferences" )
			zCategory = "Preferences";
		
		
		/* Check if this is an executable and read the category attribute */
		cFileNode.RewindAttrdir();
		os::String zAttrib;

		bool bExecAttribFound = false;
		bool bCategoryAttribFound = false;
		while( cFileNode.GetNextAttrName( &zAttrib ) == 1 )
		{
			if( zAttrib == "os::MimeType" || zAttrib == "os::Category" )
			{
				char zBuffer[PATH_MAX];
				memset( zBuffer, 0, PATH_MAX );
				if( cFileNode.ReadAttr( zAttrib, ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX ) > 0 )
				{
					if( os::String( zBuffer ) == "application/x-executable" ) {
						bExecAttribFound = true;
					} else {
						zCategory = zBuffer;
						bCategoryAttribFound = true;
					}
				}
			}
		}

		if( !bExecAttribFound && !(	cFileNode.GetMode() & ( S_IXUSR|S_IXGRP|S_IXOTH ) ) || zCategory == "Ignore" )
		{
			cFileNode.Unset();
			continue;
		}
		
		RegistrarApp cApp;
		cApp.zPath = cFilePath;
		cApp.zName = zFile;
		cApp.zCategory = zCategory;
		
		/* Try to get application name from the catalog */
		if( !cPrimaryLanguage.empty() )
		{
			try
			{
				os::File cResFile( cFilePath );
				os::Resources cRes( &cResFile );
				for( int i = 0 ; i < cRes.GetResourceCount() ; ++i )
				{
					os::String cCatalogName = cRes.GetResourceName( i );
					if( strstr( cCatalogName.c_str(), ".catalog" ) != 0 && strstr( cCatalogName.c_str(), "/" ) == 0)
					{
						os::ResStream* pcSrc = cRes.GetResourceStream( ( cPrimaryLanguage + "/" + cCatalogName ) );
						if( pcSrc != NULL )
						{
							os::Catalog c;
							c.Load( pcSrc );
							cApp.zName = c.GetString( 0, cPath.GetLeaf() );
							delete( pcSrc );
						}
						break;
					}
				}
			} catch( ... ) { }
		}
		
		psUser->m_cApps.push_back( cApp );
		
	}	
}


/* Update the application list if necessary */
void Registrar::UpdateAppList( RegistrarUser* psUser, bool bForce )
{
	if( psUser->m_bAppListValid && !bForce )
		return;
		
	/* Delete monitors */
	for( uint i = 0; i < m_cMonitors.size(); ++i )
	{
		delete( m_cMonitors[i] );
	}
	m_cMonitors.clear();
	

	/* We would like to know the users primary language, so we can get the application-names */
	os::String cPrimaryLanguage;
	try {
		os::Settings* pcSettings = new os::Settings( new os::File( os::String( "/home/" ) + psUser->m_zUser + os::String( "/Settings/System/Locale" ) ) );
		pcSettings->Load();
		cPrimaryLanguage = pcSettings->GetString("LANG","",0);
		delete( pcSettings );
	} catch(...) { }

	
	/* Create progress window */
	ProgressWindow* pcWindow = new ProgressWindow( "Updating application database...\n" );
	pcWindow->CenterInScreen();
	pcWindow->Show();
		
	/* Clear list */
	psUser->m_cApps.clear();
	
	/* Scan */
	ScanAppPath( psUser, 0, os::Path( "/boot/Applications" ), cPrimaryLanguage );
	
	pcWindow->PostMessage( os::M_QUIT );
	
	
	psUser->m_bAppListValid = true;
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
		case REGISTRAR_GET_APP_LIST:
			if( pcMessage->IsSourceWaiting() )
				GetAppList( pcMessage );
		break;
		case REGISTRAR_UPDATE_APP_LIST:
		{
			bool bForce = false;
			os::String zUser;
			if( pcMessage->FindString( "user", &zUser ) != 0 )
			{
				dbprintf( "Registrar::UpdateAppList() called without user\n" );
				if( pcMessage->IsSourceWaiting() )
					pcMessage->SendReply( REGISTRAR_ERROR );
				break;
			}
	
			RegistrarUser* psUser = GetUser( zUser );
			if( psUser == NULL ) {
				if( pcMessage->IsSourceWaiting() )
				{			
					pcMessage->SendReply( REGISTRAR_ERROR );
					break;
				}
			}

			pcMessage->FindBool( "force", &bForce );
			UpdateAppList( psUser, bForce );
			if( pcMessage->IsSourceWaiting() )
				pcMessage->SendReply( REGISTRAR_OK );
		}
		break;
		case -1:
			ProcessKilled( pcMessage );
		break;
		case os::M_NODE_MONITOR:
		{
			/* Delete monitors and invalidate the application lists */
			for( uint i = 0; i < m_cUsers.size(); i++ )
			{
				m_cUsers[i].m_bAppListValid = false;
			}
			for( uint i = 0; i < m_cMonitors.size(); ++i )
			{
				delete( m_cMonitors[i] );
			}
			m_cMonitors.clear();
			/* Send event */
			os::Message cDummy;
			m_pcAppListEvent->PostEvent( &cDummy );
	
			break;
		}
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



