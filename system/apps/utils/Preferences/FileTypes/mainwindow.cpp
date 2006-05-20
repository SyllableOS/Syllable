#include <storage/file.h>
#include "mainwindow.h"
#include "messages.h"
#include "newtype.h"
#include <iostream>


/* Main window */

MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 500, 400 ), "main_wnd", "File Types" )
{
	/* Create type manager */
	try
	{
		m_pcManager = NULL;
		m_pcManager = os::RegistrarManager::Get();
		m_pcManager->SetSynchronousMode( true );
	} catch( ... )
	{
		PostMessage( os::M_QUIT );
		return;
	}
	
	/* Dummy image */
	uint8* pData = (uint8*)malloc( 48 * 48 * 4 );
	memset( pData, 0, 48 * 48 * 4 );
	m_pcDummyImage = new os::BitmapImage( pData, os::IPoint( 48, 48 ), os::CS_RGB32 );
	delete( pData );
	
	/* Create root view */
	m_pcRoot = new os::LayoutView( GetBounds(), "type_prefs_root" );
	
	/* Create vertical node */
	m_pcHLayout = new os::HLayoutNode( "type_prefs_hlayout" );
	m_pcHLayout->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	
	/* Create list of types */
	m_pcTypeList = new os::TreeView( os::Rect(), "type_prefs_list", os::ListView::F_RENDER_BORDER | os::ListView::F_NO_HEADER,
									0, os::WID_FULL_UPDATE_ON_RESIZE );
	m_pcTypeList->InsertColumn( "Type", 200 );
	m_pcTypeList->SetSelChangeMsg( new os::Message( M_TYPE_SELECT ) );
	
	
	/* Create type vnode and its content */
	m_pcVType = new os::VLayoutNode( "type_prefs_vtype" );
	m_pcVType->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	
	/* ---------------------------- General settings --------------------------------- */
	m_pcGeneralFrame = new os::FrameView( os::Rect(), "type_prefs_gframe", "General" );
	
	os::HLayoutNode* pcHGeneral = new os::HLayoutNode( "type_prefs_hgeneral" );
	pcHGeneral->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	
	os::VLayoutNode* pcVGeneral = new os::VLayoutNode( "type_prefs_vgeneral" );
	
	/* Icon */
	m_pcIconReq = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "/system/icons",
										os::FileRequester::NODE_FILE, false );
	m_pcIconReq->Start();
										
	m_pcIconChooser = new os::ImageButton( os::Rect(), "type_prefs_icon", "", new os::Message( M_ICON ),
											new os::BitmapImage( *m_pcDummyImage ), os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
	pcHGeneral->AddChild( m_pcIconChooser, 0.0f );
	pcHGeneral->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	/* Mimetype */
	os::HLayoutNode* pcHMimeType = new os::HLayoutNode( "type_prefs_hmimetype" );
	os::StringView* pcMimeTypeLabel = new os::StringView( os::Rect(), "type_prefs_mlabel", "Mime Type:" );
	m_pcMimeType = new os::StringView( os::Rect(), "type_prefs_mimetype", "" );
	pcHMimeType->AddChild( pcMimeTypeLabel );
	pcHMimeType->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHMimeType->AddChild( m_pcMimeType );
	pcVGeneral->AddChild( pcHMimeType );
	pcVGeneral->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	/* Identifier */
	os::HLayoutNode* pcHIdentifier = new os::HLayoutNode( "type_prefs_identifier" );
	os::StringView* pcIdentifierLabel = new os::StringView( os::Rect(), "type_prefs_ilabel", "Name:" ); 
	m_pcIdentifier = new os::TextView( os::Rect(), "type_prefs_identifier", "" );
	pcHIdentifier->AddChild( pcIdentifierLabel );
	pcHIdentifier->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHIdentifier->AddChild( m_pcIdentifier );
	pcVGeneral->AddChild( pcHIdentifier );
	pcVGeneral->SameWidth( "type_prefs_mlabel", "type_prefs_ilabel", NULL );
	
	pcHGeneral->AddChild( pcVGeneral );
	
	m_pcGeneralFrame->SetRoot( pcHGeneral );
	m_pcVType->AddChild( m_pcGeneralFrame, 0.0f );
	
	
	
	/* ------------------------------ Extensions -------------------------------- */
	m_pcExtFrame = new os::FrameView( os::Rect(), "type_prefs_eframe", "Extensions" );
	
	os::VLayoutNode* pcVExtensions = new os::VLayoutNode( "type_prefs_extensions_v" );
	pcVExtensions->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	
	/* List */
	m_pcExtensionList = new os::TreeView( os::Rect(), "type_prefs_extensions_list", os::ListView::F_RENDER_BORDER, 
									0, os::WID_FULL_UPDATE_ON_RESIZE );
	m_pcExtensionList->InsertColumn( "Extensions", 500 );
	m_pcExtensionList->SetSelChangeMsg( new os::Message( M_EXTENSION_SELECT ) );
	pcVExtensions->AddChild( m_pcExtensionList );
	pcVExtensions->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	os::HLayoutNode* pcHExtensions = new os::HLayoutNode( "type_prefs_extensions_h", 0.0f );
	
	
	m_pcExtension = new os::TextView( os::Rect(), "type_prefs_extension", "" );
	pcHExtensions->AddChild( m_pcExtension );
	pcHExtensions->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	/* Buttons */
	m_pcExtensionAdd = new os::Button( os::Rect(), "type_prefs_ext_add", "Add", new os::Message( M_ADD_EXTENSION ) );
	m_pcExtensionRemove = new os::Button( os::Rect(), "type_prefs_ext_remove", "Remove", new os::Message( M_REMOVE_EXTENSION ) );
	
	pcHExtensions->AddChild( m_pcExtensionAdd, 0.0f );
	pcHExtensions->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHExtensions->AddChild( m_pcExtensionRemove, 0.0f );
	pcHExtensions->SameWidth( "type_prefs_ext_add", "type_prefs_ext_remove", NULL );

	pcVExtensions->AddChild( pcHExtensions );
	
	m_pcExtFrame->SetRoot( pcVExtensions );
	m_pcVType->AddChild( m_pcExtFrame );
	
	
	
	/* ------------------------------ Handlers --------------------------------- */
	m_pcHandlerFrame = new os::FrameView( os::Rect(), "type_prefs_hframe", "Handlers" );
	
	os::VLayoutNode* pcVHandlers = new os::VLayoutNode( "type_prefs_handlers_v" );
	pcVHandlers->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	
	/* List */
	m_pcHandlerList = new os::TreeView( os::Rect(), "type_prefs_handlers", os::ListView::F_RENDER_BORDER, 
									0, os::WID_FULL_UPDATE_ON_RESIZE );
	m_pcHandlerList->InsertColumn( "Default handler", 500 );
	
	os::HLayoutNode* pcHHandlers = new os::HLayoutNode( "type_prefs_handlers_h", 0.0f );
	
	/* Buttons */
	m_pcAddHandler = new os::Button( os::Rect(), "type_prefs_addhandler", "Add handler...", new os::Message( M_ADD_HANDLER ) );
	m_pcRemoveHandler = new os::Button( os::Rect(), "type_prefs_removehandler", "Remove handler", new os::Message( M_REMOVE_HANDLER ) );
	
	pcHHandlers->AddChild( new os::HLayoutSpacer( "" ) );
	pcHHandlers->AddChild( m_pcAddHandler );
	pcHHandlers->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHHandlers->AddChild( m_pcRemoveHandler );
	pcHHandlers->SameWidth( "type_prefs_addhandler", "type_prefs_removehandler", NULL );
	
	pcVHandlers->AddChild( m_pcHandlerList );
	pcVHandlers->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVHandlers->AddChild( pcHHandlers );
	
	m_pcHandlerFrame->SetRoot( pcVHandlers );
	m_pcVType->AddChild( m_pcHandlerFrame );
	
	m_pcHandlerReq = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "/Applications",
										os::FileRequester::NODE_FILE, false );
	m_pcHandlerReq->Start();
	
	
	/* --------------------------------- Create buttons ------------------------------- */
	m_pcHButtons = new os::HLayoutNode( "type_prefs_buttons", 0.0f );
	m_pcAddType = new os::Button( os::Rect(), "type_prefs_addtype", "Add type...", new os::Message( M_ADD_TYPE ) );
	m_pcRemoveType = new os::Button( os::Rect(), "type_prefs_removetype", "Remove type", new os::Message( M_REMOVE_TYPE ) );
	
	m_pcApply = new os::Button( os::Rect(), "type_prefs_apply", "Apply", new os::Message( M_APPLY ) );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "" ) );
	m_pcHButtons->AddChild( m_pcAddType, 0.0f );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcHButtons->AddChild( m_pcRemoveType, 0.0f );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcHButtons->AddChild( m_pcApply, 0.0f );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVType->AddChild( new os::VLayoutSpacer( "", 15.0f, 15.0f ) );
	m_pcVType->AddChild( m_pcHButtons );
	
	m_pcHLayout->AddChild( m_pcTypeList );
	m_pcHLayout->AddChild( m_pcVType );
	
	//m_pcTypeFrame->SetRoot( m_pcVType );
	m_pcRoot->SetRoot( m_pcHLayout );
	AddChild( m_pcRoot );
	
	m_nCurrentType = -1;
	m_bIconReqShown = false;
	m_bHandlerReqShown = false;
	
	/* Register at least one type */
	m_pcManager->RegisterType( "text/plain", "Plain text" );
	m_pcManager->RegisterTypeIcon( "text/plain", os::Path( "/system/icons/file.png" ) );
	m_pcManager->RegisterTypeHandler( "text/plain", os::Path( "/system/bin/aedit" ) );
	
	/* Set Icon */
	os::File cFile( open_image_file( get_image_id() ) );
	os::Resources cCol( &cFile );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	delete( pcStream );

	UpdateList();
	
	m_pcTypeList->Select( 0 );
}

