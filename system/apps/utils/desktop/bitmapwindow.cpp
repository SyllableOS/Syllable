#include "bitmapwindow.h"
#include "iconmenu_messages.h"
#include "login.h"
//#include "drives.h"
#include "debug.h"

/*
** name:       LaunchFiles
** purpose:    Launches files in ~/config/desktop/startup
** parameters:
** returns:  
*/
void LaunchFiles()
{
    std::vector<string> launch_files;
    string zName;
    string zPath;
    Directory* pcDir = new Directory();

    if(pcDir->SetTo("~/config/desktop/startup")==0)
    {
        pcDir->GetPath(&zName);
        while (pcDir->GetNextEntry(&zName))
            if ( (zName.find( "..",0,1)==string::npos) && (zName.find( "Disabled",0)==string::npos))
            {

                launch_files.push_back(zName);
            }
    }
    for (uint32 n = 0; n < launch_files.size(); n++)
    {


        pid_t nPid = fork();
        if ( nPid == 0 )
        {
            set_thread_priority( -1, 0 );
            execlp(launch_files[n].c_str(), launch_files[n].c_str(), NULL );
            exit( 1 );
        }
    }
    delete pcDir;
}


/*
** name:       SyllableInfo
** purpose:    Returns a formatted string for when "Show version on desktop" is enabled 
** parameters: 
** returns:   String
*/
string SyllableInfo()
{
    string return_version;
    return_version = "Syllable 0.4.1, Desktop V0.5";
    return (return_version);
}




/*
** name:       ScreenRes
** purpose:    Returns the size of the screen.  Useful for "Show Version on desktop" option.
** parameters: 
** returns:   IPoint with the users screen resolution in it
*/
IPoint ScreenRes()
{
    Desktop d_desk;
    return d_desk.GetResolution();
}


/*
** name:       ReadBitmap
** purpose:    Checks to see if the image the user specifies for the desktop background is an actual file
				If not it returns the default one.  If it is, it returns that image.
** parameters: A pointer to a const char* containing the filename of the file
** returns:    Bitmap with specified image
*/
Bitmap* ReadBitmap(const char* zImageName)
{
    ifstream fs_image;
    Bitmap* pcBitmap = NULL;
    DeskSettings* pcSet = new  DeskSettings();
    string zImagePath = (string)pcSet->GetImageDir() +  (string)zImageName;
	fs_image.open(zImagePath.c_str());

    if(fs_image == NULL)
    {
        fs_image.close();
        pcBitmap = LoadBitmapFromResource("logo_atheos.jpg");

    }

    else
    {
        fs_image.close();



        if (pcSet->GetImageSize() == 0)
        {
            Bitmap* pcLargeBitmap = new Bitmap(ScreenRes().x, ScreenRes().y, CS_RGB32,Bitmap::SHARE_FRAMEBUFFER);
            Scale(LoadBitmapFromFile(zImagePath.c_str()), pcLargeBitmap, filter_mitchell, 0);
            pcBitmap = pcLargeBitmap;

        }
        else
        {
            pcBitmap = LoadBitmapFromFile(zImagePath.c_str() );
        }
    }

    return (pcBitmap);
}


/*
** name:       CheckConfig
** purpose:    Checks to see if the config file exsists.  If it doesn't, it will create
**			   one with WriteConfigFile().  If it does exsist, it just exits.
** parameters: 
** returns:    
*/
void CheckConfig()
{
    ifstream filestr;
    DeskSettings* pcSet = new DeskSettings();
    filestr.open(pcSet->GetConfigFile());

    if(filestr == NULL)
    {
        filestr.close();
        //WriteConfigFile();
    }

    else
    {
        filestr.close();
    }
}

