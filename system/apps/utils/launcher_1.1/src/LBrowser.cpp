/*
    LBrowser - A simple file manager for AtheOS
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "LBrowser.h"

int main( int argc, char **argv )
{
	LBrowser *m_pcBrowser = new LBrowser( argc, argv );
	
	m_pcBrowser->Run();

	return( 0 );
}


LBrowser::LBrowser( int argc, char **argv ) : Application( "application/x-vnd.ADK-LBrowser" )
{

	string zDirectory = (argc > 1) ? argv[1] : "~/";
	
	m_pcWindow = new IconEditor( convert_path( zDirectory ) );
	m_pcWindow->Show(true);
	
}

LBrowser::~LBrowser()
{
}


bool LBrowser::OkToQuit( void )
{
	m_pcWindow->Quit();
	return true;
}



IconEditor::IconEditor( string zDirectory ) : Window( Rect(), "LBrowser", "LBrowser" )
{
	m_zDirectory = zDirectory;
	m_pcPrefsWindow = NULL;
	m_bIconLabels = true;
	
	SetTitle( "LBrowser - " + zDirectory );
	
	m_pcTVDir = new TextView( Rect(), "tv_dir", m_zDirectory.c_str() );
	m_pcDVDir = new DirectoryView( Rect(), m_zDirectory.c_str() );
	m_pcDVDir->SetDirChangeMsg( new Message( IE_DIR_CHANGED ) );
	m_pcDVDir->SetInvokeMsg( new Message( IE_DOUBLE_CLICK ) );
	m_pcDVDir->SetSelChangeMsg( new Message( IE_SELECTION_CHANGED ) );
	m_pcFRIcon = new FileRequester( FileRequester::LOAD_REQ, new Messenger(this), "/Applications/Launcher/icons/", FileRequester::NODE_FILE, false, new Message( IE_ICON_CHANGED ) );
	m_pcIcon = new ImageButton( Rect(), "icon", "", new Message( IE_ICON_CLICKED ), IB_TEXT_BOTTOM );
	
	m_pcNewBtn = new ImageButton( Rect(), "btn_new", ""/*"New Dir"*/, new Message( IE_BTN_NEW ), IB_TEXT_BOTTOM );
	m_pcNewBtn->SetImageFromFile( "/Applications/Launcher/icons/folder_new.png" );
	m_pcRenameBtn = new ImageButton( Rect(), "btn_rename", ""/*"Rename"*/, new Message( IE_BTN_RENAME ), IB_TEXT_BOTTOM );
	m_pcRenameBtn->SetImageFromFile( "/Applications/Launcher/icons/refresh.png" );
	m_pcDeleteBtn = new ImageButton( Rect(), "btn_del", ""/*"Delete"*/, new Message( IE_BTN_DELETE ), IB_TEXT_BOTTOM );
	m_pcDeleteBtn->SetImageFromFile( "/Applications/Launcher/icons/choice-no.png" );
	m_pcClearBtn = new ImageButton( Rect(), "btn_clear", ""/*"Clear Icon"*/, new Message( IE_BTN_CLEAR ), IB_TEXT_BOTTOM );	
	m_pcClearBtn->SetImageFromFile( "/Applications/Launcher/icons/unknown.png" );
	m_pcCopyBtn = new ImageButton( Rect(), "btn_copy", ""/*"Copy"*/, new Message( IE_BTN_COPY ), IB_TEXT_BOTTOM );					
	m_pcCopyBtn->SetImageFromFile( "/Applications/Launcher/icons/editcopy.png" );
	m_pcPasteBtn = new ImageButton( Rect(), "btn_paste", ""/*"Paste"*/, new Message( IE_BTN_PASTE ), IB_TEXT_BOTTOM );
	m_pcPasteBtn->SetImageFromFile( "/Applications/Launcher/icons/editpaste.png" );
	m_pcHomeBtn = new ImageButton( Rect(), "btn_home", ""/*"Home"*/, new Message( IE_BTN_HOME ), IB_TEXT_BOTTOM );
	m_pcHomeBtn->SetImageFromFile( "/Applications/Launcher/icons/home.png" );
	m_pcRefreshBtn = new ImageButton( Rect(), "btn_refresh", ""/*"Refresh"*/, new Message( IE_BTN_REFRESH ), IB_TEXT_BOTTOM );
	m_pcRefreshBtn->SetImageFromFile( "/Applications/Launcher/icons/reload.png" );
	m_pcParentBtn = new ImageButton( Rect(), "btn_parent", ""/*"Up"*/, new Message( IE_BTN_PARENT ), IB_TEXT_BOTTOM );	
	m_pcParentBtn->SetImageFromFile( "/Applications/Launcher/icons/up.png" );
	
	FrameView *pcFrame = new FrameView( Rect(), "outer_frame", "" );
			
	m_pcRoot = new VLayoutNode( "root" );
	VLayoutNode *pcInner = new VLayoutNode( "inner" );
	HLayoutNode *pcTop = new HLayoutNode( "top" );
