
#include <assert.h>

extern "C" {

#include "glheader.h"
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

}	// extern "C"

#include "GLView.h"

#include <gui/bitmap.h>
#include <util/locker.h>

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

class GLView::Private
{
public:
  
  Private();
  ~Private();

  void Init(GLView * glview, GLcontext * ctx, GLvisual * visual, GLframebuffer * b);

  void LockGL();
  void UnlockGL();
  void SwapBuffers();
  void CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height) const;

private:
  GLcontext* m_GLContext;
  GLvisual* m_GLVisual;
  GLframebuffer* m_GLFramebuffer;

  Locker* m_Lock;

  GLView*  m_View;
  Bitmap*  m_BackBuffer; 

  GLchan m_ClearColor[4];  // buffer clear color
  GLuint m_ClearIndex;      // buffer clear color index
  GLint  m_Bottom;           // used for flipping Y coords
  GLuint m_Width;
  GLuint m_Height;

  static void UpdateState(GLcontext *ctx, GLuint new_state);

  static void ClearIndex(GLcontext *ctx, GLuint index);

  static void ClearColor(GLcontext *ctx, const GLfloat color[4]);

  static void Clear(GLcontext *ctx, GLbitfield mask,
                                GLboolean all, GLint x, GLint y,
                                GLint width, GLint height);

   static void ClearFront(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                          GLint width, GLint height);

   static void ClearBack(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                         GLint width, GLint height);

   static void Index(GLcontext *ctx, GLuint index);

   static void Color(GLcontext *ctx, GLubyte r, GLubyte g,
                     GLubyte b, GLubyte a);

   static void SetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                             GLenum mode);

   static void GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                             GLuint *height);

   static const GLubyte* GetString(GLcontext *ctx, GLenum name);

   // Front-buffer functions
   static void WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);

   static void WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);

   static void WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                      GLint x, GLint y,
                                      const GLchan color[4],
                                      const GLubyte mask[]);

   static void WriteRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    CONST GLubyte rgba[][4],
                                    const GLubyte mask[]);

   static void WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                        const GLint x[], const GLint y[],
                                        const GLchan color[4],
                                        const GLubyte mask[]);

   static void WriteCI32SpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  const GLuint index[], const GLubyte mask[]);

   static void WriteCI8SpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLubyte index[], const GLubyte mask[]);

   static void WriteMonoCISpanFront(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[]);

   static void WriteCI32PixelsFront(const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[]);

   static void WriteMonoCIPixelsFront(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[]);

   static void ReadCI32SpanFront(const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[]);

   static void ReadRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 GLubyte rgba[][4]);

   static void ReadCI32PixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[]);

   static void ReadRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[]);

   // Back buffer functions
   static void WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);

   static void WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);

   static void WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[]);

   static void WriteRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[]);

   static void WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[]);

   static void WriteCI32SpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[]);

   static void WriteCI8SpanBack(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[]);

   static void WriteMonoCISpanBack(const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y, GLuint colorIndex,
                                   const GLubyte mask[]);

   static void WriteCI32PixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[]);

   static void WriteMonoCIPixelsBack(const GLcontext *ctx,
                                     GLuint n, const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[]);

   static void ReadCI32SpanBack(const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[]);

   static void ReadRGBASpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                GLubyte rgba[][4]);

   static void ReadCI32PixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLuint indx[], const GLubyte mask[]);

   static void ReadRGBAPixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[]);

};

GLParams::GLParams()
: flags(0),
  colour_format(SYLGL_COLOUR_RGB888),
  depth_bits(0),
  stencil_bits(0),
  accum_bits(0)
{
}

GLView::GLView( const Rect& cFrame, const std::string& cTitle,
      const GLParams& params,
      uint32 nResizeMask,
      uint32 nFlags )
