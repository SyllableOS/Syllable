#ifndef __F_PDFVIEW_H__
#define __F_PDFVIEW_H__

#include <gui/view.h>

using namespace os;

namespace os {
  class Bitmap;
};


class PDFView : public View
{
public:
  PDFView( const Rect& cFrame );
  virtual void  MouseDown( const Point& cPosition, uint32 nButtons );
  virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
  virtual void	Paint( const Rect& cUpdateRect );

private:
  friend int32 render_thread( void* pData );
  void ReadBitmap();
  void LoadPage();
  Bitmap*   m_pcBitmap;
  thread_id m_hRenderThread;
  int	    m_nPage;
  int	    m_nVisiblePage;
};

#endif // __F_PDFVIEW_H__