//	HLayoutNode *pcStatusNode = new HLayoutNode( "status" );
	m_pcToolBar = new HLayoutNode( "mid" );

	m_pcToolBar->AddChild( m_pcIcon );
	m_pcToolBar->AddChild( m_pcClearBtn );
	m_pcToolBar->AddChild( new HLayoutSpacer( "space_1", 1.0f ) );
	m_pcToolBar->AddChild( m_pcHomeBtn );
	m_pcToolBar->AddChild( m_pcParentBtn );
	m_pcToolBar->AddChild( m_pcRefreshBtn );
	m_pcToolBar->AddChild( new HLayoutSpacer( "space_2", 1.0f ) );	
	m_pcToolBar->AddChild( m_pcNewBtn );
	m_pcToolBar->AddChild( m_pcRenameBtn );
	m_pcToolBar->AddChild( m_pcDeleteBtn );
	m_pcToolBar->AddChild( new HLayoutSpacer( "space_3", 1.0f ) );	
	m_pcToolBar->AddChild( m_pcCopyBtn );
	m_pcToolBar->AddChild( m_pcPasteBtn );

	LoadPrefs( );
	
	m_pcStatusString = new StringView( Rect(), "status_string", "LBrowser 1.0 by Andrew Kennan, 2002" );
	
//	pcStatusNode->AddChild( m_pcStatusString );
//	pcStatusNode->AddChild( new HLayoutSpacer( "space_4", 1.0f ) );
//	FrameView *pcStatusLayout = new FrameView( Rect(), "status", "" );
//	pcStatusLayout->SetRoot( pcStatusNode );
	
//	pcStatusNode->SameWidth( "space_4", "status_string", NULL );
	
	pcInner->AddChild( m_pcToolBar );
	pcInner->AddChild( m_pcDVDir );
	pcInner->AddChild( pcTop );	
