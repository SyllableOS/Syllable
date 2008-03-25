#include <typeinfo>
#include <gui/menu.h>
#include <gui/toolbar.h>
#include <gui/imagebutton.h>
#include <gui/requesters.h>
#include <gui/splitter.h>
#include <util/variant.h>
#include <storage/path.h>
#include <storage/directory.h>
#include "mainwindow.h"
#include "messages.h"
#include "PropertyRow.h"
#include "Widgets.h"
#include "WidgetsNodes.h"
#include "WidgetsControls.h"
#include "WidgetsUtils.h"
#include "WidgetsImage.h"
#include "WidgetsMenu.h"
#include "WidgetsList.h"


/* Widget tree row */
class WidgetTreeRow : public os::TreeViewStringNode
{
public:
	WidgetTreeRow( os::LayoutNode* pcNode, os::LayoutNode* pcRealParent )
	{
		m_pcNode = pcNode;
		m_pcRealParent = pcRealParent;
	}
	os::LayoutNode* GetLayoutNode()
	{
		return( m_pcNode );
	}
	os::LayoutNode* GetRealParent()
	{
		return( m_pcRealParent );
	}
private:
	os::LayoutNode* m_pcRealParent;
	os::LayoutNode* m_pcNode;
};


void SetButtonImageFromResource( os::ImageButton* pcButton, os::String zResource )
{
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( zResource );
	pcButton->SetImage( pcStream );
	delete( pcStream );
}

os::String MainWindow::m_zFileName = "";

MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 300, 400 ), "main_wnd", "Layout Editor" )
{
	/* Set default */
	m_zFileName = "Unknown.if";
	m_pcCatalog = NULL;
	
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	
	/* Create layoutview */
	m_pcRoot = new os::VLayoutNode( "root" );
	m_pcLayoutView = new os::LayoutView( GetBounds(), "layout_view", NULL );
	
	/* Initialize widgets */
	InitWidgets();
	
	/* Create menus */
	InitMenus();
	
	/* Create toolbar */
	os::ToolBar* pcToolBar = new os::ToolBar( os::Rect(), "side_toolbar" );
	
	
	os::ImageButton* pcBreaker = new os::ImageButton(os::Rect(), "side_t_breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
    SetButtonImageFromResource( pcBreaker, "breaker.png" );
    pcToolBar->AddChild( pcBreaker, os::ToolBar::TB_FIXED_MINIMUM );
    
	os::ImageButton* pcProjOpen = new os::ImageButton( os::Rect(), "side_t_project_open", "",new os::Message( M_LOAD )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcProjOpen, "open.png" );
	pcToolBar->AddChild( pcProjOpen, os::ToolBar::TB_FIXED_MINIMUM );
	
	os::ImageButton* pcProjSave = new os::ImageButton( os::Rect(), "side_t_project_save", "",new os::Message( M_SAVE )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcProjSave, "save.png" );
	pcToolBar->AddChild( pcProjSave, os::ToolBar::TB_FIXED_MINIMUM );
	
	m_pcRoot->AddChild( pcToolBar, 0 );
	
	
	/* Create widget tree */
	m_pcWidgetTree = new os::TreeView( os::Rect(), "widget_tree", os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL );
	m_pcWidgetTree->InsertColumn( "Name", (int)GetBounds().Width() * 3 / 5 );
	m_pcWidgetTree->InsertColumn( "Type", (int)GetBounds().Width() * 2 / 5 );
	m_pcWidgetTree->SetSelChangeMsg( new os::Message( M_TREE_SEL_CHANGED ) );

	/* Create property tree */
	m_pcPropertyTree = new os::TreeView( os::Rect(), "property_tree", os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL );
	m_pcPropertyTree->InsertColumn( "Property", (int)GetBounds().Width() * 2 / 5 );
	m_pcPropertyTree->InsertColumn( "Value", (int)GetBounds().Width() * 3 / 5 );
	
	/* Create splitter */	
	os::Splitter* pcSplitter = new os::Splitter( os::Rect(), "splitter", m_pcWidgetTree, m_pcPropertyTree,
												os::VERTICAL );
	pcSplitter->SetSplitRatio( 0.6 );
	m_pcRoot->AddChild( pcSplitter );
		
	m_pcLayoutView->SetRoot( m_pcRoot );
	AddChild( m_pcLayoutView );
	
	/* Create application window */
	m_pcAppWindow = new AppWindow( this );
	m_pcAppWindow->CenterInScreen();
	m_pcAppWindow->Show();
	
	m_pcNewWin = NULL;	
	
	/* Create widget tree */
	CreateWidgetTree();
	
	/* Create save requester */
	m_pcSaveRequester = new os::FileRequester( os::FileRequester::SAVE_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, false );
	m_pcSaveRequester->Start();
	
	/* Create load requester */
	m_pcLoadRequester = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, true );
	m_pcLoadRequester->Start();

	SetTitle( os::String("Layout Editor : ") + os::String( os::Path( m_zFileName ).GetLeaf() ) );
	
}

