/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/fs_attribs.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/image.h>
#include <gui/imageview.h>
#include <gui/treeview.h>
#include <util/message.h>
#include <util/messenger.h>
#include <util/string.h>
#include <util/application.h>
#include <storage/registrar.h>
#include <storage/path.h>
#include <storage/filereference.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cassert>
#include <sys/stat.h>
#include <time.h>
#include <string>

#include "catalogs/libsyllable.h"

#define GS( x, def )		( m_pcCatalog ? m_pcCatalog->GetString( x, def ) : def )

using namespace os;


/* Structure to hold an attribute for sorting */
struct sAttributeInfo
{
	os::String zName;
	os::String zValue;
};

/* Used to sort the attributes */
struct AttributeSort
{
	bool operator() ( const sAttributeInfo psX, const sAttributeInfo psY ) const
	{
		/* All attributes with category names come first */
		if( psX.zName.size() < 1 || psY.zName.size() < 1 )
			return( false );
			
		bool bCategoryInX = ( (int)(psX.zName.find( "::" )) != os::String::npos );
		bool bCategoryInY = ( (int)(psY.zName.find( "::" )) != os::String::npos );
		
		if( bCategoryInX && !bCategoryInY )
			return( true );
		
		if( !bCategoryInX && bCategoryInY )
			return( false );

		
		return ( psX.zName < psY.zName );
	}
};

class InfoWin : public os::Window
{
public:
	InfoWin( os::Rect cFrame, os::String zFile, const os::Messenger & cViewTarget, os::Message* pcChangeMsg ) : os::Window( cFrame, "info_window", "Info", os::WND_NOT_RESIZABLE )
	{
		m_pcCatalog = os::Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );

		m_zFile = zFile;
		SetTitle( zFile );
		os::Image* pcImage = NULL;
		os::String zType = GS( ID_MSG_FILEINFO_FILETYPE_UNKNOWN, "Unknown" );
		os::String zMimeType = "application/unknown";
		struct stat sStat;
		char zSize[255];
		char zDate[255];
		bool bBlockTypeSelection = false;
		os::RegistrarManager* pcManager = NULL;
		m_cMessenger = cViewTarget;
		m_pcChangeMsg = pcChangeMsg;
		
		/* Create file system node */
		os::FSNode cNode;
		if( cNode.SetTo( zFile ) != 0 )
		{
			throw errno_exception( "Failed to open node" );
		}
		
		/* Get stat */
		cNode.GetStat( &sStat );
		
		/* Calculate size string */
		if( S_ISDIR( sStat.st_mode ) || S_ISLNK( sStat.st_mode ) )
		{
			strcpy( zSize, GS( ID_MSG_FILEINFO_FILESIZE_ZERO, os::String( "None" ) ).c_str()  );
			bBlockTypeSelection = true;
		} else {
			if( sStat.st_size >= 1000000 )
				sprintf( zSize, "%i Mb", ( int )( sStat.st_size / 1000000 + 1 ) );
			else
				sprintf( zSize, "%i Kb", ( int )( sStat.st_size / 1000 + 1 ) );
		}
		
		
		/* Get the filetype and icon first */
		try
		{
			pcManager = os::RegistrarManager::Get();
			os::Message cTypeMessage;
			pcManager->GetTypeAndIcon( zFile, os::Point( 48, 48 ), &zType, &pcImage, &cTypeMessage );
			cTypeMessage.FindString( "mimetype", &zMimeType );
		} catch( ... )
		{
		}
		
		/* Calculate date and time */
		struct tm *psTime = localtime( &sStat.st_mtime );
		strftime( zDate, sizeof( zDate ), "%c", psTime );
		
		/* Get attributes and save them in a list */
		std::vector<struct sAttributeInfo> asAttributes;
		