//	pcInner->AddChild( pcStatusLayout );
	pcInner->AddChild( m_pcStatusString );

	pcFrame->SetRoot( pcInner );
	m_pcRoot->AddChild( pcFrame );
	
	// Dialog Box for rename/new directory
	
	m_pcSVDialog = new StringView( Rect(), "sv_dialog", "" );
	m_pcTVDialog = new TextView( Rect(), "tv_dialog", "" );
	ImageButton *pcBDialogOk = new ImageButton( Rect(), "btn_ok", "OK", new Message( IE_BTN_DIALOG_OK ) );
	pcBDialogOk->SetImageFromFile( "/Applications/Launcher/icons/choice-yes.png" );
	ImageButton *pcBDialogCancel = new ImageButton( Rect(), "btn_cancel", "Cancel", new Message( IE_BTN_DIALOG_CANCEL ) );
	pcBDialogCancel->SetImageFromFile( "/Applications/Launcher/icons/choice-no.png" );
	pcBDialogOk->SetTarget( this );
	pcBDialogCancel->SetTarget( this );
	
	VLayoutNode *pcDialogRoot = new VLayoutNode( "dialog_root" );
	HLayoutNode *pcDialogBottom = new HLayoutNode( "dialog_bot" );
	pcDialogBottom->AddChild( pcBDialogCancel );
	pcDialogBottom->AddChild( new HLayoutSpacer( "space", 100 ) );
	pcDialogBottom->AddChild( pcBDialogOk );
	pcDialogRoot->AddChild( m_pcSVDialog );
	pcDialogRoot->AddChild( m_pcTVDialog );
	pcDialogRoot->AddChild( pcDialogBottom );
	
	Point cDialogSize = pcDialogRoot->GetPreferredSize( false );
	Rect cDialogRect = center_frame( Rect(0,0,cDialogSize.x + 16,cDialogSize.y + 16 ) );
	m_pcDialog = new Window( cDialogRect, "dialog", "" );	
	LayoutView *pcDialogLayout = new LayoutView( m_pcDialog->GetBounds(), "dialog_layout", pcDialogRoot );
	m_pcDialog->AddChild( pcDialogLayout );
	
	// Menus
	
	Menu *pcMenu = new Menu( Rect(), "Menu", ITEMS_IN_ROW );
	Menu *pcFileMenu = new Menu( Rect(), "File", ITEMS_IN_COLUMN );
	Menu *pcEditMenu = new Menu( Rect(), "Edit", ITEMS_IN_COLUMN );
	Menu *pcActionMenu = new Menu( Rect(), "Action", ITEMS_IN_COLUMN );
	Menu *pcHelpMenu = new Menu( Rect(), "Help", ITEMS_IN_COLUMN );
	
	pcFileMenu->AddItem( "New Window", new Message( IE_MNU_NEW_WINDOW ) );
	pcFileMenu->AddItem( new MenuSeparator( ) );
	pcFileMenu->AddItem( "Quit", new Message( IE_MNU_QUIT ) );
	
	pcEditMenu->AddItem( "Copy", new Message( IE_MNU_COPY ) );
	pcEditMenu->AddItem( "Paste", new Message( IE_MNU_PASTE ) );
	pcEditMenu->AddItem( new MenuSeparator() );
	pcEditMenu->AddItem( "Preferences", new Message( IE_MNU_OPEN_PREFS ) );
	
	pcActionMenu->AddItem( "Set Icon", new Message( IE_MNU_SET_ICON ) );
	pcActionMenu->AddItem( "Clear Icon", new Message( IE_MNU_DEL_ICON ) );
	pcActionMenu->AddItem( new MenuSeparator( ) );
	pcActionMenu->AddItem( "Home", new Message( IE_MNU_HOME ) );
	pcActionMenu->AddItem( "Up", new Message( IE_MNU_PARENT ) );
	pcActionMenu->AddItem( "Refresh", new Message( IE_MNU_REFRESH ) );
	pcActionMenu->AddItem( new MenuSeparator( ) );
	pcActionMenu->AddItem( "New Directory", new Message( IE_MNU_NEWDIR ) );
	pcActionMenu->AddItem( "Rename", new Message( IE_MNU_RENAME ) );
	pcActionMenu->AddItem( "Delete", new Message( IE_MNU_DELETE ) );
	
	pcHelpMenu->AddItem( "About", new Message( IE_MNU_ABOUT ) );
	
	pcMenu->AddItem( pcFileMenu );
	pcMenu->AddItem( pcEditMenu );
	pcMenu->AddItem( pcActionMenu );
	pcMenu->AddItem( pcHelpMenu );
	
	pcMenu->SetTargetForItems( this );

	// Put it all together

	Point cMenuSize = pcMenu->GetPreferredSize( false );
	Point cSize = m_pcRoot->GetPreferredSize( false );
	Rect cWinRect = center_frame( Rect(0,0,cSize.x + 64, cSize.x + 64 ) );
	pcMenu->SetFrame( Rect( 0,0, cWinRect.Width(), cMenuSize.y ) );
	SetFrame( cWinRect );
	
	Rect cInnerRect = GetBounds();
	cInnerRect.top += 2 + cMenuSize.y;
	cInnerRect.left += 2;
	cInnerRect.bottom -= 2;
	cInnerRect.right -= 2;
	
	LayoutView *pcLayout = new LayoutView( cInnerRect, "layout", m_pcRoot );
	AddChild( pcMenu );
	AddChild( pcLayout );
	
}
	

IconEditor::~IconEditor( )
{
	
}



bool IconEditor::OkToQuit( void )
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