MainWindow::~MainWindow()
{
}

void MainWindow::UpdateList()
{
	m_pcTypeList->Clear();
	/* Populate the type list */
	for( int32 i = 0; i < m_pcManager->GetTypeCount(); i++ )
	{
		os::Message cMsg = m_pcManager->GetType( i );
		os::String zIdentifier;
		os::String zIcon;
		
		cMsg.FindString( "identifier", &zIdentifier );
		cMsg.FindString( "icon", &zIcon );
		
		os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
		pcNode->AppendString( zIdentifier );
		
		/* Load icon if possible */
		if( !( zIcon == "" ) )
		{
			try
			{
				os::File cIconFile( zIcon );
				os::BitmapImage* pcIcon = new os::BitmapImage();
				if( pcIcon->Load( &cIconFile ) == 0 )
				{
					pcIcon->SetSize( os::Point( 24, 24 ) );
					pcNode->SetIcon( pcIcon );
				}
			} catch( ... )
			{
			}
		}
		m_pcTypeList->InsertNode( pcNode, false );
		
		//DumpType( cMsg );
	}
	m_pcTypeList->RefreshLayout();
}

void MainWindow::UpdateIcon( bool bUpdateList )
{
	/* Update the icon */
	m_pcIconChooser->SetImage( new os::BitmapImage( *m_pcDummyImage ) );
	if( !( m_zIcon == "" ) )
	{
		try
		{
			os::File cIconFile( m_zIcon );
			os::BitmapImage* pcIcon = new os::BitmapImage();
			if( pcIcon->Load( &cIconFile ) == 0 )
			{
				if( !( pcIcon->GetSize() == os::Point( 48, 48 ) ) )
					pcIcon->SetSize( os::Point( 48, 48 ) );
				m_pcIconChooser->SetImage( pcIcon );
				/*m_pcIconReq->Lock();
				m_pcIconReq->SetPath( os::Path( m_zIcon ).GetDir().GetPath() );
				m_pcIconReq->Unlock();*/
			}
		} catch( ... )
		{
		}
	}
	m_pcIconChooser->Invalidate();
	
	/* Update list */
	if( bUpdateList )
	{
		int nIndex = m_pcTypeList->GetFirstSelected();
		os::TreeViewStringNode* pcRow = static_cast<os::TreeViewStringNode*>(m_pcTypeList->GetRow( nIndex ));
		os::BitmapImage* pcIconImage = static_cast<os::BitmapImage*>(m_pcIconChooser->GetImage());
		os::BitmapImage* pcNewImage = new os::BitmapImage( *pcIconImage );
		pcNewImage->SetSize( os::Point( 24, 24 ) );
		pcRow->SetIcon( pcNewImage );
	}
}