: View(cFrame,cTitle,nResizeMask,nFlags)
{
   printf("MesaGLView constructor\n");

   const GLboolean rgbFlag = (params.colour_format == SYLGL_COLOUR_RGB888); // RGB
   const GLboolean alphaFlag = 0; // ALPHA channel?
   const GLboolean dblFlag = (params.flags & SYLGL_DOUBLE_BUFFER); // Double buffering
   const GLboolean stereoFlag = false; // stereographic?
   const GLint depth = params.depth_bits; // depth buffer bits
   const GLint stencil = params.stencil_bits; // stencil buffer bits
   const GLint accum = params.accum_bits; // accumulation buffer bits
   const GLint index = 0; // dunno
   const GLint red = 8;
   const GLint green = 8;
   const GLint blue = 8;
   const GLint alpha = 0;

  m = new Private;

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

  // create core context
  GLcontext * ctx = _mesa_create_context(visual, NULL, m, GL_FALSE);

  ctx->Driver.NewTextureObject = _mesa_new_texture_object;
  ctx->Driver.DeleteTexture = _mesa_delete_texture_object;

  _mesa_initialize_context(ctx, visual, NULL, m, GL_FALSE);

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

  m->Init(this, ctx, visual, buffer );

}
	
GLView::~GLView()
{
   printf("MesaGLView destructor\n");
   assert(m);
   delete m;

}

void GLView::LockGL()
{
  assert(m);
  m->LockGL();
}

void GLView::UnlockGL()
{
  assert(m);
  m->UnlockGL();
}

void GLView::SwapBuffers()
{
  assert(m);
  m->SwapBuffers();

}

void GLView::CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height) const
{
   assert(m);
   m->CopySubBuffer(x, y, width, height);

}

/*************************************************************************/

GLView::Private::Private()
{
  m_GLContext 		= 0;
  m_GLVisual		= 0;
  m_GLFramebuffer 	= 0;
  m_Lock = new Locker("GLView::Private");
  m_View 		= 0;
  m_BackBuffer 		= 0;

  m_ClearColor[0] = 0;
  m_ClearColor[1] = 0;
  m_ClearColor[2] = 0;
  m_ClearColor[3] = 0;

  m_ClearIndex = 0;
}


GLView::Private::~Private()
{
  m_Lock->Lock();

   _mesa_destroy_visual(m_GLVisual);
   _mesa_destroy_framebuffer(m_GLFramebuffer);
   _mesa_destroy_context(m_GLContext);

  m_Lock->Unlock();

  delete m_Lock;
}

void GLView::Private::Init(GLView * glview, 
                           GLcontext * ctx, 
                           GLvisual * visual, GLframebuffer * framebuffer)
{
  m_Lock->Lock();

	m_View 		= glview;
	m_GLContext 	= ctx;
	m_GLVisual 		= visual;
	m_GLFramebuffer = framebuffer;

	Private* md = (Private*) ctx->DriverCtx;

	struct swrast_device_driver * swdd = _swrast_GetDeviceDriverReference( ctx );
	TNLcontext * tnl = TNL_CONTEXT(ctx);

	assert(md->m_GLContext == ctx );

	// Use default TCL pipeline
	tnl->Driver.RunPipeline = _tnl_run_pipeline;

	ctx->Driver.GetString = GetString;
	ctx->Driver.UpdateState = UpdateState;
	ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
	ctx->Driver.GetBufferSize = GetBufferSize;

	ctx->Driver.Accum = _swrast_Accum;
	ctx->Driver.Bitmap = _swrast_Bitmap;
	ctx->Driver.Clear = Clear;
	// ctx->Driver.ClearIndex = ClearIndex;
	 ctx->Driver.ClearColor = ClearColor;
	ctx->Driver.CopyPixels = _swrast_CopyPixels;
   	ctx->Driver.DrawPixels = _swrast_DrawPixels;
   	ctx->Driver.ReadPixels = _swrast_ReadPixels;
   	ctx->Driver.DrawBuffer = _swrast_DrawBuffer;

   	ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
   	ctx->Driver.TexImage1D = _mesa_store_teximage1d;
   	ctx->Driver.TexImage2D = _mesa_store_teximage2d;
   	ctx->Driver.TexImage3D = _mesa_store_teximage3d;
   	ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
   	ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
   	ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
   	ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

    ctx->Driver.CompressedTexImage1D = _mesa_store_compressed_teximage1d;
    ctx->Driver.CompressedTexImage2D = _mesa_store_compressed_teximage2d;
	ctx->Driver.CompressedTexImage3D = _mesa_store_compressed_teximage3d;
	ctx->Driver.CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
	ctx->Driver.CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
	ctx->Driver.CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;

  	ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
   	ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
   	ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   	ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   	ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
   	ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   	ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   	ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   	ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
 
	swdd->SetBuffer = SetBuffer;

  m_Lock->Unlock();

}

