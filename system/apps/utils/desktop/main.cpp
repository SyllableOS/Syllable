#include "main.h"
#include "iconmenu_messages.h"

void UpdateLoginConfig(string zName)
{
	char junk[1024];
    char login_info[1024];
    char login_name[1024];
    
    ifstream filRead;
    filRead.open("/boot/atheos/sys/config/login.cfg");
    
    filRead.getline(junk,1024);
    filRead.getline((char*)login_info,1024);
    filRead.getline(junk,1024);
    filRead.getline(junk,1024);
    filRead.getline((char*)login_name,1024);
    filRead.close();
    
	
	FILE* fin;
    fin = fopen("/boot/atheos/sys/config/login.cfg","w");
    
    fprintf(fin,"<Login Name Option>\n");
    fprintf(fin, login_info);
    fprintf(fin, "\n\n<Login Name>\n");
    fprintf(fin,zName.c_str());
    fprintf(fin,"\n");
    fclose(fin);
}

void human( char* pzBuffer, off_t nValue )
{
    if ( nValue > (1024*1024*1024) ) {
	sprintf( pzBuffer, "%.1f Gb", ((double)nValue) / (1024.0*1024.0*1024.0) );
    } else if ( nValue > (1024*1024) ) {
	sprintf( pzBuffer, "%.1f Mb", ((double)nValue) / (1024.0*1024.0) );
    } else if ( nValue > 1024 ) {
	sprintf( pzBuffer, "%.1f Kb", ((double)nValue) / 1024.0 );
    } else {
	sprintf( pzBuffer, "%.1f b", ((double)nValue) );
    }
}

void SetConfigFileNames()
{
	sprintf(pzConfigFile,"%s/config/desktop/config/desktop.cfg",getenv("HOME"));
	sprintf(pzConfigDir, "%s/config/desktop/config/",getenv("HOME"));
	sprintf(pzImageDir, "%s/config/desktop/pictures/",getenv("HOME"));
}



void WriteConfigFile()
{
	Color32_s bColor(255,160,100,255);
	Color32_s fColor(0,0,0,0);
	
    Message *pcPrefs = new Message( );
	pcPrefs->AddColor32( "BackGround", bColor );
	pcPrefs->AddColor32( "IconText",   fColor );
	pcPrefs->AddString ( "DeskImage",  "logo_atheos.jpg"  );
	pcPrefs->AddBool   ( "ShowVer",    false);
	pcPrefs->AddInt32  ( "SizeImage",    0);
	
	
	File *pcConfig = new File(pzConfigFile, O_WRONLY | O_CREAT );  
	if( pcConfig ) {
		uint32 nSize = pcPrefs->GetFlattenedSize( );
		void *pBuffer = malloc( nSize );
		pcPrefs->Flatten( (uint8 *)pBuffer, nSize );
		
		pcConfig->Write( pBuffer, nSize );
		
		delete pcConfig;
		free( pBuffer );
	}
}

