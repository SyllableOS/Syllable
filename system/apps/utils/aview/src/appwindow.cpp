#include "appwindow.h"
#include "messages.h"

AppWindow::AppWindow(const Rect& cFrame, std::string& sFile) : Window(cFrame, "main_window", "AView 0.3 Beta",WND_NOT_RESIZABLE)
{
    sFileRequester = sFile;
    m_pcLoadRequester=NULL;
    g_pcHScrollBar = NULL;
    g_pcVScrollBar = NULL;
    AddItems();
}

void AppWindow::AddItems()
{
    float win_width, win_height;
    SetupMenus();


    Rect pcWinBounds=GetBounds();
    win_width=( pcWinBounds.right - pcWinBounds.left );
    win_height=( ( pcWinBounds.bottom - pcWinBounds.top ) - MENU_OFFSET );

    main_bitmap=new Bitmap(1600,1200, CS_RGB32, 0x0000);
    main_bitmap_view = new BitmapView ( main_bitmap,this );

    main_bitmap_view->MoveBy(0,MENU_OFFSET+1);  // Move the view/bitmap down so it does not obscure the menu

    Rect pcBitmapRect=main_bitmap_view->GetBounds();
    float winWidth=pcBitmapRect.left+pcBitmapRect.right;
    float winHeight=pcBitmapRect.top+pcBitmapRect.bottom;

    ResizeTo(winWidth,winHeight);
    AddChild( main_bitmap_view );

    main_bitmap_view->MakeFocus();
    SetupStatusBar();



    // We need to reset a few variables etc

    current_image=0;


}

void AppWindow::SetupMenus()
{
    cMenuBounds = GetBounds();
    cMenuBounds.bottom = MENU_OFFSET;

    Menu* pcMenuBar = new Menu(cMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE);

    // Create the menus within the bar
    Menu* pcAppMenu = new Menu(Rect(0,0,0,0),"Application",ITEMS_IN_COLUMN);
    pcAppMenu->AddItem("Settings...", NULL);
    pcAppMenu->AddItem(new MenuSeparator());
    pcAppMenu->AddItem("About...", new Message(ID_ABOUT));
    pcAppMenu->AddItem(new MenuSeparator());
    pcAppMenu->AddItem("Quit", new Message (ID_EXIT));

    Menu* pcFileMenu = new Menu(Rect(0,0,1,1),"File", ITEMS_IN_COLUMN);
    pcFileMenu->AddItem("Open...", new Message( ID_FILE_LOAD));

    Menu* pcViewMenu = new Menu(Rect(0,0,0,0),"View",ITEMS_IN_COLUMN);
    pcSizeFitAll = new CheckMenu("Fit to screen",NULL,true);
    pcSizeFitWindow = new CheckMenu("Fit to window",NULL,false);
    pcViewMenu->AddItem(pcSizeFitAll);
    pcViewMenu->AddItem(pcSizeFitWindow);

    Menu * pcHelpMenu =  new Menu(Rect(0,0,0,0),"Help",ITEMS_IN_COLUMN);
    pcHelpMenu->AddItem("Email...", new Message(ID_HELP));

    pcMenuBar->AddItem(pcAppMenu);
    pcMenuBar->AddItem(pcFileMenu);
    pcMenuBar->AddItem(pcViewMenu);
    pcMenuBar->AddItem(pcHelpMenu);

    pcMenuBar->SetTargetForItems(this);
    pcMenuBar->GetPreferredSize(false);
    // Add the menubar to the window

    AddChild(pcMenuBar);
}

void AppWindow::SetupStatusBar()
{
    pcStatusBar = new StatusBar(Rect(0,GetBounds().Height()-24,GetBounds().Width(), GetBounds().Height()-(-1)),"status_bar",3 );
    pcStatusBar->configurePanel(0, StatusBar::CONSTANT,150);
    pcStatusBar->configurePanel(1, StatusBar::CONSTANT, 100);
    pcStatusBar->configurePanel(2,StatusBar::FILL, 100);

    pcStatusBar->setText("No files open!", 0,100);

    AddChild(pcStatusBar);
}

void AppWindow::DispatchMessage( Message* pcMsg, Handler* pcHandler)
{
    Window::DispatchMessage(pcMsg, pcHandler);
}

