#include "iconview.h"
#include "loadbitmap.h"
Bitmap* Icon::s_pcBitmap[16] = {NULL,};
int	Icon::s_nCurBitmap = 0;
Color32_s fgColor, bgColor;

Icon::Icon( const char* pzTitle, const char* pzPath, const char* pzExec, Point cPoint, const struct stat& sStat ) : m_cTitle( pzTitle)
{
    m_bSelected = false;
    m_sStat     = sStat;
    m_bBoundsValid = false;
    m_bStrWidthValid = false;
    m_cExec = pzExec;
    m_cPosition = cPoint;
    
    if ( s_pcBitmap[0] == NULL ) {
        for ( int i = 0 ; i < 16 ; ++i ) {
            s_pcBitmap[i] = new Bitmap( 32,32, CS_RGB32 );
        }
    }
    

    IconDir sDir;
    IconHeader sHeader;
	
    FILE* hFile = fopen( pzPath, "r" );

    if ( hFile == NULL ) {
        printf( "Failed to open file %s\n", pzPath );
        return;
    }

    if ( fread( &sDir, sizeof( sDir ), 1, hFile ) != 1 ) {
        printf( "Failed to read icon dir\n" );
    }
    if ( sDir.nIconMagic != ICON_MAGIC ) {
        printf( "Files %s is not an icon\n", pzPath );
        return;
    }
    for ( int i = 0 ; i < sDir.nNumImages ; ++i ) {
         if ( fread( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 ) {
            printf( "Failed to read icon header\n" );
        }
        if ( sHeader.nWidth == 32 ) {
            fread( m_anLarge, 32*32*4, 1, hFile );
        } else if ( sHeader.nWidth == 16 ) {
            fread( m_anSmall, 16*16*4, 1, hFile );
        }
    }
    fclose( hFile );
}

Icon::~Icon()
{
}


void Icon::Select( IconView* pcView, bool bSelected )
{
    if ( m_bSelected == bSelected ) {
        return;
    }
    m_bSelected = bSelected;

    pcView->Erase( GetFrame( pcView->GetFont() ) );

    Paint( pcView, Point(0,0), true, true, bgColor,fgColor );
}

float Icon::GetStrWidth( Font* pcFont )
{
    if ( m_bStrWidthValid == false ) {
        m_nMaxStrLen = pcFont->GetStringLength( m_cTitle.c_str(), 72.0f );
        m_vStringWidth = pcFont->GetStringWidth( m_cTitle.c_str(), m_nMaxStrLen );
        m_bStrWidthValid = true;
    }
    return( m_vStringWidth );
}

Rect Icon::GetBounds( Font* pcFont )
{
    if ( m_bBoundsValid == false ) {
        font_height sHeight;

        pcFont->GetHeight( &sHeight );

        float vStrWidth = max( 32.0f, GetStrWidth( pcFont ) + 4 );
        m_cBounds = Rect( 0, 0, vStrWidth, 32 + 2 + sHeight.ascender + sHeight.descender );
        m_bBoundsValid = true;
    }
    return( m_cBounds );
}

Rect Icon::GetFrame( Font* pcFont )
{
    Rect cBounds = GetBounds( pcFont );
    return( cBounds + m_cPosition - Point( cBounds.Width() * 0.5f, 0.0f ) );
}

void Icon::Paint( View* pcView, const Point& cOffset, bool bLarge, bool bBlendText, Color32_s bClr,Color32_s fClr )
{
	bgColor = bClr;
	fgColor = fClr;
    Point cPosition = m_cPosition + cOffset;
    if ( bLarge )
    {
        Font* pcFont = pcView->GetFont();
        if ( pcFont == NULL ) {
            return;
        }
        float vStrWidth = GetStrWidth( pcFont );
        font_height sHeight;

        pcFont->GetHeight( &sHeight );

        float x = cPosition.x - vStrWidth / 2.0f;
        float y = cPosition.y + 32.0f + sHeight.ascender + 2.0f;


        pcView->MovePenTo( x, y );

        if ( bBlendText ) {
            pcView->SetDrawingMode( DM_OVER );
        } else {
            pcView->SetDrawingMode( DM_BLEND );
            pcView->SetBgColor(200,200,200);  //atoi (Colors.bgColor.red), atoi(Colors.bgColor.green), atoi(Colors.bgColor.blue),255
        }
        if ( m_bSelected && bBlendText ) {
            pcView->SetFgColor(bClr);
            Rect cRect( x - 2, y - sHeight.ascender,
                        x + vStrWidth + 1, y + sHeight.descender );
            pcView->FillRect( cRect );
        }

        //    pcView->SetFgColor( 0, 0, 0 );
        if ( bBlendText ) {
            pcView->SetFgColor( fClr );
        } else {
            pcView->SetFgColor( 255,255,255, 0xff );
        }

        pcView->DrawString( m_cTitle.c_str(), m_nMaxStrLen );

        pcView->SetDrawingMode( DM_BLEND );

        if ( s_nCurBitmap == 16 ) {
            s_nCurBitmap = 0;
            pcView->Sync();
        }
        memcpy( s_pcBitmap[s_nCurBitmap]->LockRaster(), m_anLarge, 32*32 * 4 );
        pcView->DrawBitmap( s_pcBitmap[s_nCurBitmap], s_pcBitmap[s_nCurBitmap]->GetBounds(),
                            s_pcBitmap[s_nCurBitmap]->GetBounds() + (cPosition - Point( 16, 0 )) );
        s_nCurBitmap++;
    }
}

Bitmap* Icon::GetBitmap()
{
    if ( s_nCurBitmap == 16 ) {
        s_nCurBitmap = 0;
    }
    memcpy( s_pcBitmap[s_nCurBitmap]->LockRaster(), m_anLarge, 32*32 * 4 );

    return( s_pcBitmap[s_nCurBitmap++] );
}

IconView::IconView( const Rect& cFrame, const char* pzPath, Bitmap* pcBitmap ) :
        View( cFrame, "_bitmap_view",CF_FOLLOW_ALL ), m_cPath( pzPath )
{
    m_pcBitmap = pcBitmap;
    m_bCanDrag = false;
    m_bSelRectActive = false;
    m_pcDirChangeMsg = NULL;
    m_pcCurReadDirSession = NULL;
    m_nHitTime = 0;
}

IconView::~IconView()
{
    delete m_pcDirChangeMsg;
}

void IconView::AttachedToWindow()
{
    View::AttachedToWindow();

    SetTarget( GetWindow() );
    ReRead();
}

void IconView::SetPath( const std::string& cPath )
{
    m_cPath = cPath.c_str();
}

std::string IconView::GetPath()
{
    return( std::string( m_cPath.GetPath() ) );
}

void IconView::LayoutIcons()
{
    Rect cFrame = GetBounds();

    Point cPos( 20, 20 );

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        m_cIcons[i]->m_cPosition.x = cPos.x + 100; //16
        m_cIcons[i]->m_cPosition.y = cPos.y + 100;//not

        Rect cIconFrame = m_cIcons[i]->GetFrame( GetFont() );

        cPos.x += cIconFrame.Width() + 1.0f + 16.0f;


        if ( cPos.x > cFrame.right - 50 ) {
            cPos.x = 20;
            cPos.y += 50;
        }
        m_cIcons[i]->Paint( this, Point(0,0), true, true, bgColor,fgColor  );
    }
    Flush();
}

