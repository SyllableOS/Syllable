/*
    launcher_plugin - A plugin interface for the AtheOS Launcher
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    See ../../include/COPYING
*/


#include "../../include/launcher_plugin.h"
#include "../../include/launcher_mime.h"


  // Center a frame in the screen
os::Rect center_frame( const os::Rect &cFrame )
{
	Rect cNewFrame;
	Desktop *pcDesktop = new Desktop( );
	IPoint *pcRes;
	pcRes = new IPoint( pcDesktop->GetResolution( ) );
	
	float vHalfWidth = cFrame.Width() / 2;
	float vHalfHeight = cFrame.Height() / 2;
	
	cNewFrame.left =   (pcRes->x / 2) - vHalfWidth;
	cNewFrame.top =    (pcRes->y / 2) - vHalfHeight;
	cNewFrame.right =  (pcRes->x / 2) + vHalfWidth;
	cNewFrame.bottom = (pcRes->y / 2) + vHalfHeight;
	
	return cNewFrame;
}


  // Make sure a frame is entirely within the screen (unless it's too big)
os::Rect frame_in_screen( const os::Rect &cFrame )
{
	Rect cNewFrame = cFrame;
	Desktop *pcDesktop = new Desktop( );
	IPoint *pcRes;
	pcRes = new IPoint( pcDesktop->GetResolution( ) );

	float vWidth = cFrame.Width();
	float vHeight = cFrame.Height();
	
	if( cNewFrame.left < 0 ) {
		cNewFrame.left = 0;
		cNewFrame.right = vWidth;
	}
	
	if( cNewFrame.top < 0 ) {
		cNewFrame.top = 0;
		cNewFrame.bottom = vHeight;
	}
	
	if( cNewFrame.right > pcRes->x ) {
		cNewFrame.left = pcRes->x - vWidth;
		cNewFrame.right = pcRes->x;
	}

	if( cNewFrame.bottom > pcRes->y ) {
		cNewFrame.top = pcRes->y - vHeight;
		cNewFrame.bottom = pcRes->y;
	}
	
	return cNewFrame;
}


	// Convert to an absolute path (resolve ~/^)
std::string convert_path( std::string zPath )
{
	string zTmpPath = "";	
	
	if( zPath.length() > 0 ) {
		int n = 0;
		while( zPath.find( "/", n ) != string::npos ) {
			n = zPath.find( "/", n ) + 1;
		}	
		zTmpPath = zPath.substr( 0, n );
	
		Directory *pcDir = new Directory( );
		if( pcDir->SetTo( zTmpPath.c_str() ) == 0 ) {
			pcDir->GetPath( &zTmpPath );
			zTmpPath = zTmpPath + (string)"/" + zPath.substr( n );
		}
			
		delete pcDir;
	}
		
	return zTmpPath;
}


	// Return a path to an icon
std::string get_icon( std::string zPath )
{
	FSNode *pcNode = new FSNode( zPath );
	string zIconPath = "";
	char zIcon[512];
	int n = pcNode->ReadAttr( "LAUNCHER_ICON", ATTR_TYPE_STRING, &zIcon, 0, 511 );
	if( n > 0 ) {
		zIcon[n] = 0;
		zIconPath = zIcon;
	} else {
		MimeInfo *pcMimeInfo = new MimeInfo();
		if( pcMimeInfo->FindMimeType( zPath ) == 0 ) {
			zIconPath = pcMimeInfo->GetDefaultIcon();
		} else {
			zIconPath = "/Applications/Launcher/icons/file.png";
		}
	}
	
	delete pcNode;
	return zIconPath;
}



std::string get_launcher_path( void )
{
	static string zLauncherPath;
	
	if( zLauncherPath == "" ) {
		zLauncherPath = convert_path( "^/" );
		zLauncherPath = zLauncherPath.substr( 0, zLauncherPath.length()-4 );
	}
	
	return zLauncherPath;
}


//*************************************************************************************

