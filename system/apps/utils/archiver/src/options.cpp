#include "options.h"

OptionsGeneralView::OptionsGeneralView() : View(Rect(0,0,50,50),"")
{

    m_pcOpenToView = new StringView( Rect( 0, 0, 0, 0 ), "Open_To:", "Open Path:" );
    m_pcOpenToBut = new ImageButton( Rect( 0, 0, 60, 15 ), "Select", "Select", new Message( ID_OPT_SELECT_1 ),NULL,false,false,false );
    m_pcOpenToBut->SetImageFromResource("folder.png");
    m_pcOpenToTextView = new TextView( Rect( 0, 0, 0, 0 ), "Open_Text", "" );

    m_pcOpenToView->SetFrame( Rect( 0, 0, 80, 20 ) + Point( 15, 15 ) );
    m_pcOpenToTextView->SetFrame( Rect( 0, 0, 150, 20 ) + Point( 100, 15 ) );
    m_pcOpenToBut->SetFrame( Rect( 0, 0, 16, 16 ) + Point( 255, 17 ) );

    AddChild( m_pcOpenToView );
    AddChild( m_pcOpenToTextView );
    AddChild( m_pcOpenToBut );


    //Set Extract To Option
    m_pcExtractToBut = new ImageButton( Rect( 0, 0, 60, 15 ), "Select", "Select", new Message( ID_OPT_SELECT_2 ),NULL,false,false,false );
    m_pcExtractToBut->SetImageFromResource("folder.png");
    m_pcExtractToView = new StringView( Rect( 0, 0, 60, 20 ), "ExtractToView", "Extract Path:" );
    m_pcExtractToTextView = new TextView( Rect( 0, 0, 0, 0 ), "", "" );

    //Set Position of Extract To Option
    m_pcExtractToTextView->SetFrame( Rect( 0, 0, 150, 20 ) + Point( 100, 45 ) );
    m_pcExtractToBut->SetFrame( Rect( 0, 0, 16, 16 ) + Point( 255, 47 ) );
    m_pcExtractToView->SetFrame( Rect( 0, 0, 80, 20 ) + Point( 15, 45 ) );

    //Add Extract To Option to The View
    AddChild( m_pcExtractToView );
    AddChild( m_pcExtractToTextView );
    AddChild( m_pcExtractToBut );

    //Set Buttonbar option
    m_pcSelectButtonBarPath =  new ImageButton( Rect( 0, 0, 60, 15 ), "Select", "Select", new Message( ID_OPT_SELECT_3 ),NULL,false,false,false );
    m_pcSelectButtonBarPath->SetImageFromResource("folder.png");
    m_pcSelectButtonBarPath->SetEnable(false);
    m_pcButtonBarView = new StringView( Rect( 0, 0, 60, 20 ), "ButtonBarView", "Button Path:" );
    m_pcButtonBarTextView = new TextView( Rect( 0, 0, 0, 0 ), "", "" );
	m_pcButtonBarTextView->SetEnable(false);
    //Set Position of Extract To Option
    m_pcButtonBarTextView->SetFrame( Rect( 0, 0, 150, 20 ) + Point( 100, 75 ) );
    m_pcSelectButtonBarPath->SetFrame( Rect( 0, 0, 16, 16 ) + Point( 255, 77 ) );
    m_pcButtonBarView->SetFrame( Rect( 0, 0, 80, 20 ) + Point( 15, 75 ) );

    //Add Extract To Option to The View
    AddChild( m_pcButtonBarView );
    AddChild( m_pcButtonBarTextView );
    AddChild( m_pcSelectButtonBarPath );


    m_pcExtractClose = new CheckBox( Rect( 0, 0, 0, 0 ), "close_box", "Close Extract Window after extracting.", new Message( ID_OPT_EXTRACT_CLOSE ) );
    m_pcExtractClose->SetFrame( Rect( 0, 0, 240, 20 ) + Point( 15, 102 ) );
    AddChild( m_pcExtractClose );

    m_pcNewClose = new CheckBox( Rect( 0, 0, 0, 0 ), "bew_box", "Close New Window after creating a file.", new Message(ID_OPT_NEW_CLOSE) );
    m_pcNewClose->SetFrame( Rect( 0, 0, 242, 20 ) + Point( 15, 122 ) );
    AddChild( m_pcNewClose );

    m_pcExtractToBut->SetTabOrder( 4 );
    m_pcExtractToTextView->SetTabOrder( 3 );
    m_pcOpenToBut->SetTabOrder( 2 );
    m_pcOpenToTextView->SetTabOrder( 1 );


}