void MainWindow::SelectType( int32 nIndex )
{
	/* Close icon chooser */
	if( m_bIconReqShown )
	{
		m_pcIconReq->Lock();
		m_pcIconReq->Hide();
		m_pcIconReq->Unlock();
		m_bIconReqShown = false;
	}
	
	std::cout<<"Select "<<nIndex<<std::endl;
	
	/* Update type */
	os::Message cMsg = m_pcManager->GetType( nIndex );
	os::String zIdentifier;
	os::String zMimeType;
	os::String zHandler;
	os::String zExtension;
	os::String zDefaultHandler;
	int32 nHandlerCount;
	int32 nExtensionCount;
	
	cMsg.FindString( "mimetype", &zMimeType );
	cMsg.FindString( "identifier", &zIdentifier );
	m_pcMimeType->SetString( zMimeType );
	m_pcIdentifier->Set( zIdentifier.c_str() );
	
	/* Load icon if possible */
	cMsg.FindString( "icon", &m_zIcon );
	UpdateIcon( false );
	
	cMsg.FindInt32( "extension_count", &nExtensionCount );
	m_pcExtensionList->Clear();
	m_cExtensions.clear();
	for( int32 i = 0; i < nExtensionCount; i++ )
	{
		cMsg.FindString( "extension", &zExtension, i );
		/* Add extension to the list */
		os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
		pcNode->AppendString( zExtension );
		m_pcExtensionList->InsertNode( pcNode );
		m_cExtensions.push_back( zExtension );
	}
	cMsg.FindString( "default_handler", &zDefaultHandler );
	cMsg.FindInt32( "handler_count", &nHandlerCount );
	m_pcHandlerList->Clear();
	m_cHandlers.clear();
	for( int32 i = 0; i < nHandlerCount; i++ )
	{
		cMsg.FindString( "handler", &zHandler, i );
		/* Add handler to the list */
		os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
		pcNode->AppendString( zHandler );
		
		/* Try to get an icon for the handler */
		os::String zTemp;
		os::Image* pcImage = NULL;
		if( m_pcManager->GetTypeAndIcon( zHandler, os::Point( 24, 24 ), &zTemp, &pcImage ) == 0 
			&& pcImage )
				pcNode->SetIcon( pcImage );
		
		m_pcHandlerList->InsertNode( pcNode );
		if( zHandler == zDefaultHandler )
			m_pcHandlerList->Select( m_pcHandlerList->GetRowCount() - 1 );
		m_cHandlers.push_back( zHandler );
	}
	
}


