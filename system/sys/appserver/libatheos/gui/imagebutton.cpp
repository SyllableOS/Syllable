/*  libatheos.so - the highlevel API library for Syllable
 *  Copyright (C) 2002 - 2003 Rick Caudill
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
#include <gui/imagebutton.h>
#include <storage/memfile.h>
#include <typeinfo>
#include <iostream>
using namespace os;

/** \internal */
class ImageButton::Private
{
    public:
    Private() {
    	m_pcBitmap = NULL;
    	m_pcGrayBitmap = NULL;
    	m_nTextPosition = 0;
    	bMouseOver = false;
    	bShowFrame = false;
    	bMouseTwo = false;
    }

    ~Private() {
    	if( m_pcBitmap ) delete m_pcBitmap;
    	if( m_pcGrayBitmap ) delete m_pcGrayBitmap;
    }
   
    void SetImage( Image *pcBitmap ) {
    	if( m_pcBitmap ) delete m_pcBitmap;
	m_pcBitmap = pcBitmap;
    }
    
    Image* GetImage() {
    	return m_pcBitmap;
    }

    Image* GetGrayImage() {
    	if( !m_pcGrayBitmap ) {
    		/* Note: This is not a good method, since it will only work on Bitmaps. */
    		/* We have to add some method to Image for copying Images of unknown type! */
    		if( m_pcBitmap && ( typeid( *m_pcBitmap ) == typeid( BitmapImage ) ) ) {
    			m_pcGrayBitmap = new BitmapImage( *( dynamic_cast<BitmapImage*>( m_pcBitmap ) ) );
    			m_pcGrayBitmap->ApplyFilter( Image::F_GRAY );
    		}
    	}
    	return m_pcGrayBitmap ? m_pcGrayBitmap : m_pcBitmap;
    }

    Image*	m_pcBitmap;
    Image*	m_pcGrayBitmap;
    uint32	m_nTextPosition;
    bool	bMouseOver;		// true for mouse over mode
    bool	bShowFrame;		// true when frames are shown
    bool	bMouseTwo;		// true if the mouse is actually over the frame
};

/** Initialize the imagebutton.
 * \par Description:
 *	The imagebutton contructor initialize the local object.
 * \param cFrame - The size and position of the imagebutton.
 * \param pzName - The name of the imagebutton.
 * \param pzLabel - The label of the imagebutton.
 * \param pcMessage - The message passed to the imagebutton.
 * \param pcImage - The bitmapimage for the imagebutton.
 * \param nTextPosition - The Position of the label and the image for the imagebutton.
 * \param bShowFrames - Determines whether the imagebutton will have frames or not.
 * \param bShowText - Determines whether the imagebutton will show the label or not.
 * \param bMouse - Determines whether the imagebutton will have mouseover abilities(this is not working yet).
 * \param nResizeMask - Determines what way the imagebutton will follow the rest of the window.  Default is CF_FOLLOW_LEFT|CF_FOLLOW_TOP.
 * \param nFlags - Deteremines what flags will be sent to the imagebutton.
 * \author	Rick Caudill (cau0730@cup.edu) based on Andrew Keenan's ImageButton class
 *****************************************************************************/

ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage,Image *pcBitmap, uint32 nTextPosition, bool bShowFrames,
                          bool bShowText, bool bMouse, uint32 nResizeMask, uint32 nFlags) :
        Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
    m = new Private;

    m->m_nTextPosition = nTextPosition;
    m->bShowFrame = bShowFrames;
    m->bMouseOver = bMouse;

    if (bShowText == true)
        SetLabel(pzLabel);

    else
    {
        SetLabel("");
    }
    m->SetImage( pcBitmap );

} /*end ImageButton Constructor*/

/** \internal */
ImageButton::~ImageButton( )
{
    delete m;
} /*end ImageButton Destructor*/


void ImageButton::Activated(bool bIsActive)
{
    if ((bIsActive == true) && (GetWindow()->IsActive() == true))
    {
        Paint(GetBounds());
    }
} /*end Activated()*/


/** Clears the image...
 * \par Description:
 *	Clears the BitmapImage that is passed to the ImageButton.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void ImageButton::ClearImage( void )
{
    m->SetImage( NULL );
    Invalidate();
    Flush();
}/*end ClearImage()*/

/** Gets the Image...
 * \par Description:
 * Gets the Image that is passed to the ImageButton.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
Image *ImageButton::GetImage( void )
{
    return m->m_pcBitmap;
} /*end GetImage()*/