OptionsMiscSettingsView::OptionsMiscSettingsView() : View( Rect( 0, 0, 50, 50 ), "opt_misc_set_view")
{
    m_pcSlideView = new StringView( Rect( 0, 0, 0, 0 ), "Setcomp", "Set Gzip Compression:" );
    m_pcSlideView->SetFrame(Rect(0,0,150,20) + Point(10,20));
    AddChild(m_pcSlideView);

    m_pcSlider = new Slider(Rect(0,0,10,10),"Gzip Compression:",new Message(ID_SLIDE),Slider::TICKS_BELOW,9,Slider::KNOB_TRIANGLE,HORIZONTAL);
    m_pcSlider->SetFrame(Rect(0,0,100,100) + Point(170,21));
    m_pcSlider->SetEnable(false);
    AddChild(m_pcSlider);

    m_pcSetShortView = new StringView(Rect(0,0,0,0),"SetShort", "Set Shortcut Key:");
    m_pcSetShortView->SetFrame(Rect(0,0,150,20) + Point(10,55));
    AddChild(m_pcSetShortView);

    m_pcSetShort = new DropdownMenu(Rect(0,0,0,0),"SetShortBox");
    m_pcSetShort->SetFrame(Rect(0,0,55,15) + Point(150,58));
    m_pcSetShort->InsertItem(0,"Alt");
    m_pcSetShort->InsertItem(1,"Ctrl");
    m_pcSetShort->SetReadOnly();
    m_pcSetShort->SetEnable(false);
    AddChild(m_pcSetShort);
    
    m_pcRecentView = new StringView(Rect(0,0,0,0),"SetRecent","Set RFL Number:");
    m_pcRecentView->SetFrame(Rect(0,0,150,20) + Point(10,85));
    AddChild(m_pcRecentView);
    
    m_pcRecentDrop = new DropdownMenu(Rect(0,0,0,0),"SetRecentList");
    m_pcRecentDrop->SetFrame(Rect(0,0,55,15) + Point(150,88));
    m_pcRecentDrop->InsertItem(0,"0");
    m_pcRecentDrop->InsertItem(1,"5");
    m_pcRecentDrop->InsertItem(2,"10");
    m_pcRecentDrop->InsertItem(3,"15");
    m_pcRecentDrop->SetReadOnly();
    m_pcRecentDrop->SetEnable(false);
    AddChild(m_pcRecentDrop);
}





OptionsTabView::OptionsTabView(const Rect & r): TabView(r,"")
{
    m_pcGenView = new OptionsGeneralView();
    m_pcMisc = new OptionsMiscSettingsView();
    AppendTab("General Settings",m_pcGenView);
    AppendTab("Misc Settings",m_pcMisc);
}


OptionsWindow::OptionsWindow(Window* pcParent) : Window( Rect(0,0,295,210), "opt_window", "Archiver Settings", WND_NOT_RESIZABLE | WND_NOT_MOVEABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT )
{
    Rect r(0,0,295,180);
    r.Resize(5,5,-1,-5);
    m_pcTabView = new OptionsTabView(r);
    AddChild(m_pcTabView);
    pcParentWindow = pcParent;

    m_pcCancelButton = new Button( Rect( 0, 0, 0, 0 ), "Cancel", "Cancel", new Message( ID_OPT_QUIT ) );
    m_pcCancelButton->SetFrame( Rect( 0, 0, 60, 20 ) + Point( 60, 185 ) );
    AddChild( m_pcCancelButton );

    m_pcSaveButton = new Button( Rect( 0, 0, 0, 0 ), "Save", "Save", new Message( ID_OPT_WRITE ) );
    m_pcSaveButton->SetFrame( Rect( 0, 0, 60, 20 ) + Point( 160, 185 ) );
    AddChild( m_pcSaveButton );

    Extract = false;
    Open = false;

    MoveTo( sDesktop.GetResolution().x / 2 - GetBounds().Width() / 2, sDesktop.GetResolution().y / 2 - GetBounds().Height() / 2 );
    sprintf( zConfigFile, "%s/config/Archiver/Archiver.cfg", getenv( "HOME" ) );

    m_pcTabView->m_pcMisc->m_pcSlider->SetLimitLabels("1","9");
    m_pcTabView->m_pcMisc->m_pcSlider->SetTickCount(9);
    m_pcTabView->m_pcMisc->m_pcSlider->SetStepCount(9);
    m_pcTabView->m_pcMisc->m_pcSlider->ValToPos(4);


    Read();  //reads the config file
    Tab();

}

float OptionsWindow::GetSlide(void)
{
    return m_pcTabView->m_pcMisc->m_pcSlider->GetValue().AsFloat();
}

void OptionsWindow::SetSlide(float vol)
{
    m_pcTabView->m_pcMisc->m_pcSlider->SetValue(vol);
    return;
}