void LaunchFiles()
{
	std::vector<string> launch_files;
    string zName;
    string zPath;
    Directory* pcDir = new Directory();

    if(pcDir->SetTo("~/config/desktop/startup")==0){
        pcDir->GetPath(&zName);
        while (pcDir->GetNextEntry(&zName))
            if ( (zName.find( ".",0,1)==string::npos) && (zName.find( "Disabled",0)==string::npos))
            {
        
                launch_files.push_back(zName);
            }
		}
    for (uint32 n = 0; n < launch_files.size(); n++){


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
	

string SyllableInfo()
{
    string return_version;
	return_version = "Syllable 0.3.9, Desktop V0.3.5";
   	return (return_version);
}

mounted_drives mounteddrives()
{
	 mounted_drives d_drives;
	 
	 fs_info     fsInfo;
   
    int		 nMountCount;

    char        szTmp[1024];  char zSize[64];
    char        zUsed[64];   char zAvail[64];
   
	int x = get_mount_point_count();
    nMountCount = 0;
    for( int i = 0; i < x; i++ ){
      if( get_mount_point( i, szTmp, PATH_MAX ) < 0 )
	continue;
       
      int nFD = open( szTmp, O_RDONLY );
      if( nFD < 0 )
        continue;
      
      if( get_fs_info( nFD, &fsInfo ) >= 0 )
	nMountCount++;
       
      close( nFD );
    }
   
    for ( int i = 0 ; i < nMountCount ; ++i ) {
      if ( get_mount_point( i, szTmp, PATH_MAX ) < 0 ) {
      	
        continue;
      }
 
      int nFD = open( szTmp, O_RDONLY );
      if ( nFD < 0 ) {
        continue;
      }
      
         if ( get_fs_info( nFD, &fsInfo ) >= 0 ) {
        human( zSize, fsInfo.fi_total_blocks * fsInfo.fi_block_size );
        human( zUsed, (fsInfo.fi_total_blocks - fsInfo.fi_free_blocks) *fsInfo.fi_block_size );
        human( zAvail, fsInfo.fi_free_blocks * fsInfo.fi_block_size );
      }
      
      d_drives.vol_name = fsInfo.fi_volume_name;
      d_drives.zSize = zSize;
      d_drives.zUsed = zUsed;
      d_drives.zAvail = zAvail;
      d_drives.zType = fsInfo.fi_driver_name;
      
      sprintf(d_drives.zPer,"%.1f%%", ((double)fsInfo.fi_free_blocks / ((double)fsInfo.fi_total_blocks)) * 100.0);
      
      sprintf(d_drives.zMenu,"%s     ",fsInfo.fi_volume_name);  
      }
      
	return(d_drives);
}


IPoint ScreenRes()
{
	Desktop d_desk;
	return d_desk.GetResolution();
}


Bitmap* ReadBitmap(const char* zImageName)
{
	ifstream fs_image;
    Bitmap* pcBitmap = NULL;
    string zImagePath = pzImageDir + (string)zImageName;
	fs_image.open(zImagePath.c_str());

    if(fs_image == NULL)
    {
        fs_image.close();
        pcBitmap = LoadBitmapFromResource("logo_atheos.jpg");
       
    }

    else
    {
        fs_image.close();
        
       
       
        if (nSizeImage == 0){
        Bitmap* pcLargeBitmap = new Bitmap(ScreenRes().x, ScreenRes().y, CS_RGB32,Bitmap::SHARE_FRAMEBUFFER);
        Scale(LoadBitmapFromFile(zImagePath.c_str()), pcLargeBitmap, filter_mitchell, 0);
        pcBitmap = pcLargeBitmap;
        
        }else {
        pcBitmap = LoadBitmapFromFile(zImagePath.c_str() ); }
	}
	
	return (pcBitmap);
}


void CheckConfig()
{
	ifstream filestr;
    filestr.open(pzConfigFile);

    if(filestr == NULL)
    {
        filestr.close();
        WriteConfigFile();
    }

    else
    {
        filestr.close();
    }
}


void SetDefaults()
{
	system("mkdir ~/config 2> /dev/null");
    system("mkdir ~/config/desktop 2> /dev/null");
    system("mkdir ~/config/desktop/config 2> /dev/null");
    system("mkdir ~/config/desktop/startup 2> /dev/null");
    system("mkdir ~/config/desktop/pictures 2> /dev/null");
    system("mkdir ~/config/desktop/disks 2> /dev/null");
    
    CheckConfig();
}


bool ReadShowVer()
{
	char junk[1024];
	char version[1024];
	bool return_version = false;
	ifstream fileRead;
    fileRead.open(pzConfigFile);
    
 	for (int i = 0; i<=9; i++){
        fileRead.getline(junk,1024);
    }
    
    fileRead.getline(version,1024);
    
    if (strcmp(version,"true")==0){return_version = true;}
    
    return (return_version);
}


void Icon::Select( BitmapView* pcView, bool bSelected )
{
    if ( m_bSelected == bSelected ) {
        return;
    }
    m_bSelected = bSelected;

    pcView->Erase( GetFrame( pcView->GetFont() ) );

    Paint( pcView, Point(0,0), true, true, pcView->zBgColor, pcView->zFgColor );
}

BitmapView::BitmapView( const Rect& cFrame ) :
        View( cFrame, "_bitmap_view", CF_FOLLOW_ALL)
{
	
	m_drives = mounteddrives();
    pcMainMenu = new Menu(Rect(0,0,0,0),"",ITEMS_IN_COLUMN);
    pcMountMenu = new Menu(Rect(0,0,0,0),"Mount Drives    ",ITEMS_IN_COLUMN);
    
    Menu* pcAtheMenu = new Menu(Rect(0,0,0,0), m_drives.zMenu,ITEMS_IN_COLUMN);
    pcAtheMenu->AddItem("Show Info...",new Message(M_SHOW_DRIVE_INFO));
    pcAtheMenu->AddItem("Unmount...", new Message(M_DRIVES_UNMOUNT));   
    pcMountMenu->AddItem(new ImageItem(pcAtheMenu,NULL,LoadBitmapFromResource("mount.png")));
    
    pcMountMenu->AddItem(new MenuSeparator());
    pcMountMenu->AddItem("Settings...",NULL);
    
    pcMainMenu->AddItem(new ImageItem(pcMountMenu,NULL,LoadBitmapFromResource("mount.png")));
    
    pcMainMenu->AddItem(new MenuSeparator());
    pcMainMenu->AddItem(new ImageItem("Properties", new Message(M_PROPERTIES_SHOW),"", LoadBitmapFromResource("kcontrol.png")));
    pcMainMenu->AddItem(new MenuSeparator());
    pcMainMenu->AddItem(new ImageItem("Logout",new Message(M_LOGOUT_SHOW),"", LoadBitmapFromResource("exit.png")));
	pcMainMenu->GetPreferredSize(true);
	
	pcIconMenu = new IconMenu();
	pcIconMenu->SetTargetForItems(this); 
	
    struct stat sStat;

    m_cIcons.push_back( new Icon("Home","/system/icons/root.icon", sStat ) );
    m_cIcons.push_back( new Icon( "ABrowse", "/system/icons/ABrowse.icon", sStat ) );
    m_cIcons.push_back( new Icon( "Terminal", "/system/icons/terminal.icon", sStat ) );

    m_bCanDrag = false;
    m_bSelRectActive = false;

    Point cPos( 20, 20 );
    
    ReadPrefs();
   
    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        m_cIcons[i]->m_cPosition.x = cPos.x + 16;
        m_cIcons[i]->m_cPosition.y = cPos.y;

        cPos.y += 50;
        if ( cPos.y > 500 ) {
            cPos.y = 20;
            cPos.x += 50;
        
	m_cIcons[i]->Paint( this, Point(0,0), true, true, zBgColor, zFgColor );
	
	}
    }
    m_nHitTime = 0;
    
    
    
    pzSyllableVer = SyllableInfo();
   
    
   
}

BitmapView::~BitmapView()
{
    delete m_pcBitmap;
}

void BitmapView::Paint( const Rect& cUpdateRect)
{
    Rect cBounds = GetBounds();
    SetDrawingMode( DM_COPY );

    Font* pcFont = GetFont();

    Erase( cUpdateRect );

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cUpdateRect ) ) {
            m_cIcons[i]->Paint( this, Point(0,0), true, true, zBgColor, zFgColor );
        }
    }
    
    
    if (bShow == true){
    	SetFgColor(0,0,0);
    	SetDrawingMode(DM_BLEND);
    	MovePenTo(ScreenRes().x - 15 - GetStringWidth(pzSyllableVer) , ScreenRes().y -10);
    	DrawString(pzSyllableVer);
    	}
}

