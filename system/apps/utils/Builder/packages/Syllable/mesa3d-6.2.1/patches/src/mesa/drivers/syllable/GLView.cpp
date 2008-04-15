#include <assert.h>
#include <stdio.h>

extern "C" {

#include "glheader.h"
#include "version.h"
#include "buffers.h"
#include "bufferobj.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mtypes.h"
#include "texformat.h"
#include "texobj.h"
#include "teximage.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

}	// extern "C"

#include "GLView.h"
#include "GLDriver.h"

#include <gui/bitmap.h>
#include <util/locker.h>

#ifdef DEBUG
	#define LOGOUT	printf
#else
	#define LOGOUT	(void *(0))
#endif

namespace os {

// Byte-order for RGBA
#define SYL_RCOMP 2
#define SYL_GCOMP 1
#define SYL_BCOMP 0
#define SYL_ACOMP 3

#define PACK_SYL_RGBA32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
							(color[RCOMP] << 16) | (color[ACOMP] << 24))

#define PACK_SYL_RGB32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
							(color[RCOMP] << 16) | 0xFF000000)


GLView::GLView( const Rect& cFrame, const String& cTitle,
      uint32 nOptions,
      uint32 nResizeMask,
      uint32 nFlags )
: View(cFrame,cTitle,nResizeMask,nFlags)
{
   LOGOUT("MesaGLView constructor\n");
   
   // We don't support single buffering (yet): double buffering forced.
   nOptions |= SYLGL_DOUBLE;

   const GLboolean rgbFlag = ((nOptions & SYLGL_INDEX) == 0); // RGB
   const GLboolean alphaFlag = ((nOptions & SYLGL_ALPHA) == SYLGL_ALPHA); // ALPHA channel?
   const GLboolean dblFlag = ((nOptions & SYLGL_DOUBLE) == SYLGL_DOUBLE); // Double buffering
   const GLboolean stereoFlag = false; // stereographic?
   const GLint depth = (nOptions & SYLGL_DEPTH) ? 16 : 0; // depth buffer bits
   const GLint stencil = (nOptions & SYLGL_STENCIL) ? 8 : 0; // stencil buffer bits
   const GLint accum = (nOptions & SYLGL_ACCUM) ? 16 : 0; // accumulation buffer bits
   const GLint index = (nOptions & SYLGL_INDEX) ? 32 : 0; // dunno
   const GLint red = rgbFlag ? 8 : 0;
   const GLint green = rgbFlag ? 8 : 0;
   const GLint blue = rgbFlag ? 8 : 0;
   const GLint alpha = alphaFlag ? 8 : 0;
   
   m_options = nOptions | SYLGL_INDIRECT;
   
   struct dd_function_table functions;
 
   if (!rgbFlag) {
      fprintf(stderr, "Mesa Warning: colour index mode not supported\n");
   }

  // Allocate auxiliary data object
  GLDriver *gldrv = new GLDriver();

  // examine option flags and create gl_context struct
  GLvisual * visual = _mesa_create_visual( rgbFlag,
                                            dblFlag,
                                            stereoFlag,
                                            red, green, blue, alpha,
                                            index,
                                            depth,
                                            stencil,
                                            accum, accum, accum, accum,
                                            1
                                            );

	// Initialize device driver function table
	_mesa_init_driver_functions(&functions);

	functions.GetString 	= gldrv->GetString;
	functions.UpdateState 	= gldrv->UpdateState;
	functions.GetBufferSize = gldrv->GetBufferSize;
	functions.Clear 		= gldrv->Clear;
	functions.ClearIndex 	= gldrv->ClearIndex;
	functions.ClearColor 	= gldrv->ClearColor;
	functions.Error			= gldrv->Error;
	functions.Flush			= _swrast_flush;

  // create core context
  GLcontext * ctx = _mesa_create_context(visual, NULL, &functions, gldrv);
  if (! ctx) {
         _mesa_destroy_visual(visual);
         delete gldrv;
         return;
      }  

  _mesa_initialize_context(ctx, visual, NULL, &functions, gldrv);

  _mesa_enable_sw_extensions(ctx);
  _mesa_enable_1_3_extensions(ctx);
  _mesa_enable_1_4_extensions(ctx);
  _mesa_enable_1_5_extensions(ctx);

  // create core framebuffer
  GLframebuffer * buffer = _mesa_create_framebuffer(visual,
                                              depth > 0 ? GL_TRUE : GL_FALSE,
                                              stencil > 0 ? GL_TRUE: GL_FALSE,
                                              accum > 0 ? GL_TRUE : GL_FALSE,
                                              alphaFlag
                                              );

  /* Initialize the software rasterizer and helper modules.
   */
  _swrast_CreateContext(ctx);
  _ac_CreateContext(ctx);
  _tnl_CreateContext(ctx);
  _swsetup_CreateContext(ctx);
  _swsetup_Wakeup(ctx);

  gldrv->Init(this, ctx, visual, buffer );
  
  // Hook aux data into GLView object
  m_drv = (void *)gldrv;
   
  // some stupid applications (Quake2) don't even think about calling LockGL()
  // before using glGetString and friends... so make sure there is at least a
  // valid context.
  if (!_mesa_get_current_context()) {
     LockGL();
     // not needed, we don't have a looper yet: UnlockLooper();
  }
  
  LOGOUT("MesaGLView constructor finished\n");
}

