#ifndef __F_ICONVIEW_H__
#define __F_ICONVIEW_H__

#include <gui/view.h>

using namespace os;

namespace os {
  class Bitmap;
}

class IconView : public View
{
public:
  IconView( const Rect& cFrame );

  void SetBitmap( Bitmap* pcBitmap );
  
  virtual void		Paint( const Rect& cUpdateRect );
  virtual void		MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
  virtual void		MouseDown( const Point& cPosition, uint32 nButtons );
  virtual void		MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
  virtual void		KeyDown( int nChar, uint32 nQualifiers );
  
  
  bool LoadFile( const char* pzPath );
private:
  Bitmap* m_pcBitmap;
};

#endif // __F_ICONVIEW_H__