void GLView::Private::LockGL()
{
//TODO: make this thread safe!
  m_Lock->Lock();

  UpdateState(m_GLContext, 0);
  _mesa_make_current(m_GLContext, m_GLFramebuffer);

}

void GLView::Private::UnlockGL()
{
//TODO: make this thread safe!
//	m_View->Unlock();
   // Could call _mesa_make_current(NULL, NULL) but it would just
   // hinder performance

  m_Lock->Unlock();
}

void GLView::Private::SwapBuffers()
{
//TODO: make this thread safe!
  m_Lock->Lock();

  // _mesa_notifySwapBuffers(m_glcontext);

  //printf("SwapBuffers %i\n",m_BackBuffer);

	if (m_BackBuffer) {
		//m_View->Lock();
        Rect r = m_BackBuffer->GetBounds();
		m_View->DrawBitmap(m_BackBuffer,r,r);
		//m_View->UnlockLooper();
	}

  m_Lock->Unlock();

}

void GLView::Private::CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height) const
{
//TODO: make this thread safe!
  m_Lock->Lock();

   if (m_BackBuffer) {
      // Source bitmap and view's bitmap are same size.
      // Source and dest rectangle are the same.
      // Note (x,y) = (0,0) is the lower-left corner, have to flip Y
      Rect srcAndDest;
      srcAndDest.left = x;
      srcAndDest.right = x + width - 1;
      srcAndDest.bottom = m_Bottom - y;
      srcAndDest.top = srcAndDest.bottom - height + 1;
      m_View->DrawBitmap(m_BackBuffer, srcAndDest, srcAndDest);
   }

  m_Lock->Unlock();

}

void GLView::Private::UpdateState( GLcontext *ctx, GLuint new_state )
{
	struct swrast_device_driver *	swdd = _swrast_GetDeviceDriverReference( ctx );

	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_ac_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );

	if (ctx->Color.DrawBuffer == GL_FRONT) {
      /* read/write front buffer */
      swdd->WriteRGBASpan = WriteRGBASpanFront;
      swdd->WriteRGBSpan = WriteRGBSpanFront;
      swdd->WriteRGBAPixels = WriteRGBAPixelsFront;
      swdd->WriteMonoRGBASpan = WriteMonoRGBASpanFront;
      swdd->WriteMonoRGBAPixels = WriteMonoRGBAPixelsFront;
      swdd->WriteCI32Span = WriteCI32SpanFront;
      swdd->WriteCI8Span = WriteCI8SpanFront;
      swdd->WriteMonoCISpan = WriteMonoCISpanFront;
      swdd->WriteCI32Pixels = WriteCI32PixelsFront;
      swdd->WriteMonoCIPixels = WriteMonoCIPixelsFront;
      swdd->ReadRGBASpan = ReadRGBASpanFront;
      swdd->ReadRGBAPixels = ReadRGBAPixelsFront;
      swdd->ReadCI32Span = ReadCI32SpanFront;
      swdd->ReadCI32Pixels = ReadCI32PixelsFront;
   }
   else {
      /* read/write back buffer */
      swdd->WriteRGBASpan = WriteRGBASpanBack;
      swdd->WriteRGBSpan = WriteRGBSpanBack;
      swdd->WriteRGBAPixels = WriteRGBAPixelsBack;
      swdd->WriteMonoRGBASpan = WriteMonoRGBASpanBack;
      swdd->WriteMonoRGBAPixels = WriteMonoRGBAPixelsBack;
      swdd->WriteCI32Span = WriteCI32SpanBack;
      swdd->WriteCI8Span = WriteCI8SpanBack;
      swdd->WriteMonoCISpan = WriteMonoCISpanBack;
      swdd->WriteCI32Pixels = WriteCI32PixelsBack;
      swdd->WriteMonoCIPixels = WriteMonoCIPixelsBack;
      swdd->ReadRGBASpan = ReadRGBASpanBack;
      swdd->ReadRGBAPixels = ReadRGBAPixelsBack;
      swdd->ReadCI32Span = ReadCI32SpanBack;
      swdd->ReadCI32Pixels = ReadCI32PixelsBack;
    }
}