/*
** name:       LauncherPlugin
** purpose:    Provides an interface between the LauncherWindow and the plugin itself.
** parameters: A string containing the name of the plugin and a prefs message to pass to it.
** returns:    none.
*/
LauncherPlugin::LauncherPlugin( string zName, LauncherMessage *pcPrefs )
{  

  int nError = 0;

  m_nId = INIT_FAIL;
  m_nHideStatus = LW_VISIBLE_ALWAYS;

  m_zName = zName;

  pcPrefs->SetPlugin( this );
  
  pcPrefs->FindPointer( "__WINDOW", (void **)&m_pcWindow );
  pcPrefs->FindInt32( "HideStatus", &m_nHideStatus );
  
  string zPluginPath = get_launcher_path() + (string)"plugins/" + zName + (string)".so";
  m_nLibHandle = load_library( zPluginPath.c_str( ), 0 );
  if( m_nLibHandle > 0 )
  {
    int nErr = get_symbol_address( m_nLibHandle, "init", -1, (void**)&m_pfInit );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For init\n" ); nError = 1; }

    nErr = get_symbol_address( m_nLibHandle, "finish", -1, (void**)&m_pfDelete );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For finish\n" ); nError = 1; }
    
    nErr = get_symbol_address( m_nLibHandle, "get_view", -1, (void**)&m_pfGetView );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For get_view\n" ); nError = 1; }
    
    nErr = get_symbol_address( m_nLibHandle, "remove_view", -1, (void**)&m_pfRemoveView );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For remove_view\n" ); nError = 1; }    
    
    nErr = get_symbol_address( m_nLibHandle, "get_prefs", -1, (void**)&m_pfGetPrefs );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For get_prefs\n" ); nError = 1; }
    
    nErr = get_symbol_address( m_nLibHandle, "set_prefs", -1, (void**)&m_pfSetPrefs );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For set_prefs\n" ); nError = 1; }
    
    nErr = get_symbol_address( m_nLibHandle, "get_info", -1, (void**)&m_pfGetInfo );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For get_info\n" ); nError = 1; }

    nErr = get_symbol_address( m_nLibHandle, "handle_message", -1, (void**)&m_pfHandleMessage );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For handle_message\n" ); nError = 1; }

    nErr = get_symbol_address( m_nLibHandle, "get_prefs_view", -1, (void**)&m_pfGetPrefsView );
    if( nErr < 0 ) { dbprintf( "Could Not Get Symbol Address For get_prefs_view\n" ); nError = 1; }
    
    if( nError == 0 )
    {
      m_nId = m_pfInit( pcPrefs );
      if( m_nId == INIT_FAIL )
      {

        dbprintf( "Failed to initialise plugin.\n" );
        nError = 1;

      }
  
    } else {

      dbprintf( "Failed to load plugin.\n" );

    }

  } else {

    dbprintf( "open_library() failed\n" );
    nError = 1;

  }

}



/*
** name:       LauncherPlugin::~LauncherPlugin
** purpose:    Deletes a plugin
** parameters: none.
** returns:    none.
*/
LauncherPlugin::~LauncherPlugin( )
{

  if( m_nLibHandle > 0 )
  {

    if( m_pfDelete )
      m_pfDelete( m_nId );
    unload_library( m_nLibHandle );

  }

}




/*
** name:       LauncherPlugin::GetView
** purpose:    Returns the plugin's View
** parameters: none.
** returns:    A View to display in the Window
*/
View *LauncherPlugin::GetView( void )
{
  return m_pfGetView( m_nId );
}



/*
** name:       LauncherPlugin::GetPrefs
** purpose:    Returns the plugin's prefs View
** parameters: none.
** returns:    A View to display in a prefs window
*/
View *LauncherPlugin::GetPrefsView( void )
{
  return m_pfGetPrefsView( m_nId );
}


/*
** name:       LauncherPlugin::RemoveView
** purpose:    Tells a plugin to remove it's View from the Window
** parameters: none.
** returns:    none.
*/
void LauncherPlugin::RemoveView( void )
{
  m_pfRemoveView( m_nId );
}


