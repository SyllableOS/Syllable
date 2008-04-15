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

#include <GLView.h>
#include "GLDriver.h"

#include <gui/bitmap.h>
#include <util/locker.h>

#ifdef DEBUG
	#define LOGOUT	printf
#else
	#define LOGOUT	(void *(0))
#endif

namespace os {

GLDriver::GLDriver()
{
	LOGOUT("GLDriver constructor\n");
	
  m_GLContext 		= NULL;
  m_GLVisual		= NULL;
  m_GLFramebuffer 	= NULL;
  
    m_Lock = new Locker("GLView::GLDriver");
  LOGOUT("GLDriver:   Locker object GLView::GLDriver created\n");
  
  m_View 		= NULL;
  m_Bitmap 		= NULL;

  m_ClearColor[SYL_RCOMP] = 0;
  m_ClearColor[SYL_GCOMP] = 0;
  m_ClearColor[SYL_BCOMP] = 0;
  m_ClearColor[SYL_ACOMP] = 0;

  m_ClearIndex = 0;

  LOGOUT("GLDriver constructor finished\n");
}


GLDriver::~GLDriver()
{
	LOGOUT("GLDriver destructor\n");
   m_Lock->Lock();

   _mesa_destroy_visual(m_GLVisual);
   _mesa_destroy_framebuffer(m_GLFramebuffer);
   _mesa_destroy_context(m_GLContext);

  m_Lock->Unlock();

  delete m_Lock;
  
  LOGOUT("GLDriver destructor finished\n");
}

void GLDriver::Init(GLView * glview, 
                           GLcontext * ctx, 
                           GLvisual * visual, GLframebuffer * framebuffer)
{
	LOGOUT("GLDriver Init\n");
    m_Lock->Lock();

	m_View 		= glview;
	m_GLContext 	= ctx;
	m_GLVisual 		= visual;
	m_GLFramebuffer = framebuffer;
	
	
	LOGOUT("GLDriver Init:  setup software driver\n");
	struct swrast_device_driver * swdd = _swrast_GetDeviceDriverReference( ctx );
	TNLcontext * tnl = TNL_CONTEXT(ctx);

	// Use default TCL pipeline
	tnl->Driver.RunPipeline = _tnl_run_pipeline;

	ctx->Driver.GetString = GetString;
	ctx->Driver.UpdateState = UpdateState;
	ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
	ctx->Driver.GetBufferSize = GetBufferSize;
	ctx->Driver.Finish = Finish;
	ctx->Driver.Flush = Flush;
	
	ctx->Driver.Accum = _swrast_Accum;
	ctx->Driver.Bitmap = _swrast_Bitmap;
	ctx->Driver.Clear = Clear;
	ctx->Driver.ClearIndex = ClearIndex;
	ctx->Driver.ClearColor = ClearColor;
	ctx->Driver.CopyPixels = _swrast_CopyPixels;
   	ctx->Driver.DrawPixels = _swrast_DrawPixels;
   	ctx->Driver.ReadPixels = _swrast_ReadPixels;
   	ctx->Driver.DrawBuffer = _swrast_DrawBuffer;

   	ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
   	ctx->Driver.NewTextureObject = _mesa_new_texture_object;
  	ctx->Driver.DeleteTexture = _mesa_delete_texture_object;
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
  
  LOGOUT("GLDriver Init:  Finished\n");
}

void GLDriver::LockGL()
{
	LOGOUT("GLDriver::LockGL started\n");
//TODO: make this thread safe!
	if (m_Lock->IsLocked()!= true)
		m_Lock->Lock();
  LOGOUT("GLDriver::LockGL m_Lock->Lock() called\n");

  UpdateState(m_GLContext, 0);
  LOGOUT("GLDriver::LockGL UpdateState(m_GLContext, 0); called\n");
  
  _mesa_make_current(m_GLContext, m_GLFramebuffer);
  LOGOUT("GLDriver::LockGL _mesa_make_current(m_GLContext, m_GLFramebuffer); called\n");
  
  LOGOUT("GLDriver::LockGL finished\n");
}

void GLDriver::UnlockGL()
{
	LOGOUT("GLDriver::UnlockGL started\n");
	//TODO: make this thread safe!
	
	if (m_Lock->IsLocked())
		m_Lock->Unlock();
		
   // Could call _mesa_make_current(NULL, NULL) but it would just
   // hinder performance
   
   LOGOUT("GLDriver::UnlockGL finished\n");
}

void GLDriver::SwapBuffers()
{
//TODO: make this thread safe!
  
  _mesa_notifySwapBuffers(m_GLContext);

  LOGOUT("GLDriver::SwapBuffers %p\n",m_Bitmap);

	if (m_Bitmap) {
		m_Lock->Lock();
        Rect r = m_Bitmap->GetBounds();
		m_View->DrawBitmap(m_Bitmap,r,r);
		m_Lock->Unlock();
	}
}

void GLDriver::CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height)
{
	LOGOUT("GLDriver::CopySubBuffer %p\n",m_Bitmap);
//TODO: make this thread safe!
  m_Lock->Lock();

   if (m_Bitmap) {
      // Source bitmap and view's bitmap are same size.
      // Source and dest rectangle are the same.
      // Note (x,y) = (0,0) is the lower-left corner, have to flip Y
      Rect srcAndDest;
      srcAndDest.left = x;
      srcAndDest.right = x + width - 1;
      srcAndDest.bottom = m_Bottom - y;
      srcAndDest.top = srcAndDest.bottom - height + 1;
      m_View->DrawBitmap(m_Bitmap, srcAndDest, srcAndDest);
   }

  m_Lock->Unlock();

}