GLView::~GLView()
{
   LOGOUT("MesaGLView destructor\n");
   GLDriver * gldrv = (GLDriver *) m_drv;
   assert(gldrv);
   delete gldrv;
}

void GLView::__GLVW_reserved2__() {}
void GLView::__GLVW_reserved3__() {}
void GLView::__GLVW_reserved4__() {}
void GLView::__GLVW_reserved5__() {}
void GLView::__GLVW_reserved6__() {}
void GLView::__GLVW_reserved7__() {}
void GLView::__GLVW_reserved8__() {}
void GLView::__GLVW_reserved9__() {}
void GLView::__GLVW_reserved10__() {}
void GLView::__GLVW_reserved11__() {}
void GLView::__GLVW_reserved12__() {}
void GLView::__GLVW_reserved13__() {}
void GLView::__GLVW_reserved14__() {}
void GLView::__GLVW_reserved15__() {}
void GLView::__GLVW_reserved16__() {}
void GLView::__GLVW_reserved17__() {}
void GLView::__GLVW_reserved18__() {}
void GLView::__GLVW_reserved19__() {}
void GLView::__GLVW_reserved20__() {}
  
void GLView::LockGL()
{
	LOGOUT("LockGL started\n");
	GLDriver * gldrv = (GLDriver *) m_drv;
	assert(gldrv);
	gldrv->LockGL();
	LOGOUT("LockGL finished\n");
}

void GLView::UnlockGL()
{
	LOGOUT("UnlockGL started\n");
	GLDriver * gldrv = (GLDriver *) m_drv;
	assert(gldrv);
	gldrv->UnlockGL();
	LOGOUT("UnlockGL finished\n");
}

void GLView::SwapBuffers()
{
	LOGOUT("SwapBuffers started\n");
	GLDriver * gldrv = (GLDriver *) m_drv;
	assert(gldrv);
	gldrv->SwapBuffers();
	LOGOUT("SwapBuffers finished\n");
}

void GLView::CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height) const
{
	LOGOUT("CopySubBuffer started\n");
	GLDriver * gldrv = (GLDriver *) m_drv;
	assert(gldrv);
	gldrv->CopySubBuffer(x, y, width, height);
	LOGOUT("CopySubBuffer finished\n");
}

void GLView::ErrorCallback(unsigned long errorCode) // Mesa's GLenum is not ulong but uint!
{
	LOGOUT("ErrorCallback started\n");
	char msg[32];
	sprintf(msg, "GL: Error code $%04lx.", errorCode);
	// debugger(msg);
	fprintf(stderr, "%s\n", msg);
	LOGOUT("ErrorCallback finished\n");
}

void GLView::AttachedToWindow()
{
	LOGOUT("AttachedToWindow started\n");
	View::AttachedToWindow();
	
	// don't paint window background white when resized
	LOGOUT("AttachedToWindow finished\n");
}

void GLView::AllAttached()
{
	LOGOUT("AllAttached started\n");
	View::AllAttached();
	LOGOUT("AllAttached finished\n");
}

void GLView::DetachedFromWindow()
{
	LOGOUT("DetachedFromWindow started\n");
	View::DetachedFromWindow();
	LOGOUT("DetachedFromWindow finished\n");
}

void GLView::AllDetached()
{
	LOGOUT("AllDetached started\n");
	View::AllDetached();
	LOGOUT("AllDetached finished\n");
}

void GLView::Show()
{
	LOGOUT("Show started\n");
	View::Show();
	LOGOUT("Show finished\n");
}

void GLView::Hide()
{
	LOGOUT("Hide started\n");
	View::Hide();
	LOGOUT("Hide finished\n");
}

} // namespace os
