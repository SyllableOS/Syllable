#if !defined GLVIEW_H
#define GLVIEW_H

#include <gui/view.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stdio.h>

#ifdef LO
	#define LOGOUT	printf
#else
	#define LOGOUT	(void *(0))
#endif

namespace os {


#define SYLGL_RGB			0x00000001
#define SYLGL_INDEX			0x00000002

#define SYLGL_SINGLE		0x00000004
#define SYLGL_DOUBLE		0x00000008

#define SYLGL_STEREO		0x00000010

#define SYLGL_DIRECT		0x00000020
#define SYLGL_INDIRECT		0x00000040

#define SYLGL_ACCUM			0x00000040
#define SYLGL_ALPHA			0x00000100
#define SYLGL_DEPTH			0x00000200

#define SYLGL_OVERLAY		0x00000400
#define SYLGL_UNDERLAY		0x00000800

#define SYLGL_STENCIL		0x00001000

							
class GLView : public View
{
public:

  GLView( const Rect& cFrame, const String& cTitle,
      uint32 nOptions,
      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
      uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
	
  virtual ~GLView();

  void LockGL();
  void UnlockGL();
  void SwapBuffers();
  void CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height) const;
  virtual void ErrorCallback(unsigned long errorCode); // GLenum errorCode);
  
  
  void AttachedToWindow();
  void AllAttached();
  void DetachedFromWindow();
  void AllDetached();
 
  void Show();
  void Hide();
	

private:
  virtual void	__GLVW_reserved2__();
  virtual void	__GLVW_reserved3__();
  virtual void	__GLVW_reserved4__();
  virtual void	__GLVW_reserved5__();
  virtual void	__GLVW_reserved6__();
  virtual void	__GLVW_reserved7__();
  virtual void	__GLVW_reserved8__();
  virtual void	__GLVW_reserved9__();
  virtual void	__GLVW_reserved10__();
  virtual void	__GLVW_reserved11__();
  virtual void	__GLVW_reserved12__();
  virtual void	__GLVW_reserved13__();
  virtual void	__GLVW_reserved14__();
  virtual void	__GLVW_reserved15__();
  virtual void	__GLVW_reserved16__();
  virtual void	__GLVW_reserved17__();
  virtual void	__GLVW_reserved18__();
  virtual void	__GLVW_reserved19__();
  virtual void	__GLVW_reserved20__();

  void* m_drv;
  uint32 m_options;
};

} // namespace os

#endif