void IconEditor::HandleMessage( Message *pcMessage )
{
	int nCode = pcMessage->GetCode();
	
	string zNewIcon;
	
	switch( nCode ) {
		
		case IE_DIR_CHANGED:
			m_zDirectory = m_pcDVDir->GetPath( );
			m_pcTVDir->Set( m_zDirectory.c_str() );
			SetTitle( "LBrowser - " + m_zDirectory );			
			break;
			
		case IE_SELECTION_CHANGED:
			SetStatus( );
			GetIcon( );
			break;
			
		case IE_MNU_SET_ICON:
		case IE_ICON_CLICKED:
			m_pcFRIcon->Show( true );
			break;

		case IE_ICON_CHANGED:
			pcMessage->FindString( "file/path", &zNewIcon );
			SetIcon( zNewIcon );
			break;
			
		case IE_MNU_DEL_ICON:
		case IE_BTN_CLEAR:
			SetIcon( "" );
			break;
						
		case IE_MNU_RENAME:
		case IE_BTN_RENAME:
			OpenRenameDialog();
			break;
			
		case IE_MNU_NEWDIR:
		case IE_BTN_NEW:
			OpenNewDirDialog();
			break;
			
		case IE_MNU_DELETE:
		case IE_BTN_DELETE:
			OpenDeleteDialog();
			break;
			
		case IE_BTN_DIALOG_CANCEL:
			m_nDialogMode = 0;
			m_pcDialog->Hide();
			break;
			
		case IE_BTN_DIALOG_OK:
			CompleteDialog( pcMessage );
			break;

		case IE_MNU_REFRESH:
		case IE_BTN_REFRESH:
			m_pcDVDir->Clear();
			m_pcDVDir->ReRead();
			break;
			
		case IE_MNU_PARENT:
		case IE_BTN_PARENT:
			ParentDir();
			break;

		case IE_MNU_HOME:
		case IE_BTN_HOME:
			m_pcDVDir->SetPath( convert_path( "~/" ) );
			m_zDirectory = m_pcDVDir->GetPath( );
			SetTitle( "LBrowser - " + m_zDirectory );
			m_pcDVDir->Clear();
			m_pcDVDir->ReRead();	
			break;

		case IE_DOUBLE_CLICK:
			Launch();
			break;
			
		case IE_MNU_QUIT:
			OkToQuit();
			break;
			
		case IE_MNU_NEW_WINDOW:
			OpenNewWindow();
			break;
			
		case IE_BTN_COPY:
		case IE_MNU_COPY:
			CopyFiles();
			break;
			
		case IE_BTN_PASTE:
		case IE_MNU_PASTE:
			PasteFiles();
			break;
			
		case IE_MNU_ABOUT:
			OpenAbout();
			break;
			
		case IE_MNU_OPEN_PREFS:
			OpenPrefs( );
			break;
			
		case IE_SET_PREFS:
			pcMessage->FindBool( "IconLabels", &m_bIconLabels );
			SavePrefs( );
			LoadPrefs( );
			m_pcPrefsWindow = NULL;
			SizeWindow();
			break;
			
		case IE_CANCEL_PREFS:
			m_pcPrefsWindow = NULL;
			break;
			
		default:
			break;
	}
}


void IconEditor::OpenRenameDialog( void )
{
	if( m_zFile != "" ) {
		m_nDialogMode = IE_DIALOG_MODE_RENAME;
		string zNode = m_zDirectory + (string)"/" + m_zFile;
		string zLabel = (string)"Renaming " + m_zFile;
		m_pcSVDialog->SetString( zLabel.c_str() );
		m_pcTVDialog->Set( zNode.c_str() );
		m_pcDialog->SetTitle( "Renaming..." );
		m_pcDialog->Show( true );
	}
}

void IconEditor::OpenNewDirDialog( void )
{
	m_nDialogMode = IE_DIALOG_MODE_NEWDIR;
	m_pcSVDialog->SetString( "Enter the name of the new directory." );
	m_pcTVDialog->Set( "" );
	m_pcDialog->SetTitle( "New Directory..." );
	m_pcDialog->Show( true );
}

void IconEditor::OpenDeleteDialog( void )
{
	char nKey = VK_DELETE;
	m_pcDVDir->KeyDown( &nKey, &nKey, 0 );
}

