/*
    Locator - A fast file finder for Syllable
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

/***********************************************
**
** Title: locator_window.cpp
**
** Author: Andrew Kennan
**
** Date: 25/10/2001
**
***********************************************/ 

#include "locator_window.h"

/***********************************************
**
**
**
***********************************************/ 
LocatorWindow::LocatorWindow( ) : Window( Rect( 0,0,0,0 ), "locator_window", APP_NAME )
{

  string zStatus;

  bSafeToSearch = false;
  bFollowSubDirs = false;
  bMatchDirNames = true;
  bUseRegex = true;

  m_pcPrefWindow = NULL;

  zStatus = ReadInfo( );

  VLayoutNode *pcRoot = new VLayoutNode( "vlayout" );
  HLayoutNode *pcTopLayout = new HLayoutNode( "toplayout", 1.0f, pcRoot );
  HLayoutNode *pcSearchLayout = new HLayoutNode( "midlayout", 1.0f );
  HLayoutNode *pcBotLayout = new HLayoutNode( "botlayout", 100, pcRoot );
  HLayoutNode *pcStatusLayout = new HLayoutNode( "statuslayout", 1.0f );
  VLayoutNode *pcSLLeft = new VLayoutNode( "slleft", 1.0f );
  VLayoutNode *pcSLRight = new VLayoutNode( "slright", 1.0f );

  
  pcMenu = new Menu( Rect(0,0,0,0), "Menu", ITEMS_IN_ROW );
  Menu *pcFileMenu = new Menu( Rect(0,0,0,0), "File", ITEMS_IN_COLUMN );
  Menu *pcEditMenu = new Menu( Rect(0,0,0,0), "Edit", ITEMS_IN_COLUMN );
  pcOptMenu = new Menu( Rect(0,0,0,0), "Options", ITEMS_IN_COLUMN );
  Menu *pcHelpMenu = new Menu( Rect(0,0,0,0), "Help", ITEMS_IN_COLUMN );

  pcMenu->AddItem( pcFileMenu );
  pcMenu->AddItem( pcEditMenu );
  pcMenu->AddItem( pcOptMenu );
  pcMenu->AddItem( pcHelpMenu );

  pcFileMenu->AddItem( "Search", new Message( EV_BTN_SEARCH ) );
  pcFileMenu->AddItem( new MenuSeparator( ) );
  pcFileMenu->AddItem( "Quit", new Message( EV_BTN_CLOSE ) );
 
  pcEditMenu->AddItem( "Select All", new Message( EV_MENU_SELECT_ALL ) );
  pcEditMenu->AddItem( "Select None", new Message( EV_MENU_SELECT_NONE ) );
  pcEditMenu->AddItem( "Copy", new Message( EV_MENU_COPY ) );

  pcOptMenu->AddItem( "Preferences...", new Message( EV_MENU_OPEN_PREF ) );

  pcHelpMenu->AddItem( "About Locator", new Message( EV_MENU_ABOUT ) );

  AddChild( pcMenu );
  pcMenu->SetTargetForItems( this );

  FrameView *pcStatusFrame = new FrameView( Rect(0,0,0,0), "status", "" );
  pcStatusString = new StringView( Rect(0,0,0,0), "statusstring", zStatus.c_str( ), ALIGN_LEFT );
//  pcStatusString->SetMinPreferredSize( 30 );
  FrameView *pcFrame = new FrameView( Rect(0,0,0,0), "frame", "" );
  pcTopLayout->AddChild( pcFrame );

  HLayoutNode *pcButtonLayout = new HLayoutNode( "btnlayout", 1.0f );

  pcSearch = new DropdownMenu( Rect(0,0,0,0), "search" );
  pcSearch->SetEditMessage( new Message( EV_SEARCH_STRING_CHANGED ) );
  pcSearch->SetSelectionMessage( new Message( EV_SEARCH_ITEM_CHANGED ) );
  pcSearch->SetTarget( this );

  pcSearchDir = new DropdownMenu( Rect(0,0,0,0), "searchdir" );
  pcSearchDir->SetSelectionMessage( new Message( EV_SEARCH_DIR_ITEM_CHANGED ) );
  pcSearchDir->SetCurrentString( (std::string)"/");

  ReadHistory( );

  pcSearchButton = new ImageButton( Rect(0,0,0,0), "searchbtn", "Search", new Message( EV_BTN_SEARCH ), IB_TEXT_BOTTOM ) ;
  pcSearchButton->SetImageFromFile( app_path() + (string)"icons/filefind.png" );
  
  ImageButton *pcCloseButton = new ImageButton( Rect(0,0,0,0), "closebtn", "Quit", new Message( EV_BTN_CLOSE ), IB_TEXT_BOTTOM );
  pcCloseButton->SetImageFromFile( app_path() + (string)"icons/quit.png" );

  ImageButton *pcPrefButton = new ImageButton( Rect(0,0,0,0), "prefbtn", "Prefs...", new Message( EV_BTN_PREF ), IB_TEXT_BOTTOM );
  pcPrefButton->SetImageFromFile( app_path() + (string)"icons/configuration.png" );

  ImageButton *pcCopyButton = new ImageButton( Rect(0,0,0,0), "copybtn", "Copy", new Message( EV_BTN_COPY ), IB_TEXT_BOTTOM );
  pcCopyButton->SetImageFromFile( app_path() + (string)"icons/editcopy.png" );
  
  pcSLLeft->AddChild( new StringView( Rect(0,0,0,0), "searchlabel", "Search For", ALIGN_RIGHT ) );
  pcSLLeft->AddChild( new StringView( Rect(0,0,0,0), "searchdirlabel", "Look In", ALIGN_RIGHT ) );
  pcSLLeft->SameWidth( "searchlabel", "searchdirlabel", NULL );
  pcSLLeft->SetBorders( Rect( 8,4,8,4 ), "searchdirlabel", "searchlabel", NULL );
  pcSLRight->AddChild( pcSearch );
  pcSLRight->AddChild( pcSearchDir );
  pcSLRight->SameWidth( "search", "searchdir", NULL );
  pcSLRight->SetBorders( Rect( 8,4,8,4 ), "search", "searchdir", NULL );

  pcButtonLayout->AddChild( pcSearchButton );
  pcButtonLayout->AddChild( pcCopyButton );
  pcButtonLayout->AddChild( pcPrefButton );
  pcButtonLayout->AddChild( pcCloseButton );
  pcButtonLayout->SameWidth( "searchbtn", "closebtn", "prefbtn", "copybtn", NULL );
  pcButtonLayout->SameHeight( "searchbtn", "closebtn", "prefbtn", "copybtn", NULL );  
  pcButtonLayout->SetBorders( Rect( 4,4,4,4 ), "searchbtn", "closebtn", "prefbtn", "copybtn", NULL );

  pcSearchLayout->AddChild( pcSLLeft );
  pcSearchLayout->AddChild( pcSLRight );
  pcSearchLayout->AddChild( pcButtonLayout );
  pcSearchLayout->SetBorders( Rect( 4,4,4,4 ), "slleft", "slright", NULL );
  pcFrame->SetRoot( pcSearchLayout );

  pcFoundItems = new ListView( Rect(0,0,0,0), "", os::ListView::F_MULTI_SELECT | os::ListView::F_NO_COL_REMAP );

  pcFoundItems->InsertColumn( "Name", 250 );
  pcFoundItems->InsertColumn( "Path", 250 );
  pcFoundItems->InsertColumn( "Type", 32 );

  pcBotLayout->AddChild( pcFoundItems );
  pcBotLayout->AddChild( new VLayoutSpacer( "space", 200 ) );
  pcTopLayout->AddChild( new HLayoutSpacer( "space", 100 ) );
  
  pcStatusLayout->AddChild( pcStatusString );
  pcStatusLayout->AddChild( new HLayoutSpacer( "space", 100 ) );
  pcStatusLayout->SetBorders( Rect( 2,2,2,2 ), "statusstring", NULL );
  pcStatusFrame->SetRoot( pcStatusLayout );

  pcRoot->AddChild( pcStatusFrame );
  pcRoot->AddChild( new HLayoutSpacer( "space", 100 ) );
  pcRoot->SetBorders( Rect( 4,4,2,2 ), "toplayout", NULL );
  pcRoot->SetBorders( Rect( 2,2,2,2 ), "botlayout", "status", NULL );

  Desktop *pcDesktop = new Desktop( );
  IPoint *pcRes = new IPoint( pcDesktop->GetResolution( ) );
  int nResX = (int)(pcRes->x / 2);
  int nResY = (int)(pcRes->y / 2);

  Point cSize = pcRoot->GetPreferredSize( false );
  int nWinX = (int)(cSize.x / 2) + 8;
  int nWinY = (int)(cSize.y / 2) + 28;

  Rect cWinRect = Rect( nResX - nWinX, nResY - nWinY, nResX + nWinX, nResY + nWinY );

  SetFrame( cWinRect );
  SetSizeLimits( cWinRect.Size( ), Point( pcRes->x, pcRes->y ) );

  // Add the window contents
  Rect cMainFrame = GetBounds( );
  Rect cMenuFrame = cMainFrame;

  cMenuFrame.bottom = pcMenu->GetPreferredSize( false ).y - 1;
  cMainFrame.top = cMenuFrame.bottom + 1;
  pcMenu->SetFrame( cMenuFrame );
  LayoutView *pcLayout = new LayoutView( cMainFrame, "", pcRoot );
  AddChild( pcLayout );

  SetDefaultButton( pcSearchButton );
  MakeFocus( true );
  pcSearchButton->SetEnable( false );
  pcSearch->MakeFocus( true );

  pcSearchThread = new SearchThread( this );

}