		os::String zAttribute;
		while( cNode.GetNextAttrName( &zAttribute ) == 1 )
		{
			attr_info sInfo;
			char zBuffer[4096];
			if( cNode.StatAttr( zAttribute, &sInfo ) == 0 )
			{
				sAttributeInfo sAttrib;
				sAttrib.zName = zAttribute;
				sAttrib.zValue = GS( ID_MSG_FILEINFO_FILETYPE_UNKNOWN, "Unknown" );
				switch( sInfo.ai_type )
				{
					case ATTR_TYPE_STRING:
					{
						int nLength = cNode.ReadAttr( zAttribute, sInfo.ai_type, zBuffer, 0, 4095 );
						zBuffer[nLength] = 0;						
						sAttrib.zValue = zBuffer;
					}
					break;
					case ATTR_TYPE_INT32:
					{
						int32 nVal = 0;
						if( cNode.ReadAttr( zAttribute, sInfo.ai_type, &nVal, 0, sizeof( int32 ) ) != sizeof( int32 ) )
							break;
						sAttrib.zValue = os::Variant( nVal ).AsString();
					}
					break;
					case ATTR_TYPE_INT64:
					{
						int64 nVal = 0;
						if( cNode.ReadAttr( zAttribute, sInfo.ai_type, &nVal, 0, sizeof( int64 ) ) != sizeof( int64 ) )
							break;
						/* Special handling for the Media::Length attribute */
						if( zAttribute == "Media::Length" )
						{
							int nH = nVal / 3600;
							int nM = ( nVal - nH * 3600 ) / 60;
							int nS = ( nVal - nH * 3600 - nM * 60 );
							if( nVal >= 3600 )
								sprintf( zBuffer, "%i:%i:%i", nH, nM, nS );
							else
								sprintf( zBuffer, "%i:%i", nM, nS );
							sAttrib.zValue = zBuffer;							
						}
						else
							sAttrib.zValue = os::Variant( nVal ).AsString();
					}
					break;

				}
				asAttributes.push_back( sAttrib );
			}
		}

		/* Sort attributes */
		std::sort( asAttributes.begin(), asAttributes.end(  ), AttributeSort() );	
		
		/* Create main view */
		m_pcView = new os::LayoutView( GetBounds(), "main_view" );
	
		os::VLayoutNode* pcVRoot = new os::VLayoutNode( "v_root" );
		pcVRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
		
	
		os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node", 0.0f );
		
		/* Icon */
		m_pcImageView = new os::ImageView( os::Rect(), "icon", pcImage );
		
		pcHNode->AddChild( m_pcImageView, 0.0f );
		pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		
		os::VLayoutNode* pcVInfo = new os::VLayoutNode( "v_info" );
		
		/* Name */
		os::HLayoutNode* pcHFileName = new os::HLayoutNode( "h_filename" );
		
		m_pcFileNameLabel = new os::StringView( os::Rect(), "filename_label", GS( ID_MSG_FILEINFO_WINDOW_NAME_LABEL, "Name:" ) );	
		m_pcFileName = new os::StringView( os::Rect(), "filename", os::Path( zFile ).GetLeaf() );	
	
		pcHFileName->AddChild( m_pcFileNameLabel, 0.0f );
		pcHFileName->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHFileName->AddChild( m_pcFileName );
		
		
		/* Path */
		os::HLayoutNode* pcHPath = new os::HLayoutNode( "h_path" );
		
		m_pcPathLabel = new os::StringView( os::Rect(), "path_label", GS( ID_MSG_FILEINFO_WINDOW_PATH_LABEL, "Path:" ) );	
		m_pcPath = new os::StringView( os::Rect(), "path", os::Path( zFile ).GetDir() );	
	
		pcHPath->AddChild( m_pcPathLabel, 0.0f );
		pcHPath->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHPath->AddChild( m_pcPath );
		
		/* Size */
		os::HLayoutNode* pcHSize = new os::HLayoutNode( "h_size" );
		
		m_pcSizeLabel = new os::StringView( os::Rect(), "size_label", GS( ID_MSG_FILEINFO_WINDOW_SIZE_LABEL, "Size:" ) );	
		m_pcSize = new os::StringView( os::Rect(), "size", zSize );	
	
		pcHSize->AddChild( m_pcSizeLabel, 0.0f );
		pcHSize->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHSize->AddChild( m_pcSize );
		
		/* Data */
		os::HLayoutNode* pcHDate = new os::HLayoutNode( "h_date" );
		
		m_pcDateLabel = new os::StringView( os::Rect(), "date_label", GS( ID_MSG_FILEINFO_WINDOW_MODIFIEDDATE_LABEL, "Modified:" ) );	
		m_pcDate = new os::StringView( os::Rect(), "date", zDate );	
	
		pcHDate->AddChild( m_pcDateLabel, 0.0f );
		pcHDate->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHDate->AddChild( m_pcDate );
		
		/* Type */
		
		os::HLayoutNode* pcHType = new os::HLayoutNode( "h_type" );
		
