/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <gui/font.h>
#include <util/invoker.h>

using namespace os;


/** Initialize the Alert.
 * \par Description:
 *	The Alert is like a Messagebox in windows.  It provides a way to show errors,
 *  About boxes or even tips without having to create your own Window. 
 * \param cTitle - The title of the window.
 * \param cText  - The text that will be shown in the Alert.
   \param nFlags - The flags that will be passed to the appserver to tell the appserver what options
   				   when creating this.
 * \param ...    - The name of buttons to create(IE: would be like "ok" or "cancel").  You can create 
 					more than one button(IE: new Alert("Title","Text",WND_NOT_RESIZABLE,"OK", "CANCEL");).  
 * \sa Go() 
 * \author	Kurt Skauen (kurt@atheos.cx) with modifications by Rick Caudill
 *****************************************************************************/
Alert::Alert( const std::string& cTitle, const std::string& cText, int nFlags, ... ) :
    Window( Rect( 100, 50, 100, 50 ), "alert_window", cTitle.c_str(), WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT)
{
    
    va_list pArgs;
	va_start( pArgs, nFlags );
	cm_pcBitmap = NULL;
    
    m_pcView = new AlertView( cText, pArgs );
    m_pcView->SetFgColor( 0, 0, 0 );
    m_pcView->SetBgColor( get_default_color(COL_NORMAL) );
    Point cSize = m_pcView->GetPreferredSize( true );
    m_pcView->ResizeTo( cSize );
    ResizeTo( cSize );
    
    if (nFlags > 0){
    	SetFlags(nFlags);
    	}
    Desktop cDesktop;
    MoveTo( cDesktop.GetResolution().x / 2 - cSize.x / 2, cDesktop.GetResolution().y / 2 - cSize.y / 2 );
  
    AddChild( m_pcView );
    va_end( pArgs );
    m_pcInvoker = NULL;
    m_hMsgPort  = -1;
    
    Flush();
    Sync();
}

/** Initialize the Alert.
 * \par Description:
 *	The Alert is like a Messagebox in windows.  It provides a way to show errors,
 *  About boxes or even tips without having to create your own Window. 
 * \param cTitle   - The title of the window.
 * \param cText    - The text that will be shown in the Alert.
 * \param nFlags   - The flags that will be passed to the appserver to tell the appserver what options
   				     when creating this.
 * \param pcBitmap - Bitmap that will show up in the Alert Window. 
 * \param ...      - The name of buttons to create(IE: would be like "ok" or "cancel").  You can create 
 					more than one button(IE: new Alert("Title","Text",WND_NOT_RESIZABLE,"OK", "CANCEL");).  
 * \sa Go() 
 * \author	Kurt Skauen (kurt@atheos.cx) with modifications by Rick Caudill
 *****************************************************************************/
Alert::Alert( const std::string& cTitle, const std::string& cText, Bitmap* pcBitmap, int nFlags, ... ) :
    Window( Rect( 100, 50, 100, 50 ), "alert_window", cTitle.c_str(), WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT)
{
    va_list pArgs;

    va_start( pArgs, nFlags );
    
     if (nFlags > 0){
    	SetFlags(nFlags);
    	}
    	
    cm_pcBitmap = NULL;	
    m_pcView = new AlertView( cText, pArgs, pcBitmap );
    
    m_pcView->SetEraseColor(get_default_color(COL_NORMAL));
    m_pcView->SetFgColor( 0, 0, 0 );
    m_pcView->SetBgColor( get_default_color(COL_NORMAL) );
    Point cSize = m_pcView->GetPreferredSize( true );
    m_pcView->ResizeTo( cSize );
    ResizeTo( cSize );

    Desktop cDesktop;
    MoveTo( cDesktop.GetResolution().x / 2 - cSize.x / 2, cDesktop.GetResolution().y / 2 - cSize.y / 2 );
  
    AddChild( m_pcView );
    va_end( pArgs );
    m_pcInvoker = NULL;
    m_hMsgPort  = -1;
   
   	m_pcView->Paint(GetBounds());
   	
   	Flush();
    Sync();
}


/** Initialize the Alert.
 * \par Description:
 *	The Alert is like a Messagebox in windows.  It provides a way to show errors,
 *  About boxes or even tips without having to create your own Window. 
 * \param cTitle    - The title of the window.
 * \param cText     - The text that will be shown in the Alert.
 * \param nFlags    - The flags that will be passed to the appserver to tell the appserver what options
   				       when creating this.
 * \param nAlertNum - Static icons that will show up when you call this constructor. To call the icons you would do something like this: (new Alert("Title","Text", Alert::ALERT_WARNING,0."OK",NULL);)
 * \param ...       - The name of buttons to create(IE: would be like "ok" or "cancel").  You can create 
 					more than one button(IE: new Alert("Title","Text",WND_NOT_RESIZABLE,"OK", "CANCEL");).  
 * \sa Go() 
 * \author	Kurt Skauen (kurt@atheos.cx) with modifications by Rick Caudill
 *****************************************************************************/