/***********************************************
**
**
**
***********************************************/ 
LocatorWindow::~LocatorWindow( )
{

}



/***********************************************
**
**
**
***********************************************/ 
bool LocatorWindow::OkToQuit( )
{

  WriteHistory( );
  Application::GetInstance( )->PostMessage( M_QUIT );
  return true;

}



/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::Close( )
{

  OkToQuit( );
   
}



/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::HandleMessage( Message *pcMessage )
{

  int nId = pcMessage->GetCode( );

  Message *pcPrefs = new Message();

  switch( nId )
  {

    case EV_BTN_CLOSE:
      OkToQuit( );

    case EV_BTN_SEARCH:
      if( bSafeToSearch )
        StartSearch( pcMessage );
      break;

    case EV_SEARCH_STRING_CHANGED:
      if( !(pcSearch->GetCurrentString( ) == "") )
      {

        pcSearchButton->SetEnable( true );
        bSafeToSearch = true;

      } else {

        pcSearchButton->SetEnable( false );
        bSafeToSearch = false;

      }
      break;

    case EV_ITEM_FOUND:
      AddFoundItem( pcMessage );
      break;

    case EV_MENU_SELECT_ALL:
      if( pcFoundItems->GetRowCount( ) > 0 )
        pcFoundItems->Select( 0, (int)pcFoundItems->GetRowCount( ) - 1 );
      break;

    case EV_MENU_SELECT_NONE:
      pcFoundItems->ClearSelection( );
      break;

	case EV_BTN_COPY:
    case EV_MENU_COPY:
      CopySelected( );
      break;

    case EV_MENU_MATCH_DIR:
      bMatchDirNames = bMatchDirNames ? false : true;
      UpdateOptionMenu( );
      break;

    case EV_MENU_FOLLOW_DIR:
      bFollowSubDirs = bFollowSubDirs ? false : true;
      UpdateOptionMenu( );
      break;

    case EV_MENU_REGEX:
      bUseRegex = bUseRegex ? false : true;
      UpdateOptionMenu( );
      break;

    case EV_MENU_SORT:
      bSortResults = bSortResults ? false : true;
      UpdateOptionMenu( );
      break;

    case EV_MENU_ABOUT:
      DisplayAbout( );
      break;

    case ERR_DIR_NOT_FOUND:
      pcStatusString->SetString("Directory Not Found.");
      pcSearchButton->SetEnable( true );
      bSafeToSearch = true;
      break;

    case ERR_REGEX:
      pcStatusString->SetString("Error In Regex.");
      break;

    case ERR_DATA_FILE:
      pcStatusString->SetString("Could Not Open Data File.");
      break;

    case ERR_INDEX_FILE:
      pcStatusString->SetString("Could Not Open Index File.");
      pcSearchThread = NULL;
      if( !(pcSearch->GetCurrentString( ) == "") )
      {
        pcSearchButton->SetEnable( true );
        bSafeToSearch = true;
      }
      break;

    case EV_SEARCH_FINISHED:
      pcSearchButton->SetEnable( true );
      bSafeToSearch = true;
      break;

	case EV_BTN_PREF:
	case EV_MENU_OPEN_PREF:
	  if( m_pcPrefWindow == NULL ) {
		pcPrefs->AddBool( "MatchDirNames", bMatchDirNames );
		pcPrefs->AddBool( "SortResults", bSortResults );
		pcPrefs->AddBool( "FollowSubDirs", bFollowSubDirs );
		pcPrefs->AddBool( "UseRegex", bUseRegex );

	  	m_pcPrefWindow = new LocatorPrefWindow( this, pcPrefs );
	  	m_pcPrefWindow->Show();
	  }
	  break;
	  
	case EV_PREF_BTN_OK:
	  pcMessage->FindBool( "MatchDirNames", &bMatchDirNames );
	  pcMessage->FindBool( "SortResults", &bSortResults );
	  pcMessage->FindBool( "FollowSubDirs", &bFollowSubDirs );
	  pcMessage->FindBool( "UseRegex", &bUseRegex );
	  m_pcPrefWindow->Quit();
	  break;
	  
	case EV_PREF_BTN_CANCEL:
	  m_pcPrefWindow->Quit();
	  break;		

	case EV_PREF_WINDOW_CLOSED:
	  m_pcPrefWindow = NULL;
	  break;
 
    default:
      break;

  }
}