/*
** name:       LauncherPlugin::GetHideStatus
** purpose:    Returns the visibility of the plugin
** parameters: none.
** returns:    An int set to either LW_VISIBLE_ALWAYS or LW_VISIBLE_WHEN_ZOOMED
*/
int LauncherPlugin::GetHideStatus( void )
{
	return m_nHideStatus;
}


/*
** name:       LauncherPlugin::SetHideStatus
** purpose:    Sets the visibility of the plugin.
** parameters: An int set to either LW_VISIBLE_ALWAYS or LW_VISIBLE_WHEN_ZOOMED
** returns:    none.
*/
void LauncherPlugin::SetHideStatus( int nHideStatus )
{
	if( (nHideStatus == LW_VISIBLE_ALWAYS) || (nHideStatus == LW_VISIBLE_WHEN_ZOOMED) )
		m_nHideStatus = nHideStatus;
}


/*
** name:       LauncherPlugin::GetPrefs
** purpose:    Returns a Message containing the preferences of a plugin.
** parameters: none.
** returns:    A LauncherMessage
*/
LauncherMessage *LauncherPlugin::GetPrefs( void )
{
  LauncherMessage *pcPrefs = m_pfGetPrefs( m_nId );
  pcPrefs->AddString( "__Name", m_zName );
  pcPrefs->AddInt32( "HideStatus", m_nHideStatus );
  return pcPrefs;
}


/*
** name:       LauncherPlugin::SetPrefs
** purpose:    Sets the preferences of a plugin
** parameters: A LauncherMessage
** returns:    none.
*/
void LauncherPlugin::SetPrefs( LauncherMessage *pcPrefs )
{
  pcPrefs->FindInt32( "HideStatus", &m_nHideStatus );
  m_pfSetPrefs( m_nId, pcPrefs );
}


/*
** name:       LauncherPlugin::GetInfo
** purpose:    Returns a Message containing info about a plugin
** parameters: none.
** returns:    A LauncherMessage.
*/
LauncherMessage *LauncherPlugin::GetInfo( void )
{
  return m_pfGetInfo( );
}


/*
** name:       LauncherPlugin::SetWindowPos
** purpose:    Tells the plugin it's location in the window
** parameters: An int
** returns:    none.
*/
void LauncherPlugin::SetWindowPos( int nPos )
{
	m_nWindowPos = nPos;
	LauncherMessage *pcMessage = new LauncherMessage( LW_PLUGIN_WINDOW_POS_CHANGED,/* 0,*/ this );
	m_pfHandleMessage( m_nId, pcMessage );
	delete pcMessage;
}


/*
** name:       LauncherPlugin::GetWindowPos
** purpose:    Returns the plugin's position in the Window
** parameters: none.
** returns:    An int.
*/
int LauncherPlugin::GetWindowPos( void )
{
  return m_nWindowPos;
}


/*
** name:       LauncherPlugin::GetId
** purpose:    Returns the plugin's ID number.
** parameters: none.
** returns:    An int.
*/
int LauncherPlugin::GetId( void )
{
  return m_nId;
}


/*
** name:       LauncherPlugin::HandleMessage
** purpose:    Passes messages to the plugin.
** parameters: A Message
** returns:    none.
*/
void LauncherPlugin::HandleMessage( LauncherMessage *pcMessage )
{
  m_pfHandleMessage( m_nId, pcMessage );
}
	

/*
** name:       LauncherPlugin::AddTimer
** purpose:    Adds a Timer to the Window
** parameters: An int - the timeout and a bool. Passed to the AddTimer method of the Window.
** returns:    none.
*/
void LauncherPlugin::AddTimer( int nTimeOut, bool bOneShot = true )
{
	RemoveTimer( );
		
	m_pcWindow->AddTimer( m_pcWindow, m_nWindowPos, nTimeOut, bOneShot );
	m_bTimerActive = true;
}


/*
** name:       LauncherPlugin::RemoveTimer
** purpose:    Removes a timer from the Window
** parameters: none.
** returns:    none.
*/
void LauncherPlugin::RemoveTimer( void )
{
	if( m_bTimerActive ) {
		m_pcWindow->RemoveTimer( m_pcWindow, m_nWindowPos );
		m_bTimerActive = false;
	}
}	


