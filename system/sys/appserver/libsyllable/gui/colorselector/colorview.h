#ifndef __F_GUI_COLORVIEW_H__
#define __F_GUI_COLORVIEW_H__

#include <gui/tabview.h>

namespace os
{

/** ColorView
 * \ingroup gui
 * \par Description:
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class ColorView : public View
{
	public:
	ColorView( const Rect& cRect, const String& cName, Color32_s cColor = 0xFFFFFFFF, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
	~ColorView();

	void SetColor( const Color32_s &cColor );
	virtual Point GetPreferredSize( bool bLargest ) const;

	virtual void Paint( const Rect& cUpdateRect );
/*	virtual void MouseDown( const Point& cPosition, uint32 nButtons );
	virtual void MouseUp( const Point& cPosition, uint32 nButtons, Message * pcData );
	virtual void MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData );
	virtual void FrameSized( const Point & cDelta );
	virtual void AttachedToWindow();
	virtual void HandleMessage( Message *pcMsg );
	virtual void PostValueChange( const Variant& cNewValue );
    virtual bool Invoked( Message* pcMessage );*/

	private:
	virtual void 	_reserved1();
	virtual void 	_reserved2();
	virtual void 	_reserved3();
	virtual void 	_reserved4();
	virtual void 	_reserved5();

	private:

	class Private;
	Private* m;
};

}

#endif	/* __F_GUI_COLORVIEW_H__*/