MainWindow::~MainWindow()
{
	/* Delete catalog */
	delete( m_pcCatalog );
	
	/* Delete widgets */
	DeleteWidgets();
}

void MainWindow::InitWidgets()
{
	/* Build widget list */
	m_apcWidgets.push_back( static_cast<Widget*>(new HLayoutNodeWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new VLayoutNodeWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new HLayoutSpacerWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new VLayoutSpacerWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new FrameViewWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new TabViewWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new SeparatorWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new StringViewWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new ButtonWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new ImageButtonWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new CheckBoxWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new RadioButtonWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new DropdownMenuWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new TextViewWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new ImageViewWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new HorizontalSliderWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new VerticalSliderWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new MenuWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new ListViewWidget) );
	m_apcWidgets.push_back( static_cast<Widget*>(new SpinnerWidget) );
}

void MainWindow::InitMenus()
{
	/* Initalize menus */
	os::Menu* pcMenuBar = new os::Menu( os::Rect(), "id_menu_bar", os::ITEMS_IN_ROW, os::CF_FOLLOW_LEFT |
							os::CF_FOLLOW_RIGHT, os::WID_FULL_UPDATE_ON_RESIZE );  

	os::Menu* pcMenuApp = new os::Menu( os::Rect(), "Application", os::ITEMS_IN_COLUMN );
	pcMenuApp->AddItem( "About", new os::Message( M_APP_ABOUT ) );
	pcMenuApp->AddItem( new os::MenuSeparator() );
	pcMenuApp->AddItem( "Quit", new os::Message( os::M_QUIT ) );
	
	/* Add Widgets */
	os::Menu* pcMenuWidget = new os::Menu( os::Rect(), "Widgets", os::ITEMS_IN_COLUMN );
	for( uint i = 0; i < m_apcWidgets.size(); i++ )
	{
		os::Message* pcMsg = new os::Message( M_ADD_WIDGET );
		pcMsg->AddInt32( "id", i );
		pcMenuWidget->AddItem( os::String( "Add " ) + m_apcWidgets[i]->GetName(), pcMsg );
		if( i == 5 )
			pcMenuWidget->AddItem( new os::MenuSeparator() );
	}
	pcMenuWidget->AddItem( new os::MenuSeparator() );
	pcMenuWidget->AddItem( "Move up", new os::Message( M_MOVE_WIDGET_UP ) );
	pcMenuWidget->AddItem( "Move down", new os::Message( M_MOVE_WIDGET_DOWN ) );
	pcMenuWidget->AddItem( "Remove", new os::Message( M_REMOVE_WIDGET ) );
	
	pcMenuBar->AddItem( pcMenuApp );
	pcMenuBar->AddItem( pcMenuWidget );
	pcMenuBar->SetTargetForItems( this );
	m_pcRoot->AddChild( pcMenuBar, 0 );
}

void MainWindow::DeleteWidgets()
{
	/* Delete widgets */
	for( uint i = 0; i < m_apcWidgets.size(); i++ )
		delete( m_apcWidgets[i] );
	m_apcWidgets.clear();
}

Widget* MainWindow::GetNodeWidget( os::LayoutNode* pcNode )
{
	/* Returns the widget object for one node using its typeid */
	const std::type_info* psInfo;
	if( pcNode->GetView() != NULL )
		psInfo = &typeid( *(pcNode->GetView()) );
	else
		psInfo = &typeid( *pcNode );
	for( uint i = 0; i < m_apcWidgets.size(); i++ )
	{
		if( m_apcWidgets[i]->GetTypeID() == psInfo )
			return( m_apcWidgets[i] );
	}
	return( NULL );
}