void IconEditor::CompleteDialog( Message *pcMessage )
{
	m_pcDialog->Hide();
	
	string zResult = m_pcTVDialog->GetBuffer()[0];
	if( zResult.find( "/", 0,1 ) == string::npos )
		zResult = m_zDirectory + (string)"/" + zResult;
	
	string zOriginal = m_zDirectory + (string)"/" + m_zFile;	
	switch( m_nDialogMode ) {
		
		case IE_DIALOG_MODE_RENAME:
			if( rename( zOriginal.c_str(), zResult.c_str() ) == 0 ) {
				m_zFile = "";
				m_pcDVDir->Clear();
				m_pcDVDir->ReRead();
			}	
			break;
			
		case IE_DIALOG_MODE_NEWDIR:
			mkdir( zResult.c_str(),  S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH );
			break;
			
		default:
			break;
			
	}
	
	m_nDialogMode = 0;
}

void IconEditor::GetIcon( void )
{
	int nSel = m_pcDVDir->GetFirstSelected();
	if( nSel > -1 ) {
		FileRow *pcFile = m_pcDVDir->GetFile( nSel );
		m_zFile = pcFile->GetName();
		string zNode = m_zDirectory + (string)"/" + m_zFile;
		MimeNode *pcNode = new MimeNode( zNode.c_str() );	
		char zIcon[256];
		int n = pcNode->ReadAttr("LAUNCHER_ICON", ATTR_TYPE_STRING, &zIcon, 0, 255 );
		
		delete pcNode;
		if( n > 0 ) {
			zIcon[n] = 0;	
			m_pcIcon->SetImageFromFile( zIcon );
		} else {
			m_pcIcon->ClearImage( );
			m_pcIcon->SetImageFromFile( pcNode->GetDefaultIcon() );
		}
			
	} else { m_zFile = ""; }

	SizeWindow();
}


void IconEditor::SetIcon( string zIcon )
{
	FSNode *pcNode = new FSNode( m_zDirectory + (string)"/" + m_zFile );
	pcNode->WriteAttr( "LAUNCHER_ICON", O_TRUNC, ATTR_TYPE_STRING, zIcon.c_str(), 0, zIcon.length() );
	
	if( zIcon != "" ) {
		m_pcIcon->SetImageFromFile( zIcon.c_str() );	
	} else {
		m_pcIcon->ClearImage( );
	}

	SizeWindow();	
}

void IconEditor::SizeWindow( void )
{
	
	LayoutView *pcLayout = (LayoutView *)FindView( "layout" );
	pcLayout->InvalidateLayout();

//	SetFrame( GetFrame() );
	
	Rect cFrame = GetFrame();
	cFrame.bottom++;
//	cFrame.right++;
	SetFrame( cFrame );
	cFrame.bottom--;
//	cFrame.right--;
	SetFrame( cFrame );

	
}	


void IconEditor::ParentDir( void )
{
	m_pcDVDir->SetPath( m_zDirectory + "/.." );
	m_zDirectory = m_pcDVDir->GetPath( );
	SetTitle( m_zDirectory + " - LBrowser"  );
	m_pcDVDir->Clear();
	m_pcDVDir->ReRead();	
}


void IconEditor::Launch( void )
{

	string zFile = m_zDirectory + "/" + m_zFile;
	MimeNode *pcNode = new MimeNode( zFile );
	pcNode->Launch();
	delete pcNode;
}


void IconEditor::OpenNewWindow( void )
{
	pid_t nPid = fork();
	
	if( nPid == 0 )
	{	
		set_thread_priority( -1, 0 );
		execlp( "/Applications/Launcher/bin/LBrowser", "LBrowser", m_zDirectory.c_str(), NULL );
		exit( 1 );
	}
}

void IconEditor::CopyFiles( void )
{
	Clipboard cClipboard;
	cClipboard.Lock( );
	cClipboard.Clear( );
	Message *pcData = cClipboard.GetData( );
	
	for( int n = m_pcDVDir->GetFirstSelected(); n <= m_pcDVDir->GetLastSelected(); n++ ) {
		FileRow *pcFile = m_pcDVDir->GetFile( n );
		if( pcFile->IsSelected() )
			pcData->AddString( "file/path", m_zDirectory + (string)"/" + pcFile->GetName() );
	}
	
	cClipboard.Commit( );
	cClipboard.Unlock( );
}