void BitmapView::Erase( const Rect& cFrame )
{
	
    if ( m_pcBitmap != NULL ) {
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
    } else {
        FillRect( cFrame, Color32_s( 0x00, 0x00, 0x00, 0 ) );
    }
}

Icon* BitmapView::FindIcon( const Point& cPos )
{
    Font* pcFont = GetFont();

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cPos ) ) {
            return( m_cIcons[i] );
        }
    }
    return( NULL );
}

void BitmapView::MouseDown( const Point& cPosition, uint32 nButtons )
{
    MakeFocus( true );

    Icon* pcIcon = FindIcon( cPosition );
    if(nButtons ==1){
        if ( pcIcon != NULL )
        {
            if (  pcIcon->m_bSelected ) {
                if ( m_nHitTime + 500000 >= get_system_time() ) {
                    if ( pcIcon->GetName() == "Home" ) {

                        pid_t nPid = fork();
                        if ( nPid == 0 ) {
                            set_thread_priority( -1, 0 );
                            execlp( "LBrowser", "LBrowser",getenv("HOME"), NULL );
                            exit( 1 );
                        }
                    } else  if ( pcIcon->GetName() == "Terminal" ) {
                        pid_t nPid = fork();
                        if ( nPid == 0 ) {
                            set_thread_priority( -1, 0 );
                            execlp( "aterm", "aterm", "-i", NULL );
                            exit( 1 );
                        }
                    } else  if ( pcIcon->GetName() == "ABrowse" ) {
                        pid_t nPid = fork();
                        if ( nPid == 0 ) {
                            set_thread_priority( -1, 0 );
                            execlp( "/Applications/ABrowse/ABrowse", "ABrowse", NULL );
                            exit( 1 );


                        }
                    }

                } else {
                    m_bCanDrag = true;
                }
                m_nHitTime = get_system_time();
                return;
            }
        }
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
            m_cIcons[i]->Select( this, false );
        }
        if ( pcIcon != NULL ) {
            m_bCanDrag = true;
            pcIcon->Select( this, true );
        } else {
            m_bSelRectActive = true;
            m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
            SetDrawingMode( DM_INVERT );
            DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        }
        Flush();
        m_cLastPos = cPosition;
        m_nHitTime = get_system_time();
    }

    else if (nButtons == 2){
    	
    	if (pcIcon !=NULL){
    		
    	 	m_bSelRectActive = true;
            m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
            SetDrawingMode( DM_INVERT );
            DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
            pcIcon->Select( this,true );
            Flush();
            
             if (  pcIcon->m_bSelected )
             	cIconName = pcIcon->GetName(); 
             	
            	pcIconMenu->Open(ConvertToScreen(cPosition));
            	pcIconMenu->SetTargetForItems(this);
            }	
            	
          else if(pcIcon == NULL){
          	pcMainMenu->Open(ConvertToScreen(cPosition));
          	pcMainMenu->SetTargetForItems(this);
          	 }
            	
}
}