void GLView::Private::ClearIndex(GLcontext *ctx, GLuint index)
{
   Private* md = (Private*) ctx->DriverCtx;
   md->m_ClearIndex = index;

}


void GLView::Private::ClearColor(GLcontext *ctx, const GLfloat color[4])
{
  /*
  printf("ClearColor %f %f %f %f\n",color[0],color[1],color[2],color[3]);
  */
 
   Private* md = (Private*) ctx->DriverCtx;
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_RCOMP], color[0]);
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_GCOMP], color[1]);
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_BCOMP], color[2]);
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_ACOMP], color[3]); 


  /*
  printf("%i %i %i %i\n",md->m_ClearColor[SYL_RCOMP],
  md->m_ClearColor[SYL_GCOMP],
  md->m_ClearColor[SYL_BCOMP],
  md->m_ClearColor[SYL_ACOMP]);
  */

   assert(md->m_View);
}

void GLView::Private::Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height)
{
  //  printf("Clear\n");

   if (mask & DD_FRONT_LEFT_BIT)
		ClearFront(ctx, all, x, y, width, height);
   if (mask & DD_BACK_LEFT_BIT)
		ClearBack(ctx, all, x, y, width, height);

	mask &= ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);

	if (mask)
   {
 
    //printf("_swrast_Clear\n");
     _swrast_Clear( ctx, mask, all, x, y, width, height );
   }

   return;
}

void GLView::Private::ClearFront(GLcontext *ctx,
                         GLboolean all, GLint x, GLint y,
                         GLint width, GLint height)
{

  //printf("ClearFront\n");

   Private* md = (Private*) ctx->DriverCtx;
   GLView* view = md->m_View;

   assert(view);

   view->SetFgColor(md->m_ClearColor[SYL_RCOMP],
                         md->m_ClearColor[SYL_GCOMP],
                         md->m_ClearColor[SYL_BCOMP],
                         md->m_ClearColor[SYL_ACOMP]);
   view->SetBgColor(md->m_ClearColor[SYL_RCOMP],
                        md->m_ClearColor[SYL_GCOMP],
                        md->m_ClearColor[SYL_BCOMP],
                        md->m_ClearColor[SYL_ACOMP]);
   if (all) {
      Rect b = view->GetBounds();
      view->FillRect(b);
   }
   else {
      // XXX untested
      Rect b;
      b.left = x;
      b.right = x + width;
      b.bottom = md->m_Height - y - 1;
      b.top = b.bottom - height;
      view->FillRect(b);
   }

   // restore drawing color
#if 0
   bglview->SetHighColor(md->mColor[BE_RCOMP],
                         md->mColor[BE_GCOMP],
                         md->mColor[BE_BCOMP],
                         md->mColor[BE_ACOMP]);
   bglview->SetLowColor(md->mColor[BE_RCOMP],
                        md->mColor[BE_GCOMP],
                        md->mColor[BE_BCOMP],
                        md->mColor[BE_ACOMP]);
#endif
}