void MainWindow::AddExtension()
{
	os::String zExtension = m_pcExtension->GetBuffer()[0];
	
	/* Check if the input field is empty */
	if( zExtension == "" )
		return;
	/* Check if the extension already exists */
	for( uint i = 0; i < m_cExtensions.size(); i++ )
		if( m_cExtensions[i] == zExtension )
			return;
	
	/* Add it */	
	os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
	pcNode->AppendString( zExtension );
	m_pcExtensionList->InsertNode( pcNode );
	m_cExtensions.push_back( zExtension );
}


void MainWindow::RemoveExtension()
{
	os::String zExtension = m_pcExtension->GetBuffer()[0];
	
	/* Check if the extension exists */
	for( uint i = 0; i < m_cExtensions.size(); i++ )
		if( m_cExtensions[i] == zExtension )
		{
			delete( m_pcExtensionList->RemoveRow( i ) );
			m_cExtensions.erase( m_cExtensions.begin() + i );
			return;
		}
}

void MainWindow::ApplyType()
{
	/* Apply current type settings */
	os::String zMimeType = m_pcMimeType->GetString();
	
	m_pcManager->RegisterType( zMimeType, m_pcIdentifier->GetBuffer()[0], true );
	
	m_pcManager->RegisterTypeIcon( zMimeType, m_zIcon, true );
	m_pcManager->ClearTypeExtensions( zMimeType );
	for( uint i = 0; i < m_cExtensions.size(); i++ )
		m_pcManager->RegisterTypeExtension( zMimeType, m_cExtensions[i] );
		
	m_pcManager->ClearTypeHandlers( zMimeType );
	for( uint i = 0; i < m_cHandlers.size(); i++ )
		m_pcManager->RegisterTypeHandler( zMimeType, m_cHandlers[i] );
	
	if( ( m_pcHandlerList->GetFirstSelected() >= 0 ) && 
		( m_pcHandlerList->GetFirstSelected() < (int)m_pcHandlerList->GetRowCount() ) )
	{
		std::cout<<"Default "<<m_cHandlers[m_pcHandlerList->GetFirstSelected()].c_str()<<std::endl;
		
		m_pcManager->SetDefaultHandler( zMimeType, m_cHandlers[m_pcHandlerList->GetFirstSelected()] );
		
	}
	
	/* Update name in list */
	int nIndex = m_pcTypeList->GetFirstSelected();
	os::TreeViewStringNode* pcRow = static_cast<os::TreeViewStringNode*>(m_pcTypeList->GetRow( nIndex ));
	pcRow->SetString( 0, m_pcIdentifier->GetBuffer()[0] );
	m_pcTypeList->RefreshLayout();
}