/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::StartSearch( Message *pcMessage )
{

  static bool bRestartedThread;
  Message *pcSearchMessage;
  Invoker *pcSearchInvoker;
  string pcTemp;

  if( pcSearchThread )
  {

    bRestartedThread = false;
    pcSearchButton->SetEnable( false );
    bSafeToSearch = false;

    pcFoundItems->Clear( );
    pcSearchMessage = new Message( EV_START_SEARCH );

    pcTemp = pcSearch->GetCurrentString( );
    pcSearchMessage->AddString( "search", pcTemp );

    for( int32 n = 0; n < pcSearch->GetItemCount( ); n++ )
      if( pcSearch->GetItem( n ) == pcTemp )
        pcSearch->DeleteItem( n );

    pcSearch->InsertItem( 0, pcTemp.c_str( ) );
    if( pcSearch->GetItemCount( ) > MAX_HISTORY )
      pcSearch->DeleteItem( MAX_HISTORY );


    if( !( pcSearchDir->GetCurrentString( ) == "" ) )
    {

      pcTemp = pcSearchDir->GetCurrentString( );
      pcSearchMessage->AddString( "dir", pcTemp );

      for( int32 n = 0; n < pcSearchDir->GetItemCount( ); n++ )
        if( pcSearchDir->GetItem( n ) == pcTemp )
          pcSearchDir->DeleteItem( n );

      pcSearchDir->InsertItem( 0, pcTemp.c_str( ) );
      if( pcSearchDir->GetItemCount( ) > MAX_HISTORY )
        pcSearchDir->DeleteItem( MAX_HISTORY );

    } else {
      pcSearchMessage->AddString( "dir", "/" );
    }

    pcSearchMessage->AddBool( "FollowSubDirs", bFollowSubDirs );
    pcSearchMessage->AddBool( "MatchDirNames", bMatchDirNames );
    pcSearchMessage->AddBool( "UseRegex", bUseRegex );

    nItemsFound = 0;
    pcStatusString->SetString( "0 Items Found" );

    pcSearchInvoker = new Invoker( pcSearchMessage, pcSearchThread );
    pcSearchInvoker->Invoke( );

  } else {

    if( bRestartedThread == false )
    {

      pcSearchThread = new SearchThread( this );
      bRestartedThread = true;
      StartSearch( pcMessage );

    } else {

      pcStatusString->SetString("Problem With Search Thread. Could Not Search.\n");
      bRestartedThread = false;

    }

  }
      
}