void MainWindow::CreateWidgetTreeLevel( int nLevel, os::LayoutNode* pcNode, os::LayoutNode* pcRealParent )
{
	/* Create tree entry */
	Widget* pcWidget = GetNodeWidget( pcNode );
	WidgetTreeRow* pcRow = new WidgetTreeRow( pcNode, pcRealParent );
	pcRow->AppendString( pcNode->GetName() );
	
	if( pcNode == m_pcAppWindow->GetRootNode() )
	{
		pcRow->AppendString( "Root Node" );
	}
	else
	{
		/* Get the widget belonging to this node */
		if( pcWidget != NULL )
			pcRow->AppendString( pcWidget->GetName() );
		else
			pcRow->AppendString( "Unknown" );
		pcRow->SetIndent( nLevel );
	}
	
	m_pcWidgetTree->InsertNode( pcRow );
	
	/* Create the widget tree entries for the children */
	std::vector<os::LayoutNode*> apcWidgets;
	if( pcWidget )
	{
		apcWidgets = pcWidget->GetChildList( pcNode ); }
	else
	{
		apcWidgets = pcNode->GetChildList();
	}
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		CreateWidgetTreeLevel( nLevel + 1, apcWidgets[i], pcNode );
	}
}

void MainWindow::CreateWidgetTree()
{
	/* Creates the widget tree */
	m_pcWidgetTree->Clear();
	os::LayoutNode* pcRoot = m_pcAppWindow->GetRootNode();
	CreateWidgetTreeLevel( 1, pcRoot, NULL );
	
	m_pcWidgetTree->Invalidate();
	m_pcWidgetTree->Flush();
}

void MainWindow::CreatePropertyList( os::LayoutNode* pcNode )
{
	/* Create the property list for one node */
	m_pcPropertyTree->Clear();
	std::vector<WidgetProperty> cProperties;
	Widget* pcWidget = GetNodeWidget( pcNode );
	if( pcWidget == NULL )
		return;
	cProperties = pcWidget->GetProperties( pcNode );
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetType() )
		{
			case PT_STRING:
			case PT_INT:
			case PT_FLOAT:
			case PT_BOOL:
			case PT_STRING_SELECT:
			case PT_STRING_LIST:
			case PT_STRING_CATALOG:
			{
				PropertyTreeNode* pcRow = new PropertyTreeNode( pcNode, *pcProp );
				pcRow->AppendString( pcProp->GetName() );
				if( pcProp->GetType() == PT_BOOL )
					pcRow->AppendString( pcProp->GetValue().AsBool() == true ? "true" : "false" );
				else
					pcRow->AppendString( pcProp->GetValue().AsString() );
				m_pcPropertyTree->InsertNode( pcRow );
			}
			break;
			default:
			break;
		}
	}

}

void MainWindow::LoadCatalog()
{
	delete( m_pcCatalog );
	m_pcCatalog = NULL;

	/* Try to find the catalog in the resources directory */
	os::Path cCatalogDirPath( m_zFileName );
	cCatalogDirPath = cCatalogDirPath.GetDir();
	cCatalogDirPath.Append( "resources/" );
	try
	{
		os::Directory cDir( cCatalogDirPath );
		os::String cEntry;
		while( cDir.GetNextEntry( &cEntry ) == 1 )
		{
			/* TODO: Use the registrar to detect the catalogs once we have defined a mimetype for the catalogs */
			if( cEntry.Length() < 4 )
				continue;
			os::String cExtension = cEntry.substr( cEntry.Length() - 3, 3 );
			
			if( cExtension != ".cd" )
				continue;
			
			os::Path cCatalogPath( cCatalogDirPath );
			cCatalogPath.Append( cEntry );
			
			/* Load the catalog */
			try
			{
				printf( "Load catalog %s...\n", cCatalogPath.GetPath().c_str() );
				os::CCatalog* pcCatalog = new os::CCatalog();
				
				if( pcCatalog->ParseCatalogDescription( cCatalogPath.GetPath().c_str() ) == false )
				{
					printf("Error!\n");
					delete( pcCatalog );
					continue;
				}
				printf( "Catalog %s loaded\n", cCatalogPath.GetPath().c_str() );
				
				for( os::CCatalog::const_iterator i = pcCatalog->begin(); i != pcCatalog->end(); i++ )
					printf( "%i %s %s\n", (*i).first, pcCatalog->GetMnemonic( (*i).first ).c_str(), (*i).second.c_str() );
				m_pcCatalog = pcCatalog;
				return;
			} catch( ... )
			{
				continue;
			}
		}
	}
	catch( ... )
	{
		return;
	}
}

os::String MainWindow::GetString( os::String& zString )
{
	/* Returns a string from the catalog file */
	if( m_pcCatalog == NULL )
		return( zString );	
	os::String zS = zString.substr( 2, zString.Length() - 2 );
	for( os::CCatalog::const_iterator i = m_pcCatalog->begin(); i != m_pcCatalog->end(); i++ )
	{
		if( m_pcCatalog->GetMnemonic( (*i).first ) == zS )
		{
			printf( "Found: %i %s %s\n", (*i).first, m_pcCatalog->GetMnemonic( (*i).first ).c_str(), (*i).second.c_str() );
			return( (*i).second );
		}
	}
	return( zString );
}