void GLDriver::Draw(Rect updateRect)
{
	LOGOUT("GLDriver::Draw\n");
   if (m_Bitmap)
      m_View->DrawBitmap(m_Bitmap, updateRect, updateRect);
}


// GL specific functions
void GLDriver::UpdateState( GLcontext *ctx, GLuint new_state )
{
	LOGOUT("GLDriver::UpdateState %i\n",new_state);
	struct swrast_device_driver *	swdd = _swrast_GetDeviceDriverReference( ctx );

	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_ac_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );
	
	if (ctx->Color.DrawBuffer == GL_FRONT) {
		LOGOUT("GLDriver::UpdateState:  ctx->Color.DrawBuffer == GL_FRONT\n");
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
   LOGOUT("GLDriver::UpdateState:  ctx->Color.DrawBuffer != GL_FRONT\n");
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
    
    LOGOUT("GLDriver::UpdateState finished\n");
}

void GLDriver::ClearIndex(GLcontext *ctx, GLuint index)
{
	LOGOUT("GLDriver::ClearIndex %i\n",index);
   GLDriver* md = (GLDriver*) ctx->DriverCtx;
   md->m_ClearIndex = index;

}

void GLDriver::ClearColor(GLcontext *ctx, const GLfloat color[4])
{
  LOGOUT("GLDriver::ClearColor %f %f %f %f\n",color[0],color[1],color[2],color[3]);
   
   GLDriver* md = (GLDriver*) ctx->DriverCtx;
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_RCOMP], color[0]);
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_GCOMP], color[1]);
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_BCOMP], color[2]);
   CLAMPED_FLOAT_TO_CHAN(md->m_ClearColor[SYL_ACOMP], color[3]); 


  /*
  LOGOUT("%i %i %i %i\n",md->m_ClearColor[SYL_RCOMP],
  md->m_ClearColor[SYL_GCOMP],
  md->m_ClearColor[SYL_BCOMP],
  md->m_ClearColor[SYL_ACOMP]);
  */

   assert(md->m_View);
}

void GLDriver::Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height)
{
  LOGOUT("GLDriver::Clear\n");

   if (mask & DD_FRONT_LEFT_BIT)
		ClearFront(ctx, all, x, y, width, height);
   if (mask & DD_BACK_LEFT_BIT)
		ClearBack(ctx, all, x, y, width, height);

	mask &= ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);

	if (mask)
   {
 
    LOGOUT("_swrast_Clear\n");
     _swrast_Clear( ctx, mask, all, x, y, width, height );
   }

   return;
}

void GLDriver::Error(GLcontext *ctx)
{
	LOGOUT("GLDriver::Error\n");
	GLDriver* md = (GLDriver*) ctx->DriverCtx;
	if (md && md->m_View)
		md->m_View->ErrorCallback((unsigned long) ctx->ErrorValue);
}

void GLDriver::ClearFront(GLcontext *ctx,
                         GLboolean all, GLint x, GLint y,
                         GLint width, GLint height)
{

  LOGOUT("GLDriver::ClearFront\n");

   GLDriver* md = (GLDriver*) ctx->DriverCtx;
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

void GLDriver::ClearBack(GLcontext *ctx,
                        GLboolean all, GLint x, GLint y,
                        GLint width, GLint height)
{
	LOGOUT("GLDriver::ClearBack\n");
   GLDriver *md = (GLDriver *) ctx->DriverCtx;

  //LOGOUT("ClearBack %i %i %i %i\n",x,y,width,height);

   GLView* view = md->m_View;
   assert(view);

   Bitmap* bitmap = md->m_Bitmap;
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

void GLDriver::SetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                            GLenum mode)
{
	LOGOUT("GLDriver::SetBuffer\n");
   /* TODO */
	(void) ctx;
	(void) buffer;
	(void) mode;
}

void GLDriver::GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                            GLuint *height)
{
	LOGOUT("GLDriver::GetBufferSize\n");
   GET_CURRENT_CONTEXT(ctx);
   LOGOUT("GLDriver::GetBufferSize:  GET_CURRENT_CONTEXT(ctx) completed\n");

   if (!ctx){
   		LOGOUT("GLDriver::GetBufferSize:  GET_CURRENT_CONTEXT(ctx) successful\n");
		return;
	}
	LOGOUT("GLDriver::GetBufferSize:  GET_CURRENT_CONTEXT(ctx) successful\n");

   GLDriver* md = (GLDriver*) ctx->DriverCtx;
   GLView* view = md->m_View;
   assert(view);

   Rect b = view->GetBounds();
   *width = (GLuint) (b.right - b.left + 1);
   *height = (GLuint) (b.bottom - b.top + 1);
   md->m_Bottom = (GLint) b.bottom;

   if (ctx->Visual.doubleBufferMode) {
      if (*width != md->m_Width || *height != md->m_Height) {
         // allocate new size of back buffer bitmap
         if(md->m_Bitmap)
            delete md->m_Bitmap;

         Rect rect(0.0, 0.0, *width - 1, *height - 1);
         md->m_Bitmap = new Bitmap(*width,*height, CS_RGBA32);
      }
   }
   else
   {
      md->m_Bitmap = NULL;
   }


   md->m_Width = *width;
   md->m_Height = *height;
}


