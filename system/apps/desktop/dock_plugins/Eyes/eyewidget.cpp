// Eyes -:-  (C)opyright 2005 Jonas Jarvoll
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <gui/rect.h>
#include <gui/point.h>
#include <gui/view.h>
#include <gui/desktop.h>
#include <gui/requesters.h>

#include "eyewidget.h"
#include "messages.h"

using namespace os;

#define DRAG_THRESHOLD 4

static os::Color32_s BlendColours( const os::Color32_s& sColour1, const os::Color32_s& sColour2, float vBlend )
{
	int r = int( (float(sColour1.red)   * vBlend + float(sColour2.red)   * (1.0f - vBlend)) );
 	int g = int( (float(sColour1.green) * vBlend + float(sColour2.green) * (1.0f - vBlend)) );
 	int b = int( (float(sColour1.blue)  * vBlend + float(sColour2.blue)  * (1.0f - vBlend)) );
 	if ( r < 0 ) r = 0; else if (r > 255) r = 255;
 	if ( g < 0 ) g = 0; else if (g > 255) g = 255;
 	if ( b < 0 ) b = 0; else if (b > 255) b = 255;
 	return os::Color32_s(r, g, b, sColour1.alpha);
}

/* From the Photon Decorator */
static os::Color32_s Tint( const os::Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
 
 	if( r < 0 )
 		r = 0;
 	else if( r > 255 )
 		r = 255;
 	if( g < 0 )
 		g = 0;
 	else if( g > 255 )
 		g = 255;
 	if( b < 0 )
 		b = 0;
 	else if( b > 255 )
 		b = 255;
 	return ( os::Color32_s( r, g, b, sColor.alpha ) );
}

#define ROUND(a) ((int)(a+0.5f))

#define BALL_SIZE 3
#define BALL_DIST 0.75

EyeWidget::EyeWidget(os::DockPlugin* pcPlugin, os::Looper* pcDock, const String& cName, uint32 nResizeMask, uint32 nFlags ) : View(Rect(0,0,60,40), cName, nResizeMask, nFlags)
{
	os::File* pcFile;
	os::ResStream *pcStream;

	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;

	/* Load default icons */
	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cCol( pcFile );
	pcStream = cCol.GetResourceStream( "icon48x48.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete( pcFile );

	// Create pop up menu
	pcContextMenu=new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);

	pcContextMenu->AddItem("About Eyes...", new Message(M_MENU_ABOUT));

	m_bAttachedToWindow = false;
	UpdateEyes();
}

EyeWidget::~EyeWidget()
{
	delete( pcContextMenu );
	delete( m_pcIcon );
}

void EyeWidget::AttachedToWindow(void)
{
	m_bAttachedToWindow = true;
	pcContextMenu->SetTargetForItems(this);
}

void EyeWidget::DetachedFromWindow(void)
{
	m_bAttachedToWindow = false;
}

void EyeWidget::Paint( const Rect& cUpdateRect )
{
	//FillRect(GetBounds(),get_default_color(COL_NORMAL));
	
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
   	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
   
   	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30,
   											( sCurrentColor.green-sBottomColor.green ) / 30,
   											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
   
   	if( cUpdateRect.DoIntersect( os::Rect( 0, 0, GetBounds().right, 29 ) ) )
   	{
   		sCurrentColor.red -= (int)cUpdateRect.top * sColorStep.red;
   		sCurrentColor.green -= (int)cUpdateRect.top * sColorStep.green;
   		sCurrentColor.blue -= (int)cUpdateRect.top * sColorStep.blue;
   		for( int i = (int)cUpdateRect.top; i < ( (int)cUpdateRect.bottom < 30 ? (int)cUpdateRect.bottom + 1 : 30 ); i++ )
   		{
   			SetFgColor( sCurrentColor );
   			DrawLine( os::Point( cUpdateRect.left, i ), os::Point( cUpdateRect.right, i ) );
   			sCurrentColor.red -= sColorStep.red;
   			sCurrentColor.green -= sColorStep.green;
   			sCurrentColor.blue -= sColorStep.blue;
   		}
   	}

	// Calculate each eye
	float width=Width()/4;
	float height=(Height()/2);

	// Draw inner eye
	SetFgColor( 255,255,255 );
	DrawFillEllipse( width, height, width/*-5*/, height/*-5*/ );
	DrawFillEllipse( width*3, height, width, height/*-5*/ );
}