/*
** name:       SetDefaults
** purpose:    Makes sure that all directories are created and also Runs CheckConfig
** parameters: 
** returns:
*/
void SetDefaults()
{
    system("mkdir ~/config 2> /dev/null");
    system("mkdir ~/config/desktop 2> /dev/null");
    system("mkdir ~/config/desktop/config 2> /dev/null");
    system("mkdir ~/config/desktop/startup 2> /dev/null");
    system("mkdir ~/config/desktop/pictures 2> /dev/null");
    system("mkdir ~/config/desktop/disks 2> /dev/null");
    system("mkdir ~/Desktop 2> /dev/null");

    //CheckConfig();
}

/*
** name:       Icon::Select
** purpose:    Invokes paint after erasing the BitmapView
** parameters: A pointer to the BitmapView and a bool that tells if the icon is selected
** returns:    
*/
void Icon::Select( BitmapView* pcView, bool bSelected )
{
    if ( m_bSelected == bSelected )
    {
        return;
    }
    m_bSelected = bSelected;

    pcView->Erase( GetFrame( pcView->GetFont() ) );

    Paint( pcView, Point(0,0), true, true, pcView->zBgColor, pcView->zFgColor );
}


/*
** name:       BitmapView Constructor
** purpose:    Constructor for the BitmapView
** parameters: Rect for the Frame
** returns:   
*/
BitmapView::BitmapView( const Rect& cFrame ) :
        View( cFrame, "_bitmap_view", CF_FOLLOW_ALL)
{


    //m_drives = mounteddrives();
    pcSettings = new DeskSettings();
    ReadPrefs();
	pcMainMenu = new Menu(Rect(0,0,0,0),"",ITEMS_IN_COLUMN);
    pcMountMenu = new Menu(Rect(0,0,0,0),"Mount Drives    ",ITEMS_IN_COLUMN);

   /* Menu* pcAtheMenu = new Menu(Rect(0,0,0,0), m_drives.zMenu,ITEMS_IN_COLUMN);
    pcAtheMenu->AddItem("Show Info...",new Message(M_SHOW_DRIVE_INFO));
    pcAtheMenu->AddItem("Unmount...", new Message(M_DRIVES_UNMOUNT));
    pcMountMenu->AddItem(new ImageItem(pcAtheMenu,NULL,LoadBitmapFromResource("mount.png")));

    pcMountMenu->AddItem(new MenuSeparator());
    pcMountMenu->AddItem("Settings...",NULL);

    pcMainMenu->AddItem(new ImageItem(pcMountMenu,NULL,LoadBitmapFromResource("mount.png")));
    */
    pcMainMenu->AddItem(new MenuSeparator());
    pcMainMenu->AddItem(new ImageItem("Properties", new Message(M_PROPERTIES_SHOW),"", LoadBitmapFromResource("kcontrol.png")));
    pcMainMenu->AddItem(new MenuSeparator());
    pcMainMenu->AddItem(new ImageItem("Logout",new Message(M_LOGOUT_USER),"", LoadBitmapFromResource("exit.png")));
    pcMainMenu->GetPreferredSize(true);

    pcIconMenu = new IconMenu();
    pcIconMenu->SetTargetForItems(this);


    m_bCanDrag = false;
    m_bSelRectActive = false;

    
	SetIcons();   
	m_nHitTime = 0;
	pzSyllableVer = SyllableInfo();
	
	
	LaunchFiles();

}


/*
** name:       ~BitmapView
** purpose:    Destructor for the BitmapView
** parameters:
** returns:    
*/
BitmapView::~BitmapView()
{
    delete m_pcBitmap;
}


/*
** name:       BitmapView::IconList
** purpose:    Builds a list of .desktop files in ~/Desktop
** parameters: 
** returns:    t_icon - a vector of strings containing the names of the .desktop(s) for later use
*/
t_Icon BitmapView::IconList()
{
    string zName;
    t_Icon t_icon;
    Directory *pcDir = new Directory( );
    

    if( pcDir->SetTo( pcSettings->GetIconDir() ) == 0 )
    {

        while( pcDir->GetNextEntry( &zName ) )

            if (!(zName.find( ".desktop",0)==string::npos) )
            {
                t_icon.push_back(zName);
            }
    }

    delete pcDir;
    return (t_icon);
}