bool OptionsWindow::ExtractClose()
{
    return m_pcTabView->m_pcGenView->m_pcExtractClose->GetValue().AsBool() ;
}

bool OptionsWindow::NewClose()
{
    return  m_pcTabView->m_pcGenView->m_pcNewClose->GetValue().AsBool();
}

void OptionsWindow::Tab()
{
    OptionsWindow::SetDefaultButton( m_pcSaveButton );
    OptionsWindow::SetFocusChild( m_pcTabView->m_pcGenView->m_pcOpenToTextView );
}

OptionsWindow::~OptionsWindow()
{
    Extract = false;
    Open = false;

    Invoker *pcParentInvoker = new Invoker(new Message(ID_OPT_CLOSE_OPT),pcParentWindow);
    pcParentInvoker->Invoke();
}


void OptionsWindow::HandleMessage( Message* pcMessage )
{
    switch ( pcMessage->GetCode() )
    {


    case ID_SLIDE:
        {
        }
        break;


    case ID_OPT_EXTRACT_CLOSE:
        {

            ExtractClose() ? true : false;
            break;
        }


    case ID_OPT_NEW_CLOSE:
        {
            NewClose() ? true : false;
            break;
        }


    case ID_OPT_QUIT:
        {
            OptionsWindow::Close();

        }
        break;


    case ID_OPT_WRITE:
        {
            Write();

        }

        break;


    case ID_OPT_SELECT_1:
        {
            Open = true;
            Extract = false;
            m_pcPathRequester->Show();
            m_pcPathRequester->MakeFocus();

        }
        break;


    case ID_OPT_SELECT_2:
        {
            Open = false;
            Extract = true;
            m_pcPathRequester->Show();
            m_pcPathRequester->MakeFocus();
        }
        break;

        /*case ID_OPT_SELECT_3:
            {
                Open = false;
                Extract = false;
                ButtonBar = true;

                m_pcPathRequester->Show();
                m_pcPathRequester->MakeFocus();
            }
            break;*/

    case M_LOAD_REQUESTED:
        {

            if ( pcMessage->FindString( "file/path", &pzOPath ) == 0 )
            {
                struct stat my_stat;
                stat( pzOPath, &my_stat );

                if ( my_stat.st_mode & S_IFDIR )
                {

                  if ( Open == true )
                    {
                        m_pcTabView->m_pcGenView->m_pcOpenToTextView->Set( pzOPath );
                        OptionsWindow::MakeFocus();

                    }

                    if ( Extract == true )
                    {
                        m_pcTabView->m_pcGenView->m_pcExtractToTextView->Set( pzOPath );
                        OptionsWindow::MakeFocus();

                    }
                }

                else
                {
                    Alert* m_pcError = new Alert( "Must be a directory", "You must select a directory.\n", 0x00, "OK", NULL );
                    m_pcError->Go( new Invoker() );
                }

            }


        }
        break;

    default:
        Window::HandleMessage( pcMessage );
    }
}



bool OptionsWindow::Write()
{
    unlink( zConfigFile );
    FILE *nFile;
    if ( ( nFile = fopen( zConfigFile, "w" ) ) )
    {

        //Gets the path from the TextViews

        cExtractPath = m_pcTabView->m_pcGenView->m_pcExtractToTextView->GetBuffer() [ 0 ];
        cOpenPath = m_pcTabView->m_pcGenView->m_pcOpenToTextView->GetBuffer() [ 0 ];
        int key = m_pcTabView->m_pcMisc->m_pcSetShort->GetSelection();
        int rec = m_pcTabView->m_pcMisc->m_pcRecentDrop->GetSelection();
        //writes the config file
        fprintf( nFile, "[Open Path]\n" );
        fprintf( nFile, cOpenPath.c_str() );
        fprintf( nFile, "\n" );
        fprintf( nFile, "[Extract Path]\n" );
        fprintf( nFile, cExtractPath.c_str() );
        fprintf( nFile, "\n" );
        fprintf( nFile, "[Close ExtractWindow after extracting]\n" );
        fprintf( nFile, ExtractClose() ? "Yes\n" : "No\n");
        fprintf( nFile, "[Close New Window after creating file]\n" );
        fprintf( nFile, NewClose() ? "Yes\n" : "No\n");
        fprintf( nFile, "[Gzip Compression]\n" );
        fprintf( nFile, "%f",GetSlide());
        fprintf( nFile,"\n");
        fprintf( nFile, "[Keyboard Shortcut]\n");

        if(key == 1)
        {

            fprintf( nFile, "Ctrl");
        }

        else if(key == 0)
        {

            fprintf( nFile, "Alt");
        }
        
        
        fprintf(nFile,"\n");
        fprintf(nFile,"[Recent List #'s]\n");
        
        if(rec == 0)
        {
        	fprintf(nFile,"0");
        }	
        
         if(rec == 1)
        {
        	fprintf(nFile,"5");
        }	
        
         if(rec == 2)
        {
        	fprintf(nFile,"10");
        }	
        
         if(rec == 3)
        {
        	fprintf(nFile,"15");
        }	




        //Closes the config file
        fclose( nFile );

        //quits the window
        sleep( 1 );
        Message* nMes;
        nMes = new Message(ID_CHANGE_FILEPATH);
        pcParentWindow->PostMessage(nMes,pcParentWindow);
        PostMessage( M_QUIT );
        return true;

    }
    return false;

}