bool MainWindow::Save( os::File* pcFile, int nLevel, os::LayoutNode* pcParentNode )
{
	/* Save nodes */
	Widget* pcWidget = GetNodeWidget( pcParentNode );
	std::vector<os::LayoutNode*> apcWidgets;
	if( pcWidget )
		apcWidgets = pcWidget->GetChildList( pcParentNode );
	else
		apcWidgets = pcParentNode->GetChildList();
	
	std::vector<WidgetProperty> cProperties;
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		/* Write level */
		pcFile->Write( &nLevel, 4 );
		uint8 nBuffer[8192];
		os::LayoutNode* pcNode = apcWidgets[i];
		/* Get the widget belonging to this node */
		pcWidget = GetNodeWidget( pcNode );
		
		if( pcWidget == NULL )
		{
			printf("Error: Unknown widget!\n" );
			return( false );
		}
		int nDataLength;
		/* Save type name */
		nDataLength = pcWidget->GetName().Length();
		pcFile->Write( &nDataLength, 4 );
		pcFile->Write( pcWidget->GetName().c_str(), nDataLength );
		/* Save name */
		nDataLength = pcNode->GetName().Length();
		pcFile->Write( &nDataLength, 4 );
		pcFile->Write( pcNode->GetName().c_str(), nDataLength );
		
		printf("Save %s %s: %i\n", pcWidget->GetName().c_str(), pcNode->GetName().c_str(), nLevel );
		
		/* Save properties */
		cProperties = pcWidget->GetProperties( pcNode );
		for( uint i = 0; i < cProperties.size(); i++ )
		{
			if( cProperties[i].Flatten( nBuffer, 8192 ) != 0 )
			{
				printf("Error: Could not flatten property!\n" );
				return( false );
			}
			nDataLength = cProperties[i].GetFlattenedSize();
			pcFile->Write( &nDataLength, 4 );
			pcFile->Write( &nBuffer, nDataLength );
		}
		
		/* Write end */
		nDataLength = 0;
		pcFile->Write( &nDataLength, 4 );
		
		/* Save child nodes */
		Save( pcFile, nLevel + 1, pcNode );
	}
	return( true );
}