void AppWindow::HandleMessage(Message* pcMessage)
{

    switch(pcMessage->GetCode()) //Get the message code from the message
    {
    case ID_FILE_LOAD:
        {
            if (m_pcLoadRequester==NULL)
                m_pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),getenv("$HOME"));
            m_pcLoadRequester->CenterInWindow(this);
            m_pcLoadRequester->Show();
            m_pcLoadRequester->MakeFocus();
        }

    case M_LOAD_REQUESTED:
        {
            if(pcMessage->FindString( "file/path", &pzFPath) == 0)
            {

                if( !stat ( pzFPath, &FPath_buf ) )
                {
                    if( FPath_buf.st_mode & S_IFDIR )
                    {
                        BuildDirList(pzFPath);
                        break;
                    }
                    else
                    {
                        Load( pzFPath );
                        break;
                    }
                }
            }

            break;
        }

    case M_KEY_DOWN:
        // This is the message passed back from BitmapView::Keydown.  We *know*
        // that the message has the data we need, so we'll get it & handle it from here.
        {
            pcMessage->FindInt( "_raw_key", &nKeyCode);
            HandleKeyEvent(nKeyCode);
            break;
        }

    case ID_EXIT:
        OkToQuit();
        break;

    case ID_SCROLL_VERTICAL:
        {
            main_bitmap_view->ScrollBack(g_pcHScrollBar->GetValue(), g_pcVScrollBar->GetValue()  );
        }
        break;

    case ID_SCROLL_HORIZONTAL:
        {
            main_bitmap_view->ScrollBack( g_pcHScrollBar->GetValue(), g_pcVScrollBar->GetValue() );
        }
        break;

    case ID_ABOUT:
        {
            Alert* pcAbout = new Alert("About AView","AView 0.3  Beta\n\nImage Viewer for Syllable    \nCopyright 2002 - 2003 Syllable Desktop Team\n\nAView is released under the GNU General\nPublic License. Please go to www.gnu.org\nmore information.\n",  Alert::ALERT_INFO, 0x00, "OK", NULL);
            pcAbout->CenterInWindow(this);
            pcAbout->Go(new Invoker());
            break;
        }

    case ID_HELP:
        {
            Alert* pcHelp = new Alert("Comments/Suggestions/Bugs/Suggestions","If you have any questions, comments, suggestions, or bugs,\nplease direct them towards the Syllable Developer List.\n\n ",  Alert::ALERT_INFO, 0x00, "OK", NULL);
            pcHelp->CenterInWindow(this);
            pcHelp->Go(new Invoker());
        }
    default:
        Window::HandleMessage( pcMessage);
        break;
    }
}

bool AppWindow::OkToQuit( void )
{
    Application::GetInstance()->PostMessage( M_QUIT);
    return( false );
}