/* GetPreferredSize()*/
Point ImageButton::GetPreferredSize( bool bLargest )  const
{
    if( bLargest ) {
    	return Point( COORD_MAX, COORD_MAX );
    }
   
    font_height sHeight;
    GetFontHeight( &sHeight );
    float vCharHeight = sHeight.ascender + sHeight.descender + 8;
    float vStrWidth = GetStringWidth( GetLabel( ) ) + 16;

    float x = vStrWidth;
    float y = vCharHeight;

    if( m->m_pcBitmap )
    {
        Rect cBitmapRect = m->m_pcBitmap->GetBounds( );
        if( vStrWidth > 16 )
        {
            if( (m->m_nTextPosition == IB_TEXT_RIGHT ) || ( m->m_nTextPosition == IB_TEXT_LEFT ) )
            {
                x += cBitmapRect.Width() + 2;
                y = ( (cBitmapRect.Height() + 8) > y ) ? cBitmapRect.Height() + 8 : y;
            }
            else
            {
                x = ( (cBitmapRect.Width() + 8) > x ) ? cBitmapRect.Width() + 8 : x;
                y += cBitmapRect.Height() + 2;
            }
        }
        else
        {
            x = cBitmapRect.Width() + 8;
            y = cBitmapRect.Height() + 8;
        }
    }

    return Point( x, y );
} /*end GetPreferredSize()*/

/** Gets the text postion...
 * \par Description:
 *	Gets the text positon of the ImageButton.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
uint32 ImageButton::GetTextPosition( void )
{
    return m->m_nTextPosition;
}/*end GetTextPosition()*/

/* MouseDown*/
void ImageButton::MouseDown( const Point& cPosition, uint32 nButton )
{
        Button::MouseDown( cPosition, nButton );
} /*end MouseDown()*/

/* MouseMove */
void ImageButton::MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message* pcData)
{
    Button::MouseMove( cNewPos, nCode, nButtons, pcData );

    if( m->bMouseOver ) {
        bool mouseover = GetBounds().DoIntersect( cNewPos );
        if( mouseover != m->bMouseTwo ) {
        	m->bMouseTwo = mouseover;
        	Invalidate();
        	Flush();
        }
    }

    return;
} /*end MouseMove()*/

/*MouseUp*/
void ImageButton::MouseUp( const Point& cPosition, uint32 nButton, Message* pcData )
{
	Button::MouseUp( cPosition, nButton, pcData );
}/*ehd MouseUp()*/

void ImageButton::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds( );
    Rect cTextBounds = GetBounds( );
    float vStrWidth = GetStringWidth( GetLabel( ) );
    Point cBitPoint;
    font_height sHeight;
    GetFontHeight( &sHeight );
    float vCharHeight = sHeight.ascender + sHeight.descender;
    Image *pcImg = IsEnabled() ? m->GetImage() : m->GetGrayImage();

    if( pcImg )
    {

        switch( m->m_nTextPosition )
        {
        case IB_TEXT_RIGHT:
            cTextBounds.left -= pcImg->GetSize().x-2;
            cBitPoint.x = 4;
            cBitPoint.y = GetBounds().top + 3;
            break;

        case IB_TEXT_LEFT:
            cTextBounds.left = 0;
            cTextBounds.right = vStrWidth;
            cBitPoint.x = cTextBounds.right+2;
            cBitPoint.y = GetBounds().top+3;
            break;

        case IB_TEXT_BOTTOM:
            cTextBounds.top -=  pcImg->GetSize().y;
            cBitPoint.x = (GetBounds().Width() - pcImg->GetSize().x)/2;
            cBitPoint.y = 4;
            break;

        case IB_TEXT_TOP:
            cTextBounds.bottom = vCharHeight;
            cTextBounds.left = 0;
            cTextBounds.top = 0;
            cBitPoint.y = cTextBounds.bottom+2;
            cBitPoint.x = (GetBounds().Width()/2 - pcImg->GetSize().x/2);
            break;
        default:
            break;
        }

    }

    SetEraseColor( get_default_color( COL_NORMAL ) );
    SetBgColor( get_default_color( COL_NORMAL ) );
    SetFgColor( get_default_color(COL_NORMAL));

    Rect border = GetBounds();

    if (m->bShowFrame == false)
    {
        if(m->bMouseOver == true)
        {
            FillRect( cBounds, get_default_color( COL_NORMAL ) );
            SetFgColor( 0, 255,0 );
            Rect cBounds2 = ConvertToScreen( GetBounds() );
            Rect cWinBounds = GetWindow()->GetFrame();
        }
        else
        {
            FillRect( cBounds, get_default_color( COL_NORMAL ) );
            SetFgColor( 0, 255,0 );
            Rect cBounds2 = ConvertToScreen( GetBounds() );
            Rect cWinBounds = GetWindow()->GetFrame();
        }
    }
    else
    {
    	if( m->bMouseOver == false || ( m->bMouseTwo == true && IsEnabled() ) ) {
	        uint32 nFrameFlags = (GetValue().AsBool() && IsEnabled()) ? FRAME_RECESSED : FRAME_RAISED;
        	DrawFrame( cBounds, nFrameFlags );
        } else {
        	FillRect( cBounds, get_default_color( COL_NORMAL ) );
        }
    }

    float y = floor( 2.0f + (cTextBounds.Height()+1.0f)*0.5f - vCharHeight*0.5f + sHeight.ascender );
    float x = floor((cTextBounds.Width()+1.0f) / 2.0f - vStrWidth / 2.0f);

    if( GetWindow()->GetDefaultButton() == this && IsEnabled() )
    {
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

    if( GetLabel() != "" ) {
    	if ( IsEnabled() )
    	{
            if( HasFocus( ) ){
     	         SetFgColor( 0,0,0 );
   	    	 	DrawLine( Point( (cTextBounds.Width()+1.0f)*0.5f - vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ),Point( (cTextBounds.Width()+1.0f)*0.5f + vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ) );
            }
        
            SetFgColor( 0,0,0 );
            MovePenTo( x,y );
            DrawString( GetLabel( ) );
        }
    	else
    	{
            MovePenTo( x,y );
            DrawString( GetLabel( ) );
            MovePenTo( x - 1.0f, y - 1.0f );
            SetFgColor( 100,100,100 );
            SetDrawingMode( DM_OVER );
            DrawString( GetLabel( ) );
            SetDrawingMode( DM_COPY );
        }
    }

    if( pcImg )
    {
        SetDrawingMode( DM_BLEND );//DM_BLEND
        pcImg->Draw(cBitPoint,this);
        SetDrawingMode(DM_OVER);  //sets the mode to dm_over, so that hovering works completely
    }
}/*end Paint()*/