void GLView::Private::ClearBack(GLcontext *ctx,
                        GLboolean all, GLint x, GLint y,
                        GLint width, GLint height)
{
   Private *md = (Private *) ctx->DriverCtx;

  //printf("ClearBack %i %i %i %i\n",x,y,width,height);

   GLView* view = md->m_View;
   assert(view);

   Bitmap* bitmap = md->m_BackBuffer;
   assert(bitmap);

   GLuint* start = (GLuint *) bitmap->LockRaster();

   const GLuint *clearPixelPtr = (const GLuint *) md->m_ClearColor;
   const GLuint clearPixel = *clearPixelPtr;

   if (all) {
      const int numPixels = md->m_Width * md->m_Height;
      if (clearPixel == 0) {
         memset(start, 0, numPixels * 4);
      }
      else {
         for (int i = 0; i < numPixels; i++) {
             start[i] = clearPixel;
         }
      }
   }
   else {
      // XXX untested
      start += y * md->m_Width + x;
      for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
            start[j] = clearPixel;
         }
         start += md->m_Width;
      }
   }

   bitmap->UnlockRaster();

}

void GLView::Private::SetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                            GLenum mode)
{
   /* TODO */
	(void) ctx;
	(void) buffer;
	(void) mode;
}

void GLView::Private::GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                            GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx)
		return;

   Private* md = (Private*) ctx->DriverCtx;
   GLView* view = md->m_View;
   assert(view);

   Rect b = view->GetBounds();
   *width = (GLuint) (b.right - b.left + 1);
   *height = (GLuint) (b.bottom - b.top + 1);
   md->m_Bottom = (GLint) b.bottom;

   if (ctx->Visual.doubleBufferMode) {
      if (*width != md->m_Width || *height != md->m_Height) {
         // allocate new size of back buffer bitmap
         if(md->m_BackBuffer)
            delete md->m_BackBuffer;

         Rect rect(0.0, 0.0, *width - 1, *height - 1);
         md->m_BackBuffer = new Bitmap(*width,*height, CS_RGBA32);
      }
   }
   else
   {
      md->m_BackBuffer = NULL;
   }


   md->m_Width = *width;
   md->m_Height = *height;
}


const GLubyte* GLView::Private::GetString(GLcontext *ctx, GLenum name)
{
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa os::GLView (software)";
      default:
         // Let core library handle all other cases
         return NULL;
   }
}

void GLView::Private::WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
   Private *md = (Private *) ctx->DriverCtx;
   GLView *glview = md->m_View;
   assert(glview);
   int flippedY = md->m_Bottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            //bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
            //Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         //bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
         //Plot(bglview, x++, flippedY);
      }
   }
}

void GLView::Private::WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgba[][3],
                                const GLubyte mask[])
{
  //TODO:
}

void GLView::Private::WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[])
{
  //TODO:
}

void GLView::Private::WriteRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
  //TODO:
}


void GLView::Private::WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[])
{
  //TODO:
}


