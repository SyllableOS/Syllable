//========================================================================
//
// XSplashOutputDev.cc
//
// Copyright 2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif


#include "gmem.h"
#include "SplashTypes.h"
#include "SplashBitmap.h"
#include "Object.h"
#include "GfxState.h"
#include "TextOutputDev.h"
#include "XSplashOutputDev.h"

//------------------------------------------------------------------------
// Constants and macros
//------------------------------------------------------------------------

#define xoutRound(x) ((int)(x + 0.5))

//------------------------------------------------------------------------
// XSplashOutputDev
//------------------------------------------------------------------------

XSplashOutputDev::XSplashOutputDev(os::Window* win,
				   GBool reverseVideoA,
				   SplashColor paperColorA,
				   GBool installCmapA, int rgbCubeSizeA,
				   GBool incrementalUpdateA,
				   void (*redrawCbkA)(void *data),
				   void *redrawCbkDataA):
  SplashOutputDev(splashModeRGB8, reverseVideoA, paperColorA)
{
  Gulong mask;
  int nVisuals;
  int r, g, b, n, m;
  GBool ok;

  incrementalUpdate = incrementalUpdateA;
  redrawCbk = redrawCbkA;
  redrawCbkData = redrawCbkDataA;

  // create text object
  text = new TextPage(gFalse);

  //----- set up the X color stuff
  trueColor = gTrue;

 rShift = 16;
 gShift = 8;
 bShift = 0;
 rDiv = 0;
 gDiv = 0;
 bDiv = 0;
 
}

XSplashOutputDev::~XSplashOutputDev() {
  delete text;
}

void XSplashOutputDev::drawChar(GfxState *state, double x, double y,
				double dx, double dy,
				double originX, double originY,
				CharCode code, Unicode *u, int uLen) {
  text->addChar(state, x, y, dx, dy, code, u, uLen);
  SplashOutputDev::drawChar(state, x, y, dx, dy, originX, originY,
			    code, u, uLen);
}

GBool XSplashOutputDev::beginType3Char(GfxState *state, double x, double y,
				       double dx, double dy,
				       CharCode code, Unicode *u, int uLen) {
  text->addChar(state, x, y, dx, dy, code, u, uLen);
  return SplashOutputDev::beginType3Char(state, x, y, dx, dy, code, u, uLen);
}

void XSplashOutputDev::clear() {
  startDoc(NULL);
  startPage(0, NULL);
}

void XSplashOutputDev::startPage(int pageNum, GfxState *state) {
  SplashOutputDev::startPage(pageNum, state);
  text->startPage(state);
}

void XSplashOutputDev::endPage() {
  SplashOutputDev::endPage();
  if (!incrementalUpdate) {
    (*redrawCbk)(redrawCbkData);
  }
  text->coalesce(gTrue);
}

void XSplashOutputDev::dump() {

  if (incrementalUpdate) {
    (*redrawCbk)(redrawCbkData);
  }
}

void XSplashOutputDev::updateFont(GfxState *state) {
  SplashOutputDev::updateFont(state);
  text->updateFont(state);
}

void XSplashOutputDev::redraw(int srcX, int srcY,
			      os::View* view, os::Bitmap* image/*Drawable destDrawable, GC destGC*/,
			      int destX, int destY,
			      int width, int height) {
  //XImage *image;
  SplashColorPtr dataPtr;
  SplashRGB8 *p;
  SplashRGB8 rgb;
  Gulong pixel;
  int bw, x, y, r, g, b, gray;

  //printf( "%i %i %i %i %i %i\n", srcX, srcY, destX, destY, width, height );

 
  //~ optimize for known XImage formats
  bw = getBitmap()->getWidth();
  dataPtr = getBitmap()->getDataPtr();

  if (trueColor) {
     uint32* pPtr = (uint32*)image->LockRaster() + image->GetBytesPerRow() / 4 * destY + destX;
    for (y = 0; y < height; ++y) {
      p = dataPtr.rgb8 + (y + srcY) * bw + srcX;
      for (x = 0; x < width; ++x) {
    *pPtr++ = *p++/*pixel*/;
      }
	pPtr += image->GetBytesPerRow() / 4 - width;
    }
  } 
  else {
   printf( "SHOULD NOT HAPPEN!\n" );
  }

  os::Rect cFrame = os::Rect( destX, destY, destX + width - 1, destY + height - 1 );
  os::Rect cDest = os::Rect( srcX, srcY, srcX + width - 1, srcY + height - 1 );
  view->DrawBitmap( image, cFrame, cDest );
  view->Flush();
}

GBool XSplashOutputDev::findText(Unicode *s, int len,
				 GBool startAtTop, GBool stopAtBottom,
				 GBool startAtLast, GBool stopAtLast,
				 int *xMin, int *yMin,
				 int *xMax, int *yMax) {
  double xMin1, yMin1, xMax1, yMax1;
  
  xMin1 = (double)*xMin;
  yMin1 = (double)*yMin;
  xMax1 = (double)*xMax;
  yMax1 = (double)*yMax;
  if (text->findText(s, len, startAtTop, stopAtBottom,
		     startAtLast, stopAtLast,
		     &xMin1, &yMin1, &xMax1, &yMax1)) {
    *xMin = xoutRound(xMin1);
    *xMax = xoutRound(xMax1);
    *yMin = xoutRound(yMin1);
    *yMax = xoutRound(yMax1);
    return gTrue;
  }
  return gFalse;
}

GString *XSplashOutputDev::getText(int xMin, int yMin, int xMax, int yMax) {
  return text->getText((double)xMin, (double)yMin,
		       (double)xMax, (double)yMax);
}