void MainWindow::Save()
{
	os::File cFile;
	/* Create file */
	if( cFile.SetTo( m_zFileName, O_RDWR | O_CREAT | O_TRUNC ) != 0 )
	{
		os::Alert* pcAlert = new os::Alert( "Layout Editor", "Failed to save!\n", os::Alert::ALERT_WARNING, 0x00, "Ok", NULL);
		pcAlert->CenterInWindow(this);
		pcAlert->Go( new os::Invoker() );
		return;
	}
	
	/* Write attribute */
	os::String zMimeType = "application/x-interface";
	cFile.WriteAttr( "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, zMimeType.c_str(), 0, zMimeType.Length() + 1 );

	
	/* Save */
	os::LayoutNode* pcRoot = m_pcAppWindow->GetRootNode();
	Save( &cFile, 2, pcRoot );
	cFile.Flush();
	
	/* Create code */
	CreateCode();
	
	/* Load catalog (we now know the filename) */
	LoadCatalog();
	
	SetTitle( os::String("Layout Editor : ") + os::String( os::Path( m_zFileName ).GetLeaf() ) );
}

bool MainWindow::Load( os::File* pcFile, int *pnLevel, os::LayoutNode* pcParentNode )
{
	/* Load nodes */	
	int nLoadLevel = *pnLevel;
	uint8 nBuffer[8192];
	const char* pBuffer = (const char*)nBuffer;
	os::LayoutNode* pcLastNode = pcParentNode;
	while( 1 )
	{
		printf("Level %i %i\n", *pnLevel, nLoadLevel );
		if( nLoadLevel > *pnLevel )
		{
			if( Load( pcFile, &nLoadLevel, pcLastNode ) == false )
			{
				ResetWidthAndHeight( pcLastNode );
				return( false );
			}
			ResetWidthAndHeight( pcLastNode );
		} 
		if( nLoadLevel < *pnLevel )
		{
			*pnLevel = nLoadLevel;
			return( true );
		}
		int nDataLength;
		/* Load type */
		os::String zType;
		if( pcFile->Read( &nDataLength, 4 ) != 4 )
			return( false );
		if( pcFile->Read( nBuffer, nDataLength ) != nDataLength )
			return( false );
		nBuffer[nDataLength] = 0;

		zType = pBuffer;
		printf( "%s\n", nBuffer );
		/* Load name */
		os::String zName;
		if( pcFile->Read( &nDataLength, 4 ) != 4 )
			return( false );
		if( pcFile->Read( nBuffer, nDataLength ) != nDataLength )
			return( false );
		nBuffer[nDataLength] = 0;
		zName = pBuffer;
		printf( "%s\n", nBuffer );
		
		/* Find widget */
		Widget* pcWidget = NULL;
		for( uint i = 0; i < m_apcWidgets.size(); i++ )
		{
			if( m_apcWidgets[i]->GetName() == zType )
			{
				pcWidget = m_apcWidgets[i];
				break;
			}
		}
		if( pcWidget == NULL )
		{
			printf("Error: Could not find widget %s\n", zType.c_str() );
			return( false );
		}
		pcLastNode = pcWidget->CreateLayoutNode( zName );
		
		Widget* pcParentWidget = GetNodeWidget( pcParentNode );
		if( pcParentWidget )
			pcParentWidget->AddChild( pcParentNode, pcLastNode );
		else
			pcParentNode->AddChild( pcLastNode );
		
		/* Load properties */
		std::vector<WidgetProperty> cProperties;
		if( pcFile->Read( &nDataLength, 4 ) != 4 )
			return( false );
		while( nDataLength != 0 )
		{
			if( pcFile->Read( nBuffer, nDataLength ) != nDataLength )
				return( false );
			printf("Read message %i\n", nDataLength );
			WidgetProperty cMsg;
			if( cMsg.Unflatten( nBuffer ) != 0 )
			{
				printf( "Could not unflatten message!\n" );
				return( false );
			}
			cProperties.push_back( cMsg );
			if( pcFile->Read( &nDataLength, 4 ) != 4)
				return( false );
		}
		pcWidget->SetProperties( pcLastNode, cProperties );
		/* Read next level */
		if( pcFile->Read( &nLoadLevel, 4 ) != 4 )
		{
			return( false );
		}
	}
	return( true );
}

void MainWindow::Load( os::String zFileName )
{
	/* Save old file */
	if( m_zFileName != "Unknown.if" )
		Save();
			
	os::File cFile;
	if( cFile.SetTo( zFileName ) != 0 )
	{
		os::Alert* pcAlert = new os::Alert( "Layout Editor", "Failed to open file!\n", os::Alert::ALERT_WARNING, 0x00, "Ok", NULL);
		pcAlert->CenterInWindow(this);
		pcAlert->Go( new os::Invoker() );
		return;
	}
	
	m_zFileName = zFileName;
	
	/* Clear tree */
	std::vector<os::LayoutNode*> apcWidgets = m_pcAppWindow->GetRootNode()->GetChildList();
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		delete( apcWidgets[i] );
	}
	
	/* Load catalog */
	LoadCatalog();
	
	int nLoadLevel;
	/* Read level */
	if( cFile.Read( &nLoadLevel, 4 ) != 4 )
	{
		return;
	}	
	/* Load file */
	os::LayoutNode* pcRoot = m_pcAppWindow->GetRootNode();
	Load( &cFile, &nLoadLevel, pcRoot );
	
	/* Rebuild tree */
	CreateWidgetTree();
	m_pcAppWindow->Lock();
	m_pcAppWindow->ReLayout();
	m_pcAppWindow->Unlock();
	
	SetTitle( os::String("Layout Editor : ") + os::String( os::Path( m_zFileName ).GetLeaf() ) );
}

bool MainWindow::CreateCode( os::File* pcSourceFile, os::File* pcHeaderFile, int nLevel, os::LayoutNode* pcParentNode )
{
	/* Create code for nodes */	
	Widget* pcWidget = GetNodeWidget( pcParentNode );
	std::vector<os::LayoutNode*> apcWidgets;
	if( pcWidget )
		apcWidgets = pcWidget->GetChildList( pcParentNode );
	else
		apcWidgets = pcParentNode->GetChildList();
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		os::LayoutNode* pcNode = apcWidgets[i];
		/* Get the widget belonging to this node */
		Widget* pcWidget = GetNodeWidget( pcNode );
		
		if( pcWidget == NULL )
		{
			printf("Error: Unknown widget!\n" );
			return( false );
		}
		pcWidget->CreateHeaderCode( pcHeaderFile, pcNode );
		pcWidget->CreateCode( pcSourceFile, pcNode );
		
		CreateCode( pcSourceFile, pcHeaderFile, nLevel + 1, pcNode );
		
		pcWidget->CreateCodeEnd( pcSourceFile, pcNode );
	}
	return( true );
}