void GLView::Private::WriteCI32SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::WriteCI8SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::WriteMonoCISpanFront( const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLView::Private::WriteCI32PixelsFront( const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::WriteMonoCIPixelsFront( const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLView::Private::ReadCI32SpanFront( const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[] )
{
 	printf("ReadCI32SpanFront() not implemented yet!\n");
  //TODO:
}


void GLView::Private::ReadRGBASpanFront( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y, GLubyte rgba[][4] )
{
 	printf("ReadRGBASpanFront() not implemented yet!\n");
   //TODO:
}


void GLView::Private::ReadCI32PixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	printf("ReadCI32PixelsFront() not implemented yet!\n");
   //TODO:
}


void GLView::Private::ReadRGBAPixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[] )
{
 	printf("ReadRGBAPixelsFront() not implemented yet!\n");
   //TODO:
}

void GLView::Private::WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
	Private *md = (Private *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_BackBuffer;

	static bool already_called = false;
	if (! already_called) {
		//printf("WriteRGBASpanBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	int row = md->m_Bottom - y;
//	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
	uint8 * ptr = (uint8 *) bitmap->LockRaster() + (row * bitmap->GetBytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	

	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_SYL_RGBA32(rgba[0]);
			pixel++;
			rgba++;
		}
	} else {
		while(n--) {
			*pixel++ = PACK_SYL_RGBA32(rgba[0]);
			rgba++;
		}
	}

  bitmap->UnlockRaster();

}

void GLView::Private::WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgb[][3],
                                const GLubyte mask[])
{
	Private *md = (Private *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_BackBuffer;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteRGBSpanBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	int row = md->m_Bottom - y;
//	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
	uint8 * ptr = (uint8 *) bitmap->LockRaster() + (row * bitmap->GetBytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_SYL_RGB32(rgb[0]);
			pixel++;
			rgb++;
		}
	} else {
		while(n--) {
			*pixel++ = PACK_SYL_RGB32(rgb[0]);
			rgb++;
		}
	}
  bitmap->UnlockRaster();
}

void GLView::Private::WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    const GLchan color[4], const GLubyte mask[])
{
	Private *md = (Private *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_BackBuffer;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteMonoRGBASpanBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	int row = md->m_Bottom - y;
//	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint8 * ptr = (uint8 *) bitmap->LockRaster() + (row * bitmap->GetBytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	uint32 pixel_color = PACK_SYL_RGBA32(color);
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = pixel_color;
			pixel++;
		}
	} else {
		while(n--) {
			*pixel++ = pixel_color;
		}
	}
  bitmap->UnlockRaster();
}

void GLView::Private::WriteRGBAPixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   Private *md = (Private *) ctx->DriverCtx;
   Bitmap *bitmap = md->m_BackBuffer;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteRGBAPixelsBack() called.\n");
		already_called = true;
	}

	assert(bitmap);
#if 0
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;
			*((uint32 *) pixel) = PACK_B_RGBA32(rgba[0]);
		};
		x++;
		y++;
		rgba++;
	};
#else
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
//            GLubyte *pixel = (GLubyte *) bitmap->Bits()
//            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
            GLubyte *pixel = (GLubyte *) bitmap->LockRaster()
            + ((md->m_Bottom - y[i]) * bitmap->GetBytesPerRow()) + x[i] * 4;
            pixel[SYL_RCOMP] = rgba[i][RCOMP];
            pixel[SYL_GCOMP] = rgba[i][GCOMP];
            pixel[SYL_BCOMP] = rgba[i][BCOMP];
            pixel[SYL_ACOMP] = rgba[i][ACOMP];
  bitmap->UnlockRaster();
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
//         GLubyte *pixel = (GLubyte *) bitmap->Bits()
//            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
           GLubyte *pixel = (GLubyte *) bitmap->LockRaster()
           + ((md->m_Bottom - y[i]) * bitmap->GetBytesPerRow()) + x[i] * 4;
         pixel[SYL_RCOMP] = rgba[i][RCOMP];
         pixel[SYL_GCOMP] = rgba[i][GCOMP];
         pixel[SYL_BCOMP] = rgba[i][BCOMP];
         pixel[SYL_ACOMP] = rgba[i][ACOMP];
  bitmap->UnlockRaster();
      }
   }
#endif
}

void GLView::Private::WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      const GLchan color[4],
                                      const GLubyte mask[])
{
	Private *md = (Private *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_BackBuffer;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteMonoRGBAPixelsBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	uint32 pixel_color = PACK_SYL_RGBA32(color);
#if 0	
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;

			*((uint32 *) pixel) = pixel_color;
		};
		x++;
		y++;
	};