/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::AddFoundItem( Message *pcFoundMessage )
{

  int32 nType = 0;
  string zName;
  string zDir;

  char pzBuf[128];

  nItemsFound++;
  sprintf( pzBuf, "%d Items Found", nItemsFound );
  pcStatusString->SetString( pzBuf );

  pcFoundMessage->FindInt32( "type", &nType );
  pcFoundMessage->FindString( "name", &zName );
  pcFoundMessage->FindString( "dir", &zDir );

  ListViewStringRow *pcRow = new ListViewStringRow( );

  if( nType == L_FILE )
  {

    pcRow->AppendString( zName );
    pcRow->AppendString( zDir );
    pcRow->AppendString( (std::string)"File" );

  } else {

    string zPath = zDir + zName;
    pcRow->AppendString( zPath );
    pcRow->AppendString( (std::string)"" );
    pcRow->AppendString( (std::string)"Dir" );

  }    


  pcFoundItems->InsertRow( pcRow, true );
  pcFoundItems->SetAutoSort( bSortResults );   

}




/***********************************************
**
**
**
***********************************************/ 
std::string LocatorWindow::ReadInfo( void )
{

  FSNode *pcNode = new FSNode();
  string zInfo = APP_NAME;
  
  if( pcNode->SetTo( (string)(app_path() + INFO_FILE) ) == 0 )
  {
 
    File *pcInfo = new File( *pcNode );
    uint32 nSize = pcInfo->GetSize( );
    void *pBuffer = malloc( nSize );
    pcInfo->Read( pBuffer, nSize );
    Message *pcInfoMsg = new Message( );
    pcInfoMsg->Unflatten( (uint8 *)pBuffer );
	
	int32 nTimeStamp = 0;
	int32 nFileCount = 0;
	int32 nDirCount = 0;
	
	pcInfoMsg->FindInt32( "TimeStamp", &nTimeStamp );
	pcInfoMsg->FindInt32( "FileCount", &nFileCount );
	pcInfoMsg->FindInt32( "DirCount", &nDirCount );
    struct tm *psTime = gmtime( &nTimeStamp );
    
    char zLine[1024];
	sprintf( zLine, " - %ld Files And %ld Directories Indexed At %02d:%02d:%02d %02d/%02d/%4d.", nFileCount, nDirCount,  psTime->tm_hour, psTime->tm_min, psTime->tm_sec, psTime->tm_mday, psTime->tm_mon + 1, psTime->tm_year + 1900 );
	
	zInfo += (string)zLine;

	delete pcInfo;
  }
  
  delete pcNode;

  return zInfo;

}

      