/*
** name:       LauncherPlugin::TimerTick
** purpose:    Passes a LW_PLUGIN_TIMER Message to the plugin.
** parameters: none.
** returns:    none.
*/
void LauncherPlugin::TimerTick( void )
{
	LauncherMessage *pcMessage = new LauncherMessage( LW_PLUGIN_TIMER, this );
	m_pfHandleMessage( m_nId, pcMessage );
	delete pcMessage;
}


/*
** name:       LauncherPlugin::GetName
** purpose:    Returns the name of the plugin
** parameters: none.
** returns:    A string.
*/
string LauncherPlugin::GetName( void )
{
	return m_zName;
}


/*
** name:       LauncherPlugin::RequestWindowRefresh
** purpose:    Sends an LW_REQUEST_REFRESH Message to the Window
** parameters: none.
** returns:    none.
*/
void LauncherPlugin::RequestWindowRefresh( void )
{
	Invoker *pcInvoker = new Invoker( new Message( LW_REQUEST_REFRESH ), m_pcWindow );
	pcInvoker->Invoke( );
}


/*
** name:       LauncherPlugin::LockWindow
** purpose:    Sends an LW_ZOOM_LOCK Message to the Window
** parameters: none.
** returns:    none.
*/
void LauncherPlugin::LockWindow( void )
{
	Invoker *pcInvoker = new Invoker( new Message( LW_ZOOM_LOCK ), m_pcWindow );
	pcInvoker->Invoke( );
}


/*
** name:       LauncherPlugin::UnlockWindow
** purpose:    Sends an LW_ZOOM_UNLOCK Message to the Window
** parameters: none.
** returns:    none.
*/
void LauncherPlugin::UnlockWindow( void )
{
	Invoker *pcInvoker = new Invoker( new Message( LW_ZOOM_UNLOCK ), m_pcWindow );
	pcInvoker->Invoke( );
}


Window *LauncherPlugin::GetWindow( void )
{
	return m_pcWindow;
}

void LauncherPlugin::BroadcastMessage( uint32 nBroadcastCode )
{
	LauncherMessage *pcMessage = new LauncherMessage( LW_EVENT_BROADCAST, this );
	pcMessage->AddInt32( "BroadcastCode", nBroadcastCode );
	Invoker *pcInvoker = new Invoker( pcMessage, m_pcWindow );
	pcInvoker->Invoke( );
}

//*************************************************************************************


LauncherMessage::LauncherMessage( int32 nCode1 = 0, /*int32 nCode2 = 0,*/ LauncherPlugin *pcPlugin = NULL ) : Message( nCode1 )
{
//	AddInt32( "__LM_CODE2", nCode2 );
	AddPointer( "__LM_PLUGIN", (void **)pcPlugin );
}

LauncherMessage::~LauncherMessage( )
{
	
}

/*
int32 LauncherMessage::GetCode2( void )
{
	int32 nCode2 = 0;
	FindInt32( "__LM_CODE2", &nCode2 );
	return nCode2;
}

void LauncherMessage::SetCode2( int32 nCode2 )
{
	RemoveName( "__LM_CODE2" );
	AddInt32( "__LM_CODE2", nCode2 );
}
*/

LauncherPlugin *LauncherMessage::GetPlugin( void )
{
	LauncherPlugin *pcPlugin = NULL;
	FindPointer( "__LM_PLUGIN", (void **)&pcPlugin );
	return pcPlugin;
}

void LauncherMessage::SetPlugin( LauncherPlugin *pcPlugin )
{
	RemoveName( "__LM_PLUGIN" );
	AddPointer( "__LM_PLUGIN", (void **)pcPlugin );
}