void BitmapView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    m_bCanDrag = false;

    Font* pcFont = GetFont();
    if ( m_bSelRectActive ) {
        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_bSelRectActive = false;

        if ( m_cSelRect.left > m_cSelRect.right ) {
            float nTmp = m_cSelRect.left;
            m_cSelRect.left = m_cSelRect.right;
            m_cSelRect.right = nTmp;
        }
        if ( m_cSelRect.top > m_cSelRect.bottom ) {
            float nTmp = m_cSelRect.top;
            m_cSelRect.top = m_cSelRect.bottom;
            m_cSelRect.bottom = nTmp;
        }

        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
            m_cIcons[i]->Select( this, m_cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
        }
        Flush();
    } else if ( pcData != NULL && pcData->ReturnAddress() == Messenger( this ) ) {
        Point cHotSpot;
        pcData->FindPoint( "_hot_spot", &cHotSpot );


        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
            Rect cFrame = m_cIcons[i]->GetFrame( pcFont );
            Erase( cFrame );
        }
        Flush();
        
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
            if ( m_cIcons[i]->m_bSelected ) {
                m_cIcons[i]->m_cPosition += cPosition - m_cDragStartPos;
            }
            m_cIcons[i]->Paint( this, Point(0,0), true, true, zBgColor, zFgColor );
        }
        Flush();
    }
}

