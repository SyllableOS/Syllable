/*******************************************************************\
*  Example of OpenGL application for Syllable, demonstating
*  initialization and use of the GLView class.
*
\*******************************************************************/

#include <util/application.h>
#include <gui/window.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <gui/GLView.h>
#include <gui/rect.h>
#include <stdio.h>

using namespace os;

/****************************************************************/
// Utility functions for loading and initializing a texture


bool load_raw(const char* filename,int w,int h,char* buffer)
{
  FILE* fp = fopen(filename,"rb");

  if(fp == NULL)
  {
    printf("Failed to open file %s\n",filename);
    return false;
  }

  fread(buffer,w*h*3,1,fp);
  fclose(fp);

  return true;
}


void init_texture(GLuint texture,const char* filename)
{
  char* buffer = 0;

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D,texture);

  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );


  // when texture area is small, bilinear filter the closest mipmap
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                 GL_LINEAR_MIPMAP_NEAREST );

  // when texture area is large, bilinear filter the original
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // the texture wraps over at the edges (repeat)
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

  buffer = (char*)malloc(256*256*3);

  load_raw(filename,256,256,buffer);

  gluBuild2DMipmaps( GL_TEXTURE_2D, 3, 256, 256,
                     GL_RGB, GL_UNSIGNED_BYTE, buffer );

  free(buffer);
}

/****************************************************************/
// Classes for the test app

class GLTestView : public GLView
{
public:
  GLTestView( const Rect& r, const GLParams& params);
  void Paint(const Rect& cUpdateRect);
  void FrameSized( const Point& cDelta );

  void Render();

  GLuint m_Texture;

};

class GLTestWindow : public Window
{
public:
  GLTestWindow( const Rect& r );
  bool OkToQuit();

private:
  GLTestView* m_View;

};

class GLTestApplication : public Application
{
public:
  GLTestApplication();

private:
  GLTestWindow* m_Window;
};

/****************************************************************/

GLTestView::GLTestView( const Rect& r, const GLParams& params ) 
: GLView( r, "GLTestView", params,CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT | CF_FOLLOW_BOTTOM, WID_WILL_DRAW | WID_CLEAR_BACKGROUND)
{
  LockGL();

  glGenTextures(1,&m_Texture);

  init_texture(m_Texture,"texture.raw");

  UnlockGL();
}

void GLTestView::Paint(const Rect& cUpdateRect)
{
  Render();
}

void GLTestView::FrameSized( const Point& cDelta )
{
  Render();
}

void GLTestView::Render()
{
  Rect r = GetBounds();
  //printf("Render %f %f\n",r.right,r.bottom);

  LockGL();

  glClearColor(0.0,0.0,1.0,0.0);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glViewport(0,0,(int)r.right - 1,(int)r.bottom - 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef(30.0,0.0,0.0,1.0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,m_Texture);
  glBegin(GL_QUADS); 
  glTexCoord2f(0.0,0.0); glVertex2f(-0.5, -0.5); 
  glTexCoord2f(0.0,1.0); glVertex2f(-0.5, 0.5); 
  glTexCoord2f(1.0,1.0); glVertex2f(0.5, 0.5); 
  glTexCoord2f(1.0,0.0); glVertex2f(0.5, -0.5); 
  glEnd(); 

  glFlush();

  UnlockGL();

  SwapBuffers();
}

/****************************************************************/


GLTestWindow::GLTestWindow( const Rect& r) 
:Window( r, "GLTestWindow", "GLView Test", 0, CURRENT_DESKTOP)
{
  GLParams params;
  params.flags |= SYLGL_DOUBLE_BUFFER;
  params.depth_bits = 16;
  m_View = new GLTestView( GetBounds(),params );
  AddChild(m_View);
}

bool GLTestWindow::OkToQuit()
{
  Application::GetInstance()->PostMessage(M_QUIT);
  return true;
}


GLTestApplication::GLTestApplication() : Application("application/x-vnd.GLTest")
{
  m_Window = new GLTestWindow(Rect(20,20,500,400));
  m_Window->Show();
}

int main(int argc,char** argv)
{
  GLTestApplication* app = 0;

  app = new GLTestApplication();
  app->Run();
  return 0;
}