//*************************************************************************************



ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE) : 
				Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
	m_nTextPosition = nTextPosition;
	m_pcBitmap = 0;
	m_bDrawBorder = bDrawBorder;
}



ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, Bitmap *pcBitmap, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE) : 
				Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
	m_nTextPosition = nTextPosition;
	m_pcBitmap = 0;
	m_bDrawBorder = bDrawBorder;	
	SetImage( pcBitmap );	
}



ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nWidth, uint32 nHeight, uint8 *pnBuffer, uint32 nTextPosition = IB_TEXT_RIGHT,bool bDrawBorder = true, 
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE) : 
				Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
	m_nTextPosition = nTextPosition;
	m_pcBitmap = 0;	
	m_bDrawBorder = bDrawBorder;
	SetImage( nWidth, nHeight, pnBuffer );
}


ImageButton::~ImageButton( )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;
}


void ImageButton::SetImage( uint32 nWidth, uint32 nHeight, uint8 *pnBuffer )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;
		
	uint32 nBufferSize = nWidth * nHeight * 4;
		
	m_pcBitmap = new Bitmap( nWidth, nHeight, CS_RGBA32 );
	
	Color32_s cBGCol = get_default_color( COL_NORMAL );
	
	uint8 *pnRaster = m_pcBitmap->LockRaster( );
	
	for( uint32 nByte = 0; nByte < nBufferSize; nByte+=4 ) {
		if( (pnBuffer[nByte] != 0) || (pnBuffer[nByte + 1] != 0) || (pnBuffer[nByte + 2] != 0) ) {
			(*pnRaster++) = pnBuffer[nByte+2];
			(*pnRaster++) = pnBuffer[nByte+1];
			(*pnRaster++) = pnBuffer[nByte];
			(*pnRaster++) = pnBuffer[nByte+3];
		} else {
			(*pnRaster++) = cBGCol.blue;
			(*pnRaster++) = cBGCol.green;
			(*pnRaster++) = cBGCol.red;
			(*pnRaster++) = cBGCol.alpha;
		}
	
	}
	
	m_pcBitmap->UnlockRaster( );

	Invalidate();
	Flush();	
}


void ImageButton::SetImage( Bitmap *pcBitmap )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;

	m_pcBitmap = pcBitmap;
	
	Invalidate();
	Flush();	
}




