#ifndef __F_GUI_GRADIENTVIEW_H__
#define __F_GUI_GRADIENTVIEW_H__

#include <gui/view.h>

namespace os
{

/** GradientView
 * \ingroup gui
 * \par Description:
 * Displays a colour gradient.
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class GradientView : public View
{
	public:
	GradientView( const Rect& cRect, const String& cName, Color32 cColor1 = 0xFFFFFFFF, Color32 cColor2 = 0xFF000000, uint32 nOrientation = VERTICAL, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
	~GradientView();

	virtual void Paint( const Rect& cUpdateRect );
	virtual Point GetPreferredSize( bool bLargest ) const;
	virtual void FrameSized( const Point & cDelta );
	virtual void AttachedToWindow();
	virtual void HandleMessage( Message *pcMsg );

	virtual void SetColors( Color32 cColor1, Color32 cColor2 );

	private:
	virtual void 	_reserved1();
	virtual void 	_reserved2();
	virtual void 	_reserved3();
	virtual void 	_reserved4();
	virtual void 	_reserved5();

	private:
	void _Render();
	void _MakeBuffer();

	class Private;
	Private* m;
};

}

#endif	/* __F_GUI_GRADIENTVIEW_H__*/