/*
** name:       BitmapView::SetIcons
** purpose:    Sets the icons up using IconList() as a template for the icon names.
** parameters: A pointer to a LauncherMessage containing config info for the plugin
** returns:   
*/
void BitmapView::SetIcons()
{
    char zIconName[1024];
    char zIconImage[1024];
    char zIconExec[1024];
    char zPos[1024];
    char junk[1024];
    struct stat sStat;
    Point zIconPoint;
    uint iconExec = 0;
    Point cPos( 20, 20 );
    
    //RemoveIcons();
    
    for (iconExec = 0; iconExec < IconList().size(); iconExec++)
    {
        ifstream filRead;
        string pzIconPath = (string)pcSettings->GetIconDir() + IconList()[iconExec].c_str();
        filRead.open(pzIconPath.c_str());
        filRead.getline(junk,1024);
        filRead.getline(zIconName,1024);
        for(int i=0; i <2; i++){filRead.getline(junk,1024);}
        filRead.getline(zIconImage,1024);
        for(int i=0; i <2; i++){filRead.getline(junk,1024);}
        filRead.getline(zIconExec,1024);
        for(int i=0; i <2; i++){filRead.getline(junk,1024);}
        filRead.getline(zPos,1024);
        filRead.close();

        sscanf(zPos,"%f %f\n",&zIconPoint.x,&zIconPoint.y);

        m_cIcons.push_back(new Icon(zIconName, zIconImage,zIconExec, zIconPoint,sStat));
    }
    
    
       for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        cPos.y += 50;
        if ( cPos.y > 500 )
        {
            cPos.y = 20;
            cPos.x += 50;

            m_cIcons[i]->Paint( this, Point(0,0), true, true, zBgColor, zFgColor );

        }
        
        Paint(GetBounds());
    }

}



/*
** name:       BitmapView::Paint
** purpose:    Paints the BitmapView on to the screen
** parameters: Rect that holds the size of the BitmapView
** returns:    
*/
void BitmapView::Paint( const Rect& cUpdateRect)
{
    Rect cBounds = GetBounds();
    SetDrawingMode( DM_COPY );
    Font* pcFont = GetFont();
    
    Erase( cUpdateRect );
    
    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cUpdateRect ) )
        {
            m_cIcons[i]->Paint( this, Point(0,0), true, true, zBgColor, zFgColor );
        }
    }

    if (bShow == true)
    {
        SetFgColor(0,0,0);
        SetDrawingMode(DM_BLEND);
        MovePenTo(ScreenRes().x - 15 - GetStringWidth(pzSyllableVer) , ScreenRes().y -10);
        DrawString(pzSyllableVer);
    }

}

/*
** name:       BitmapView::Erase
** purpose:    Erases the BitmapView
** parameters: Rect that holds the size of the BitmapView
** returns:    
*/
void BitmapView::Erase( const Rect& cFrame )
{
	
    if ( m_pcBitmap != NULL )
    {
        Rect cBmBounds = m_pcBitmap->GetBounds();
        int nWidth  = int(cBmBounds.Width()) + 1;
        int nHeight = int(cBmBounds.Height()) + 1;
        SetDrawingMode( DM_COPY );
        for ( int nDstY = int(cFrame.top) ; nDstY <= cFrame.bottom ; )
        {
            int y = nDstY % nHeight;
            int nCurHeight = min( int(cFrame.bottom) - nDstY + 1, nHeight - y );

            for ( int nDstX = int(cFrame.left) ; nDstX <= cFrame.right ; )
            {
                int x = nDstX % nWidth;
                int nCurWidth = min( int(cFrame.right) - nDstX + 1, nWidth - x );

                Rect cRect( 0, 0, nCurWidth - 1, nCurHeight - 1 );
                DrawBitmap( m_pcBitmap, cRect + Point( x, y ), cRect + Point( nDstX, nDstY ) );
                nDstX += nCurWidth;
            }
            nDstY += nCurHeight;


        }
    }
    else
    {
        FillRect( cFrame, Color32_s( 0x00, 0x00, 0x00, 0 ) );
    }
}

