#ifndef __F_GUI_CHARMAP_H__
#define __F_GUI_CHARMAP_H__

#include <gui/control.h>

//namespace os
//{

/** CharMapView
 * \ingroup gui
 * \par Description:
 * Displays a colour gradient.
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class CharMapView : public os::Control
{
	public:
	CharMapView( const os::Rect& cRect, const os::String& cName, os::Message* pcMsg, uint32 nResizeMask=os::CF_FOLLOW_LEFT|os::CF_FOLLOW_TOP, uint32 nFlags = os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE );
	~CharMapView();

	virtual void Paint( const os::Rect& cUpdateRect );
	virtual os::Point GetPreferredSize( bool bLargest ) const;
	virtual void FrameSized( const os::Point & cDelta );
	virtual void AttachedToWindow();

    virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData );
    virtual void	FontChanged( os::Font* pcNewFont );

	void SetCharMap( std::vector<uint32> cMap );
	void SetLine( int nLine );
	os::IPoint GetCharPosition( const os::Point& cPos );
	uint32 GetCharAtPosition( const os::IPoint& cPos );
	
	uint32 GetNumberOfRows() const;
	
	void SetMouseOverMessage( os::Message* pcMsg );
	void SetScrollBarChangeMessage( os::Message* pcMsg );

	virtual void EnableStatusChanged( bool ) {}

	private:
	virtual void 	_reserved1();
	virtual void 	_reserved2();
	virtual void 	_reserved3();
	virtual void 	_reserved4();
	virtual void 	_reserved5();

	private:
	void _Render( os::View* pcView, const os::Rect& cUpdateRect );
	void _MakeBuffer();
	void _UpdateScrollBar();
	void _Invalidate( os::IPoint cBlock );

	class Private;
	Private* m;
};

//}

#endif	/* __F_GUI_GRADIENTVIEW_H__*/

