#ifndef __F_EDITVIEW_H__
#define __F_EDITVIEW_H__

#include <gui/view.h>

#include "bitmapscale.h"

using namespace os;

class IconView;

class EditView : public View
{
public:
  EditView( const Rect& cFrame );

  void  SetFilterType( bitmapscale_filtertype nFilter ) { m_nFilterType = nFilter; }
  void	GenerateSmall();
  void	GenerateLarge();

  void	Save( const char* pzName );
  void  Load( const char* pzName );
  
  virtual void		Paint( const Rect& cUpdateRect );
  virtual void		MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
  virtual void		MouseDown( const Point& cPosition, uint32 nButtons );
  virtual void		MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
  virtual void		KeyDown( int nChar, uint32 nQualifiers );
  
private:
  enum { ID_DRAG_LARGE, ID_DRAG_SMALL };
  IconView* 		 m_pcScaleIconView;
  bitmapscale_filtertype m_nFilterType;
  bool	    		 m_bLargeSelected;
  Rect	    		 m_cLargeRect;
  Rect	    		 m_cSmallRect;
  
  Bitmap*   		 m_pcLargeBitmap;
  Bitmap*   		 m_pcSmallBitmap;
};

#endif // __F_EDITVIEW_H__