void BitmapView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    m_cLastPos = cNewPos;

    if ( (nButtons & 0x01) == 0 ) {
        return;
    }

    if ( m_bSelRectActive ) {
        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_cSelRect.right = cNewPos.x;
        m_cSelRect.bottom = cNewPos.y;

        Rect cSelRect = m_cSelRect;
        if ( cSelRect.left > cSelRect.right ) {
            float nTmp = cSelRect.left;
            cSelRect.left = cSelRect.right;
            cSelRect.right = nTmp;
        }
        if ( cSelRect.top > cSelRect.bottom ) {
            float nTmp = cSelRect.top;
            cSelRect.top = cSelRect.bottom;
            cSelRect.bottom = nTmp;
        }
        Font* pcFont = GetFont();
        SetDrawingMode( DM_COPY );
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
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
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
            if ( m_cIcons[i]->m_bSelected ) {
                cData.AddString( "file/path", m_cIcons[i]->GetName().c_str() );
                cSelFrame |= m_cIcons[i]->GetFrame( pcFont );
                pcSelIcon = m_cIcons[i];
            }
        }
        if ( pcSelIcon != NULL ) {
            m_cDragStartPos = cNewPos; // + cSelFrame.LeftTop() - cNewPos;

            if ( (cSelFrame.Width()+1.0f) * (cSelFrame.Height()+1.0f) < 12000 )
            {
                Bitmap cDragBitmap( cSelFrame.Width()+1.0f, cSelFrame.Height()+1.0f, CS_RGB32, Bitmap::ACCEPT_VIEWS | Bitmap::SHARE_FRAMEBUFFER );

                View* pcView = new View( cSelFrame.Bounds(), "" );
                cDragBitmap.AddChild( pcView );

                pcView->SetFgColor( 255, 255, 255, 0 );
                pcView->FillRect( cSelFrame.Bounds() );


                for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
                    if ( m_cIcons[i]->m_bSelected ) {
                        m_cIcons[i]->Paint( pcView, -cSelFrame.LeftTop(), true, false, zBgColor, zFgColor );
                    }
                }
                cDragBitmap.Sync();

                uint32* pRaster = (uint32*)cDragBitmap.LockRaster();

                for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y ) {
                    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x ) {
                        if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0x00ffffff &&
                                (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0xff000000 ) {
                            pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0xb0000000;
                        }
                    }
                }
                BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), &cDragBitmap );

            } else {
                BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), cSelFrame.Bounds() );
            }
        }
        m_bCanDrag = false;
    }
    Flush();
}

void BitmapView::ReadPrefs(void)
{
	FSNode *pcNode = new FSNode();
	if( pcNode->SetTo(pzConfigFile) == 0 ) {
		File *pcConfig = new File( *pcNode );
		uint32 nSize = pcConfig->GetSize( );
		void *pBuffer = malloc( nSize );
		pcConfig->Read( pBuffer, nSize );
		Message *pcPrefs = new Message( );
		pcPrefs->Unflatten( (uint8 *)pBuffer );
		
		pcPrefs->FindColor32( "BackGround", &zBgColor );
		pcPrefs->FindColor32( "IconText",   &zFgColor );
		pcPrefs->FindString ( "DeskImage",  &zDImage  );
		pcPrefs->FindBool   ( "ShowVer",    &bShow   );
		pcPrefs->FindInt32    ( "SizeImage",  &nSizeImage);
		
		m_pcBitmap = ReadBitmap(zDImage.c_str());
}
}