/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::ReadHistory( void )
{
  FSNode *pcConfigDir = new FSNode();
  if( pcConfigDir->SetTo( PREFS_DIR ) != 0 )
  {
  	dbprintf( "Config Directory Not Found. Creating Default Config Directory\n" );
	mkdir( convert_path( PREFS_DIR ).c_str(), 700 );
  }

  FSNode *pcNode = new FSNode();
  
  if( pcNode->SetTo( PREFS_FILE ) != 0 )
  {
  	dbprintf( "Config File Not Found. Using Default Config File\n" );
  } else {
  
    File *pcConfig = new File( *pcNode );
    uint32 nSize = pcConfig->GetSize( );
    void *pBuffer = malloc( nSize );
    pcConfig->Read( pBuffer, nSize );
    Message *pcPrefs = new Message( );
    pcPrefs->Unflatten( (uint8 *)pBuffer );

	pcPrefs->FindBool( "MatchDirNames", &bFollowSubDirs );
	pcPrefs->FindBool( "SortResults", &bSortResults );
	pcPrefs->FindBool( "FollowSubDirs", &bFollowSubDirs );
	pcPrefs->FindBool( "UseRegex", &bUseRegex );

	int nNameCount = 0;
	int nJunk = 0;
	string zName;
	pcPrefs->GetNameInfo( "Name", &nJunk, &nNameCount );
	for( int n = 0; n < nNameCount; n++ ) {
		pcPrefs->FindString( "Name", &zName, n );
		pcSearch->AppendItem( zName.c_str() );
	}

	pcPrefs->GetNameInfo( "Dir", &nJunk, &nNameCount );
	for( int n = 0; n < nNameCount; n++ ) {
		pcPrefs->FindString( "Dir", &zName, n );
		pcSearchDir->AppendItem( zName.c_str() );
	}

	free( pBuffer ); 
	delete pcConfig;
	delete pcPrefs;
	
  }
  
  delete pcNode;
  delete pcConfigDir;
  
}