		m_pcTypeLabel = new os::StringView( os::Rect(), "type_label", GS( ID_MSG_FILEINFO_WINDOW_FILETYPE_LABEL, "Type:" ) );
		m_pcTypeSelector = new os::DropdownMenu( os::Rect(), "type" );	
		m_pcTypeSelector->SetEnable( !bBlockTypeSelection );
		
		pcHType->AddChild( m_pcTypeLabel, 0.0f );
		pcHType->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHType->AddChild( m_pcTypeSelector );
		
		pcHType->AddChild( new os::HLayoutSpacer( "" ) );
		
		
		/* Attributes tree */
		os::TreeView* pcAttributes = NULL;
		float vAttributeTreeHeight = 0;
		if( asAttributes.size() > 0 )
		{
			pcAttributes = new os::TreeView( os::Rect(), "attributes", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
			pcAttributes->SetDrawExpanderBox( true );
			pcAttributes->SetDrawTrunk( false );
			pcAttributes->InsertColumn( "Attributes", 150 );
			pcAttributes->InsertColumn( "", 1000 );
			os::String zLastCategory = "";
			
			for( uint i = 0; i < asAttributes.size(); i++ )
			{
				/* Add the attributes. They are already sorted */
				int nCategory = 0;
				if( ( nCategory = asAttributes[i].zName.find( "::" ) ) != os::String::npos )
				{
					if( !( asAttributes[i].zName.substr( 0, nCategory ) == zLastCategory ) )
					{
						os::TreeViewStringNode* pcCategory = new os::TreeViewStringNode();
						zLastCategory = asAttributes[i].zName.substr( 0, nCategory );
						if( zLastCategory == "os" )
							pcCategory->AppendString( "System" );
						else
							pcCategory->AppendString( zLastCategory );
						pcCategory->AppendString( "" );
						pcCategory->SetIndent( 1 );
						pcAttributes->InsertNode( pcCategory );
						vAttributeTreeHeight += pcCategory->GetHeight( pcAttributes );					
					}
					os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
					pcNode->AppendString( asAttributes[i].zName.substr( nCategory + 2, asAttributes[i].zName.Length() - nCategory - 2 ) );
					pcNode->AppendString( asAttributes[i].zValue );
					pcNode->SetIndent( 2 );
					pcAttributes->InsertNode( pcNode );
					vAttributeTreeHeight += pcNode->GetHeight( pcAttributes );
				}
				else
				{
					os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
					pcNode->AppendString( asAttributes[i].zName );
					pcNode->AppendString( asAttributes[i].zValue );				
					pcNode->SetIndent( 1 );
					pcAttributes->InsertNode( pcNode );
	
					vAttributeTreeHeight += pcNode->GetHeight( pcAttributes );
				}
			}
		}
		
		pcVInfo->AddChild( pcHFileName );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHPath );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHSize );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHDate );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHType );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		
		pcVInfo->SameWidth( "filename_label", "path_label", "size_label", "date_label", "type_label", NULL );
		
		pcHNode->AddChild( pcVInfo );
		
		
		
		/* create buttons */
		os::HLayoutNode* pcHButtons = new os::HLayoutNode( "h_buttons", 0.0f );
		
		m_pcOk = new os::Button( os::Rect(), "ok", 
						GS( ID_MSG_FILEINFO_WINDOW_OK_BUTTON, "Ok" ), new os::Message( 0 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
		pcHButtons->AddChild( new os::HLayoutSpacer( "" ) );
		pcHButtons->AddChild( m_pcOk, 0.0f );
	
		pcVRoot->AddChild( pcHNode );
		pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		if( asAttributes.size() > 0 ) {
			pcVRoot->AddChild( pcAttributes );
			pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		} else
			pcVRoot->AddChild( new os::VLayoutSpacer( "" ) );
		pcVRoot->AddChild( pcHButtons );
		
		
		/* Populate dropdown menu */
		m_pcTypeSelector->AppendItem( GS( ID_MSG_FILEINFO_WINDOW_FILETYPE_DROPDOWN_CURRENT_LABEL, os::String( "Current: " ) ) + zType );
		m_pcTypeSelector->AppendItem( GS( ID_MSG_FILEINFO_WINDOW_FILETYPE_DROPDOWN_EXECUTABLE, "Executable" ) );
		if( !bBlockTypeSelection && pcManager )
		{
			for( int32 i = 0; i < pcManager->GetTypeCount(); i++ )
			{
				os::Message cMsg = pcManager->GetType( i );
				os::String zRType;
				if( cMsg.FindString( "identifier", &zRType ) == 0 )
					m_pcTypeSelector->AppendItem( zRType );
			}
		}
		
		
		m_pcTypeSelector->SetSelection( 0, false );
		
		
		m_pcView->SetRoot( pcVRoot );
	
		AddChild( m_pcView );
		
		m_pcOk->SetTabOrder( 0 );
		
		SetDefaultButton( m_pcOk );
		
		m_pcOk->MakeFocus();
		
		/* Resize window */
		if( asAttributes.size() == 0 ) {
			ResizeTo( m_pcView->GetPreferredSize( false ) );			
			SetFlags( os::WND_NOT_RESIZABLE );
		} else {
			SetSizeLimits( m_pcView->GetPreferredSize( false ) + os::Point( 0, 40 ), os::Point( USHRT_MAX, USHRT_MAX ) );
			ResizeTo( m_pcView->GetPreferredSize( false ) + os::Point( 0, 40 ) + 
						os::Point( 0, ( vAttributeTreeHeight < 200 ) ? vAttributeTreeHeight : 200 ) );				
		}

		if( pcManager )
			pcManager->Put(); 
		
	}

	~InfoWin() { 
		os::Image* pcImage = m_pcImageView->GetImage();
		m_pcImageView->SetImage( NULL );
		delete( pcImage );
		if( m_pcCatalog )
			m_pcCatalog->Release();
	}
	
	
	void HandleMessage( os::Message* pcMessage )
	{
		
		switch( pcMessage->GetCode() )
		{
			
			case 0:
			{
				int nSelection = m_pcTypeSelector->GetSelection();
				if( nSelection == 1 )
				{
					/* Executable */
					char zString[255] = "application/x-executable";
					int nFd = open( m_zFile.c_str(), O_RDWR );
					if( nFd >= 0 )
					{
						write_attr( nFd, "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, zString,
							0, strlen( zString ) );
						close( nFd );
					}
				}
				else if( nSelection > 1 )
				{
					/* Get mimetype */
					try
					{
						os::RegistrarManager* pcManager = os::RegistrarManager::Get();
						os::Message cMsg = pcManager->GetType( nSelection - 2 );
						os::String zMimeType;
						if( cMsg.FindString( "mimetype", &zMimeType ) == 0 )
						{
							/* Write it */
							int nFd = open( m_zFile.c_str(), O_RDWR );
							if( nFd >= 0 )
							{
								write_attr( nFd, "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, zMimeType.c_str(),
								0, zMimeType.Length() );
								close( nFd );
							}
						}
						pcManager->Put();
					} catch( ... )
					{
					}
				}
				
				if( nSelection > 0 && m_pcChangeMsg )
					m_cMessenger.SendMessage( m_pcChangeMsg );
				
				PostMessage( os::M_QUIT );
				break;
			}
			default:
				break;
		}
		
		os::Window::HandleMessage( pcMessage );
	}
	
private:
	os::Messenger		m_cMessenger;
	os::Message*		m_pcChangeMsg;
	os::String			m_zFile;
	os::LayoutView*		m_pcView;
	os::ImageView*		m_pcImageView;
	os::StringView*		m_pcFileNameLabel;
	os::StringView*		m_pcFileName;
	os::StringView*		m_pcPathLabel;
	os::StringView*		m_pcPath;
	os::StringView*		m_pcSizeLabel;
	os::StringView*		m_pcSize;
	os::StringView*		m_pcDateLabel;
	os::StringView*		m_pcDate;
	os::StringView*		m_pcTypeLabel;
	os::DropdownMenu*	m_pcTypeSelector;
	os::Button*			m_pcOk;
	os::Catalog*		m_pcCatalog;
};


/** Create a dialog which shows information about a file or directory.
 * \par Description:
 *	This dialog will show information about a file like its name, size
 *  and type. It is also possible to change the file type. If any 
 *  changes have been made, then a message will be sent to the supplied 
 *  message target.
 * \par Note:
 *	Don’t forget to place the window at the right position and show it.
 * \param cMsgTarget - The target that will receive the message.
 * \param pcMsg - The message that will be sent.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

Window* FileReference::InfoDialog( const Messenger & cMsgTarget, Message* pcChangeMsg )
{
	os::String cPath;
	GetPath( &cPath );
	InfoWin* pcWin = new InfoWin( os::Rect( 50, 50, 350, 300 ), cPath, cMsgTarget, pcChangeMsg );
	
	return( pcWin );
}