Alert::Alert( const std::string& cTitle, const std::string& cText, alert_icon nAlertNum, int nFlags, ... ) :
    Window( Rect( 100, 50, 100, 50 ), "alert_window", cTitle.c_str(), WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT )
{
    va_list pArgs;

    va_start( pArgs, nFlags );
    
    nImage = nAlertNum;
    
     if (nFlags > 0){
    	SetFlags(nFlags);
    	}
    
    cm_pcBitmap = 0;
    	
    switch (nImage){
    	
     case ALERT_WARNING:  SetImage(nWarning_Icon.width, nWarning_Icon.height, (uint8 *) nWarning_Icon.pixel_data); break;
	 case ALERT_INFO:     SetImage(nInfo_Icon.width, nInfo_Icon.height, (uint8 *) nInfo_Icon.pixel_data); break;
     case ALERT_QUESTION: SetImage(nQuestion_Icon.width, nQuestion_Icon.height, (uint8 *) nQuestion_Icon.pixel_data); break;
     default: break;	
    }
    m_pcView = new AlertView( cText, pArgs, cm_pcBitmap );
    	
    m_pcView->SetEraseColor(get_default_color(COL_NORMAL));
    m_pcView->SetFgColor( 0, 0, 0 );
    m_pcView->SetBgColor( get_default_color(COL_NORMAL) );
    Point cSize = m_pcView->GetPreferredSize( true );
    m_pcView->ResizeTo( cSize );
    ResizeTo( cSize );

    Desktop cDesktop;
    MoveTo( cDesktop.GetResolution().x / 2 - cSize.x / 2, cDesktop.GetResolution().y / 2 - cSize.y / 2 );
  
    AddChild( m_pcView );
    va_end( pArgs );
    m_pcInvoker = NULL;
    m_hMsgPort  = -1;
    m_pcView->Paint(GetBounds());
    
    Flush();
    Sync();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Alert::~Alert()
{
    if ( m_hMsgPort != -1 ) {
	delete_port( m_hMsgPort );
    }
    delete m_pcInvoker;
    
    if (cm_pcBitmap != NULL){
   	delete cm_pcBitmap;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Alert::HandleMessage( Message* pcMessage )
{
    if ( pcMessage->GetCode() < int32(m_pcView->m_cButtons.size()) ) {
	if ( m_hMsgPort >= 0 ) {
	    send_msg( m_hMsgPort, pcMessage->GetCode(), NULL, 0 );
	} else {
	    if ( m_pcInvoker != NULL ) {
		Message* pcMsg = m_pcInvoker->GetMessage();
		if ( pcMsg == NULL ) {
		    dbprintf( "Error: Invoker registered with this Alert requester does not have a message!\n" );
		} else {
		    pcMsg->AddInt32( "which", pcMessage->GetCode() );
		    m_pcInvoker->Invoke();
		}
	    }
	    PostMessage( M_QUIT );
	}
    } else {
	Handler::HandleMessage( pcMessage );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Alert::Go()
{
    uint32 nCode;
    int	 nError;
  
    m_hMsgPort = create_port( "alert_port", DEFAULT_PORT_SIZE );

    Show();
    MakeFocus();
    if ( m_hMsgPort < 0 ) {
	dbprintf( "Alert::WaitForSelection() failed to create message port\n" );
	PostMessage( M_QUIT );
	return( -1 );
    }
    nError = get_msg( m_hMsgPort, &nCode, NULL, 0 );
    if ( nError < 0 ) {
	PostMessage( M_QUIT );
	return( -1 );
    } else {
	PostMessage( M_QUIT );
	return( nCode );
    }
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


void Alert::Go( Invoker* pcInvoker )
{
    m_pcInvoker = pcInvoker;
    Show();
    MakeFocus();
}



//----------------------------------------------------------------------------
// NAME: SetImage()
// DESC: SetImage() is a private method for adding alert icons to the alert
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Alert::SetImage( uint32 nWidth, uint32 nHeight, uint8 *pnBuffer )
{
	
	if( cm_pcBitmap != 0 )
		delete cm_pcBitmap;
		
	uint32 nBufferSize = nWidth * nHeight * 4;
		
	cm_pcBitmap = new Bitmap( nWidth, nHeight, CS_RGB32 );
	
	Color32_s cBGCol = get_default_color( COL_NORMAL );
	
	uint8 *pnRaster = cm_pcBitmap->LockRaster( );
	
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
	
	cm_pcBitmap->UnlockRaster( );

	Flush();	
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

AlertView::AlertView( const std::string& cText,va_list pButtons, Bitmap* pcBitmap ) : View( Rect(0,0,1,1), "alert", CF_FOLLOW_ALL )
{
    const char* pzButName;
	pcBitmp = pcBitmap;
    uint i = 0;

    while( (pzButName = va_arg( pButtons, const char* )) != NULL )
    {
	Button* pcButton = new Button( Rect(0,0,1,1), pzButName, pzButName, new Message( i++ ) );
	m_cButtons.push_back( pcButton );
	AddChild( pcButton );

	Point cSize = pcButton->GetPreferredSize( true );
	if ( cSize.x > m_cButtonSize.x ) {
	    m_cButtonSize.x = cSize.x;
	}
	if ( cSize.y > m_cButtonSize.y ) {
	    m_cButtonSize.y = cSize.y;
	}
    }
  
    float nWidth = (m_cButtonSize.x + 5) * m_cButtons.size();

    int nStart = 0;
    for ( i = 0 ; i <= cText.size() ; ++i )
    {
	if ( i == cText.size() || cText[i] == '\n' ) {
	    int nLen = i - nStart;
	    std::string cLine( cText, nStart, nLen );
	    m_cLines.push_back( cLine );
	    float nStrLen = GetStringWidth( cLine.c_str() ) + 20;
	    if ( nStrLen > nWidth ) {
		nWidth = nStrLen;
	    }
	    nStart = i + 1;
	}
    }

    float x;
    font_height sFontHeight;
    GetFontHeight( &sFontHeight );
    m_vLineHeight = sFontHeight.ascender + sFontHeight.descender + sFontHeight.line_gap;

    float nHeight = m_cLines.size() * m_vLineHeight;
    nHeight += m_cButtonSize.y + 30;

    float y = nHeight - m_cButtonSize.y - 5;
    
    if(pcBitmap != NULL){
    		x = nWidth - m_cButtons.size() * (m_cButtonSize.x + 5) + pcBitmap->GetBounds().Width();
    	}
    	
    	else{
    		x = nWidth - m_cButtons.size() * (m_cButtonSize.x + 5) - 5; // sets button placement
  }
    for ( i = 0 ; i < m_cButtons.size() ; ++i ) {
    	if(pcBitmap != NULL){
			
			m_cButtons[i]->SetFrame( Rect( x, y, x + m_cButtonSize.x, y + m_cButtonSize.y - 1 ) ); //look here for button position manip
			x += m_cButtonSize.x + 5;
			m_cButtons[i]->Flush();
			m_cButtons[i]->Sync();
			
			} else {
		
    		m_cButtons[i]->SetFrame( Rect( x, y, x + m_cButtonSize.x + 5, y + m_cButtonSize.y - 1 ) ); //look here for button position manip
			x += m_cButtonSize.x + 5; // was 5
			m_cButtons[i]->Flush();
			m_cButtons[i]->Sync();
			
			}
    
    }
    
    if (pcBitmp !=NULL){
    	m_cMinSize.x = nWidth + pcBitmp->GetBounds().Width() + 5 ;
    	 
   	}	else{
   		m_cMinSize.x = nWidth;  //was 0
    
    }m_cMinSize.y = nHeight + 4 ;

	Invalidate();
	
}

void AlertView::AllAttached()
{
    m_cButtons[0]->MakeFocus();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point AlertView::GetPreferredSize( bool bLargest )
{
    return( m_cMinSize );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void AlertView::Paint( const Rect& cUpdateRect )
{
    FillRect( GetBounds(), get_default_color(COL_NORMAL) );
    
	if (pcBitmp !=NULL){
	
	SetDrawingMode(DM_BLEND);
	DrawBitmap(pcBitmp,pcBitmp->GetBounds(),Rect(5,5,pcBitmp->GetBounds().Width() + 1,pcBitmp->GetBounds().Height()));
	float y = 20.0f;
    
    for ( uint i = 0 ; i < m_cLines.size() ; ++i ){
   
	MovePenTo( 20 + pcBitmp->GetBounds().Width(), y  );
	DrawString( m_cLines[i].c_str() );
	y += m_vLineHeight;
    }
    
    }
    
    else{
	float y = 20.0f;
    for ( uint i = 0 ; i < m_cLines.size() ; ++i )
    {
	if(pcBitmp !=NULL){
		MovePenTo( pcBitmp->GetBounds().Width()/2 + 2, pcBitmp->GetBounds().Height()/2 + 7);
		
	} else {
		MovePenTo(10,y);
		}	
	DrawString( m_cLines[i].c_str() );
	y += m_vLineHeight;
    }
    }
    
    //Invalidate();
   
    
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ProgressRequester::ProgressRequester( const Rect& cFrame, const char* pzName,
				      const char* pzTitle, bool bCanSkip ) :
    Window( cFrame, pzName, pzTitle )
{
    Lock();
    m_bDoCancel = false;
    m_bDoSkip   = false;
  
    m_pcProgView = new ProgressView( GetBounds(), bCanSkip );
    AddChild( m_pcProgView );
    Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressRequester::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case IDC_CANCEL:
	    m_bDoCancel = true;
	    break;
	case IDC_SKIP:
	    m_bDoSkip = true;
	    break;
	default:
	    Handler::HandleMessage( pcMessage );
	    break;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ProgressRequester::DoCancel() const
{
    return( m_bDoCancel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ProgressRequester::DoSkip()
{
    bool bSkip = m_bDoSkip;
    m_bDoSkip = false;
    return( bSkip );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressRequester::SetPathName( const char* pzString )
{
    m_pcProgView->m_pcPathName->SetString( pzString );
    m_bDoSkip   = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressRequester::SetFileName( const char* pzString )
{
    m_pcProgView->m_pcFileName->SetString( pzString );
    m_bDoSkip   = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ProgressView::ProgressView( const Rect& cFrame, bool bCanSkip ) : View( cFrame, "progress_view", CF_FOLLOW_ALL )
{
    m_pcFileName = new StringView( Rect(0,0,1,1), "file_name", "" );
    m_pcPathName = new StringView( Rect(0,0,1,1), "path_name", "" );
    m_pcCancel = new Button( Rect(0,0,1,1), "cancel", "Cancel", new Message( ProgressRequester::IDC_CANCEL ) );
    if ( bCanSkip ) {
	m_pcSkip   = new Button( Rect(0,0,1,1), "skip", "Skip", new Message( ProgressRequester::IDC_SKIP ) );
    } else {
	m_pcSkip = NULL;
    }
  
    AddChild( m_pcPathName );
    AddChild( m_pcFileName );
    AddChild( m_pcCancel );

    if ( m_pcSkip != NULL ) {
	AddChild( m_pcSkip );
    }

    m_pcPathName->SetBgColor( 220, 220, 220 );
    m_pcPathName->SetFgColor( 0, 0, 0 );

    m_pcFileName->SetBgColor( 220, 220, 220 );
    m_pcFileName->SetFgColor( 0, 0, 0 );
  
    Layout( GetBounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressView::Paint( const Rect& cUpdateRect )
{
    SetFgColor( 220, 220, 220 );
    FillRect( cUpdateRect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressView::FrameSized( const Point& cDelta )
{
    Layout( GetBounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressView::Layout( const Rect& cBounds )
{
    Point cSize1 = m_pcCancel->GetPreferredSize( false );
    Point cSize2;
  
    if ( m_pcSkip != NULL ) {
	cSize2 = m_pcSkip->GetPreferredSize( false );
    } else {
	cSize2 = cSize1;
    }
  
    Point cStrSize = m_pcFileName->GetPreferredSize( false );

    cStrSize.x = cBounds.Width() - 9.0f;
  
    if ( cSize2.x > cSize1.x ) {
	cSize1.x = cSize2.x;
    }
    if ( cSize2.y > cSize1.y ) {
	cSize1.y = cSize2.y;
    }
  
    if ( m_pcSkip != NULL ) {
	m_pcSkip->ResizeTo( cSize1 );
    }
    m_pcCancel->ResizeTo( cSize1 );

    Point cPos1 = cBounds.RightBottom() - cSize1 - Point( 5, 5 );
    Point cPos2 = cPos1;
    cPos2.x -= cSize1.x + 5;

    float nCenter = ((cBounds.Height()+1.0f) - cSize1.y - 5.0f) / 2.0f;
  
    m_pcPathName->ResizeTo( cStrSize );
    m_pcPathName->MoveTo( 5, nCenter - cStrSize.y / 2 - cStrSize.y );

    m_pcFileName->ResizeTo( cStrSize );
    m_pcFileName->MoveTo( 5, nCenter - cStrSize.y / 2 + cStrSize.y );
  
    if ( m_pcSkip != NULL ) {
	m_pcSkip->MoveTo( cPos1  );
	m_pcCancel->MoveTo( cPos2 );
    } else {
	m_pcCancel->MoveTo( cPos1 );
    }
    Flush();
}















