/*
** name:       BitmapView::FindIcon
** purpose:    Find the icon that is selected
** parameters: Point that tell where the mouse is
** returns:    NULL or the Icon that is selected
*/
Icon* BitmapView::FindIcon( const Point& cPos )
{
    Font* pcFont = GetFont();

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cPos ) )
        {
            return( m_cIcons[i] );
        }
    }
    return( NULL );
}

/*
** name:       BitmapView::MouseDown
** purpose:    Paints the BitmapView on to the screen
** parameters: Point where the tip of the cursor is and an int that tells what button is pushed
** returns:    
*/
void BitmapView::MouseDown( const Point& cPosition, uint32 nButtons )
{
    MakeFocus( true );

    Icon* pcIcon = FindIcon( cPosition );
    if(nButtons ==1)
    {
        if ( pcIcon != NULL )
        {
            if (  pcIcon->m_bSelected )
            	{
                if ( m_nHitTime + 500000 >= get_system_time() )
                {
                    pid_t nPid = fork();
                    if ( nPid == 0 )
                    {
                        set_thread_priority( -1, 0 );
                        execlp( pcIcon->GetExecName().c_str(), pcIcon->GetExecName().c_str(), NULL );
                        exit( 1 );

                    }
                }
               else
                {
                    m_bCanDrag = true;
                }
                m_nHitTime = get_system_time();
                return;
            }
        }
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            m_cIcons[i]->Select( this, false );
        }
        if ( pcIcon != NULL )
        {
            m_bCanDrag = true;
            pcIcon->Select( this, true );
        }
        else
        {
            m_bSelRectActive = true;
            m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
            SetDrawingMode( DM_INVERT );
            DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        }
        Flush();
        m_cLastPos = cPosition;
        m_nHitTime = get_system_time();
    }

    else if (nButtons == 2)
    {

        if (pcIcon !=NULL)
        {

            m_bSelRectActive = true;
            m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
            SetDrawingMode( DM_INVERT );
            DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
            pcIcon->Select( this,true );
            Flush();

            if (  pcIcon->m_bSelected )
                cIconName = pcIcon->GetName();
                cIconExec = pcIcon->GetExecName();

            pcIconMenu->Open(ConvertToScreen(cPosition));
            pcIconMenu->SetTargetForItems(this);
        }

        else if(pcIcon == NULL)
        {
            pcMainMenu->Open(ConvertToScreen(cPosition));
            pcMainMenu->SetTargetForItems(this);
        }

    }
}


void BitmapView::RemoveIcons()
{
	for (uint i=m_cIcons.size(); i>0; i--){
	    m_cIcons.pop_back();
		}
	
	Erase(this->GetBounds());
	Flush();	
	
}

/*
** name:       BitmapView::MouseUp
** purpose:    When the mouse button is depressed, this is executed
** parameters: Point where the cursor is, int that tells what button is pressed, Message
** returns:    
*/
void BitmapView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    m_bCanDrag = false;

    Font* pcFont = GetFont();
    if ( m_bSelRectActive )
    {
        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_bSelRectActive = false;

        if ( m_cSelRect.left > m_cSelRect.right )
        {
            float nTmp = m_cSelRect.left;
            m_cSelRect.left = m_cSelRect.right;
            m_cSelRect.right = nTmp;
        }
        if ( m_cSelRect.top > m_cSelRect.bottom )
        {
            float nTmp = m_cSelRect.top;
            m_cSelRect.top = m_cSelRect.bottom;
            m_cSelRect.bottom = nTmp;
        }

        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            m_cIcons[i]->Select( this, m_cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
        }
        Flush();
    }
    else if ( pcData != NULL && pcData->ReturnAddress() == Messenger( this ) )
    {
        Point cHotSpot;
        pcData->FindPoint( "_hot_spot", &cHotSpot );


        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            Rect cFrame = m_cIcons[i]->GetFrame( pcFont );
            Erase( cFrame );
        }
        Flush();

        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            if ( m_cIcons[i]->m_bSelected )
            {
                m_cIcons[i]->m_cPosition += cPosition - m_cDragStartPos;
            }
            m_cIcons[i]->Paint( this, Point(0,0), true, true, zBgColor, zFgColor );
        }
        Flush();
    }
}