/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::WriteHistory( void )
{

	Message *pcPrefs = new Message();
	pcPrefs->AddBool( "MatchDirNames", bFollowSubDirs );
	pcPrefs->AddBool( "SortResults", bSortResults );
	pcPrefs->AddBool( "FollowSubDirs", bFollowSubDirs );
	pcPrefs->AddBool( "UseRegex", bUseRegex );
	
	for( int32 n = 0; n < pcSearch->GetItemCount( ); n++ )
		pcPrefs->AddString( "Name", pcSearch->GetItem( n ) );
		
	for( int32 n = 0; n < pcSearchDir->GetItemCount( ); n++ )
		pcPrefs->AddString( "Dir", pcSearchDir->GetItem( n ) );
	

	File *pcConfig = new File( convert_path( (string)PREFS_FILE), O_WRONLY | O_CREAT );  
	if( pcConfig ) {
	    uint32 nSize = pcPrefs->GetFlattenedSize( );
    	void *pBuffer = malloc( nSize );
	    pcPrefs->Flatten( (uint8 *)pBuffer, nSize );

    	pcConfig->Write( pBuffer, nSize );
      
	    delete pcConfig;
    	free( pBuffer );
	}

}




/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::CopySelected( void )
{

  string zItems;
  
//  printf( "%d, %d\n", pcFoundItems->GetFirstSelected(), pcFoundItems->GetLastSelected() );
  
  if( pcFoundItems->GetLastSelected() > 0 ) {
    for( int n = pcFoundItems->GetFirstSelected( ); n <= pcFoundItems->GetLastSelected( ); n++ )
    {

      ListViewStringRow *pcRow = (ListViewStringRow *)pcFoundItems->GetRow( n );

      if( pcRow->IsSelected( ) )
      {

        zItems += pcRow->GetString( 1 );
        if( zItems.length( ) > 0 )
          zItems += "/";
        zItems += pcRow->GetString( 0 );
        if( n < pcFoundItems->GetLastSelected( ) )
          zItems += "\n";

      }

    }

    Clipboard cClipboard;
    cClipboard.Lock( );
    cClipboard.Clear( );
    Message *pcData = cClipboard.GetData( );
    pcData->AddString( "text/plain", zItems.c_str( ) );
    pcData->AddString( "file/path", zItems.c_str() );
    cClipboard.Commit( );
    cClipboard.Unlock( );

  }
}





