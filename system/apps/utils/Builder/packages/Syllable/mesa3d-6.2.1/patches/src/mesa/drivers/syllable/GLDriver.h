#if !defined GLDRIVER_H
#define GLDRIVER_H

#include <GLView.h>

#include <gui/bitmap.h>
#include <util/locker.h>
#include <stdio.h>

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

class GLDriver
{
friend class GLView;
public:
  
  GLDriver();
  virtual ~GLDriver();

  virtual void Init(GLView * glview, GLcontext * ctx, GLvisual * visual, GLframebuffer * b);

  virtual void LockGL();
  virtual void UnlockGL();
  virtual void SwapBuffers();
  
  virtual void CopySubBuffer(GLint x,GLint y,GLuint width,GLuint height);
  virtual void Draw(Rect updateRect);

private: // GL device driver specific functions
  GLDriver(const GLDriver &rhs);  // copy constructor illegal
  GLDriver &operator=(const GLDriver &rhs);  // assignment oper. illegal
  
  GLcontext* m_GLContext;
  GLvisual* m_GLVisual;
  GLframebuffer* m_GLFramebuffer;

  Locker* m_Lock;

  GLView*  m_View;
  Bitmap*  m_Bitmap; 

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
   static void Error(GLcontext *ctx);
   static const GLubyte* GetString(GLcontext *ctx, GLenum name);
   static void Finish(GLcontext *ctx);
   static void Flush(GLcontext *ctx);
   
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

} // namespace os

#endif
