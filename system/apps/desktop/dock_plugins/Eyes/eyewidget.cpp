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

#define ROUND(a) ((int)(a+0.5f))

#define BALL_SIZE 3
#define BALL_DIST 0.75

EyeWidget::EyeWidget(const String& cName, uint32 nResizeMask, uint32 nFlags ) : View(Rect(0,0,60,40), cName, nResizeMask, nFlags)
{
	// Create pop up menu
	pcContextMenu=new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);

	pcContextMenu->AddItem("About Eyes", new Message(M_MENU_ABOUT));
	pcContextMenu->AddItem(new MenuSeparator());
	pcContextMenu->AddItem("Remove",new Message(M_MENU_REMOVE));

	UpdateEyes();
}

EyeWidget::~EyeWidget()
{
	delete( pcContextMenu );
}

void EyeWidget::AttachedToWindow(void)
{
	pcContextMenu->SetTargetForItems(this);
}

void EyeWidget::Paint( const Rect& cUpdateRect )
{
	FillRect(GetBounds(),get_default_color(COL_NORMAL));

	// Calculate each eye
	float width=Width()/4;
	float height=(Height()/2);

	// Draw inner eye
	SetFgColor( 255,255,255 );
	DrawFillEllipse( width, height, width/*-5*/, height/*-5*/ );
	DrawFillEllipse( width*3, height, width, height/*-5*/ );


}

void EyeWidget::MouseDown(const Point& cPosition, uint32 nButtons )
{
	if( nButtons == 2 )
		pcContextMenu->Open(ConvertToScreen(cPosition));	// Open the menu where the mouse is
	else
		View::MouseDown(cPosition, nButtons);
		
}

void EyeWidget::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_MENU_ABOUT:
		{
			Alert* pcAboutAlert = new Alert( "About Eyes","Eyes is clone of the famous XEyes but for Syllable OS.\n\nCopyright (C) 2005 Jonas Jarvoll.\n\nEyes are released under the license GPL.", (Alert::alert_icon) Alert::ALERT_INFO, "OK", NULL );
			pcAboutAlert->Go( new Invoker() );
			break;
		}

		case M_MENU_REMOVE:
		{
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

	// Get mous position and convert to screen coordinates
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
