#ifndef __F_CIMAGEBUTTON_H__
#define __F_CIMAGEBUTTON_H__

#include <gui/button.h>
#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/image.h>
#include <gui/font.h>
#include <storage/file.h>
#include <util/resources.h>
#include <storage/memfile.h>

namespace os
{
#if 0
}
#endif


/** Imagebutton gui element...
 * \ingroup gui
 * \par Description:
 * Simple image pushbutton class... 
 * \author	Rick Caudill ( cau0730@cup.edu)
 *****************************************************************************/
class CImageButton : public Button
{
public:

    /** Positions
       * \par Description:
       *      These values are used to specify the positon of the image and label 
       *      on the imagebutton.
       */
    enum ImageButton_Position{
        /** Sets the text to the right of the imagebutton.*/
        IB_TEXT_RIGHT = 0x0001,
        /** Sets the text to the left of the imagebutton.*/
        IB_TEXT_LEFT = 0x0002,
        /** Sets the text to the top of the imagebutton.*/
        IB_TEXT_TOP  = 0x0003,
        /** Sets the text to the bottom of the imagebutton.*/
        IB_TEXT_BOTTOM = 0x0004
    } Positon;

public:
    CImageButton( Rect cFrame, const String& cName, const String& cLabel, Message *pcMessage, Image *pcBitmap, uint32 nTextPosition = CImageButton::IB_TEXT_BOTTOM, bool bShowFrames=false,bool bShowText=false, bool bMouse=false, uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    virtual ~CImageButton( );
   	
    uint32 GetTextPosition( void );
    void SetTextPosition( uint32 nTextPosition );

	void SetImage( StreamableIO* pcStream );
    void SetImage( Image* pcImage );
    
    void SetSelectedImage( StreamableIO* pcStream );
    void SetSelectedImage( Image* pcImage );

	Image *GetImage( void );
    void ClearImage();
    
    virtual void Activated(bool);
    virtual void MouseDown( const Point& cPosition, uint32 nButton );
    virtual void MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message* pcData);
    virtual void MouseUp( const Point& cPosition, uint32 nButton, Message* pcData );
    virtual void Paint( const Rect &cUpdateRect );   
    virtual Point GetPreferredSize( bool bLargest ) const;

	CImageButton& operator=( CImageButton& cImageButton );

private:
    /** \internal */
    class Private;
    Private *m;
};

}

#endif // __F_CIMAGEBUTTON_H__