void IconEditor::PasteFiles( void )
{
	Clipboard cClipboard;
	cClipboard.Lock( );
	Message *pcData = cClipboard.GetData( );
	string zPath;
	if( pcData->FindString( "file/path", &zPath ) == 0 ) {
		printf( "Copying Files %s\n", zPath.c_str() );
		m_pcDVDir->MouseUp( Point(10,10), 0, pcData );
	}
	cClipboard.Unlock( );
}

void IconEditor::OpenAbout( void )
{
	Alert *pcAboutAlert = new Alert( "About LBrowser","LBrowser\n\nVersion 0.1 (April 2002)\n\nAuthor: Andrew Kennan\n\nSee /Applications/Launcher/README\nfor general information.\n\nDevelopers should read\n/Applications/Launcher/Developers\n\nThis program is licenced under the GNU GPL.\nFor more information see\n/Applications/Launcher/COPYING", 0, "OK", NULL );
	pcAboutAlert->Go(new Invoker( new Message(), this ));
	

}


void IconEditor::OpenPrefs( void )
{
	if( m_pcPrefsWindow == NULL )
	{
		Message *pcPrefs = new Message( );
		pcPrefs->AddBool( "IconLabels", m_bIconLabels );
		m_pcPrefsWindow = new LBrowsePrefs( this, pcPrefs );
		m_pcPrefsWindow->Show( true );
	}
}

void IconEditor::SavePrefs( void )
{
	Message *pcPrefs = new Message( );
	pcPrefs->AddBool( "IconLabels", m_bIconLabels );
	File *pcConfig = new File( convert_path( CONFIG_FILE ), O_WRONLY | O_CREAT );  
	if( pcConfig ) {
		uint32 nSize = pcPrefs->GetFlattenedSize( );
		void *pBuffer = malloc( nSize );
		pcPrefs->Flatten( (uint8 *)pBuffer, nSize );
		
		pcConfig->Write( pBuffer, nSize );
		
		delete pcConfig;
		free( pBuffer );
	}
}

void IconEditor::LoadPrefs( void )
{
	FSNode *pcNode = new FSNode();
	
	if( pcNode->SetTo( convert_path( CONFIG_FILE ) ) == 0 ) {
		File *pcConfig = new File( *pcNode );
		uint32 nSize = pcConfig->GetSize( );
		void *pBuffer = malloc( nSize );
		pcConfig->Read( pBuffer, nSize );
		Message *pcPrefs = new Message( );
		pcPrefs->Unflatten( (uint8 *)pBuffer );
		
		pcPrefs->FindBool( "IconLabels", &m_bIconLabels );
	}
	
	if( m_bIconLabels ) {
		m_pcNewBtn->SetLabel( "New Dir" );
		m_pcRenameBtn->SetLabel("Rename" );
		m_pcDeleteBtn->SetLabel("Delete" );
		m_pcClearBtn->SetLabel("Clear Icon" );
		m_pcCopyBtn->SetLabel("Copy" );
		m_pcPasteBtn->SetLabel("Paste" );
		m_pcHomeBtn->SetLabel("Home" );
		m_pcRefreshBtn->SetLabel("Refresh" );
		m_pcParentBtn->SetLabel( "Parent" );
	} else {
		m_pcNewBtn->SetLabel( "" );
		m_pcRenameBtn->SetLabel("" );
		m_pcDeleteBtn->SetLabel("" );
		m_pcClearBtn->SetLabel("" );
		m_pcCopyBtn->SetLabel("" );
		m_pcPasteBtn->SetLabel("" );
		m_pcHomeBtn->SetLabel("" );
		m_pcRefreshBtn->SetLabel("" );
		m_pcParentBtn->SetLabel( "" );
	}
	
	m_pcToolBar->SameHeight( "btn_new","btn_rename", "btn_del","btn_clear","btn_copy","btn_paste","btn_home","btn_refresh","btn_parent",NULL );
}