void MainWindow::DumpType( os::Message cMsg )
{
	os::String zMimeType;
	os::String zIdentifier;
	os::String zIcon;
	
	cMsg.FindString( "mimetype", &zMimeType );
	cMsg.FindString( "identifier", &zIdentifier );
	cMsg.FindString( "icon", &zIcon );
	
	std::cout<<"Dump: "<<zIdentifier.c_str()<<std::endl;
	std::cout<<"---------------------------"<<std::endl;
	std::cout<<"MimeType: "<<zMimeType.c_str()<<std::endl;
	std::cout<<"Icon: "<<zIcon.c_str()<<std::endl;
}

void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_APP_QUIT:
		{
			OkToQuit();
		}
		break;
		case M_ICON:
		{
			/* Open filerequester */
			if( m_bIconReqShown || m_bHandlerReqShown )
				break;
			m_pcIconReq->Lock();
			m_pcIconReq->Show();
			m_pcIconReq->MakeFocus();
			m_pcIconReq->Unlock();
			m_bIconReqShown = true;
		}
		break;
		case M_EXTENSION_SELECT:
		{
			/* Copy extension to the input field */
			if( ( m_pcExtensionList->GetFirstSelected() >= 0 )
				&& ( m_pcExtensionList->GetFirstSelected() < (int)m_pcExtensionList->GetRowCount() ) )
					m_pcExtension->Set( m_cExtensions[m_pcExtensionList->GetFirstSelected()].c_str() );
		}
		break;
		case M_ADD_EXTENSION:
		{
			AddExtension();
		}
		break;
		case M_REMOVE_EXTENSION:
		{
			RemoveExtension();
		}
		break;
		case M_ADD_TYPE:
		{
			NewTypeWin* pcWin = new NewTypeWin( this, os::Rect( 0, 0, 250, 65 ) );
			pcWin->CenterInWindow( this );
			pcWin->Show();
			pcWin->MakeFocus();
		}
		break;
		case M_REMOVE_TYPE:
		{
			if( ( m_pcTypeList->GetFirstSelected() >= 0 )
				&& ( m_pcTypeList->GetFirstSelected() < (int)m_pcTypeList->GetRowCount() ) )
			{
				if( m_pcTypeList->GetRowCount() < 2 )
					break;
				
				if( m_pcMimeType->GetString() == "" )
					break;
				
				/* Remove this type */
				m_pcManager->UnregisterType( m_pcMimeType->GetString() );
				UpdateList();
				m_nCurrentType = -1;
				m_pcTypeList->Select( 0 );
			}
		}
		break;
		case M_ADD_HANDLER:
		{
			/* Open filerequester */
			if( m_bHandlerReqShown || m_bIconReqShown )
				break;
			m_pcHandlerReq->Lock();
			m_pcHandlerReq->Show();
			m_pcHandlerReq->MakeFocus();
			m_pcHandlerReq->Unlock();
			m_bHandlerReqShown = true;
		}
		break;
		case M_REMOVE_HANDLER:
		{
			if( ( m_pcHandlerList->GetFirstSelected() >= 0 )
				&& ( m_pcHandlerList->GetFirstSelected() < (int)m_pcHandlerList->GetRowCount() ) )
			{
				m_cHandlers.erase( m_cHandlers.begin() + m_pcHandlerList->GetFirstSelected() );
				delete( m_pcHandlerList->RemoveRow( m_pcHandlerList->GetFirstSelected() ) );
			}
		}
		break;
		case M_APPLY:
		{
			ApplyType();
		}
		break;
		
		case M_TYPE_SELECT:
		{
			if( m_nCurrentType != m_pcTypeList->GetFirstSelected() )
				SelectType( m_pcTypeList->GetFirstSelected() );
			m_nCurrentType = m_pcTypeList->GetFirstSelected();
		}
		break;
		case os::M_LOAD_REQUESTED:
		{
			if( m_bIconReqShown )
			{
				if( pcMessage->FindString( "file/path", &m_zIcon ) == 0 )
				{
					UpdateIcon( true );
				}
				m_bIconReqShown = false;
			} else if( m_bHandlerReqShown )
			{
				os::String zHandler;
				if( pcMessage->FindString( "file/path", &zHandler ) == 0 )
				{
					if( m_pcMimeType->GetString() == "" )
						break;
					
					for( uint i = 0; i < m_cHandlers.size(); i++ )
						if( m_cHandlers[i] == zHandler )
							break;
					
					/* Add handler to the list */
					os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
					pcNode->AppendString( zHandler );
					/* Try to get an icon for the handler */
					os::String zTemp;
					os::Image* pcImage = NULL;
					if( m_pcManager->GetTypeAndIcon( zHandler, os::Point( 24, 24 ), &zTemp, &pcImage ) == 0 
							&& pcImage )
					pcNode->SetIcon( pcImage );
					m_pcHandlerList->InsertNode( pcNode );
					m_cHandlers.push_back( zHandler );
					std::cout<<"New handler "<<zHandler.c_str()<<std::endl;
				}
				m_bHandlerReqShown = false;
			}
		}
		break;
		case os::M_FILE_REQUESTER_CANCELED:
		{
			m_bIconReqShown = false;
			m_bHandlerReqShown = false;
		}
		break;
		case M_ADD_TYPE_CALLBACK:
		{
			os::String zMimetype;
			if( pcMessage->FindString( "mimetype", &zMimetype ) == 0 )
			{
				m_pcManager->RegisterType( zMimetype, "New type" );
				m_pcManager->RegisterTypeIcon( zMimetype, os::Path( "/system/icons/file.png" ) );
				UpdateList();
			}
		}
		break;
	}
}
bool MainWindow::OkToQuit()
{
	if( m_pcManager )
	{
		m_pcManager->Put();
		delete( m_pcDummyImage );
		m_pcTypeList->Clear();
		m_pcHandlerList->Clear();
		m_pcIconReq->Quit();
		m_pcHandlerReq->Quit();
		
	}
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( false );
}


