bool OptionsWindow::Read()
{
    ifstream ConfigFile;
    char junk[ 1024 ];
    char lineOpen[ 1024 ];
    char lineExtract[ 1024 ];
    char Extract_Close[ 1024 ];
    char New_Close[1024];
    char gzipcomp[1024];
    char keycomb[1024];
    char recent_list[1024];

	ConfigFile.open(zConfigFile);
   if ( !ConfigFile.eof() )
    {

        ConfigFile.getline( junk, 1024 );
        ConfigFile.getline( lineOpen, 1024 );
        ConfigFile.getline( junk, 1024 );
        ConfigFile.getline( lineExtract, 1024 );
        ConfigFile.getline( junk, 1024 );
        ConfigFile.getline( Extract_Close, 1024 );
        ConfigFile.getline( junk, 1024 );
        ConfigFile.getline( New_Close, 1024 );
        ConfigFile.getline( junk, 1024 );
        ConfigFile.getline( gzipcomp, 1024 );
        ConfigFile.getline( junk, 1024);
        ConfigFile.getline( keycomb,1024);
        ConfigFile.getline( junk, 1024);
        ConfigFile.getline( recent_list,1024);
    }


    ConfigFile.close();


    SetSlide(atof(gzipcomp));  //calls SetSlide() with char as float

    m_pcTabView->m_pcGenView->m_pcExtractToTextView->Set( lineExtract );
    m_pcTabView->m_pcGenView->m_pcOpenToTextView->Set( lineOpen );

	
	if(strstr( Extract_Close, "Yes" ))
    {
        m_pcTabView->m_pcGenView->m_pcExtractClose->SetValue(true);
    }

    if(strstr( Extract_Close, "No" ))
    {
        m_pcTabView->m_pcGenView->m_pcExtractClose->SetValue(false);
    }

    if(strstr(New_Close,"Yes"))
    {
        m_pcTabView->m_pcGenView->m_pcNewClose->SetValue( true );
    }

    if(strstr(New_Close,"No"))
    {
        m_pcTabView->m_pcGenView->m_pcNewClose->SetValue( false );
    }

    if(strstr(keycomb, "Alt"))
    {
        m_pcTabView->m_pcMisc->m_pcSetShort->SetSelection(0);
    }


    if(strstr(keycomb, "Ctrl"))
    {
        m_pcTabView->m_pcMisc->m_pcSetShort->SetSelection(1);
    }
   
   if(strstr(recent_list,"0"))
     {
	m_pcTabView->m_pcMisc->m_pcRecentDrop->SetSelection(0);
     }
   
   
   
   if(strstr(recent_list,"5"))
     {
	m_pcTabView->m_pcMisc->m_pcRecentDrop->SetSelection(1);
     }
   
   
   if(strstr(recent_list,"10"))
     {
	m_pcTabView->m_pcMisc->m_pcRecentDrop->SetSelection(2);
     }
   
   
   if(strstr(recent_list,"15"))
     {
	m_pcTabView->m_pcMisc->m_pcRecentDrop->SetSelection(3);
     }
   
   

    //works, but is really iffy//checkup on this, i don't think this will work too wall...
    m_pcPathRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), lineOpen, FileRequester::NODE_DIR, false, NULL, NULL, true, true, "Open", "Cancel" );
	

	
	/*else {
		
	SetSlide(1);  //calls SetSlide() with char as float
	m_pcTabView->m_pcGenView->m_pcExtractToTextView->Set( getenv("$HOME") );
    m_pcTabView->m_pcGenView->m_pcOpenToTextView->Set( getenv("$HOME") );
    m_pcTabView->m_pcMisc->m_pcRecentDrop->SetSelection(0);
    m_pcTabView->m_pcGenView->m_pcExtractClose->SetValue(true);
    m_pcTabView->m_pcGenView->m_pcNewClose->SetValue( true );	
    }*/
    
    //ConfigFile.close();
    return false;
}













