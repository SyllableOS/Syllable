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
using namespace os;
AListView::AListView(const Rect & r) : ListView(r,"arc_listview",ListView::F_MULTI_SELECT,CF_FOLLOW_ALL)
{
    InsertColumn( "Name", ( GetBounds().Width() / 5 ) + 51 );
    InsertColumn( "Owner", ( GetBounds().Width() / 5 ) - 15 );
    InsertColumn( "Size", GetBounds().Width() / 5 - 20 );
    InsertColumn( "Date", GetBounds().Width() / 5 + 10);
    InsertColumn( "Perm", ( GetBounds().Width() / 5 )  - 9 );
    SetAutoSort( false );
    SetInvokeMsg(new Message(ID_CHANGE_LIST));

}





ArcWindow::ArcWindow() : Window( Rect( 0, 0, 525, 280 ), "main_wnd", "Archiver 0.5 Reborn" )
{
    m_pcNewWindow = NULL;
    m_pcExtractWindow = NULL;
    m_pcExeWindow = NULL;
    pcOpenRequest = NULL;
    m_pcOptions = NULL;
    pcOpenRequest = NULL;
    
    sprintf( zConfigFile, "%s/config/Archiver/Archiver.cfg", getenv( "HOME" ) );
    Read();
    
    Rect r = GetBounds();
    r.top = 79;
    r.bottom=256;
    pcView = new AListView(r);
    AddChild(pcView);
    
    SetupMenus();
    SetupToolBar();
    SetupStatusBar();
    
    

    
    MoveTo( dDesktop.GetResolution().x / 2 - GetBounds().Width() / 2, dDesktop.GetResolution().y / 2 - GetBounds().Height() / 2 ); //moves the main window to the center of the desktop
}


ArcWindow::~ArcWindow()
{
}