const GLubyte* GLDriver::GetString(GLcontext *ctx, GLenum name)
{
	LOGOUT("GLDriver::GetString\n");
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa os::GLView::GLDriver (software)";
      default:
         // Let core library handle all other cases
         return NULL;
   }
}

void GLDriver::Flush(GLcontext *ctx)
{
	LOGOUT("GLDriver::Flush\n");
	(void *)ctx;
}

void GLDriver::Finish(GLcontext *ctx)
{
	LOGOUT("GLDriver::Finish\n");
	(void *)ctx;
}

void GLDriver::WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
   GLDriver *md = (GLDriver *) ctx->DriverCtx;
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

void GLDriver::WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgba[][3],
                                const GLubyte mask[])
{
  //TODO:
}

void GLDriver::WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[])
{
  //TODO:
}

void GLDriver::WriteRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
  //TODO:
}


void GLDriver::WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[])
{
  //TODO:
}


void GLDriver::WriteCI32SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::WriteCI8SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::WriteMonoCISpanFront( const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLDriver::WriteCI32PixelsFront( const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::WriteMonoCIPixelsFront( const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLDriver::ReadCI32SpanFront( const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[] )
{
 	LOGOUT("ReadCI32SpanFront() not implemented yet!\n");
  //TODO:
}


void GLDriver::ReadRGBASpanFront( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y, GLubyte rgba[][4] )
{
 	LOGOUT("ReadRGBASpanFront() not implemented yet!\n");
   //TODO:
}


void GLDriver::ReadCI32PixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	LOGOUT("ReadCI32PixelsFront() not implemented yet!\n");
   //TODO:
}


void GLDriver::ReadRGBAPixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[] )
{
 	LOGOUT("ReadRGBAPixelsFront() not implemented yet!\n");
   //TODO:
}

void GLDriver::WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
	GLDriver *md = (GLDriver *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_Bitmap;

	static bool already_called = false;
	if (! already_called) {
		//LOGOUT("WriteRGBASpanBack() called.\n");
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

void GLDriver::WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgb[][3],
                                const GLubyte mask[])
{
	GLDriver *md = (GLDriver *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_Bitmap;

	static bool already_called = false;
	if (! already_called) {
		LOGOUT("WriteRGBSpanBack() called.\n");
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

void GLDriver::WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    const GLchan color[4], const GLubyte mask[])
{
	GLDriver *md = (GLDriver *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_Bitmap;

	static bool already_called = false;
	if (! already_called) {
		LOGOUT("WriteMonoRGBASpanBack() called.\n");
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

void GLDriver::WriteRGBAPixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   GLDriver *md = (GLDriver *) ctx->DriverCtx;
   Bitmap *bitmap = md->m_Bitmap;

	static bool already_called = false;
	if (! already_called) {
		LOGOUT("WriteRGBAPixelsBack() called.\n");
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

void GLDriver::WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      const GLchan color[4],
                                      const GLubyte mask[])
{
	GLDriver *md = (GLDriver *) ctx->DriverCtx;
	Bitmap *bitmap = md->m_Bitmap;

	static bool already_called = false;
	if (! already_called) {
		LOGOUT("WriteMonoRGBAPixelsBack() called.\n");
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

void GLDriver::WriteCI32SpanBack( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::WriteCI8SpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::WriteMonoCISpanBack( const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLDriver::WriteCI32PixelsBack( const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::WriteMonoCIPixelsBack( const GLcontext *ctx, GLuint n,
                                     const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[] )
{
   //TODO:
}


void GLDriver::ReadCI32SpanBack( const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[] )
{
   //TODO:
}

void GLDriver::ReadRGBASpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, GLubyte rgba[][4] )
{
   GLDriver *md = (GLDriver *) ctx->DriverCtx;
   Bitmap *bitmap = md->m_Bitmap;
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

void GLDriver::ReadCI32PixelsBack( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
   //TODO:
}

void GLDriver::ReadRGBAPixelsBack( const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] )
{
   GLDriver *md = (GLDriver *) ctx->DriverCtx;
   Bitmap *bitmap = md->m_Bitmap;
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