/*
** name:       BitmapView::MouseMove
** purpose:    Executed when the mouse is moved
** parameters: Point that holds where the cursor is, 
** returns:    
*/
void BitmapView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    m_cLastPos = cNewPos;

    if ( (nButtons & 0x01) == 0 )
    {
        return;
    }

    if ( m_bSelRectActive )
    {
        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_cSelRect.right = cNewPos.x;
        m_cSelRect.bottom = cNewPos.y;

        Rect cSelRect = m_cSelRect;
        if ( cSelRect.left > cSelRect.right )
        {
            float nTmp = cSelRect.left;
            cSelRect.left = cSelRect.right;
            cSelRect.right = nTmp;
        }
        if ( cSelRect.top > cSelRect.bottom )
        {
            float nTmp = cSelRect.top;
            cSelRect.top = cSelRect.bottom;
            cSelRect.bottom = nTmp;
        }
        Font* pcFont = GetFont();
        SetDrawingMode( DM_COPY );
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            m_cIcons[i]->Select( this, cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
        }

        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );

        Flush();
        return;
    }

    if ( m_bCanDrag )
    {
        Flush();

        Icon* pcSelIcon = NULL;

        Rect cSelFrame( 1000000, 1000000, -1000000, -1000000 );

        Font* pcFont = GetFont();
        Message cData(1234);
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            if ( m_cIcons[i]->m_bSelected )
            {
                cData.AddString( "file/path", m_cIcons[i]->GetName().c_str() );
                cSelFrame |= m_cIcons[i]->GetFrame( pcFont );
                pcSelIcon = m_cIcons[i];
            }
        }
        if ( pcSelIcon != NULL )
        {
            m_cDragStartPos = cNewPos; // + cSelFrame.LeftTop() - cNewPos;

            if ( (cSelFrame.Width()+1.0f) * (cSelFrame.Height()+1.0f) < 12000 )
            {
                Bitmap cDragBitmap( cSelFrame.Width()+1.0f, cSelFrame.Height()+1.0f, CS_RGB32, Bitmap::ACCEPT_VIEWS | Bitmap::SHARE_FRAMEBUFFER );

                View* pcView = new View( cSelFrame.Bounds(), "" );
                cDragBitmap.AddChild( pcView );

                pcView->SetFgColor( 255, 255, 255, 0 );
                pcView->FillRect( cSelFrame.Bounds() );


                for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
                {
                    if ( m_cIcons[i]->m_bSelected )
                    {
                        m_cIcons[i]->Paint( pcView, -cSelFrame.LeftTop(), true, false, zBgColor, zFgColor );
                    }
                }
                cDragBitmap.Sync();

                uint32* pRaster = (uint32*)cDragBitmap.LockRaster();

                for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y )
                {
                    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x )
                    {
                        if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0x00ffffff &&
                                (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0xff000000 )
                        {
                            pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0xb0000000;
                        }
                    }
                }
                BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), &cDragBitmap );

            }
            else
            {
                BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), cSelFrame.Bounds() );
            }
        }
        m_bCanDrag = false;
    }
    Flush();
}


/*
** name:       BitmapView::ReadPrefs
** purpose:    Reads the Flattend message from the config file.
** parameters: void
** returns:    
*/
void BitmapView::ReadPrefs(void)
{
	pcSetPrefs = NULL;
	pcSetPrefs =  pcSettings->GetSettings();
   	pcSetPrefs->FindColor32( "Back_Color", &zBgColor );
  	pcSetPrefs->FindColor32( "Icon_Color",   &zFgColor );
   	pcSetPrefs->FindString ( "DeskImage",  &zDImage  );
	pcSetPrefs->FindBool   ( "ShowVer",    &bShow   );
 	//pcSetPrefs->FindInt32  ( "SizeImage",  &nSizeImage);
   	pcSetPrefs->FindBool   ( "Alphabet",   &bAlphbt);
   	
   	Debug(zDImage.c_str());
	m_pcBitmap = ReadBitmap(zDImage.c_str());
    
}