void EyeWidget::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	// Get the frame of the dock
	// If the plugin is dragged outside of the dock's frame
	// then remove the plugin
	Rect cRect = ConvertFromScreen( GetWindow()->GetFrame() );

	if( ( m_bDragging && ( cPosition.x < cRect.left ) ) || ( m_bDragging && ( cPosition.x > cRect.right ) ) || ( m_bDragging && ( cPosition.y < cRect.top ) ) || ( m_bDragging && ( cPosition.y > cRect.bottom ) ) ) 
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", m_pcPlugin );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	} /*else if ( nButtons == 1 ) { // left button
		// Check to see if the coordinates passed match when the left mouse button was pressed
		// if so, then it was a single click and not a drag
		if ( abs( (int)(m_cPos.x - cPosition.x) ) < DRAG_THRESHOLD && abs( (int)(m_cPos.y - cPosition.y) ) < DRAG_THRESHOLD )
		{ 
			// Just eat the message for right now.	
		}
	} */

	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void EyeWidget::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			m_bDragging = true;
			os::Message cMsg;
			BeginDrag( &cMsg, os::Point( m_pcIcon->GetBounds().Width() / 2,
											m_pcIcon->GetBounds().Height() / 2 ), m_pcIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void EyeWidget::MouseDown(const Point& cPosition, uint32 nButtons )
{
	os:: Point cPos;
	
	if ( nButtons == 1 ) /* left button */
	{
		MakeFocus ( true );
		m_bCanDrag = true;

		// Store these coordinates for later use in the MouseUp procedure
		m_cPos.x = cPosition.x;
		m_cPos.y = cPosition.y;
	} else if ( nButtons == 2 ) { /* right button */
		//MakeFocus ( false );
		pcContextMenu->Open(ConvertToScreen(cPosition));	// Open the menu where the mouse is
	}

	os::View::MouseDown( cPosition, nButtons );

	/*if( nButtons == 0x02 )
		pcContextMenu->Open(ConvertToScreen(cPosition));	// Open the menu where the mouse is
	else
		View::MouseDown(cPosition, nButtons); */
		
}

void EyeWidget::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_MENU_ABOUT:
		{
			Alert* pcAboutAlert = new Alert( "About Eyes...",
				"Eyes is a clone of the famous\nXEyes but for Syllable OS.\n\nCopyright (C) 2005 Jonas Jarvoll.\n\nEyes is released under the license GPL.", 
				m_pcIcon->LockBitmap(), 0, "Close", NULL );
			m_pcIcon->UnlockBitmap();
			pcAboutAlert->Go( new Invoker() );
			pcAboutAlert->MakeFocus();
			break;
		}

		default:
			printf("Unhandled Message\n");
			break;
	}
}

///////////////////////////////////////////////////////////////7
// 
// P R I V A T E   M E T H O D S
// 
///////////////////////////////////////////////////////////////7


// Calculations taken from the famous XEye which is included in 
// the X-Windows distribution for Linux
Point EyeWidget::CalculatePupil(Point orgin, Point mouse, Point eyeradius)
{
	float  cx, cy;
	float  dx, dy;

	dx = mouse.x - orgin.x;
	dy = mouse.y - orgin.y;
	if (dx == 0 && dy == 0) 
	{
		cx = orgin.x;
		cy = orgin.y;
	} 
	else 
	{
		float angle = atan2 ((double) dy, (double) dx);
		float cosa = cos (angle);
		float sina = sin (angle);
		float h = hypot (eyeradius.y * cosa, eyeradius.x * sina);
		float x = (eyeradius.x * eyeradius.y) * cosa / h;
		float y = (eyeradius.x * eyeradius.y) * sina / h;
		float dist = BALL_DIST * hypot (x, y);
		
		if (dist > hypot ((double) dx, (double) dy)) 
		{
			cx = dx + orgin.x;
			cy = dy + orgin.y;
		} 
		else 
		{
			cx = dist * cosa + orgin.x;
			cy = dist * sina + orgin.y;
		}
    }
    
    return Point( cx, cy );
}

void EyeWidget::FrameSized( const Point& cDelta )
{
	// force a redraw if the size of the window has changed
	Invalidate();

	View::FrameSized( cDelta );
}

os::Point EyeWidget::GetPreferredSize(bool bLargest) const
{
	return os::Point(60,40);
}

void EyeWidget::UpdateEyes()
{

	Point pos;
	uint32 buttons;

	if(!m_bAttachedToWindow)
		return;

	// Get mouse position and convert to screen coordinates
	GetMouse(&m_Mouse, &buttons);

	// Only redraw if the mouse has moved
	if( m_Mouse != m_MouseOld )
	{
		// Calculate size of eye
		float width=Width()/4;
		float height=Height()/2;

		// Remove old pupils
		SetFgColor( 255,255,255 );
		DrawFillCircle( CalculatePupil( Point( width, height ) , m_MouseOld, Point( width, height ) ), BALL_SIZE);
		DrawFillCircle( CalculatePupil( Point( width + width*2, height ) , m_MouseOld, Point( width, height ) ), BALL_SIZE);

		// calculate the pupils	
		SetFgColor( 0,0,0 );
		DrawFillCircle( CalculatePupil( Point( width, height ) , m_Mouse, Point( width, height ) ), BALL_SIZE);
		DrawFillCircle( CalculatePupil( Point( width + width*2, height ) , m_Mouse, Point( width, height ) ), BALL_SIZE);

		// Remember current mouse position
		m_MouseOld = m_Mouse;
	
		Flush();
	}
}