void MainWindow::CreateCode()
{
	os::File cSourceFile;
	os::File cHeaderFile;
	os::String zSourceFileName = m_zFileName.substr( 0, m_zFileName.Length() - 3 );
	zSourceFileName += "Layout.cpp";
	os::String zHeaderFileName = m_zFileName.substr( 0, m_zFileName.Length() - 3 );
	zHeaderFileName += "Layout.h";

//	os::String zWidgetName = os::Path( zSourceFileName ).GetLeaf();

	if( cSourceFile.SetTo( zSourceFileName, O_RDWR | O_CREAT | O_TRUNC ) != 0 ||
		cHeaderFile.SetTo( zHeaderFileName, O_RDWR | O_CREAT | O_TRUNC ) != 0 )
	{
		os::Alert* pcAlert = new os::Alert( "Layout Editor", "Failed to create code!\n", os::Alert::ALERT_WARNING, 0x00, "Ok", NULL);
		pcAlert->CenterInWindow(this);
		pcAlert->Go( new os::Invoker() );
		return;
	}
	char nBuffer[8192];
	os::String zRootName = "Root";
	sprintf( nBuffer, "m_pcRoot = new os::VLayoutNode( \"%s\" );\n", zRootName.c_str() );
	cSourceFile.Write( nBuffer, strlen( nBuffer ) );

	sprintf( nBuffer, "os::VLayoutNode* m_pcRoot;\n" );
	cHeaderFile.Write( nBuffer, strlen( nBuffer ) );
	
	os::LayoutNode* pcRoot = m_pcAppWindow->GetRootNode();
	CreateCode( &cSourceFile, &cHeaderFile, 2, pcRoot );
	cSourceFile.Flush();
	cHeaderFile.Flush();
}

bool MainWindow::FindNode( os::String zName, os::LayoutNode* pcParentNode )
{
	/* Find a node using its name */
	/* NOTE: We cannot use the FindNode() member of the LayoutNode class here
	because the search doesn’t include integrated layoutvies */
	Widget* pcWidget = GetNodeWidget( pcParentNode );
	std::vector<os::LayoutNode*> apcWidgets;
	if( pcWidget )
		apcWidgets = pcWidget->GetChildList( pcParentNode );
	else
		apcWidgets = pcParentNode->GetChildList();
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		os::LayoutNode* pcNode = apcWidgets[i];
		
		if( pcNode->GetName() == zName )
			return( true );
		
		/* Get the widget belonging to this node */
		Widget* pcWidget = GetNodeWidget( pcNode );
		
		if( pcWidget == NULL )
		{
			return( false );
		}
		
		if( FindNode( zName, pcNode ) == true )
			return( true );
	}
	return( false );
}

void MainWindow::ResetWidthAndHeight( os::LayoutNode* pcNode )
{
	/* Called to recreate the width and height settings */
	Widget* pcWidget = GetNodeWidget( pcNode );
	std::vector<os::LayoutNode*> apcWidgets;
	if( pcWidget )
		apcWidgets = pcWidget->GetChildList( pcNode );
	else
		apcWidgets = pcNode->GetChildList();
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		apcWidgets[i]->SameWidth( NULL );
		apcWidgets[i]->SameHeight( NULL );
	}
	std::vector<WidgetProperty> cProperties;
	if( pcWidget == NULL )
		return;
	cProperties = pcWidget->GetProperties( pcNode );
	pcWidget->SetProperties( pcNode, cProperties );
}

