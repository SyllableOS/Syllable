/*  Archiver 0.5.0 Reborn -:-  (C)opyright 2002-2003 Rick Caudill
 
  This is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "archiver.h"
#include "settings.h"

using namespace os;

AListView::AListView( const Rect & r ):ListView( r, "arc_listview", ListView::F_MULTI_SELECT, CF_FOLLOW_ALL )
{
	InsertColumn( "Name", ( GetBounds().Width(  ) / 5 ) + 51 );
	InsertColumn( "Owner", ( GetBounds().Width(  ) / 5 ) - 15 );
	InsertColumn( "Size", GetBounds().Width(  ) / 5 - 20 );
	InsertColumn( "Date", GetBounds().Width(  ) / 5 + 10 );
	InsertColumn( "Perm", ( GetBounds().Width(  ) / 5 ) - 9 );
	SetAutoSort( false );
	SetInvokeMsg( new Message( ID_CHANGE_LIST ) );
}

ArcWindow::ArcWindow(AppSettings* pcSettings):Window( Rect( 0, 0, 525, 280 ), "main_wnd", "Archiver 0.5 Reborn" )
{
	m_pcNewWindow = NULL;
	m_pcExtractWindow = NULL;
	m_pcExeWindow = NULL;
	pcOpenRequest = NULL;
	m_pcOptions = NULL;
	pcOpenRequest = NULL;
	pcAppSettings = pcSettings;

	SetupMenus();
	SetupToolBar();
	SetupStatusBar();

	Rect r = GetBounds();

	r.top = m_pcMenu->GetBounds().bottom + pcButtonBar->GetBounds(  ).bottom + 2;
	r.bottom = 256;
	pcView = new AListView( r );
	AddChild( pcView );

	SetIcon(LoadImageFromResource("icon24x24.png")->LockBitmap());
}


ArcWindow::~ArcWindow()
{
}

void ArcWindow::SetupMenus()
{
	cMenuFrame = GetBounds();
	Rect cMainFrame = cMenuFrame;

	m_pcMenu = new Menu( cMenuFrame, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );

	aMenu = new Menu( Rect( 0, 0, 0, 0 ), "_Application", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	aMenu->AddItem( new MenuItem( "Settings...", new Message( ID_APP_SET ), "Ctrl+S", LoadImageFromResource( "settings-16.png" ) ) );
	aMenu->AddItem( new MenuSeparator() );
	aMenu->AddItem( new MenuItem( "Exit", new Message( ID_QUIT ), "Ctrl+Q", LoadImageFromResource( "close-16.png" ) ) );
	m_pcMenu->AddItem( aMenu );

	fMenu = new Menu( Rect( 0, 0, 1, 1 ), "_File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	fMenu->AddItem( new MenuItem( "Open...", new Message( ID_OPEN ), "Ctrl+O", LoadImageFromResource( "open-16.png" ) ) );
	fMenu->AddItem( new MenuSeparator() );
	fMenu->AddItem( new MenuItem( "New...", new Message( ID_NEW ), "Ctrl+N", LoadImageFromResource( "new-16.png" ) ) );
	m_pcMenu->AddItem( fMenu );

	actMenu = new Menu( Rect( 0, 0, 1, 4 ), "A_ctions", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	actMenu->AddItem( new MenuItem( "Extract...", new Message( ID_EXTRACT ), "Ctrl+E", LoadImageFromResource( "extract-16.png" ) ) );
	actMenu->AddItem( new MenuSeparator() );
	actMenu->AddItem( new MenuItem( "MakeSelf...", new Message( ID_EXE ), "Ctrl+M", LoadImageFromResource( "exe-16.png" ) ) );
	m_pcMenu->AddItem( actMenu );

	hMenu = new Menu( Rect( 0, 0, 1, 2 ), "_Help", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	hMenu->AddItem( new MenuItem( "About...", new Message( ID_HELP_ABOUT ), "Ctrl+A", LoadImageFromResource( "about-16.png" ) ) );
	m_pcMenu->AddItem( hMenu );

	cMenuFrame.bottom = m_pcMenu->GetPreferredSize( true ).y;
	m_pcMenu->SetFrame( cMenuFrame );
	m_pcMenu->SetTargetForItems( this );
	AddChild( m_pcMenu );
}


void ArcWindow::SetupToolBar()
{

	pcButtonBar = new ToolBar( Rect(), "" );
	HLayoutNode *pcNode = new HLayoutNode( "" );
	pcNode->SetHAlignment( ALIGN_LEFT );

	ImageButton *pcOpen = new ImageButton( Rect(), "imagebut1", "  Open", new Message( ID_OPEN ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	pcOpen->SetImage( LoadImageFromResource( "open.png" ) );
	pcNode->AddChild( pcOpen );
	pcNode->AddChild( new HLayoutSpacer( "", 2, 2 ) );	// Looks better when resizing if they're fixed width... :o)

	ImageButton *pcNew = new ImageButton( Rect(), "imagebut2", "   New", new Message( ID_NEW ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	pcNew->SetImage( LoadImageFromResource( "new.png" ) );
	pcNode->AddChild( pcNew );
	pcNode->AddChild( new HLayoutSpacer( "", 2, 2 ) );	// Looks better when resizing if they're fixed width... :o)

	ImageButton *pcExtract = new ImageButton( Rect(), "imagebut3", "Extract", new Message( ID_EXTRACT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	pcExtract->SetImage( LoadImageFromResource( "extract.png" ) );
	pcNode->AddChild( pcExtract );
	pcNode->AddChild( new HLayoutSpacer( "", 2, 2 ) );	// Looks better when resizing if they're fixed width... :o)

	ImageButton *pcExe = new ImageButton( Rect(), "imagebut4", "MakeSelf", new Message( ID_EXE ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	pcExe->SetImage( LoadImageFromResource( "exe.png" ) );
	pcNode->AddChild( pcExe );
	pcNode->AddChild( new HLayoutSpacer( "", 2, 2 ) );	// Looks better when resizing if they're fixed width... :o)

	ImageButton *pcSettingsBut = new ImageButton( Rect(), "imagebut5", "Settings", new Message( ID_APP_SET ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	pcSettingsBut->SetImage( LoadImageFromResource( "settings.png" ) );
	pcNode->AddChild( pcSettingsBut );
	pcNode->AddChild( new HLayoutSpacer( "", 2, 2 ) );	// Looks better when resizing if they're fixed width... :o)

	ImageButton *pcAboutBut = new ImageButton( Rect(), "imagebut6", "About", new Message( ID_HELP_ABOUT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	pcAboutBut->SetImage( LoadImageFromResource( "about.png" ) );
	pcNode->AddChild( pcAboutBut );
	pcNode->AddChild( new HLayoutSpacer( "", 0, COORD_MAX, NULL, 100.0f ) );

	pcButtonBar->SetRoot( pcNode );
	pcButtonBar->SetFrame( Rect( 0, m_pcMenu->GetFrame().bottom + 1, GetBounds(  ).Width(  ), m_pcMenu->GetFrame(  ).bottom + pcButtonBar->GetPreferredSize( false ).y + 4 ) );
	pcNode->SameWidth( "imagebut4", "imagebut2", "imagebut3", "imagebut1", "imagebut5", "imagebut6", NULL );
	AddChild( pcButtonBar );
}


void ArcWindow::LoadAtStart( char *cFileName )
{
	strcpy( pzStorePath, cFileName );
	ListFiles( cFileName );
}


void ArcWindow::Load( char *cFileName )
{
	pcOpenRequest->Close();
	MakeFocus( pcView );
	strcpy( pzStorePath, cFileName );
	ListFiles( cFileName );
}


void ArcWindow::SetupStatusBar()
{
	pcStatus = new StatusBar( Rect( 0, GetBounds().Height(  ) - 24, GetBounds(  ).Width(  ), GetBounds(  ).Height(  ) - ( -1 ) ), "status_bar", 1 );
	pcStatus->ConfigurePanel( 0, StatusBar::FILL, GetBounds().Width(  ) );
	pcStatus->SetText( "No archives open!", 0, 80 );
	AddChild( pcStatus );
}


void ArcWindow::HandleMessage( Message * pcMessage )
{
	Alert *m_pcError = new Alert( "Error", "Only one Window can \nbe opened at a time.", Alert::ALERT_WARNING, 0x00, "OK", NULL );
	switch ( pcMessage->GetCode() )
	{
		case ID_NEW:
		{
			if( m_pcNewWindow == NULL )
			{
				m_pcNewWindow = new NewWindow(this, pcAppSettings->GetCloseNewWindow());
				m_pcNewWindow->CenterInWindow( this );
				m_pcNewWindow->Show();
				m_pcNewWindow->MakeFocus();
			}
			else
			{
				m_pcError->CenterInWindow(this);
				m_pcError->Go( new Invoker() );		
			}	
			break;
		}

		case ID_NEW_CLOSE:
		{
			m_pcNewWindow = NULL;
			break;
		}

		case ID_EXE:
		{
			if( m_pcExeWindow == NULL )
			{
				m_pcExeWindow = new ExeWindow( this );
				m_pcExeWindow->CenterInWindow( this );
				m_pcExeWindow->Show();
				m_pcExeWindow->MakeFocus();
			}
			else
			{
				m_pcError->CenterInWindow(this);
				m_pcError->Go( new Invoker() );
			}
			break;
		}

		case ID_EXE_CLOSE:
		{
			m_pcExeWindow = NULL;
			break;
		}

		case ID_APP_SET:
		{
			if( m_pcOptions == NULL )
			{
				m_pcOptions = new OptionsWindow(this, pcAppSettings);
				m_pcOptions->CenterInWindow( this );
				m_pcOptions->Show();
				m_pcOptions->MakeFocus();
			}
			else
			{
				m_pcError->CenterInWindow(this);
				m_pcError->Go( new Invoker() );
			}	
			break;
		}

		case ID_OPT_CLOSE_OPT:
		{
			m_pcOptions = NULL;
			break;
		}

		case ID_HELP_ABOUT:
		{
			Alert *aAbout = new Alert( "About Archiver 0.5 Reborn", "Archiver 0.5 Reborn\n\nGui Archiver for Syllable    \nCopyright 2002 - 2003 Rick Caudill\n\nArchiver is released under the GNU General\nPublic License. Please go to www.gnu.org\nmore information.\n", Alert::ALERT_INFO, 0x00, "OK", NULL );
			aAbout->CenterInWindow( this );
			aAbout->Go( new Invoker() );
			break;
		}

		case ID_GOING_TO_VIEW:
		{
			const char *FileName;
			pcMessage->FindString( "newPath", &FileName );
			LoadAtStart( ( char * )FileName );
			break;
		}

		case ID_EXTRACT:
		{
			if( m_pcExtractWindow == NULL )
			{
				if( Lst != false )
				{
					m_pcExtractWindow = new ExtractWindow( this,pcAppSettings->GetExtractPath(), String(pzStorePath));
					m_pcExtractWindow->CenterInWindow( this );
					m_pcExtractWindow->Show();
					m_pcExtractWindow->MakeFocus();
				}
				else
				{
					Alert *aFile = new Alert( "Error", "You must select a file first.", Alert::ALERT_WARNING, 0x00, "OK", NULL );
					aFile->CenterInWindow( this );
					aFile->Go( new Invoker() );
				}
			}
			else
			{
				m_pcError->CenterInWindow(this);
				m_pcError->Go( new Invoker() );
			}
			break;
		}

		case ID_EXTRACT_CLOSE:
		{
			m_pcExtractWindow = NULL;
			break;
		}
	
		case ID_OPEN:
		{
			pcOpenRequest = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ),pcAppSettings->GetOpenPath().c_str(), FileRequester::NODE_FILE, false, NULL, NULL, true, true, "Open", "Cancel" );
			pcOpenRequest->CenterInWindow( this );
			pcOpenRequest->Show();
			pcOpenRequest->MakeFocus();
			break;
		}

		case M_LOAD_REQUESTED:
		{
			if( pcMessage->FindString( "file/path", &pzFPath ) == 0 )
			{
				strcpy( pzStorePath, pzFPath );
				Load( ( char * )pzStorePath );
				pcOpenRequest = NULL;
				break;
			}
		}


		case ID_QUIT:
		{
			OkToQuit();
			break;
		}

		default:
			Window::HandleMessage( pcMessage );
			break;
	}
}


bool ArcWindow::OkToQuit( void )
{
	pcAppSettings->SaveSettings();
	delete pcAppSettings;
	Application::GetInstance()->PostMessage( M_QUIT );
	return ( false );
}



void ArcWindow::ListFiles( char *cFileName )
{
	FILE *f;
	FILE *fTmp;
	char zTmp[1024];
	char output_buffer[1024];
	int i;
	int j;

	if( strstr( pzStorePath, ".ace" ) )
	{

		char ace_n_args[300] = "";

		strcpy( ace_n_args, "" );
		strcat( ace_n_args, "unace l " );
		strcat( ace_n_args, pzStorePath );
		strcat( ace_n_args, " 2>&1" );
		f = NULL;
		fTmp = NULL;
		f = popen( ace_n_args, "r" );
		fTmp = popen( ace_n_args, "r" );

		for( i = 0; i <= 3; i++ )
			fgets( output_buffer, 1024, fTmp );

		char trash2[1024] = "Invalid archive file: ";

		strcat( trash2, pzStorePath );
		strcat( trash2, "\n" );

		if( strcmp( output_buffer, trash2 ) == 0 )
		{
			sprintf( zTmp, "%s is not an ace file.", pzStorePath );
			pcStatus->SetText( zTmp, 0 );
			pclose( f );
			pclose( fTmp );
			return;
		}

		else
		{
			pcView->Clear();
			j = 0;
			ListViewStringRow *row;

			while( !feof( f ) )
			{

				fscanf( f, "%s %s %s %s %s %s ", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5] );

				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[5] );
				row->AppendString( "      ---   " );
				row->AppendString( ( String )( const char * )tmp[3] );
				row->AppendString( ( String )( const char * )tmp[1] + "  " + ( const char * )tmp[0] );
				row->AppendString( "      ---   " );
				pcView->InsertRow( row );

				j++;

			}
			pclose( f );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}

	if( strstr( pzStorePath, ".jar" ) )
	{

		char jar_n_args[300] = "";

		strcpy( jar_n_args, "" );
		strcat( jar_n_args, "fastjar tfv " );
		strcat( jar_n_args, pzStorePath );
		strcat( jar_n_args, " 2>&1" );
		f = NULL;
		fTmp = NULL;
		fTmp = popen( jar_n_args, "r" );
		f = popen( jar_n_args, "r" );
		pcView->Clear();
		j = 0;
		fgets( output_buffer, 1024, fTmp );

		if( strcmp( output_buffer, "Error in JAR file format. zip-style comment?\n" ) == 0 )
		{
			sprintf( zTmp, "%s is not a jar archive", pzStorePath );
			pclose( fTmp );
			pclose( f );
			pcStatus->SetText( zTmp, 0 );
			return;
		}

		else
		{
			pclose( fTmp );
			ListViewStringRow *row;

			while( !feof( f ) )
			{

				fscanf( f, "%s %s %s %s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7] );
				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[7] );
				row->AppendString( "      ---   " );
				row->AppendString( ( String )( const char * )tmp[0] );
				row->AppendString( ( String )( const char * )tmp[4] + ",  " + ( const char * )tmp[2] + " " + ( const char * )tmp[3] + ", " + ( const char * )tmp[6] );
				row->AppendString( "      ---   " );
				pcView->InsertRow( row );

				j++;

			}

			pcView->RemoveRow( j - 1 );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			pclose( f );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}


	if( strstr( pzStorePath, ".cab" ) )
	{

		char cab_n_args[300] = "";

		strcpy( cab_n_args, "" );
		strcat( cab_n_args, "uncab -v " );
		strcat( cab_n_args, pzStorePath );
		strcat( cab_n_args, " 2>&1" );
		f = NULL;
		fTmp = NULL;
		f = popen( cab_n_args, "r" );
		fTmp = popen( cab_n_args, "r" );
		pcView->Clear();

		fgets( trash, 1024, fTmp );

		if( strcmp( trash, "\0\n" ) == 0 )
		{
			sprintf( zTmp, "%s is not a cab archive", pzStorePath );
			pclose( fTmp );
			pclose( f );
			pcStatus->SetText( zTmp, 0 );
			return;
		}

		else
		{
			pclose( fTmp );
			for( int i = 0; i <= 1; i++ )
			{
				fgets( trash, 1023, f );
			}
			j = 0;
			ListViewStringRow *row;

			while( !feof( f ) )
			{

				fscanf( f, "%s %s %s %s %s %s ", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5] );


				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[5] );
				row->AppendString( "      ---   " );
				row->AppendString( ( String )( const char * )tmp[0] );
				row->AppendString( ( String )( const char * )tmp[3] + "  " + ( const char * )tmp[2] );
				row->AppendString( "      ---   " );
				pcView->InsertRow( row );

				j++;

			}
			pclose( f );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}

	if( strstr( pzStorePath, ".zip" ) )
	{
		char zip_n_args[300] = "";

		strcpy( zip_n_args, "" );
		strcat( zip_n_args, "unzip -Z " );
		strcat( zip_n_args, pzStorePath );
		f = NULL;
		f = popen( zip_n_args, "r" );
		pcView->Clear();
		j = 0;
		char junk2[1024];

		fgets( junk2, sizeof( junk2 ), f );
		fread( junk2, sizeof( junk2 ), 0, f );
		sprintf( output_buffer, "[%s]\n", pzStorePath );
		if( strcmp( junk2, output_buffer ) == 0 )
		{
			sprintf( zTmp, "%s is not a zip file.", pzStorePath );
			pcStatus->SetText( zTmp, 0 );
			pclose( f );
			return;
		}

		else
		{
			while( !feof( f ) )
			{

				fscanf( f, "%s %s %s %s %s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7], tmp[8] );
				ListViewStringRow *row;

				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[8] );
				row->AppendString( ( String )"" );
				row->AppendString( ( String )( const char * )tmp[3] );
				row->AppendString( ( String )( const char * )tmp[7] + "  " + ( const char * )tmp[6] );
				row->AppendString( ( String )( const char * )tmp[0] );
				pcView->InsertRow( row );

				j++;
			}

			pcView->RemoveRow( j - 1 );
			pcView->RemoveRow( j - 2 );
			pclose( f );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}


	if( ( strstr( pzStorePath, ".tar" ) ) && ( !strstr( pzStorePath, ".gz" ) ) && ( !strstr( pzStorePath, ".bz2" ) ) )
	{

		char tar_n_args[300] = "";

		strcpy( tar_n_args, "" );
		strcat( tar_n_args, "tar -tvf " );
		strcat( tar_n_args, pzStorePath );
		strcat( tar_n_args, " 2>&1" );
		f = NULL;
		f = popen( tar_n_args, "r" );
		fTmp = popen( tar_n_args, "r" );
		pcView->Clear();
		j = 0;


		fgets( output_buffer, sizeof( output_buffer ), fTmp );

		if( strcmp( output_buffer, "tar: Hmm, this doesn't look like a tar archive\n" ) == 0 )
		{
			sprintf( zTmp, "%s is not a tar archive", pzStorePath );
			pclose( fTmp );
			pclose( f );
			pcStatus->SetText( zTmp, 0 );
			return;
		}

		else
		{
			pclose( fTmp );

			while( !feof( f ) )
			{

				fscanf( f, "%s %s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5] );
				ListViewStringRow *row;

				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[5] );
				row->AppendString( ( String )( const char * )tmp[1] );
				row->AppendString( ( String )( const char * )tmp[2] );
				row->AppendString( ( String )( const char * )tmp[4] + "  " + ( const char * )tmp[3] );
				row->AppendString( ( String )( const char * )tmp[0] );
				pcView->InsertRow( row );
				j++;
			}
			pcView->RemoveRow( j - 1 );
			pclose( f );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}


	if( strstr( pzStorePath, ".bz2" ) )
	{

		char bzip_n_args[300] = "";

		strcpy( bzip_n_args, "" );
		strcat( bzip_n_args, "tar --use-compress-prog=bzip2 -tvf " );
		strcat( bzip_n_args, pzStorePath );
		strcat( bzip_n_args, " 2>&1" );
		f = NULL;
		fTmp = NULL;
		f = popen( bzip_n_args, "r" );
		fTmp = popen( bzip_n_args, "r" );
		fgets( output_buffer, 1024, fTmp );
		pcView->Clear();
		j = 0;

		if( strcmp( output_buffer, "bzip2: (stdin) is not a bzip2 file.\n" ) == 0 )
		{
			sprintf( zTmp, "file '%s' is not a bzip file", pzStorePath );
			pcStatus->SetText( zTmp, 0 );
			pclose( fTmp );
			pclose( f );
			return;
		}

		else
		{
			pclose( fTmp );
			while( !feof( f ) )
			{

				fscanf( f, "%s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4] );
				fgets( tmp[5], 1024, f );
				tmp[5][strlen( tmp[5] ) - 1] = '\0';

				ListViewStringRow *row;

				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[5] );
				row->AppendString( ( String )( const char * )tmp[1] );
				row->AppendString( ( String )( const char * )tmp[2] );
				row->AppendString( ( String )( const char * )tmp[4] + "  " + ( const char * )tmp[3] );
				row->AppendString( ( String )( const char * )tmp[0] );
				pcView->InsertRow( row );

				j++;
			}
			pcView->RemoveRow( j - 1 );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			pclose( f );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}

	if( strstr( pzStorePath, ".lha" ) )
	{

		char lha_n_args[300] = "";

		strcpy( lha_n_args, "" );
		strcat( lha_n_args, "lha -l " );
		strcat( lha_n_args, pzStorePath );
		f = NULL;
		f = popen( lha_n_args, "r" );
		pcView->Clear();
		fgets( trash, 1023, f );
		fgets( trash, 1023, f );
		j = 0;
		while( !feof( f ) )
		{

			fscanf( f, "%s %s %s %s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7] );

			ListViewStringRow *row;

			row = new ListViewStringRow();
			row->AppendString( ( String )( const char * )tmp[6] );
			row->AppendString( "      ---   " );
			row->AppendString( ( String )( const char * )tmp[1] );
			row->AppendString( ( String )( const char * )tmp[3] + " " + ( const char * )tmp[4] + " " + ( const char * )tmp[5] );
			row->AppendString( "      ---   " );
			pcView->InsertRow( row );

			j++;
		}
		pcView->RemoveRow( j - 1 );
		pcView->RemoveRow( j - 2 );
		pclose( f );
		Lst = true;
		ArcWindow::MakeFocus();
		return;
	}


	if( strstr( pzStorePath, ".gz" ) )
	{

		char gzip_n_args[300] = "";

		strcpy( gzip_n_args, "" );
		strcat( gzip_n_args, "tar -ztvf " );
		strcat( gzip_n_args, pzStorePath );
		strcat( gzip_n_args, " 2>&1" );
		f = NULL;
		fTmp = NULL;
		fTmp = popen( gzip_n_args, "r" );
		f = popen( gzip_n_args, "r" );
		pcView->Clear();
		j = 0;
		for( i = 0; i <= 1; i++ )
		{
			fgets( output_buffer, 1024, fTmp );
		}

		if( strcmp( output_buffer, "gzip: stdin: unexpected end of file\n" ) == 0 )
		{
			sprintf( zTmp, "file '%s' is not a gzip file", pzStorePath );
			pcStatus->SetText( zTmp, 0 );
			pclose( fTmp );
			pclose( f );
			return;
		}

		else
		{
			pclose( fTmp );
			while( !feof( f ) )
			{
				fscanf( f, "%s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4] );
				fgets( tmp[5], 1024, f );
				tmp[5][strlen( tmp[5] ) - 1] = '\0';
				ListViewStringRow *row;

				row = new ListViewStringRow();
				row->AppendString( ( String )( const char * )tmp[5] );	//tmp[6] );
				row->AppendString( ( String )( const char * )tmp[1] );
				row->AppendString( ( String )( const char * )tmp[2] );
				row->AppendString( ( String )( const char * )tmp[4] + "  " + ( const char * )tmp[3] );
				row->AppendString( ( String )( const char * )tmp[0] );
				pcView->InsertRow( row );
				j++;
			}
			pcView->RemoveRow( j - 1 );
			pclose( f );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}
	}


	if( strstr( pzStorePath, ".tgz" ) )
	{
		char gzip_n_args[300] = "";

		strcpy( gzip_n_args, "" );
		strcat( gzip_n_args, "tar -ztvf " );
		strcat( gzip_n_args, pzStorePath );
		strcat( gzip_n_args, " 2>&1" );
		f = NULL;
		fTmp = NULL;
		fTmp = popen( gzip_n_args, "r" );
		f = popen( gzip_n_args, "r" );
		pcView->Clear();
		j = 0;
		for( i = 0; i <= 1; i++ )
		{
			fgets( output_buffer, 1024, fTmp );
		}

		if( strcmp( output_buffer, "gzip: stdin: unexpected end of file\n" ) == 0 )
		{
			sprintf( zTmp, "file '%s' is not a gzip file", pzStorePath );
			pcStatus->SetText( zTmp, 0 );
			pclose( fTmp );
			pclose( f );
			return;
		}

		else
		{
			String zStatusStuff;

			pclose( fTmp );
			while( !feof( f ) )
			{
				fscanf( f, "%s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4] );
				fgets( tmp[5], 1024, f );
				tmp[5][strlen( tmp[5] ) - 1] = '\0';
				ListViewStringRow *row;

				row = new ListViewStringRow();

				row->AppendString( ( String )( const char * )tmp[5] );
				row->AppendString( ( String )( const char * )tmp[1] );
				row->AppendString( ( String )( const char * )tmp[2] );
				row->AppendString( ( String )( const char * )tmp[4] + "  " + ( const char * )tmp[3] );
				row->AppendString( ( String )( const char * )tmp[0] );
				pcView->InsertRow( row );
				j++;
			}
			pcView->RemoveRow( j - 1 );
			pclose( f );
			sprintf( zTmp, "%s has %d files located in it", pzStorePath, pcView->GetRowCount() );
			pcStatus->SetText( zTmp, 0 );
			Lst = true;
			ArcWindow::MakeFocus();
			return;
		}

	}



	if( strstr( pzStorePath, ".rar" ) )
	{

		char rar_n_args[300] = "";

		strcpy( rar_n_args, "" );
		strcat( rar_n_args, "unrar l " );
		strcat( rar_n_args, pzStorePath );
		f = NULL;
		f = popen( rar_n_args, "r" );
		pcView->Clear();

		for( int i = 0; i <= 7; i++ )
		{
			fgets( trash, 1023, f );
		}
		j = 0;
		while( !feof( f ) )
		{

			fscanf( f, "%s %s %s %s %s %s %s %s %s %s", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7], tmp[8], tmp[9] );

			ListViewStringRow *row;

			row = new ListViewStringRow();

			row->AppendString( ( String )( const char * )tmp[0] );
			row->AppendString( ( String )( const char * )tmp[1] );
			row->AppendString( "    ---  " );
			row->AppendString( ( String )( const char * )tmp[5] + "  " + ( const char * )tmp[4] );
			row->AppendString( "   ---  " );
			pcView->InsertRow( row );

			j++;
		}
		pcView->RemoveRow( j - 1 );
		pclose( f );
		Lst = true;
		ArcWindow::MakeFocus();
		return;
	}

	else
	{
		sprintf( zTmp, "%s is not an archive", pzStorePath );
		pcStatus->SetText( zTmp );
		pcView->Clear();
		Lst = false;
	}
}


/*************************************************************
* Description: Using this method to catch input
*			   This method will be superceded when menu shortcuts 
*			   are enabled.
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void ArcWindow::DispatchMessage( os::Message * pcMessage, os::Handler * pcHandler )
{
	String cRawString;
	int32 nQual, nRawKey;

	if( pcMessage->GetCode() == os::M_KEY_DOWN )	//a key must have been pressed
	{
		if( pcMessage->FindString( "_raw_string", &cRawString ) != 0 || pcMessage->FindInt32( "_qualifiers", &nQual ) != 0 || pcMessage->FindInt32( "_raw_key", &nRawKey ) != 0 )
			Window::DispatchMessage( pcMessage, pcHandler );
		else
		{
			if( nQual & os::QUAL_CTRL )	//ctrl key was caught
			{
				m_pcMenu->MakeFocus();	//simple hack in order to get menus to work 
			}

			else if( nQual & os::QUAL_ALT )
			{
				if( nRawKey == 60 )	//ALT+A
				{
					Point cPoint = m_pcMenu->GetItemAt( 0 )->GetContentLocation();

					m_pcMenu->MouseDown( cPoint, 1 );
				}

				if( nRawKey == 63 )	//ALT+F
				{
					Point cPoint = m_pcMenu->GetItemAt( 1 )->GetContentLocation();

					m_pcMenu->MouseDown( cPoint, 1 );
				}

				if( nRawKey == 78 )	//ALT+C
				{
					Point cPoint = m_pcMenu->GetItemAt( 2 )->GetContentLocation();

					m_pcMenu->MouseDown( cPoint, 1 );
				}

				if( nRawKey == 65 )	//ALT+H
				{
					Point cPoint = m_pcMenu->GetItemAt( 6 )->GetContentLocation();

					m_pcMenu->MouseDown( cPoint, 1 );
				}
				else;
			}
		}
	}
	Window::DispatchMessage( pcMessage, pcHandler );
}