void IconEditor::SetStatus( void )
{
	uint32 nCount = 0;
	uint32 nSize = 0;
	
	for( int n = m_pcDVDir->GetFirstSelected(); n <= m_pcDVDir->GetLastSelected(); n++ ) {
		FileRow *pcFile = m_pcDVDir->GetFile( n );
		if( pcFile->IsSelected() ) {
			nCount++;
			struct stat sStat = pcFile->GetFileStat();
			nSize += sStat.st_size;
		}
			
	}
	
	char zSize[8];
	char zCount[8];
	string zPlural = (nCount != 1)?"s":"";
	
	sprintf( zSize, "%ld", nSize );
	sprintf( zCount, "%ld", nCount );
	
	string zStatus = (string)zCount + (string)" Item" + zPlural + (string)" Selected, " + zSize + (string)" bytes.";
	
	m_pcStatusString->SetString( zStatus.c_str() );
}



LBrowsePrefs::LBrowsePrefs( IconEditor *pcParent, Message *pcPrefs ) : Window( Rect(), "LBrowsePrefs", "LBrowser Preferences" )
{
	m_pcParent = pcParent;
	bool bIconLabels;
	pcPrefs->FindBool( "IconLabels", &bIconLabels );
	
	VLayoutNode *pcRoot = new VLayoutNode( "Root" );
	HLayoutNode *pcBottom = new HLayoutNode( "Bottom" );
	VLayoutNode *pcInner = new VLayoutNode( "Inner" );
	HLayoutNode *pcInner2 = new HLayoutNode( "Inner2" );
	
	m_pcCBIconLabels = new CheckBox( Rect(), "cb_iconlabels", "Show Toolbar Labels", new Message() );
	m_pcCBIconLabels->SetValue( bIconLabels );
	pcInner2->AddChild( m_pcCBIconLabels );
	pcInner2->AddChild( new VLayoutSpacer( "space", 1.0f ) );
	pcInner->AddChild( new HLayoutSpacer( "space", 100 ) );
	pcInner->AddChild( pcInner2 );

	
	ImageButton *pcBtnOk = new ImageButton( Rect(), "btn_ok", "OK", new Message( IE_BTN_DIALOG_OK ) );
	pcBtnOk->SetImageFromFile( "/Applications/Launcher/icons/choice-yes.png" );
	ImageButton *pcBtnCancel = new ImageButton( Rect(), "btn_cancel", "Cancel", new Message( IE_BTN_DIALOG_CANCEL ) );
	pcBtnCancel->SetImageFromFile( "/Applications/Launcher/icons/choice-no.png");
	pcBottom->AddChild( pcBtnCancel );
	pcBottom->AddChild( new HLayoutSpacer( "space", 100 ) );
	pcBottom->AddChild( pcBtnOk );
	
	Point cInnerSize = pcInner->GetPreferredSize( false );
	FrameView *pcFrame = new FrameView( Rect(0,0,cInnerSize.x, cInnerSize.y), "frame", "" );
	pcFrame->SetRoot( pcInner );
	
	pcRoot->AddChild( pcFrame );
	pcRoot->AddChild( pcBottom );
	
	Point cRootSize = pcRoot->GetPreferredSize( false );
	
	LayoutView *pcLayout = new LayoutView( Rect( 0,0, cRootSize.x + 32, cRootSize.y + 32 ), "layout" );
	pcLayout->SetRoot( pcRoot );
	
	SetFrame( center_frame( Rect( 0,0, cRootSize.x + 32, cRootSize.y + 32 ) ) );
	AddChild( pcLayout );
}



LBrowsePrefs::~LBrowsePrefs( ) { }

bool LBrowsePrefs::OkToQuit( void )
{
	Invoker *pcInvoker = new Invoker( new Message( IE_CANCEL_PREFS ), m_pcParent );
	pcInvoker->Invoke();
	return true;
}



void LBrowsePrefs::HandleMessage( Message *pcMessage )
{
	int nCode = pcMessage->GetCode();
	
	switch( nCode ) {
		
		case IE_BTN_DIALOG_OK:
			SetPrefs( );
			Quit( );
			break;
			
		case IE_BTN_DIALOG_CANCEL:
			OkToQuit( );
			Quit();
			break;
			
		default:
			break;
	}
}



void LBrowsePrefs::SetPrefs( void )
{
	Message *pcPrefs = new Message( IE_SET_PREFS );
	pcPrefs->AddBool( "IconLabels", m_pcCBIconLabels->GetValue( ) );
	
	Invoker *pcInvoker = new Invoker( pcPrefs, m_pcParent );
	pcInvoker->Invoke();
}


