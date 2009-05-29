#if !defined GLVIEW_H
#define GLVIEW_H

#include <gui/view.h>

namespace os {

enum gl_colour_format
{
  SYLGL_COLOUR_RGB888,
  SYLGL_COLOUR_MONO8
};

const uint32 SYLGL_DOUBLE_BUFFER = 0x0001;

struct GLParams
{
  uint32 flags;

  gl_colour_format colour_format;
  uint32 depth_bits;
  uint32 stencil_bits;
  uint32 accum_bits;

  GLParams();
};

class GLView : public View
{
public:

  GLView( const Rect& cFrame, const std::string& cTitle,
      const GLParams& params,
      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
      uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
	
  virtual ~GLView();

  void LockGL();
  void UnlockGL();
  void SwapBuffers();
  void CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height) const;

private:
  class Private;
  Private* m;
};

} // namespace os

#endif