bool ImageButton::SetImageFromFile( string zFile )
{
	
	File *pcFile = new File( );
	
	if( pcFile->SetTo( zFile ) != 0 )
		return false;
		
	Translator *pcTrans = NULL;
	TranslatorFactory *pcFactory = new TranslatorFactory();
	pcFactory->LoadAll();
	
	Bitmap *pcBitmap = NULL;
	BitmapFrameHeader sFrameHeader;
	
	uint8 anBuf[8192];
	
	int nBytesRead = 0;
	
	while( true ) {
		
		nBytesRead = pcFile->Read( anBuf, sizeof(anBuf) );
		
		if( nBytesRead == 0 )
			break;
			
		if( pcTrans == NULL ) {
			int nErr = pcFactory->FindTranslator( "", os::TRANSLATOR_TYPE_IMAGE, anBuf, nBytesRead, &pcTrans );
			if( nErr < 0 )
				return false;
		}
		
		pcTrans->AddData( anBuf, nBytesRead, nBytesRead != sizeof(anBuf) );
	}

	BitmapHeader sBmHeader;
	if( pcTrans->Read( &sBmHeader, sizeof(sBmHeader) ) != sizeof( sBmHeader ) )
		return false;
				
	pcBitmap = new Bitmap( sBmHeader.bh_bounds.Width() + 1, sBmHeader.bh_bounds.Height(), CS_RGBA32 );
	
	Color32_s cBGCol = get_default_color( COL_NORMAL );
	uint8 anBGPix[4];
	anBGPix[0] = cBGCol.blue;
	anBGPix[1] = cBGCol.green;
	anBGPix[2] = cBGCol.red;	
	anBGPix[3] = cBGCol.alpha;
		
	uint8 *pDstRaster = pcBitmap->LockRaster();
	
	// Why are the first 32 bytes garbage?
	pcTrans->Read(anBuf, 32 );
	
	uint32 nSize = (sBmHeader.bh_bounds.Width() + 1) * sBmHeader.bh_bounds.Height();
	for( uint32 nPix = 0; nPix < nSize; nPix++ ) {
		if( pcTrans->Read( anBuf, 4 ) < 0 )
			break;
				
		memcpy( pDstRaster, (anBuf[3] == 0) ? anBGPix : anBuf, 4 );
	
		pDstRaster += 4;
	}
	
	if( pcBitmap == NULL )
		return false;

	SetImage( pcBitmap );
	return true;
}

			
Point ImageButton::GetPreferredSize( bool bLargest ) const
{
	
    font_height sHeight;
    GetFontHeight( &sHeight );
    float vCharHeight = sHeight.ascender + sHeight.descender + 8;    
	float vStrWidth = GetStringWidth( GetLabel( ) ) + 16;
	
	float x = vStrWidth;
	float y = vCharHeight;

	if( m_pcBitmap != 0 ) {
		Rect cBitmapRect = m_pcBitmap->GetBounds( );
	
		if( vStrWidth > 16 ) {
			if( (m_nTextPosition == IB_TEXT_RIGHT ) || ( m_nTextPosition == IB_TEXT_LEFT ) ) {
				x += cBitmapRect.Width() + 2;
				y = ( (cBitmapRect.Height() + 8) > y ) ? cBitmapRect.Height() + 8 : y;
			} else {
				x = ( (cBitmapRect.Width() + 8) > x ) ? cBitmapRect.Width() + 8 : x;			
				y += cBitmapRect.Height() + 2;
			}
		} else {
			x = cBitmapRect.Width() + 8;
			y = cBitmapRect.Height() + 8;
		}
	}
	
	return Point( x,y );
}

	
void ImageButton::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds( );
	Rect cTextBounds = GetBounds( );
	Rect cBitmapRect;
	
	float vStrWidth = GetStringWidth( GetLabel( ) );
	
    if( m_pcBitmap != 0 ) {
    		
	    cBitmapRect = m_pcBitmap->GetBounds( );
    
		float vBmWidth = cBitmapRect.Width();
		float vBmHeight = cBitmapRect.Height();
	
		switch( m_nTextPosition )
		{
			case IB_TEXT_RIGHT:
				cBitmapRect.left += 4;
				cBitmapRect.right += 4;
				cBitmapRect.top = (cBounds.Height() / 2) - (vBmHeight / 2);
				cBitmapRect.bottom = cBitmapRect.top + vBmHeight;
				cTextBounds.left -= vBmWidth;	
				break;
			case IB_TEXT_LEFT:
				cBitmapRect.top += 4;
				cBitmapRect.bottom += 4;				
				cBitmapRect.left = cBounds.right - 4 - vBmWidth;
				cBitmapRect.right = cBounds.right - 4;
				cTextBounds.right -= vBmWidth;
				break;
			case IB_TEXT_BOTTOM:
				cBitmapRect.left = (cBounds.Width() / 2) - (vBmWidth / 2);
				cBitmapRect.right = cBitmapRect.left + vBmWidth;				
				cBitmapRect.top +=4 ;
				cBitmapRect.bottom += 4;
				cTextBounds.top -= vBmHeight;
				break;				
			case IB_TEXT_TOP:
				cBitmapRect.left = (cBounds.Width() / 2) - (vBmWidth / 2);
				cBitmapRect.right = cBitmapRect.left + vBmWidth;				
				cBitmapRect.top = cBounds.bottom - 4 - vBmHeight;
				cBitmapRect.bottom = cBounds.bottom -4;
				cTextBounds.bottom -= vBmHeight;
				break;
			default:
				break;
		}
		
		if( vStrWidth == 0 ) {
			cBitmapRect.left = (cBounds.Width() / 2) - (vBmWidth / 2);
			cBitmapRect.top = (cBounds.Height() / 2) - (vBmHeight / 2);
			cBitmapRect.right = cBitmapRect.left + vBmWidth;
			cBitmapRect.bottom = cBitmapRect.top + vBmHeight;
		}			
	}
	
    SetEraseColor( get_default_color( COL_NORMAL ) );
    uint32 nFrameFlags = (GetValue().AsBool() && IsEnabled()) ? FRAME_RECESSED : FRAME_RAISED;
	SetBgColor( get_default_color( COL_NORMAL ) );
	if( ( m_bDrawBorder == true ) || (nFrameFlags == FRAME_RECESSED ) ) {
		DrawFrame( cBounds, nFrameFlags );
	} else {
		FillRect( cBounds, get_default_color( COL_NORMAL ) );
		SetFgColor( 0, 255,0 );
		Rect cBounds2 = ConvertToScreen( GetBounds() );
		Rect cWinBounds = GetWindow()->GetFrame();
		
		  // Draw a border if we're near the edge of the Window
		if( abs((int)(cBounds2.top - cWinBounds.top)) < 2 ) { SetFgColor( 255,255,255 ); DrawLine( Point(0,0),Point(cBounds.right,0) ); }
		if( abs((int)(cBounds2.left - cWinBounds.left)) < 2 ) { SetFgColor( 255,255,255 ); DrawLine( Point(0,0), Point( 0, cBounds.bottom ) ); }
		if( abs((int)(cBounds2.right - cWinBounds.right)) < 2 ) { SetFgColor( 0,0,0 ); DrawLine( Point( cBounds.right, 0 ), Point( cBounds.right, cBounds.bottom) ); }
		if( abs((int)(cBounds2.bottom - cWinBounds.bottom)) < 2 ) { SetFgColor( 0,0,0 ); DrawLine( Point( 0, cBounds.bottom), Point( cBounds.right, cBounds.bottom) ); }
	}
	
	font_height sHeight;
	GetFontHeight( &sHeight );
	
    float vCharHeight = sHeight.ascender + sHeight.descender;
    float y = floor( 2.0f + (cTextBounds.Height()+1.0f)*0.5f - vCharHeight*0.5f + sHeight.ascender );
    float x = floor((cTextBounds.Width()+1.0f) / 2.0f - vStrWidth / 2.0f);
		
    if ( IsEnabled() ) {
    	if( GetWindow()->GetDefaultButton() == this ) {
			SetFgColor( 0, 0, 0 );
			DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
			DrawLine( Point( cBounds.right, cBounds.bottom ) );
			DrawLine( Point( cBounds.left, cBounds.bottom ) );
			DrawLine( Point( cBounds.left, cBounds.top ) );
			cBounds.left += 1;
			cBounds.top += 1;
			cBounds.right -= 1;
			cBounds.bottom -= 1;
    	}
    	SetFgColor( 0,0,0 );
		
		if( HasFocus( ) ) 
			DrawLine( Point( (cTextBounds.Width()+1.0f)*0.5f - vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ),
			  Point( (cTextBounds.Width()+1.0f)*0.5f + vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ) );	

		MovePenTo( x,y );	
		DrawString( GetLabel( ) );
					  
    } else {

		MovePenTo( x,y );	
		DrawString( GetLabel( ) );    	
		MovePenTo( x - 1.0f, y - 1.0f );
    	SetFgColor( 100,100,100 );
    	SetDrawingMode( DM_OVER );
    	DrawString( GetLabel( ) );
    	SetDrawingMode( DM_COPY );
    	
    }
	 
    
    if( m_pcBitmap != 0 )
    	DrawBitmap( m_pcBitmap, m_pcBitmap->GetBounds(), cBitmapRect );  
}
	
	
void ImageButton::SetTextPosition( uint32 nTextPosition )
{
	m_nTextPosition = nTextPosition;
	Invalidate();
	Flush();
}

uint32 ImageButton::GetTextPosition( void )
{
	return m_nTextPosition;
}

Bitmap *ImageButton::GetImage( void )
{
	return m_pcBitmap;
}

void ImageButton::ClearImage( void )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;
		
	m_pcBitmap = 0;

	Invalidate();
	Flush();
}


void ImageButton::SetDrawBorder( bool bDrawBorder )
{
	m_bDrawBorder = bDrawBorder;
	Invalidate();
	Flush();
}