/** Resests the text postion...
 * \par Description:
 *	Resests the text postion and then flushes the imagebutton.
 * \param nTextPosition - int that holds the text position.  Should be: ID_TEXT_BOTTOM,
 	ID_TEXT_RIGHT, ID_TEXT_TOP, or ID_TEXT_LEFT.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void ImageButton::SetTextPosition( uint32 nTextPosition )
{
    m->m_nTextPosition = nTextPosition;
    Invalidate();
    Flush();
} /*end SetTextPosition()*/

/**Sets the Image from a file...
 * \par Description:
 *	Sets the BitmapImage for the ImageButton from a file.
 * \param pzFile - The path and name of the file(IE: if you wanted to load an Image from
 	/tmp/img.png, you would call this method like this: SetImageFromFile("/tmp/img.png").
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void ImageButton::SetImageFromFile(const char* pzFile)
{
    os::File file(pzFile);
    Image* vBitmap = new BitmapImage(&file);
    m->SetImage(vBitmap);
} /*end SetImageFromFile()*/

/** Sets the Image from a Resource...
 * \par Description:
 *	Sets the Image for the ImageButton from a Resouce.
 * \param pzFile - The name of the Resource(IE: if you wanted to load an Image called img.png 
 	from your program, you would call this method like this: SetImageFromResource("img.png") .
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void ImageButton::SetImageFromResource(const char* pzFile)
{
    Resources cCol(get_image_id());
    ResStream* pcStream = cCol.GetResourceStream(pzFile);
    Image* vImage = new BitmapImage(pcStream);
    m->SetImage(vImage);
} /*end SetImageFromResource()*/

/** Sets the Image from another image...
 * \par Description:
 * Sets the Image for the ImageButton from another Image.
 * \param pcImage - The image.
 * \author  Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/  	
void ImageButton::SetImageFromImage(Image* pcImage)
{
    m->SetImage(pcImage);
} /* end SetImageFromImage()*/

/*this does not work yet*/
void ImageButton::SetImageFromMemory( const void* pData, int nSize )
{
	MemFile*	cMemFile = new MemFile( pData, nSize );
	Image* vImage = new BitmapImage( cMemFile );
	SetImageFromImage( vImage );
}
 
/*reserved*/
void ImageButton::_reserved1(){}
void ImageButton::_reserved2(){}
void ImageButton::_reserved3(){}
void ImageButton::_reserved4(){}
void ImageButton::_reserved5(){}
void ImageButton::_reserved6(){}
void ImageButton::_reserved7(){}
void ImageButton::_reserved8(){}
void ImageButton::_reserved9(){}
void ImageButton::_reserved10(){}
/*end of file*/