/***********************************************
**
**
**
***********************************************/ 
void LocatorWindow::UpdateOptionMenu( void )
{

  pcMenu->RemoveItem( pcOptMenu ); 

  delete pcOptMenu->RemoveItem( 0 );
  delete pcOptMenu->RemoveItem( 0 );
  delete pcOptMenu->RemoveItem( 0 );
  delete pcOptMenu->RemoveItem( 0 );
  delete pcOptMenu;

  pcOptMenu = new Menu( Rect(0,0,0,0), "Options", ITEMS_IN_COLUMN );
  pcMenu->AddItem( pcOptMenu, 2 );

  string zTemp;

  zTemp = (string)(bMatchDirNames ? "*" : " ") + (string)" Match Directory Names";
  pcOptMenu->AddItem( zTemp.c_str( ), new Message( EV_MENU_MATCH_DIR ) );

  zTemp = (string)(bFollowSubDirs ? "*" : " ") + (string)" Follow Subdirectories";
  pcOptMenu->AddItem( zTemp.c_str( ), new Message( EV_MENU_FOLLOW_DIR ) );

  zTemp = (string)(bUseRegex ? "*" : " ") + (string)" Use Regular Expressions";
  pcOptMenu->AddItem( zTemp.c_str( ), new Message( EV_MENU_REGEX ) );

  zTemp = (string)(bSortResults ? "*" : " ") + (string)" Sort Results";
  pcOptMenu->AddItem( zTemp.c_str( ), new Message( EV_MENU_SORT ) );

  pcMenu->InvalidateLayout( );
  pcMenu->SetTargetForItems( this );

}  


/***********************************************
**
** Display the About alert
**
***********************************************/
void LocatorWindow::DisplayAbout( )
{

  string zTitle = "About " + (string)APP_NAME;
  string zMessage = (string)APP_NAME + (string)"\n\nFile Find Utility For AtheOS\nby Andrew Kennan, 2002\n\nhttp://www.stresscow.org/bewilder/atheos\n\nThis program is licenced under the GNU GPL\nFor more information\nsee the file /Applications/Locator/COPYING.\n\n";

  Alert *pcAboutAlert = new Alert( zTitle.c_str( ), zMessage.c_str( ), 0, "OK", NULL );
  pcAboutAlert->Go( new Invoker( new Message(), this ) );

}




//**********************************************************************************************