void AppWindow::Load(const char *cFileName)
// Load requested file
{
    if (g_pcVScrollBar)
    {
        RemoveChild(g_pcVScrollBar);
        g_pcVScrollBar = NULL;
    }

    if (g_pcHScrollBar)
    {
        RemoveChild(g_pcHScrollBar);
        g_pcHScrollBar = NULL;
    }

    if(strcmp(cFileName,""))  // This is a safeguard aginst use being passed an empty string.
        // Easier than finding the actual bug at the moment! ;)
    {

        if ( strstr(cFileName, ".jpg") || (strstr(cFileName,".png")) || (strstr(cFileName, ".gif"))  || (strstr(cFileName, ".icon")))
        {

            bSetTitle=true;
            Desktop d;
            // Get rid of the old bitmap
            RemoveChild(main_bitmap_view);
            Sync();
            // delete the bitmap to free up precious memory!
            delete main_bitmap;

            // load the new jpeg & create the BitmapView for it
            char img_fname[128];           //
            strcpy(img_fname,cFileName);   // This needs fixing! :)

            main_bitmap=LoadBitmapFromFile(img_fname);

            if(main_bitmap==NULL)
            {         // We wern't able to load the given file
                float win_width=300;
                float win_height=275-MENU_OFFSET;

                // We're going to make an empty bitmap to handle not being able to load the
                // given bitmap

                main_bitmap=new Bitmap( win_width, win_height, CS_RGB32, 0x0000);
                bSetTitle=false;  // We don't want this filename in the title now
            }

            main_bitmap_view = new BitmapView ( main_bitmap,this );

            main_bitmap_view->MoveBy(0,MENU_OFFSET+1);  // Move the view/bitmap down so it does not obscure the menu

            // We need to know the size of the new bitmap so we can match the window to it
            pcBitmapRect= main_bitmap_view->GetBounds();
            pcBitmapRect.bottom += (pcStatusBar->GetBounds().top+pcStatusBar->GetBounds().bottom+15);

            winWidth=pcBitmapRect.left+pcBitmapRect.right+15;
            winHeight=pcBitmapRect.top+pcBitmapRect.bottom+MENU_OFFSET+24;
            Rect cScrollRect = main_bitmap_view->GetBounds();
            // Now we can resize the window to match the size of the bitmap



            if (main_bitmap->GetBounds().Width() <= GetBounds().Width() && main_bitmap->GetBounds().Height() <= GetBounds().Height())
            {
                ResizeTo(400,400);
                main_bitmap_view->SetFrame(Rect(0,22,GetBounds().right, GetBounds().bottom-24));
                AddChild(main_bitmap_view);
            }

            else if (winWidth > GetBounds().Width()  || winHeight > GetBounds().Height())
            {
                ResizeTo(winWidth, winHeight);
                main_bitmap_view->SetFrame(Rect(0,22,GetBounds().right-15, GetBounds().bottom-39));
                AddChild( main_bitmap_view );

                g_pcVScrollBar = new  ScrollBar(Rect(cScrollRect.Width()+1,22, GetBounds().Width(),GetBounds().Height()-(pcStatusBar->GetBounds().top+pcStatusBar->GetBounds().bottom+13)),"sc",new Message(ID_SCROLL_VERTICAL),CF_FOLLOW_ALL);
                g_pcVScrollBar->SetScrollTarget(main_bitmap_view);

                AddChild(g_pcVScrollBar);

                g_pcHScrollBar = new ScrollBar(Rect(0,GetBounds().Height()-39,GetBounds().Width(),GetBounds().Height()-24), "sc2",new Message(ID_SCROLL_HORIZONTAL),0,FLT_MAX,HORIZONTAL, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT);
                g_pcHScrollBar->SetScrollTarget(main_bitmap_view);

                AddChild(g_pcHScrollBar);

                g_pcHScrollBar->SetMinMax(0.0f,main_bitmap->GetBounds().Width()-main_bitmap_view->Width());
                g_pcVScrollBar->SetMinMax(0,main_bitmap->GetBounds().Height() - main_bitmap_view->Height() );

            }


            if(bSetTitle==true)
            {
                string sTitle = (string)"AView 0.3 Beta- " + (string)cFileName;
                char sRes[1024];
                sprintf((char*)sRes,"%.0fx%.0f",main_bitmap->GetBounds().Width(), main_bitmap->GetBounds().Height());

                SetTitle(sTitle.c_str());
                pcStatusBar->setText(cFileName,0,0);
                pcStatusBar->setText(sRes,1,0);
            }
            else
            {
                SetTitle("AView 0.3 Beta");
            }

        }

        else
        {
            Alert* pcError = new Alert("Not Supported","This filetype is not supported by AtheOS\n", Alert::ALERT_INFO,0x00, "OK",NULL);
            pcError->Go(new Invoker());
        }
    }
}

void AppWindow::HandleKeyEvent( int nKeyCode )
{
    switch(nKeyCode)
    {

    case 30:  // Backspace - Go to previous image
        {
            if(current_image)      // If we arn't already at the last image
            {
                current_image--;        // We decrement the counter that points to the image currently loaded

                next_image=file_list[current_image].c_str();  // We need the c style string of the first image
                Load( next_image );                           // so that we can load it!

                main_bitmap_view->MakeFocus();
            }

            break;
        }

    case 38:   // Tab
    case 94:   // Space - Go to next image
        {
            if(current_image!=file_list.size())  // If we arn't at the end of the list
            {
                current_image++;

                next_image=file_list[current_image].c_str();  // We need the c style string of the first image
                Load( next_image );                           // so that we can load it!

                main_bitmap_view->MakeFocus();
            }

            break;
        }

    default:   // This isn't a recognised key code that we need to deal with
        break;

    }

}

void AppWindow::BuildDirList(const char* pzFPath)
{
    FDir_handle=opendir( pzFPath);

    char file_path[256];

    while((FDir_entry = readdir(FDir_handle)))
    {
        if(strcmp(FDir_entry->d_name,".") && strcmp(FDir_entry->d_name,".."))
        {

            strcpy( file_path, (char*) pzFPath);
            strcat( file_path, "/");
            strcat( file_path, FDir_entry->d_name);
            strcat( file_path, "\0");

            AppWindow::file_list.push_back( file_path);  // We store every file in a vector

        }

    }

    current_image=0;
    const char* first_image;

    first_image=file_list[current_image].c_str();  // We need the c style string of the first image
    Load( first_image );                           // so that we can load it!


}







































