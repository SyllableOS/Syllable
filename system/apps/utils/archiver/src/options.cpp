#include "options.h"
#include "loadbitmap.h"
#include "settings.h"

OptionsGeneralView::OptionsGeneralView() : View(Rect(0,0,50,50),"")
{
    m_pcOpenToView = new StringView( Rect( 0, 0, 0, 0 ), "Open_To:", "Open Path:" );
    m_pcOpenToBut = new ImageButton( Rect( 0, 0, 60, 15 ), "Select", "Select", new Message( ID_OPT_SELECT_1 ),NULL,false,false,false );
    m_pcOpenToBut->SetImage(LoadImageFromResource("folder.png"));
    m_pcOpenToTextView = new TextView( Rect( 0, 0, 0, 0 ), "Open_Text", "" );

    m_pcOpenToView->SetFrame( Rect( 0, 0, 80, 20 ) + Point( 15, 15 ) );
    m_pcOpenToTextView->SetFrame( Rect( 0, 0, 150, 20 ) + Point( 100, 15 ) );
    m_pcOpenToBut->SetFrame( Rect( 0, 0, 16, 16 ) + Point( 255, 17 ) );

    AddChild( m_pcOpenToView );
    AddChild( m_pcOpenToTextView );
    AddChild( m_pcOpenToBut );


    //Set Extract To Option
    m_pcExtractToBut = new ImageButton( Rect( 0, 0, 60, 15 ), "Select", "Select", new Message( ID_OPT_SELECT_2 ),NULL,false,false,false );
    m_pcExtractToBut->SetImage(LoadImageFromResource("folder.png"));
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
    m_pcSelectButtonBarPath->SetImage(LoadImageFromResource("folder.png"));
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

	m_pcNewClose->SetTabOrder(4);
	m_pcExtractClose->SetTabOrder(3);
    m_pcExtractToTextView->SetTabOrder( 2 );
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


OptionsWindow::OptionsWindow(Window* pcParent, AppSettings* pcSettings) : Window( Rect(0,0,295,210), "opt_window", "Archiver Settings", WND_NOT_RESIZABLE | WND_NOT_MOVEABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT )
{
    Rect r(0,0,295,180);
    r.Resize(5,5,-1,-5);
    m_pcTabView = new OptionsTabView(r);
    AddChild(m_pcTabView);

    pcParentWindow = pcParent;
	pcAppSettings = pcSettings;

    m_pcCancelButton = new Button( Rect( 0, 0, 0, 0 ), "Cancel", "Cancel", new Message( ID_OPT_QUIT ) );
    m_pcCancelButton->SetFrame( Rect( 0, 0, 60, 20 ) + Point( 60, 185 ) );
    AddChild( m_pcCancelButton );

    m_pcSaveButton = new Button( Rect( 0, 0, 0, 0 ), "Save", "Save", new Message( ID_OPT_WRITE ) );
    m_pcSaveButton->SetFrame( Rect( 0, 0, 60, 20 ) + Point( 160, 185 ) );
    AddChild( m_pcSaveButton );

    Extract = false;
    Open = false;

    m_pcTabView->m_pcMisc->m_pcSlider->SetLimitLabels("1","9");
    m_pcTabView->m_pcMisc->m_pcSlider->SetTickCount(9);
    m_pcTabView->m_pcMisc->m_pcSlider->SetStepCount(9);
    m_pcTabView->m_pcMisc->m_pcSlider->ValToPos(4);

	Tab();
	_Init();
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
	m_pcSaveButton->SetTabOrder(5);
	m_pcCancelButton->SetTabOrder(6);
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
        	break;
		}
	
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
            Close();
        	break;
		}

		case ID_OPT_WRITE:
		{
			SaveOptions();
			Close();
			break;
		}

		case ID_OPT_SELECT_1:
        {
            Open = true;
            Extract = false;
			m_pcPathRequester->CenterInWindow(this);
            m_pcPathRequester->Show();
            m_pcPathRequester->MakeFocus();
			break;
		}

    	case ID_OPT_SELECT_2:
        {
            Open = false;
            Extract = true;
			m_pcPathRequester->CenterInWindow(this);
            m_pcPathRequester->Show();
            m_pcPathRequester->MakeFocus();
        	break;
		}
    
		case M_LOAD_REQUESTED:
        {
			String cGetPath;
            if ( pcMessage->FindString( "file/path", &cGetPath ) == 0 )
            {
				cGetPath += "/";

                struct stat my_stat;
                stat(cGetPath.c_str(), &my_stat );

                if ( my_stat.st_mode & S_IFDIR )
                {
					if ( Open == true )
                    {
                        m_pcTabView->m_pcGenView->m_pcOpenToTextView->Set( cGetPath.c_str() );
                        OptionsWindow::MakeFocus();
                    }

                    if ( Extract == true )
                    {
                        m_pcTabView->m_pcGenView->m_pcExtractToTextView->Set(cGetPath.c_str());
                        OptionsWindow::MakeFocus();
                    }
                }

                else
                {
                    Alert* m_pcError = new Alert( "Must be a directory", "You must select a directory.\n", 0x00, "OK", NULL );
                    m_pcError->Go( new Invoker() );
                }

            }
        	break;
		}
		default:
        	Window::HandleMessage( pcMessage );
			break;
    }
}

void OptionsWindow::SaveOptions()
{
	pcAppSettings->SetCloseNewWindow(m_pcTabView->m_pcGenView->m_pcNewClose->GetValue().AsBool());
	pcAppSettings->SetCloseExtractWindow(m_pcTabView->m_pcGenView->m_pcExtractClose->GetValue().AsBool());
	pcAppSettings->SetExtractPath(m_pcTabView->m_pcGenView->m_pcExtractToTextView->GetBuffer()[0]);
	pcAppSettings->SetOpenPath(m_pcTabView->m_pcGenView->m_pcOpenToTextView->GetBuffer()[0]);
}

void OptionsWindow::_Init()
{
	m_pcPathRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),pcAppSettings->GetOpenPath().c_str(),FileRequester::NODE_DIR,false,NULL,NULL,true,true,"Select","Cancel");
	m_pcTabView->m_pcGenView->m_pcNewClose->SetValue(pcAppSettings->GetCloseNewWindow());
	m_pcTabView->m_pcGenView->m_pcExtractClose->SetValue(pcAppSettings->GetCloseExtractWindow());
	m_pcTabView->m_pcGenView->m_pcExtractToTextView->Set(pcAppSettings->GetExtractPath().c_str());
	m_pcTabView->m_pcGenView->m_pcOpenToTextView->Set(pcAppSettings->GetOpenPath().c_str());
}	