int32 IconView::ReadDirectory( void* pData )
{
    ReadDirParam* pcParam = (ReadDirParam*) pData;
    IconView* pcView = pcParam->m_pcView;

    Window* pcWnd = pcView->GetWindow();

    if ( pcWnd == NULL ) {
        return(0);
    }

    pcWnd->Lock();
    for ( uint i = 0 ; i < pcView->m_cIcons.size() ; ++i ) {
        pcView->Erase( pcView->m_cIcons[i]->GetFrame( pcView->GetFont() ) );
        delete pcView->m_cIcons[i];
    }
    pcView->m_cIcons.clear();

    pcWnd->Unlock();

    DIR* pDir = opendir( pcView->GetPath().c_str() );
    if ( pDir == NULL ) {
        dbprintf( "Error: IconView::ReadDirectory() Failed to open %s\n", pcView->GetPath().c_str() );
        goto error;
    }
    struct dirent* psEnt;

    while( pcParam->m_bCancel == false && (psEnt = readdir( pDir ) ) != NULL )
    {
        struct stat sStat;
        if ( strcmp( psEnt->d_name, "." ) == 0 || strcmp( psEnt->d_name, ".." ) == 0 ) {
            continue;
        }
        Path cFilePath( pcView->GetPath().c_str() );
        cFilePath.Append( psEnt->d_name );

        stat( cFilePath.GetPath(), &sStat );

        Icon* pcIcon;

        /*if ( S_ISDIR( sStat.st_mode ) == false ) {
            pcIcon = new Icon( psEnt->d_name, "/system/icons/file.icon", NULL, , sStat );
        } else {
            pcIcon = new Icon( psEnt->d_name, "/system/icons/folder.icon", NULL, NULL, sStat );
        }*/

        pcWnd->Lock();
        pcView->m_cIcons.push_back( pcIcon );
        pcWnd->Unlock();
    }

    closedir( pDir );
error:
    pcWnd->Lock();
    if ( pcView->m_pcCurReadDirSession == pcParam ) {
        pcView->m_pcCurReadDirSession = NULL;
    }

    pcView->LayoutIcons();

    pcWnd->Unlock();
    delete pcParam;
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::ReRead()
{
    if ( m_pcCurReadDirSession != NULL ) {
        m_pcCurReadDirSession->m_bCancel = true;
    }
    m_pcCurReadDirSession = new ReadDirParam( this );

    thread_id hTread = spawn_thread( "read_dir_thread", ReadDirectory, 0, 0, m_pcCurReadDirSession );
    if ( hTread >= 0 ) {
        resume_thread( hTread );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();
    Font* pcFont = GetFont();

    Erase( cUpdateRect );

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cUpdateRect ) ) {
            m_cIcons[i]->Paint( this, Point(0,0), true, true, bgColor,fgColor  );
        }
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::Erase( const Rect& cFrame )
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
        FillRect( cFrame, Color32_s( 255, 255, 255, 0 ) );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Icon* IconView::FindIcon( const Point& cPos )
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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    char nChar = pzString[0];

    if ( isprint( nChar ) )
    {
    }
    else
    {
        switch( pzString[0] )
        {
            /*
                  case VK_DELETE:
                  {
            	std::vector<std::string> cPaths;
            	for ( int i = GetFirstSelected() ; i <= GetLastSelected() ; ++i ) {
            	  if ( IsSelected( i ) ) {
            	    FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(i));
            	    if ( pcRow != NULL ) {
            	      Path cPath = m_cPath;
            	      cPath.Append( pcRow->m_cName.c_str() );
            	      cPaths.push_back( cPath.GetPath() );
            	    }
            	  }
            	}
            	StartFileDelete( cPaths, Messenger( this ) );
            	break;
                  }
            	*/	
        case VK_BACKSPACE:
            m_cPath.Append( ".." );
            ReRead();
            //	PopState();
            DirChanged( m_cPath.GetPath() );
            break;
        case VK_FUNCTION_KEY:
            {
                Looper* pcLooper = GetLooper();
                assert( pcLooper != NULL );
                Message* pcMsg = pcLooper->GetCurrentMessage();
                assert( pcMsg != NULL );

                int32 nKeyCode;
                pcMsg->FindInt32( "_raw_key", &nKeyCode );

                switch( nKeyCode )
                {
                case 6: // F5
                    ReRead();
                    break;
                }
                break;
            }
        default:
            View::KeyDown( pzString, pzRawString, nQualifiers );
            break;
        }
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::MouseDown( const Point& cPosition, uint32 nButtons )
{
    MakeFocus( true );

    Icon* pcIcon = FindIcon( cPosition );
    if(nButtons == 1){

        if ( pcIcon != NULL )
        {
            if (  pcIcon->m_bSelected ) {
                if ( m_nHitTime + 500000 >= get_system_time() ) {
                    Invoked();
                } 
                else {
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
    
   //if (nButtons == 2){
    	//Invoked();
          //  }
    //
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
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
            m_cIcons[i]->Paint( this, Point(0,0), true, true, bgColor,fgColor  );
        }
        Flush();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
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
                Bitmap cDragBitmap( cSelFrame.Width() + 1.0f, cSelFrame.Height() + 1.0f, CS_RGB32,
                                    Bitmap::ACCEPT_VIEWS | Bitmap::SHARE_FRAMEBUFFER );

                View* pcView = new View( cSelFrame.Bounds(), "" );
                cDragBitmap.AddChild( pcView );

                pcView->SetFgColor( 255, 255, 255, 255 );
                pcView->FillRect( cSelFrame.Bounds() );


                for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
                    if ( m_cIcons[i]->m_bSelected ) {
                        m_cIcons[i]->Paint( pcView, -cSelFrame.LeftTop(), true, false, bgColor,fgColor  );
                    }
                }

                cDragBitmap.Sync();

                uint32* pRaster = (uint32*)cDragBitmap.LockRaster();

                for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y ) {
                    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x ) {
                        if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0xffffffff &&
                                (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0x00000000 ) {
                            pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0x50000000;
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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::Invoked()
{
    Icon* pcIcon = NULL;

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        if ( m_cIcons[i]->IsSelected() ) {
            pcIcon = m_cIcons[i];
            break;
        }
    }
    if ( pcIcon == NULL ) {
        return;
    }
    if ( S_ISDIR( pcIcon->m_sStat.st_mode ) == false ) {
        //    ListView::Invoked( nFirstRow, nLastRow );
        return;
    }

    //  m_cStack.push( State( this, m_cPath.GetPath() ) );
    m_cPath.Append( pcIcon->GetName().c_str() );

    ReRead();

    //  if ( bBack ) {
    //    PopState();
    //  }
    DirChanged( m_cPath.GetPath() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::SetDirChangeMsg( Message* pcMsg )
{
    delete m_pcDirChangeMsg;
    m_pcDirChangeMsg = pcMsg;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::DirChanged( const std::string& cNewPath )
{
    if ( m_pcDirChangeMsg != NULL ) {
        Invoke( m_pcDirChangeMsg );
    }

}