#else
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
//         	GLubyte * ptr = (GLubyte *) bitmap->Bits()
//            	+ ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
         	GLubyte * ptr = (GLubyte *) bitmap->LockRaster()
            	+ ((md->m_Bottom - y[i]) * bitmap->GetBytesPerRow()) + x[i] * 4;
            *((uint32 *) ptr) = pixel_color;
  bitmap->UnlockRaster();
         }
      }
   }
   else {
	  for (GLuint i = 0; i < n; i++) {
//       	GLubyte * ptr = (GLubyte *) bitmap->Bits()
//	           	+ ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
       	GLubyte * ptr = (GLubyte *) bitmap->LockRaster()
           	+ ((md->m_Bottom - y[i]) * bitmap->GetBytesPerRow()) + x[i] * 4;
       *((uint32 *) ptr) = pixel_color;
  bitmap->UnlockRaster();
      }
   }
#endif
}

void GLView::Private::WriteCI32SpanBack( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::WriteCI8SpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::WriteMonoCISpanBack( const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLView::Private::WriteCI32PixelsBack( const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::WriteMonoCIPixelsBack( const GLcontext *ctx, GLuint n,
                                     const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLView::Private::ReadCI32SpanBack( const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[] )
{
   //TODO:
}

void GLView::Private::ReadRGBASpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, GLubyte rgba[][4] )
{
   Private *md = (Private *) ctx->DriverCtx;
   Bitmap *bitmap = md->m_BackBuffer;
   assert(bitmap);
   int row = md->m_Bottom - y;
//   const GLubyte *pixel = (GLubyte *) bitmap->Bits()
//                        + (row * bitmap->BytesPerRow()) + x * 4;
   const GLubyte *pixel = (GLubyte *) bitmap->LockRaster()
                        + (row * bitmap->GetBytesPerRow()) + x * 4;

   for (GLuint i = 0; i < n; i++) {
      rgba[i][RCOMP] = pixel[SYL_RCOMP];
      rgba[i][GCOMP] = pixel[SYL_GCOMP];
      rgba[i][BCOMP] = pixel[SYL_BCOMP];
      rgba[i][ACOMP] = pixel[SYL_ACOMP];
      pixel += 4;
   }

  bitmap->UnlockRaster();

}

void GLView::Private::ReadCI32PixelsBack( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
   //TODO:
}

void GLView::Private::ReadRGBAPixelsBack( const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] )
{
   Private *md = (Private *) ctx->DriverCtx;
   Bitmap *bitmap = md->m_BackBuffer;
   assert(bitmap);

   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
//            GLubyte *pixel = (GLubyte *) bitmap->Bits()
//            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
            GLubyte *pixel = (GLubyte *) bitmap->LockRaster()
            + ((md->m_Bottom - y[i]) * bitmap->GetBytesPerRow()) + x[i] * 4;
	         rgba[i][RCOMP] = pixel[SYL_RCOMP];
    	     rgba[i][GCOMP] = pixel[SYL_GCOMP];
        	 rgba[i][BCOMP] = pixel[SYL_BCOMP];
        	 rgba[i][ACOMP] = pixel[SYL_ACOMP];
  bitmap->UnlockRaster();
         }
      }
   } else {
      for (GLuint i = 0; i < n; i++) {
//         GLubyte *pixel = (GLubyte *) bitmap->Bits()
//            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
          GLubyte *pixel = (GLubyte *) bitmap->LockRaster()
          + ((md->m_Bottom - y[i]) * bitmap->GetBytesPerRow()) + x[i] * 4;
         rgba[i][RCOMP] = pixel[SYL_RCOMP];
         rgba[i][GCOMP] = pixel[SYL_GCOMP];
         rgba[i][BCOMP] = pixel[SYL_BCOMP];
         rgba[i][ACOMP] = pixel[SYL_ACOMP];
  bitmap->UnlockRaster();
      }
   }
}

} // namespace os