void BitmapView::HandleMessage(Message* pcMessage)
{
	Alert* pcAlert;
    switch (pcMessage->GetCode())
    {
    	
    case ID_ICON_PROPERTIES:
    	{
    		Window* pcIconProp = new IconProp(cIconName);
    		pcIconProp->Show();
    		pcIconProp->MakeFocus();
    	}
    	//printf("\nIt works!!!\n");
    	break;
    	
    case M_PROPERTIES_SHOW:
        
	   		pcProp = new PropWin();
	   		pcProp->Show();
	   		pcProp->MakeFocus();
		
			break;
       
     case M_LOGOUT_SHOW:
	 	break;
       
    case M_LOGOUT_FOCUS:
    	break;
    	
    case M_PROPERTIES_FOCUS:
    	break;
    	
    case M_SHOW_DRIVE_INFO:
    	
    	    char info[1024];
    	    char vol_info[1024];
    	    
    	    sprintf(vol_info," %s info",m_drives.vol_name);
    		sprintf(info,"Type:   %s    \n\nSize:  %s    \n\nUsed:   %s    \n\nAvailable:   %s    \n\nPercent Free:   %s    ",m_drives.zType, m_drives.zSize, m_drives.zUsed,m_drives.zAvail, m_drives.zPer );
    		
    		
    		pcAlert  = new Alert(vol_info,info, 0, "OK", NULL );
                    pcAlert->Go(new Invoker());
    	break;
    	
    case M_DRIVES_UNMOUNT:
    		
    		    pcAlert = new Alert("Alert!!!","Unmount doesn't work yet!!!\n", 0, "OK", NULL );
                pcAlert->Go(new Invoker());
            
        break;
        
    
      default:
      	View::HandleMessage(pcMessage);
      	break;              
    }
}




static void authorize( const char* pzLoginName )
{
    for (;;)
    {
        std::string cName;
        std::string cPassword;

        if ( pzLoginName != NULL || get_login( &cName, &cPassword ) )
        {
            struct passwd* psPass;

            if ( pzLoginName != NULL ) {
                psPass = getpwnam( pzLoginName );
            } else {
                psPass = getpwnam( cName.c_str() );
            }

            if ( psPass != NULL ) {
                const char* pzPassWd = crypt(  cPassword.c_str(), "$1$" );

                if ( pzLoginName == NULL && pzPassWd == NULL ) {
                    perror( "crypt()" );
                    pzLoginName = NULL;
                    continue;
                }

                if ( pzLoginName != NULL || strcmp( pzPassWd, psPass->pw_passwd ) == 0 ) {
                    setgid( psPass->pw_gid );
                    setuid( psPass->pw_uid );
                    setenv( "HOME", psPass->pw_dir, true );
                    chdir( psPass->pw_dir );
                    SetConfigFileNames();
                    SetDefaults();
                    UpdateLoginConfig(cName);
                   
                    break;
                } else {
                    Alert* pcAlert = new Alert( "Login failed:",  "Incorrect password!!!", 0, "Sorry", NULL );
                    pcAlert->Go();
                }
            } else {

                Alert* pcAlert = new Alert( "Login failed:",  "No such user!!!", 0, "Sorry", NULL );
                pcAlert->Go();
            }
        }
        pzLoginName = NULL;
    }

}



BitmapWindow::BitmapWindow() : Window(Rect( 0, 0, 1599, 1199 ), "_bitmap_window", "",WND_NO_BORDER | WND_BACKMOST, ALL_DESKTOPS )
{
	pcBitmapView = new BitmapView( Rect( 0, 0, 1599, 1199 ) );
	AddChild( pcBitmapView );
	pcConfigChange = new NodeMonitor(pzConfigDir,NWATCH_ALL,this);
}

void BitmapWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_CONFIG_CHANGE:
				pcBitmapView->ReadPrefs();
				pcBitmapView->Paint(pcBitmapView->GetBounds());
    			pcBitmapView->Flush();
    			pcBitmapView->Sync();
		break;
	}
}


int main( int argc, char** argv )
{
	
	Application* pcApp = new Application( "application/x-vnd.RGC-desktop_manager" );
    
    if ( getuid() == 0 ) {
        const char* pzLoginName = NULL;

        if ( argc == 2 ) {
            pzLoginName = argv[1];
        }
        authorize( pzLoginName );
    }

    
    BitmapWindow* pcBitmapWindow = new BitmapWindow();
    pcBitmapWindow->Show();		
	LaunchFiles();
  	
  	pcApp->Run();
  	
	return( 0 );
}











































