void EyeWidget::DrawCircle(float xc, float yc, float radius)
{	
	int y, x, xsquare, radiussquare_sub_ysquare;

	y=(int)radius;
	x=xsquare=0;
	radiussquare_sub_ysquare=(int)radius*(int)radius-(y*y);

	while(1)
	{
		DrawPixel(xc+x, yc+y);
		DrawPixel(xc+x, yc-y);
	  	DrawPixel(xc-x, yc+y);
		DrawPixel(xc-x, yc-y);
	  	DrawPixel(xc+y, yc+x);
	  	DrawPixel(xc-y, yc+x);
	  	DrawPixel(xc+y, yc-x);
	  	DrawPixel(xc-y, yc-x);
	
		if(xsquare>radiussquare_sub_ysquare)
		{
			y--;
	    	radiussquare_sub_ysquare+=(y<<1)-1;
		}
		else
		{
			xsquare+=(x<<1)+1;
	    	x++;
  		}
	
	  	if (x>y)
			break;
	}
}

void EyeWidget::DrawFillCircle(float xc, float yc, float radius)
{	
	int y, x, xsquare, radiussquare_sub_ysquare;

	y=(int)radius;
	x=xsquare=0;
	radiussquare_sub_ysquare=(int)radius*(int)radius-(y*y);

	while(1)
	{
		DrawLine( Point( xc+x, yc+y ), Point( xc+x, yc-y ) );
	  	DrawLine( Point( xc-x, yc+y ), Point( xc-x, yc-y ) );
	  	DrawLine( Point( xc+y, yc+x ), Point( xc-y, yc+x ) );
	  	DrawLine( Point( xc+y, yc-x ), Point( xc-y, yc-x ) );
	
		if(xsquare>radiussquare_sub_ysquare)
		{
			y--;
	    	radiussquare_sub_ysquare+=(y<<1)-1;
		}
		else
		{
			xsquare+=(x<<1)+1;
	    	x++;
  		}
	
	  	if (x>y)
			break;
	}
}

// The algorithm for drawing ellipses is based on the one found in the book "Computer Graphics" by Hearn and Baker.
#define drawEllipsePoint( _xc, _yc, _x, _y)   { DrawPixel( _xc+_x, _yc+_y ); DrawPixel( _xc-_x, _yc+_y ); DrawPixel( _xc+_x, _yc-_y ); DrawPixel( _xc-_x, _yc-_y ); } 
#define drawFillEllipsePoint( _xc, _yc, _x, _y)   { DrawLine( Point( _xc+_x, _yc+_y ), Point( _xc-_x, _yc+_y ) ); DrawLine( Point( _xc+_x, _yc-_y ), Point( _xc-_x, _yc-_y )); } 

void EyeWidget::DrawEllipse(float xc, float yc, float xr, float yr)
{
	float Rx2=xr*xr;
	float Ry2=yr*yr;
	float twoRx2=2.0f*Rx2;
	float twoRy2=2.0f*Ry2;
	float p;
	float x=0.0f;
	float y=yr;
	float px=0.0f;
	float py=twoRx2*y;
	
	// Plot first set of points
	drawEllipsePoint( xc, yc, x, y );

	// Region 1
	p = ROUND( Ry2  - ( Rx2 * yr ) + ( 0.25f * Rx2 ) );
	while( px < py )
	{
		x++;
		px += twoRy2;
		if( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		drawEllipsePoint( xc, yc, x, y);
	}

	// Region 2
	p = ROUND( Ry2 * ( 0.5f + x ) * ( x + 0.5f ) + Rx2 * ( y - 1.0f ) * ( y- 1.0f ) - Rx2 * Ry2);
	while( y > 0.0f )
	{
		y--;
		py -= twoRx2;
		if( p > 0.0f )
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py + px;
		}

		drawEllipsePoint( xc, yc, x, y);
	}
}

void EyeWidget::DrawFillEllipse(float xc, float yc, float xr, float yr)
{
	float Rx2=xr*xr;
	float Ry2=yr*yr;
	float twoRx2=2.0f*Rx2;
	float twoRy2=2.0f*Ry2;
	float p;
	float x=0.0f;
	float y=(int)yr;
	float px=0.0f;
	float py=twoRx2*y;
	
	// Plot first set of points
	drawFillEllipsePoint(xc, yc, x, y);

	// Region 1
	p = ROUND( Ry2  - ( Rx2 * yr ) + ( 0.25f * Rx2 ) );
	while( px < py )
	{
		x++;
		px += twoRy2;
		if( p < 0.0f )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		drawFillEllipsePoint( xc, yc, x, y);
	}

	// Region 2
	p = ROUND( Ry2 * ( 0.5f + x ) * ( x + 0.5f ) + Rx2 * ( y - 1.0f ) * ( y- 1.0f ) - Rx2 * Ry2);
	while( y > 0.0f )
	{
		y--;
		py -= twoRx2;
		if( p > 0.0f )
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py + px;
		}

		drawFillEllipsePoint( xc, yc, x, y);
	}
}

void EyeWidget::DrawPixel(float x, float y) 
{	
	View::DrawLine( Point( x, y ),  Point( x, y ) );
}
