#ifndef __F_GUI_COLORWHEEL_H__
#define __F_GUI_COLORWHEEL_H__

#include <gui/control.h>
#include <gui/slider.h>
#include "gradientview.h"

namespace os
{

/** ColorWheel
 * \ingroup gui
 * \par Description:
 * ColorWheel, a colour selection widget.
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class ColorWheel : public Control
{
	public:
	ColorWheel( const Rect& cRect, const String& cName, Message* pcMsg, Color32_s cColor = 0xFFFFFFFF, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
	~ColorWheel();

	virtual void Paint( const Rect& cUpdateRect );
	virtual void MouseDown( const Point& cPosition, uint32 nButtons );
	virtual void MouseUp( const Point& cPosition, uint32 nButtons, Message * pcData );
	virtual void MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData );
	virtual Point GetPreferredSize( bool bLargest ) const;
	virtual void FrameSized( const Point & cDelta );
	virtual void EnableStatusChanged( bool );
	virtual void AttachedToWindow();
	virtual void HandleMessage( Message *pcMsg );
	virtual void PostValueChange( const Variant& cNewValue );
    virtual bool Invoked( Message* pcMessage );

	virtual void SetValue( const Variant& cNewValue, bool bInvoke = true );

	void SetBrightness( float vBrightness );
	float GetBrightness() const;

	virtual bool SetPosition( const Point& cPosition, bool bUpdate = true );
	virtual Point GetPosition() const;

	void SetForceCircular( bool bCircular = true );
	bool GetForceCircular() const;

	void SetBrightnessSlider( Slider* pcSlider );
	void SetGradientView( GradientView* pcGradient );

	private:
	virtual void 	_reserved1();
	virtual void 	_reserved2();
	virtual void 	_reserved3();
	virtual void 	_reserved4();
	virtual void 	_reserved5();

	private:
	inline bool _CalcWheelColor( int32 x, int32 y, uint32* col, bool bMargin );
	bool _CalcWheelHSB( int32 x, int32 y, uint32* col );
	uint32 _CalcColor32Val();
	void _RenderKnob();
	Point _CalcKnobPos();
	void _RenderWheel();
	void _MakeWheelBuffer();

	class Private;
	Private* m;
};

}

#endif	/* __F_GUI_COLORWHEEL_H__*/