LocatorPrefWindow::LocatorPrefWindow( Window *pcParent, Message *pcPrefs ) : Window( Rect(), "locator_prefs", "Preferences" )
{
	
	m_pcParent = pcParent;
	
	string zTitle = "Locator Preferences";
	SetTitle( zTitle.c_str( ) );
	
	LayoutView *pcPrefsView;
	VLayoutNode  *pcRoot;
	FrameView      *pcFrame;
	VLayoutNode      *pcInner;
	//Checkbox         *m_pcCBFollowDirs
	//Checkbox         *m_pcCBMatchDirs
	//Checkbox         *m_pcCBUseRegex
	//Checkbox         *m_pcCBSortResults
	HLayoutNode    *pcBottom;
	ImageButton           *pcBtnOk;
	ImageButton           *pcBtnCancel;
	
	m_pcCBFollowDirs = new CheckBox( Rect(), "cb_followdirs", "Follow Sub-Directories", NULL );
	m_pcCBMatchDirs = new CheckBox( Rect(), "cb_followdirs", "Match Directory Names", NULL );
	m_pcCBUseRegex = new CheckBox( Rect(), "cb_followdirs", "Match Using Regexes", NULL );
	m_pcCBSortResults = new CheckBox( Rect(), "cb_followdirs", "Sort Results", NULL );
	
    bool bFollowSubDirs;
    bool bMatchDirNames;
    bool bUseRegex;
    bool bSortResults;

	pcPrefs->FindBool( "MatchDirNames", &bMatchDirNames );
	pcPrefs->FindBool( "SortResults", &bSortResults );
	pcPrefs->FindBool( "FollowSubDirs", &bFollowSubDirs );
	pcPrefs->FindBool( "UseRegex", &bUseRegex );

	m_pcCBFollowDirs->SetValue( bFollowSubDirs );
	m_pcCBMatchDirs->SetValue( bMatchDirNames );
	m_pcCBUseRegex->SetValue( bUseRegex );	
	m_pcCBSortResults->SetValue( bSortResults );
	
	pcRoot = new VLayoutNode( "root" );
	pcInner = new VLayoutNode( "inner" );
//	pcInnerBottom = new HLayoutNode( "innerbottom" );
	pcBottom = new HLayoutNode( "bottom" );
	
	pcInner->AddChild( m_pcCBFollowDirs );
	pcInner->AddChild( m_pcCBMatchDirs );
	pcInner->AddChild( m_pcCBUseRegex );
	pcInner->AddChild( m_pcCBSortResults );
	pcInner->SameWidth( "MatchDirNames", "SortResults", "FollowSubDirs", "UseRegex", NULL );
	pcInner->AddChild( new HLayoutSpacer( "spacer_4", 100 ) );
	    
    pcBtnOk = new ImageButton( Rect(), "btn_ok", "OK", new Message( EV_PREF_BTN_OK ) );
    string zIconPath = app_path() + "icons/choice-yes.png";
    pcBtnOk->SetImageFromFile( zIconPath.c_str() );
    pcBtnCancel = new ImageButton( Rect(), "btn_cancel", "Cancel", new Message( EV_PREF_BTN_CANCEL ) );
    zIconPath = app_path() + "icons/choice-no.png";    
    pcBtnCancel->SetImageFromFile( zIconPath.c_str() );    
    
	pcBottom->AddChild( pcBtnCancel );
	pcBottom->AddChild( new HLayoutSpacer( "spacer_1", 100 ) );
	pcBottom->AddChild( new VLayoutSpacer( "spacer_2", 1.0f ) );
	pcBottom->AddChild( pcBtnOk );
	
	pcFrame = new FrameView( Rect(), "frame_1", "" );
	pcFrame->SetRoot( pcInner );
	
	pcRoot->AddChild( pcFrame );
	pcRoot->AddChild( pcBottom );
	
	Point cSize = pcRoot->GetPreferredSize( false );
	
	pcPrefsView = new LayoutView( Rect( 2, 2, cSize.x + 34, cSize.y + 34 ), "prefs_layout" );
	pcPrefsView->SetRoot( pcRoot );

	SetFrame( center_frame( Rect( 0, 0, cSize.x + 36, cSize.y + 36 ) ) );
	
	AddChild( pcPrefsView );
	
	SetDefaultButton( pcBtnOk );
	SetFocusChild( pcBtnOk );
	
}


LocatorPrefWindow::~LocatorPrefWindow()
{

	Invoker *pcParentInvoker = new Invoker( new Message( EV_PREF_WINDOW_CLOSED ), m_pcParent );
	pcParentInvoker->Invoke();

	
}

void LocatorPrefWindow::HandleMessage( Message *pcMessage )
{
	int32 nCode = pcMessage->GetCode();
	
	switch( nCode )
	{
		case EV_PREF_BTN_OK:
			pcMessage->AddBool( "FollowSubDirs", m_pcCBFollowDirs->GetValue() );
			pcMessage->AddBool( "UseRegex", m_pcCBUseRegex->GetValue() );
			pcMessage->AddBool( "MatchDirNames", m_pcCBMatchDirs->GetValue() );
			pcMessage->AddBool( "SortResults", m_pcCBSortResults->GetValue() );
			break;
			
		case EV_PREF_BTN_CANCEL:
			break;
			
		default:
			break;
			
	}
	
	Invoker *pcParentInvoker = new Invoker( pcMessage, m_pcParent );
	pcParentInvoker->Invoke();

}