void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_ADD_WIDGET:
		{
			if( m_pcNewWin != NULL )
				break;
			
			/* Get widget ID */
			int32 nId = 0;
			pcMessage->FindInt32( "id", &nId );
			/* Get selected row */
			int nSelected = m_pcWidgetTree->GetFirstSelected();
			if( nSelected < 0 )
				break;
			Widget* pcWidget = m_apcWidgets[nId];
			WidgetTreeRow* pcRow = static_cast<WidgetTreeRow*>(m_pcWidgetTree->GetRow( nSelected ) );
			Widget* pcRowWidget = GetNodeWidget( pcRow->GetLayoutNode() );
			os::LayoutNode* pcNode;
			pcNode = pcRow->GetLayoutNode();
			
			/* Check if this is a HLayoutNode or VLayoutNode */
			if( !( typeid( *pcNode ) == typeid( LEHLayoutNode )
				|| typeid( *pcNode ) == typeid( LEVLayoutNode )
				|| typeid( *pcNode ) == typeid( os::VLayoutNode )
				|| ( pcRowWidget && pcRowWidget->CanHaveChildren() ) ) )
				break;
			
			/* Save values */
			m_pcNewParentNode = pcNode;
			m_pcNewWidget = pcWidget;
			
			/* Create edit window */
			m_pcNewWin = new os::EditStringWin( os::Rect( 0, 0, 150, 50 ), "edit_win", os::String( "Add new " ) + pcWidget->GetName(), pcWidget->GetName(), this );
			m_pcNewWin->CenterInWindow( m_pcAppWindow );
			m_pcNewWin->Show();
			m_pcNewWin->MakeFocus();
			break;
		}
		case M_ADD_WIDGET_CANCEL:
		{
			/* Cancel */
			m_pcNewWin = NULL;
			break;
		}
		case M_ADD_WIDGET_FINISH:
		{
			/* Restore saved values */
			Widget* pcWidget = m_pcNewWidget;
			os::LayoutNode* pcNode = m_pcNewParentNode;
			m_pcNewWin = NULL;
			/* Create widget */
			os::String zWidgetName;
			pcMessage->FindString( "data", &zWidgetName );
			
			/* Check if a node with the name already exists */
			if( FindNode( zWidgetName, m_pcAppWindow->GetRootNode() ) )
			{
				os::Alert* pcAbout = new os::Alert( "Layout Editor", "A node with this name already exists!\n", os::Alert::ALERT_WARNING, 0x00, "Ok", NULL);
				pcAbout->CenterInWindow(this);
				pcAbout->Go( new os::Invoker() );
				break;
			}
			
			os::LayoutNode* pcNewNode = pcWidget->CreateLayoutNode( zWidgetName );
			m_pcAppWindow->Lock();
			
			Widget* pcParentWidget = GetNodeWidget( pcNode );
			if( pcParentWidget )
				pcParentWidget->AddChild( pcNode, pcNewNode );
			else
				pcNode->AddChild( pcNewNode );
			ResetWidthAndHeight( pcNode );
			
			m_pcAppWindow->ReLayout();
			m_pcAppWindow->Unlock();
			/* Recreate tree */
			CreateWidgetTree();
			PostMessage( M_TREE_SEL_CHANGED, this );
			printf("Add widget %s to %s\n", pcNewNode->GetName().c_str(), pcNode->GetName().c_str() );
//			printf( "%s\n", pcRow->GetString( 0 ).c_str() );
		}
		break;
		case M_REMOVE_WIDGET:
		{
			if( m_pcNewWin != NULL )
				break;

			/* Get selected row */
			int nSelected = m_pcWidgetTree->GetFirstSelected();
			if( nSelected <= 0 )
				break;
			WidgetTreeRow* pcRow = static_cast<WidgetTreeRow*>(m_pcWidgetTree->GetRow( nSelected ) );
			os::LayoutNode* pcNode = pcRow->GetLayoutNode();
			os::LayoutNode* pcParent = pcRow->GetRealParent();
			Widget* pcParentWidget = GetNodeWidget( pcParent );
			
			/* Delete the node */
			m_pcAppWindow->Lock();
			if( pcParentWidget )
				pcParentWidget->RemoveChild( pcParent, pcNode );
			else
				pcParent->RemoveChild( pcNode );
			delete( pcNode );
			/* Reset widths and */
			ResetWidthAndHeight( pcParent );
			m_pcAppWindow->ReLayout();
			m_pcAppWindow->Unlock();
			CreateWidgetTree();
			PostMessage( M_TREE_SEL_CHANGED, this );
		}
		break;
		case M_MOVE_WIDGET_UP:
		case M_MOVE_WIDGET_DOWN:
		{
			if( m_pcNewWin != NULL )
				break;

			/* Get selected row */
			int nSelected = m_pcWidgetTree->GetFirstSelected();
			if( nSelected <= 0 )
				break;
			WidgetTreeRow* pcRow = static_cast<WidgetTreeRow*>(m_pcWidgetTree->GetRow( nSelected ) );
			os::LayoutNode* pcNode = pcRow->GetLayoutNode();
			
			/* Get parent */
			
			os::LayoutNode* pcParent = pcRow->GetRealParent();
			if( pcParent == NULL )
				break;
			Widget* pcParentWidget = GetNodeWidget( pcParent );
			std::vector<os::LayoutNode*> cChildList;
			m_pcAppWindow->Lock();
			if( pcParentWidget )
				cChildList = pcParentWidget->GetChildList( pcParent );
			else
				cChildList = pcParent->GetChildList();
			int nPos = -1;
			/* Remove all children */
			for( uint i = 0; i < cChildList.size(); i++ )
			{
				if( cChildList[i] == pcNode )
				{
					nPos = i;
				}
				if( pcParentWidget )
					pcParentWidget->RemoveChild( pcParent, cChildList[i] );
				else
					pcParent->RemoveChild( cChildList[i] );
			}
			if( pcMessage->GetCode() == M_MOVE_WIDGET_UP && nPos > 0 ) {
				/* Exchange nodes */
				cChildList[nPos] = cChildList[nPos-1];
				cChildList[nPos-1] = pcNode;
				nSelected--;
			} else if( pcMessage->GetCode() == M_MOVE_WIDGET_DOWN && nPos < (int)cChildList.size() - 1 ) {
				/* Exchange nodes */
				cChildList[nPos] = cChildList[nPos+1];
				cChildList[nPos+1] = pcNode;
				nSelected++;
			}
			
			/* Add nodes again */
			for( uint i = 0; i < cChildList.size(); i++ )
			{
				pcParentWidget->AddChild( pcParent, cChildList[i] );
			}
			
			/* Update window and the tree */
			
			m_pcAppWindow->ReLayout();
			m_pcAppWindow->Unlock();
			CreateWidgetTree();
			m_pcWidgetTree->Select( nSelected );
			PostMessage( M_TREE_SEL_CHANGED, this );
		}
		break;
		case M_TREE_SEL_CHANGED:
		{
			/* Recreate the property list if the widget selection has changed */
			int nSelected = m_pcWidgetTree->GetFirstSelected();
			if( nSelected <= 0 )
			{
				m_pcPropertyTree->Clear();
				break;
			}
			WidgetTreeRow* pcRow = static_cast<WidgetTreeRow*>(m_pcWidgetTree->GetRow( nSelected ) );
			os::LayoutNode* pcNode = pcRow->GetLayoutNode();
			CreatePropertyList( pcNode );
		}
		break;
		case M_UPDATE_PROPERTY:
		{
			/* Update property */
			os::LayoutNode* pcNode;
			WidgetProperty* pcProp;
			size_t nSize = 0;
			/* Extract the data from the message */
			pcMessage->FindPointer( "node", (void**)&pcNode );
			pcMessage->FindData( "property", os::T_RAW, (const void**)&pcProp, &nSize );
			std::vector<WidgetProperty> cProperties;
			cProperties.push_back( *pcProp );
			Widget* pcWidget = GetNodeWidget( pcNode );
			if( pcWidget == NULL )
				break;
			m_pcAppWindow->Lock();
			/* Tell the widget to change the property */
			pcWidget->SetProperties( pcNode, cProperties );
			/* Relayout window */
			m_pcAppWindow->ReLayout();
			m_pcAppWindow->Unlock();
		}
		break;
		case M_SAVE:
		{
			if( m_zFileName == "Unknown.if" )
			{
				m_pcSaveRequester->Lock();
				if( !m_pcSaveRequester->IsVisible() )
				{
					m_pcSaveRequester->CenterInWindow(this);
					m_pcSaveRequester->Show();
				}
				m_pcSaveRequester->MakeFocus( true );
				m_pcSaveRequester->Unlock();
				break;
			}
			Save();
		}
		break;
		case M_LOAD:
			m_pcLoadRequester->Lock();
			if( !m_pcLoadRequester->IsVisible() )
			{
				m_pcLoadRequester->CenterInWindow(this);
				m_pcLoadRequester->Show();
			}
			m_pcLoadRequester->MakeFocus( true );
			m_pcLoadRequester->Unlock();
		break;
		case M_APP_ABOUT:
		{
			os::Alert* pcAlert = new os::Alert("About Layout Editor", "Layout Editor 0.1.2\nEditor for the dynamic layout system\nWritten by Arno Klenke, 2006-2007\n"
														"\n\nLayout Editor is released under the Gnu Public Licencse (GPL)\nPlease see the file COPYING, distributed with Layout Editor, for\nmore information\n", os::Alert::ALERT_INFO,
											0x00, "Ok", NULL);
			pcAlert->CenterInWindow(this);
			pcAlert->Go( new os::Invoker() );
		}
		break;
		case os::M_LOAD_REQUESTED:
		{
			os::String zPath;
			if( pcMessage->FindString( "file/path", &zPath ) == 0 )
			{
				Load( zPath );
			}
		}
		break;
		case os::M_SAVE_REQUESTED:
		{
			os::String zPath;
			if( pcMessage->FindString( "file/path", &zPath ) == 0 )
			{
				m_zFileName = zPath;
				Save();
			}
		}
		break;
	}
}
bool MainWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_TERMINATE );
	return( true );
}














































































