void ArcWindow::SetupMenus()
{
    cMenuFrame = GetBounds();
    Rect cMainFrame = cMenuFrame;
    cMenuFrame.bottom = 18;
    m_pcMenu = new Menu( cMenuFrame, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );

    aMenu = new Menu( Rect( 0, 0, 0, 0 ), "Application", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    aMenu->AddItem( new ImageItem("Settings...", new Message( ID_APP_SET ), "Ctrl + S", LoadBitmapFromResource("settings-16.png")) ) ;
    aMenu->AddItem( new MenuSeparator() );
    aMenu->AddItem( new ImageItem("Exit", new Message( ID_QUIT ), "Ctrl + Q", LoadBitmapFromResource("close-16.png")) );
    m_pcMenu->AddItem( aMenu );

    fMenu = new Menu( Rect( 0, 0, 1, 1 ), "File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    fMenu->AddItem( new ImageItem("Open...", new Message( ID_OPEN ),"Ctrl + O",LoadBitmapFromResource("open-16.png")) );
    fMenu->AddItem( new MenuSeparator() );
    fMenu->AddItem( new ImageItem("New...", new Message( ID_NEW ),"Ctrl + N",LoadBitmapFromResource("new-16.png")) );
    m_pcMenu->AddItem( fMenu );

    actMenu = new Menu( Rect( 0, 0, 1, 4 ), "Actions", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    actMenu->AddItem( new ImageItem("Extract...", new Message( ID_EXTRACT ),"Ctrl + E", LoadBitmapFromResource("extract-16.png")) );
    actMenu->AddItem( new MenuSeparator() );
    actMenu->AddItem( new ImageItem("MakeSelf...", new Message( ID_EXE ),"Ctrl + M",LoadBitmapFromResource("exe-16.png")) );
    m_pcMenu->AddItem( actMenu );

    hMenu = new Menu( Rect( 0, 0, 1, 2 ), "Help", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

    

    hMenu->AddItem( new ImageItem("About...", new Message( ID_HELP_ABOUT ),"Ctrl + A", LoadBitmapFromResource("about-16.png")) );
    m_pcMenu->AddItem( hMenu );

    cMenuFrame.bottom = m_pcMenu->GetPreferredSize( true ).y + -4.0f;  //Sizes the menu.  The higher the negative, the smaller the menu.
    cMainFrame.top = cMenuFrame.bottom + 1;
    m_pcMenu->SetFrame( cMenuFrame );
    m_pcMenu->SetTargetForItems( this );
    AddChild( m_pcMenu );

   
}


void ArcWindow::SetupToolBar()
{
	pcButtonBar = new ToolBar( Rect( 0, 0, GetBounds().Width(),59 ), "toolbar", true, ToolBar::Horizontal,CF_FOLLOW_RIGHT | CF_FOLLOW_LEFT | CF_FOLLOW_TOP,WID_FULL_UPDATE_ON_RESIZE | WID_WILL_DRAW);
    pcButtonBar->MoveBy(0, 19);
    AddChild(pcButtonBar);
    
    ImageButton* pcOpen = new ImageButton(Rect(2,3,62,56), "imagbut1","Open",new Message(ID_OPEN),NULL,ImageButton::IB_TEXT_BOTTOM, true,true,false, CF_FOLLOW_NONE);
    pcOpen->SetImageFromResource("open.png");
    pcButtonBar->AddChild(pcOpen);
    
    ImageButton* pcNew = new ImageButton(Rect(66,3,126,56), "imagbut2","New",new Message(ID_NEW),NULL, ImageButton::IB_TEXT_BOTTOM, true,true,false, CF_FOLLOW_NONE);
    pcNew->SetImageFromResource("new.png");
    pcButtonBar->AddChild(pcNew);
    
    ImageButton* pcExtract = new ImageButton(Rect(130,3,190,56), "imagbut3","Extract",new Message(ID_EXTRACT),NULL, ImageButton::IB_TEXT_BOTTOM, true,true,false, CF_FOLLOW_NONE);
    pcExtract->SetImageFromResource("extract.png");
    pcButtonBar->AddChild(pcExtract);
    
    ImageButton* pcExe = new ImageButton(Rect(194,3,254,56), "imagbut4","MakeSelf",new Message(ID_EXE),NULL, ImageButton::IB_TEXT_BOTTOM, true,true,false);
    pcExe->SetImageFromResource("exe.png");
    pcButtonBar->AddChild(pcExe);
    
    ImageButton* pcSettingsBut = new ImageButton(Rect(258,3,318,56), "imagbut5","Settings",new Message(ID_APP_SET),NULL, ImageButton::IB_TEXT_BOTTOM, true,true,false);
    pcSettingsBut->SetImageFromResource("settings.png");
    pcButtonBar->AddChild(pcSettingsBut);
   
    ImageButton* pcAboutBut = new ImageButton(Rect(322,3,382,56), "imagbut6","About",new Message(ID_HELP_ABOUT),NULL, ImageButton::IB_TEXT_BOTTOM, true,true,false);
    pcAboutBut->SetImageFromResource("about.png");
    pcButtonBar->AddChild(pcAboutBut);
    
    
    
   
}


void ArcWindow::LoadAtStart( char* cFileName ) 
{
    strcpy( pzStorePath, cFileName );
    ListFiles( cFileName );
}


void ArcWindow::Load( char* cFileName )
{
	pcOpenRequest->Close();
	MakeFocus(pcView);
    strcpy( pzStorePath, cFileName );
    ListFiles( cFileName );
}

bool ArcWindow::Read()
{
    ConfigFile.open( zConfigFile );
	string rec;

    if ( !ConfigFile.eof() )
    {
        ConfigFile.getline( junk, 1024 );
        ConfigFile.getline( lineOpen, 1024 );

		for(int i = 0; i<=10;i++){
			ConfigFile.getline( junk, 1024 );
		}
        ConfigFile.getline((char*)rec.c_str(),1024);
       jas = atoi(rec.c_str());
      }

    ConfigFile.close();
	return true;
}


void ArcWindow::SetupStatusBar()
{
	pcStatus = new StatusBar(Rect(0,GetBounds().Height()-24,GetBounds().Width(), GetBounds().Height()-(-1)),"status_bar",1 );
 	pcStatus->configurePanel(0, StatusBar::FILL, GetBounds().Width());
 	
 	pcStatus->setText("No archives open!", 0,80);
 	
 	AddChild(pcStatus);
}


void ArcWindow::HandleMessage( Message* pcMessage )

{
    Alert* m_pcError = new Alert("Error","Only one Window can \nbe opened at a time.",Alert::ALERT_WARNING,0x00,"OK",NULL);
    m_pcError->CenterInWindow(this);
    switch ( pcMessage->GetCode() )
    {

     /*case ID_VIEW_FILE_VIEWER_FILE_PASS:
     	break;//used to pass the file to the fileviewer
     case ID_CHANGE_LIST:
	 {
	    pcViewer = new ArcViewerWindow();
	    pcViewer->Show();
	    pcViewer->MakeFocus();
	 }
       break;
       */
    case ID_CHANGE_FILEPATH:
        {

            Read();
           
        }

        break;

        case ID_NEW:
         {
            if(m_pcNewWindow == NULL)
            {
                m_pcNewWindow = new NewWindow(this);
                m_pcNewWindow->CenterInWindow(this);
                m_pcNewWindow->Show();
                m_pcNewWindow->MakeFocus();
                Lock();
            }

            else
            {
               m_pcError->Go(new Invoker());
            }
        }

        break;

    case ID_NEW_CLOSE:
        {
			Unlock();
            m_pcNewWindow = NULL;
        }

        break;

      
    case ID_EXE:
        {

            if(m_pcExeWindow == NULL)
            {

                m_pcExeWindow = new ExeWindow(this);
                m_pcExeWindow->CenterInWindow(this);
                m_pcExeWindow->Show();
                m_pcExeWindow->MakeFocus();
            }

            else
            {

                m_pcError->Go( new Invoker() );
            }

        }

        break;

    case ID_EXE_CLOSE:
        {

            m_pcExeWindow = NULL;
        }
        break;


    case ID_DELETE:
        {
            Alert* delAlert = new Alert( "Error", "The Delete function is not \nimplemented yet", 0x00, "OK", NULL );
            delAlert->CenterInWindow(this);
            delAlert->Go( new Invoker() );
        }

        break;


    case ID_APP_SET:
        {


            if (m_pcOptions == NULL)
            {
                m_pcOptions = new OptionsWindow(this);
                m_pcOptions->CenterInWindow(this);
                m_pcOptions->Show();
                m_pcOptions->MakeFocus();
            }

            else
            {

                m_pcError->Go( new Invoker() );
           
            }
        }
        break;


    case ID_OPT_CLOSE_OPT:
        {
            m_pcOptions = NULL;
        }
        break;


    case ID_HELP_ABOUT:
        {

            Alert* aAbout = new Alert( "About Archiver 0.5 Reborn","Archiver 0.5 Reborn\n\nGui Archiver for Syllable    \nCopyright 2002 - 2003 Rick Caudill\n\nArchiver is released under the GNU General\nPublic License. Please go to www.gnu.org\nmore information.\n", Alert::ALERT_INFO, 0x00, "OK", NULL );
            aAbout->CenterInWindow(this);
            aAbout->Go( new Invoker() );

        }
        break;


        

    case ID_GOING_TO_VIEW:
        {
            const char* FileName;
            
            pcMessage->FindString( "newPath", &FileName );
            
            LoadAtStart((char*)FileName);  
        }
        break;

    case ID_EXTRACT:
        {
            if(m_pcExtractWindow == NULL){
                if ( Lst != false )
                {

                    m_pcExtractWindow = new ExtractWindow(this);
                    m_pcExtractWindow->CenterInWindow(this);
                    m_pcExtractWindow->Show();
                    m_pcExtractWindow->MakeFocus();
                    Message* newMSG;
                    newMSG = new Message( ID_GOING_TO_EXTRACT );
                    newMSG->AddString( "pzPath", pzStorePath );
                    m_pcExtractWindow->PostMessage( newMSG, m_pcExtractWindow );
                }

                else
                {

                    Alert* aFile = new Alert( "Error", "You must select a file first.",Alert::ALERT_WARNING, 0x00, "OK", NULL );
                    aFile->CenterInWindow(this);
                    aFile->Go( new Invoker() );
                }

            }
            else
            {
                m_pcError->Go(new Invoker());
            }

        }
        break;



    case ID_EXTRACT_CLOSE:
        {
            m_pcExtractWindow = NULL;
        }

        break;

    case ID_OPEN:
        {

            pcOpenRequest = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), lineOpen, FileRequester::NODE_FILE, false, NULL, NULL, true, true, "Open", "Cancel" );
            pcOpenRequest->CenterInWindow(this);
            pcOpenRequest->Show();
            pcOpenRequest->MakeFocus();

        }
        break;

    case M_LOAD_REQUESTED:
        {

            if ( pcMessage->FindString( "file/path", &pzFPath ) == 0 )
            {

                strcpy( pzStorePath, pzFPath );
                Load( ( char* ) pzStorePath );
                pcOpenRequest = NULL;
                break;
            }

        }


    case ID_QUIT:
        {
            OkToQuit();
        }
        break;



    default:

        Window::HandleMessage( pcMessage );

        break;

    }

}


bool ArcWindow::OkToQuit( void )
{
    Application::GetInstance()->PostMessage( M_QUIT );
    return ( false );
}



void ArcWindow::ListFiles( char* cFileName )
{
	FILE* f;
	FILE* fTmp;
	char zTmp[1024];
	char output_buffer[1024];
	int i;
	int j;
	
	if (  strstr(pzStorePath, ".ace"))
    {

        char ace_n_args[ 300 ] = "";
        strcpy( ace_n_args, "" );
        strcat( ace_n_args, "unace l " );
        strcat( ace_n_args, pzStorePath );
        strcat( ace_n_args, " 2>&1");
        f = NULL;
        fTmp = NULL;
        f = popen( ace_n_args, "r" );
        fTmp = popen( ace_n_args, "r" );
        
        for (i=0; i<=3; i++)
      		fgets(output_buffer, 1024, fTmp);
      	
      	char trash2[1024] = "Invalid archive file: ";
      	strcat(trash2,pzStorePath);
      	strcat(trash2,"\n");
      	
      	if (strcmp(output_buffer, trash2)==0)
      	{
      		sprintf(zTmp, "%s is not an ace file.", pzStorePath);
        	pcStatus->setText(zTmp,0);
        	pclose(f);
        	pclose(fTmp);
        	return;
        }
        
        else{
        pcView->Clear();
        j = 0;
        ListViewStringRow *row;
        while ( !feof( f ) )
        {

            fscanf( f, "%s %s %s %s %s %s ", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ], tmp[5] );

            row = new ListViewStringRow();
            row->AppendString( ( String ) ( const char* ) tmp[ 5 ] );
            row->AppendString( "      ---   " );
            row->AppendString( ( String ) ( const char* ) tmp[ 3 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 1 ]  + "  " + ( const char* ) tmp[ 0 ] );
            row->AppendString( "      ---   " );
            pcView->InsertRow( row );

            j++;

        }
		pclose( f );
        sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        pcStatus->setText(zTmp, 0);
        Lst = true;
        ArcWindow::MakeFocus();
        return ;
       }
     }

    if (strstr(pzStorePath, ".jar"))
    {

        char jar_n_args[ 300 ] = "";
        strcpy( jar_n_args, "" );
        strcat( jar_n_args, "fastjar tfv " );
        strcat( jar_n_args, pzStorePath);
        strcat( jar_n_args, " 2>&1");
        f = NULL;
        fTmp = NULL;
        fTmp = popen(jar_n_args,"r");
        f = popen( jar_n_args, "r" );
        pcView->Clear();
        j = 0;
        fgets(output_buffer, 1024, fTmp);
        
        if (strcmp(output_buffer, "Error in JAR file format. zip-style comment?\n")==0){
        	sprintf(zTmp, "%s is not a jar archive",pzStorePath);
        	pclose(fTmp);
        	pclose(f);
        	pcStatus->setText(zTmp, 0);
        	return;
        }
        
        else{
        pclose(fTmp);
        ListViewStringRow *row;
        while ( !feof( f ) )
        {

            fscanf( f, "%s %s %s %s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ], tmp[5], tmp[6],tmp[7] );
            row = new ListViewStringRow();
            row->AppendString( ( String ) ( const char* ) tmp[ 7 ] );
            row->AppendString( "      ---   " );
            row->AppendString( ( String ) (const char*) tmp[0] );
            row->AppendString( ( String ) ( const char* ) tmp[ 4 ] +",  " + ( const char* ) tmp[ 2 ] + " " +( const char* ) tmp[ 3 ] + ", " + ( const char* ) tmp[ 6 ] );
            row->AppendString( "      ---   " );
            pcView->InsertRow( row );

            j++;

        }

        pcView->RemoveRow(j-1);
        sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        pcStatus->setText(zTmp,0);
        pclose( f );
        Lst = true;
        ArcWindow::MakeFocus();
        return ;
    	}
    }
	

    if (strstr(pzStorePath, ".cab"))
    {

        char cab_n_args[ 300 ] = "";
        strcpy( cab_n_args, "" );
        strcat( cab_n_args, "uncab -v " );
        strcat( cab_n_args, pzStorePath );
        strcat( cab_n_args, " 2>&1");
        f = NULL;
        fTmp = NULL;
        f = popen( cab_n_args, "r" );
        fTmp = popen(cab_n_args,"r");
        pcView->Clear();
        
        fgets(trash, 1024, fTmp);
        
        if (strcmp(trash, "\0\n")==0){
        	sprintf(zTmp, "%s is not a cab archive",pzStorePath);
        	pclose(fTmp);
        	pclose(f);
        	pcStatus->setText(zTmp, 0);
        	return;
        	}
        	
        else{
        pclose(fTmp);	
        for(int i=0;i<=1;i++){
        fgets(trash, 1023, f);
			}
        j = 0;
        ListViewStringRow *row;

        while ( !feof( f ) )
        {

            fscanf( f, "%s %s %s %s %s %s ", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ], tmp[5] );


            row = new ListViewStringRow();
            row->AppendString( ( String ) ( const char* ) tmp[ 5 ] );
            row->AppendString( "      ---   " );
            row->AppendString( ( String ) ( const char* ) tmp[ 0 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 3 ] + "  " + ( const char* ) tmp[ 2 ] );
            row->AppendString( "      ---   " );
            pcView->InsertRow( row );

            j++;

        }
		pclose( f );
		sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        pcStatus->setText(zTmp,0);
        Lst = true;
        ArcWindow::MakeFocus();
        return ;
        }
    }

    if ( strstr( pzStorePath, ".zip" ) )
    {
    	char zip_n_args[ 300 ] = "";
        strcpy( zip_n_args, "" );
        strcat( zip_n_args, "unzip -Z " );
        strcat( zip_n_args, pzStorePath );
        f = NULL;
        f = popen( zip_n_args, "r" );
        pcView->Clear();
        j = 0;
        char junk2[1024];
       
        fgets(junk2,sizeof(junk2),f);
        fread(junk,sizeof(junk2),0,f);
        sprintf(output_buffer, "[%s]\n", pzStorePath);
        if (strcmp(junk2, output_buffer) == 0)
        {
        	sprintf(zTmp, "%s is not a zip file.", pzStorePath);
        	pcStatus->setText(zTmp,0);
        	pclose(f);
        	return; 
        }
        
        else{
        while ( !feof(f)  )
        {
			
            fscanf( f, "%s %s %s %s %s %s %s %s %s",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8] );
            ListViewStringRow *row;
            row = new ListViewStringRow();
            row->AppendString( ( String ) (const char*)tmp[8]);
            row->AppendString( ( String)"");
            row->AppendString( ( String ) (const char*)tmp[3]);
            row->AppendString( ( String )( const char* ) tmp[ 7 ] + "  " + ( const char* ) tmp[ 6 ]);
            row->AppendString( ( String ) (const char*)tmp[0]);
            pcView->InsertRow( row );

            j++;
        }
        
        pcView->RemoveRow(j-1);
        pcView->RemoveRow(j-2);
        pclose( f );
        sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        pcStatus->setText(zTmp,0);
        Lst = true;
        ArcWindow::MakeFocus();
        return;
    	}
	}
	
	
    if ( ( strstr( pzStorePath, ".tar" ) ) && ( !strstr( pzStorePath, ".gz" ) ) && ( !strstr( pzStorePath, ".bz2" ) ) )
    {
        
        char tar_n_args[ 300 ] = "";
        strcpy( tar_n_args, "" );
        strcat( tar_n_args, "tar -tvf " );
        strcat( tar_n_args, pzStorePath );
        strcat( tar_n_args, " 2>&1");
        f = NULL;
        f = popen( tar_n_args, "r" );
        fTmp = popen(tar_n_args,"r");
        pcView->Clear();
        j = 0;
        
        
        fgets(output_buffer, sizeof(output_buffer), fTmp);
        
        if (strcmp(output_buffer, "tar: Hmm, this doesn't look like a tar archive\n") == 0){
        	sprintf(zTmp, "%s is not a tar archive",pzStorePath);
        	pclose(fTmp);
        	pclose(f);
        	pcStatus->setText(zTmp, 0);
        	return;
        	}
        	
        else{
		pclose(fTmp);
		
		while ( !feof( f ) )
        	{
		
            	fscanf( f, "%s %s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ], tmp[ 5 ] );
            	ListViewStringRow *row;
            	row = new ListViewStringRow();
            	row->AppendString( ( String ) ( const char* ) tmp[ 5 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 1 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 2 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 4 ] + "  " + ( const char* ) tmp[ 3 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 0 ] );
            	pcView->InsertRow( row );
            	j++;
        	}
        	pcView->RemoveRow(j-1);
        	pclose( f );
        	sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        	pcStatus->setText(zTmp, 0);
        	Lst = true;
        	ArcWindow::MakeFocus();
        	return ;
		}
    }


    if ( strstr( pzStorePath, ".bz2" ) )
    {

        char bzip_n_args[ 300 ] = "";
		strcpy( bzip_n_args, "" );
        strcat( bzip_n_args, "tar --use-compress-prog=bzip2 -tvf " );
        strcat( bzip_n_args, pzStorePath );
        strcat( bzip_n_args, " 2>&1");        
        f = NULL;
        fTmp = NULL;
        f = popen( bzip_n_args, "r" );
        fTmp = popen(bzip_n_args,"r");
        fgets(output_buffer, 1024, fTmp);
        pcView->Clear();
        j = 0;
       
        if(strcmp(output_buffer, "bzip2: (stdin) is not a bzip2 file.\n") == 0) 
		{
       		sprintf(zTmp,"file '%s' is not a bzip file", pzStorePath); 
       		pcStatus->setText(zTmp, 0);
       		pclose(fTmp);
       		pclose(f);
       		return;
        }
       
       else
       {
       	pclose(fTmp);
        while ( !feof(f)  )
        {

            fscanf( f, "%s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ] );
			fgets(tmp[5], 1024, f);
			tmp[5][strlen(tmp[5])-1] = '\0';
            
            ListViewStringRow *row;
            row = new ListViewStringRow();
            row->AppendString( ( String ) ( const char* ) tmp[ 5 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 1 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 2 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 4] + "  " + ( const char* ) tmp[ 3]  );
            row->AppendString( ( String ) ( const char* ) tmp[ 0 ] );
            pcView->InsertRow( row );

            j++;
        }
        pcView->RemoveRow(j-1);
     	sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        pcStatus->setText(zTmp, 0);
        pclose( f );
        Lst = true;
        ArcWindow::MakeFocus();
        return ;
    }
    }






    if ( strstr( pzStorePath, ".lha" ) )
    {

        char lha_n_args[ 300 ] = "";
        strcpy( lha_n_args, "" );
        strcat( lha_n_args, "lha -l " );
        strcat( lha_n_args, pzStorePath );
        f = NULL;
        f = popen( lha_n_args, "r" );
        pcView->Clear();
        fgets(trash, 1023, f);
        fgets(trash, 1023, f);
        j = 0;
        while ( !feof(f)  )
        {

            fscanf( f, "%s %s %s %s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ], tmp[ 5 ], tmp[6],tmp[7] );

            ListViewStringRow *row;
            row = new ListViewStringRow();
            row->AppendString( ( String ) ( const char* ) tmp[ 6 ] );
            row->AppendString("      ---   " );
            row->AppendString( ( String ) ( const char* ) tmp[ 1 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 3] + " " + ( const char* ) tmp[ 4] + " " + ( const char* ) tmp[ 5]    );
            row->AppendString("      ---   " );
            pcView->InsertRow( row );

            j++;
        }
        pcView->RemoveRow(j-1);
        pcView->RemoveRow(j-2);
        pclose( f );
        Lst = true;
        ArcWindow::MakeFocus();
        return ;
    }


    if ( strstr( pzStorePath, ".gz" ) )
    {

        char gzip_n_args[ 300 ] = "";
	
		strcpy( gzip_n_args, "" );
        strcat( gzip_n_args, "tar -ztvf " );
        strcat( gzip_n_args, pzStorePath );
        strcat( gzip_n_args, " 2>&1");
        f = NULL;
        fTmp=NULL;
        fTmp = popen(gzip_n_args,"r");
        f = popen( gzip_n_args, "r" );
        pcView->Clear();
        j = 0;
      	for (i=0; i<=1; i++){
      		fgets(output_buffer, 1024, fTmp);
     }
       
       if(strcmp(output_buffer, "gzip: stdin: unexpected end of file\n") == 0) 
       {
       		sprintf(zTmp,"file '%s' is not a gzip file", pzStorePath); 
       		pcStatus->setText(zTmp, 0);
       		pclose(fTmp);
       		pclose(f);
       		return;
       }
       
       else
       {
     		pclose(fTmp);
       		while ( !feof( f ) )
        	{
   				fscanf( f, "%s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ] );
				fgets(tmp[5], 1024, f);
				tmp[5][strlen(tmp[5])-1] = '\0';
				ListViewStringRow *row;
            	row = new ListViewStringRow();
        		row->AppendString( ( String ) ( const char* ) tmp[ 5 ]); //tmp[6] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 1 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 2 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 4 ] + "  " +  (const char*) tmp[3] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 0 ] );
            	pcView->InsertRow( row );
				j++;
        	}
        	pcView->RemoveRow(j-1);
        	pclose( f );
        	sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        	pcStatus->setText(zTmp, 0);
        	Lst = true;
        	ArcWindow::MakeFocus();
        	return;
       }
    }


    if ( strstr( pzStorePath, ".tgz" ) )
    {
        char gzip_n_args[ 300 ] = "";
       	
       	strcpy( gzip_n_args, "" );
        strcat( gzip_n_args, "tar -ztvf " );
        strcat( gzip_n_args, pzStorePath );
        strcat( gzip_n_args, " 2>&1");
        f = NULL;
        fTmp=NULL;
        fTmp = popen(gzip_n_args,"r");
        f = popen( gzip_n_args, "r" );
        pcView->Clear();
        j = 0;
      	for (i=0; i<=1; i++){
      		fgets(output_buffer, 1024, fTmp);
     }
       
       if(strcmp(output_buffer, "gzip: stdin: unexpected end of file\n") == 0) 
       {
       		sprintf(zTmp,"file '%s' is not a gzip file", pzStorePath); 
       		pcStatus->setText(zTmp, 0);
       		pclose(fTmp);
       		pclose(f);
       		return;
       }
       
       else
       {
       		string zStatusStuff;
     		pclose(fTmp);
       		while ( !feof( f ) )
        	{
        		fscanf( f, "%s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ] );
				fgets(tmp[5], 1024, f);
				tmp[5][strlen(tmp[5])-1] = '\0';
				ListViewStringRow *row;
            	row = new ListViewStringRow();

        		row->AppendString( ( String ) ( const char* ) tmp[ 5 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 1 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 2 ] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 4 ] + "  " +  (const char*) tmp[3] );
            	row->AppendString( ( String ) ( const char* ) tmp[ 0 ] );
            	pcView->InsertRow( row );
				j++;
        	}
        	pcView->RemoveRow(j-1);
        	pclose( f );
        	sprintf(zTmp, "%s has %d files located in it",pzStorePath, pcView->GetRowCount());
        	pcStatus->setText(zTmp, 0);
        	Lst = true;
        	ArcWindow::MakeFocus();
        	return;
       }
        
    }



    if ( strstr( pzStorePath, ".rar" ) )
    {

        char rar_n_args[ 300 ] = "";
        strcpy( rar_n_args, "" );
        strcat( rar_n_args, "unrar l " );
        strcat( rar_n_args, pzStorePath );
        f = NULL;
        f = popen( rar_n_args, "r" );
        pcView->Clear();
        
      for (int i=0;i<=7;i++){ 
       fgets(trash,1023,f);
      }
        j = 0;
        while ( !feof(f)  )
        {

            fscanf( f, "%s %s %s %s %s %s %s %s %s %s", tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], tmp[ 3 ], tmp[ 4 ], tmp[ 5 ],tmp[6],tmp[7],tmp[8],tmp[9] );

            ListViewStringRow *row;
            row = new ListViewStringRow();

            row->AppendString( ( String ) ( const char* ) tmp[ 0 ] );
            row->AppendString( ( String ) ( const char* ) tmp[ 1 ] );
            row->AppendString( "    ---  " );
            row->AppendString( ( String ) ( const char* ) tmp[ 5] + "  " + ( const char* ) tmp[ 4]  );
            row->AppendString( "   ---  " );
            pcView->InsertRow( row );

            j++;
        }
        pcView->RemoveRow(j-1);
        pclose( f );
        Lst = true;
        ArcWindow::MakeFocus();
        return ;
    }

    else
    {
    	sprintf(zTmp, "%s is not an archive",pzStorePath);
    	pcStatus->setText(zTmp);
        pcView->Clear();
        Lst = false;
    }
}

BitmapImage* ArcWindow::GetImage(const char* pzFile)
{
	Resources cCol(get_image_id());
 	ResStream* pcStream = cCol.GetResourceStream(pzFile);
 	BitmapImage* pcImage = new BitmapImage(pcStream);
 	return (pcImage);
}


















































































