/*
** name:       BitmapView::HandleMessage
** purpose:    Handles the messages from BitmapView.
** parameters: Message that holds data from the BitmapView
** returns:    
*/
void BitmapView::HandleMessage(Message* pcMessage)
{
    Alert* pcAlert;
    switch (pcMessage->GetCode())
    {

    case ID_ICON_PROPERTIES:
        {
            Window* pcIconProp = new IconProp(cIconName, cIconExec);
            pcIconProp->Show();
            pcIconProp->MakeFocus();
        }
        break;

    case M_PROPERTIES_SHOW:

        pcProp = new PropWin();
        pcProp->Show();
        pcProp->MakeFocus();

        break;

    case M_LOGOUT_USER:
    	{
    		GetWindow()->Hide();
    		Window* pcLoginWindow = new LoginWindow(CRect(470,195));
    		pcLoginWindow->Show();
    		pcLoginWindow->MakeFocus();
    		GetWindow()->Close();
        }
        break;

    case M_LOGOUT_FOCUS:
        break;

    case M_PROPERTIES_FOCUS:
        break;

    case M_SHOW_DRIVE_INFO:

        /*char info[1024];
        char vol_info[1024];

        sprintf(vol_info," %s info",m_drives.vol_name);
        sprintf(info,"Type:   %s    \n\nSize:  %s    \n\nUsed:   %s    \n\nAvailable:   %s    \n\nPercent Free:   %s    ",m_drives.zType, m_drives.zSize, m_drives.zUsed,m_drives.zAvail, m_drives.zPer );


        pcAlert  = new Alert(vol_info,info, 0, "OK", NULL );
        pcAlert->Go(new Invoker());*/
        break;

    case M_DRIVES_UNMOUNT:

        pcAlert = new Alert("Alert!!!","Unmount doesn't work yet!!!\n", 0, "OK",NULL );
        pcAlert->Go(new Invoker());

        break;


    default:
        View::HandleMessage(pcMessage);
        break;
    }
}






/*
** name:       BitmapWindow
** purpose:    Constructor for the BitmapWindow
** parameters: 
** returns:    
*/
BitmapWindow::BitmapWindow() : Window(Rect( 0, 0, 1599, 1199 ), "_bitmap_window", "Desktop", WND_NO_BORDER | WND_BACKMOST ,ALL_DESKTOPS )//,WND_BACKMOST || WND_NO_BORDER
{
	DeskSettings* pcSet = new DeskSettings();

   pcConfigChange = new NodeMonitor(pcSet->GetSetDir(),NWATCH_ALL,this);
   pcIconChange = new NodeMonitor(pcSet->GetIconDir(),NWATCH_ALL,this);
  
   pcBitmapView = new BitmapView( GetBounds());//Rect( 0, 0, 1599, 1199 ) );
   AddChild( pcBitmapView ); 
   //pcBitmapView->Paint(GetBounds());
}

/*
** name:       BitmapWindow::HandleMessage
** purpose:    Handles the message between the BitmapWindow
** parameters: Message passed from the BitmapWindow
** returns:    
*/
void BitmapWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    case M_CONFIG_CHANGE:
        pcBitmapView->ReadPrefs();
        pcBitmapView->Paint(pcBitmapView->GetBounds());
        pcBitmapView->Flush();
        pcBitmapView->Sync();
        
    case M_DESKTOP_CHANGE:
    	pcBitmapView->RemoveIcons();
    	pcBitmapView->SetIcons();
    	pcBitmapView->Flush();
    	pcBitmapView->Paint(pcBitmapView->GetBounds());
    	break;
    }
}






















