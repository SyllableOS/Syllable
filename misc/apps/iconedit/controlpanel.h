#ifndef __F_CONTROLPANEL_H__
#define __F_CONTROLPANEL_H__

#include <gui/view.h>

#include "bitmapscale.h"

using namespace os;

namespace os {
  class RadioButton;
}

class ControlPanel : public View
{
public:
  ControlPanel( const Rect& cFrame );

  bitmapscale_filtertype GetFilterType() { return( m_nFilter ); }
  
  virtual void	HandleMessage( Message* pcMessage );
  
  virtual void 	AttachedToWindow();
//  virtual void		Paint( const Rect& cUpdateRect );
//  virtual void		MouseMove( const Point2& cNewPos, int nCode, uint32 nButtons, Message* pcData );
//  virtual void		MouseDown( const Point2& cPosition, uint32 nButtons );
//  virtual void		MouseUp( const Point2& cPosition, uint32 nButtons, Message* pcData );
//  virtual void		KeyDown( int nChar, uint32 nQualifiers );
  
private:
  enum { NUM_FILTERS = 7 };
  RadioButton* m_apcFilterButtons[NUM_FILTERS];
  bitmapscale_filtertype m_nFilter;
};

#endif // __F_CONTROLPANEL_H__
