#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <util/message.h>

#include <math.h>

#include "colorwheel.h"

#define SQR(x) 			((x) * (x))

using namespace os;

class ColorWheel::Private
{
	public:
	Private() {
		m_pcWheelBuffer = NULL;
		m_bFollowMouse = false;
		m_bCircular = false;
		m_vBrightness = 1.0;
		m_pcSlider = NULL;
		m_pcGradient = NULL;
	}
	~Private() {
		if( m_pcWheelBuffer ) {
			delete m_pcWheelBuffer;
		}
	}
	public:
	struct Color32_s	m_cRGB;

	Point			m_cWheelCenter;
	Point			m_cWheelRadius;
	
	Bitmap*			m_pcWheelBuffer;	// Wheel image, background buffer

	Point			m_cKnobPos;
	
	float			m_vBrightness;

	bool			m_bFollowMouse;
	bool			m_bCircular;
	
	Slider*			m_pcSlider;
	GradientView*	m_pcGradient;
};

ColorWheel::ColorWheel( const Rect& cRect, const String& cName, Message* pcMsg, Color32_s cColor, uint32 nResizeMask, uint32 nFlags )
:Control( cRect, cName, "", pcMsg, nResizeMask, nFlags )
{
	m = new Private;

	if( cRect.IsValid() ) {
		FrameSized( Point( 0, 0 ) );
		_MakeWheelBuffer();
	}

	Control::SetValue( cColor, false );
}

ColorWheel::~ColorWheel()
{
	delete m;
}

void ColorWheel::AttachedToWindow()
{
	SetBgColor( GetParent()->GetBgColor(  ) );
	Sync();
	_MakeWheelBuffer();
	Invalidate();
	Flush();
	Control::AttachedToWindow();

	if( m->m_pcSlider ) {
		m->m_pcSlider->SetMessage( new Message( M_VALUE_CHANGED ) );
		m->m_pcSlider->SetTarget( this );
	}
}

void ColorWheel::HandleMessage( Message *pcMsg )
{
	switch( pcMsg->GetCode() ) {
		case M_VALUE_CHANGED:
			{
				Variant cVal;
/*				bool bFinal;
				printf("%d\n", pcMsg->FindBool( "final", &bFinal ) );*/
				if( pcMsg->FindVariant( "value", &cVal  ) == 0 )
				{
//					printf("%f\n", cVal.AsFloat());
					SetBrightness( cVal.AsFloat() );
					uint32 col;
					_CalcWheelHSB( int(m->m_cKnobPos.x), int(m->m_cKnobPos.y), &col );
					Control::SetValue( Color32_s( col ), true );
				}
			}
			break;
		default:
			Control::HandleMessage( pcMsg );
	}
}

void ColorWheel::SetBrightness( float vBrightness )
{
	m->m_vBrightness = vBrightness;
}

float ColorWheel::GetBrightness() const
{
	return m->m_vBrightness;
}

void ColorWheel::SetBrightnessSlider( Slider* pcSlider )
{
	m->m_pcSlider = pcSlider;
}

void ColorWheel::SetGradientView( GradientView* pcGradient )
{
	m->m_pcGradient = pcGradient;
}

void ColorWheel::PostValueChange( const Variant& cNewValue )
{
	if( m->m_pcGradient ) {
		uint32 col;
		_CalcWheelColor( int32(m->m_cKnobPos.x), int32(m->m_cKnobPos.y), &col, false );

		m->m_pcGradient->SetColors( col, 0xFF000000 );
	}

	Invalidate();
	Flush();
}

void ColorWheel::SetValue( const Variant& cNewValue, bool bUpdate )
{
	m->m_cRGB = cNewValue.AsColor32();
	m->m_cKnobPos = _CalcKnobPos();

	Color32 col = m->m_cRGB;
	float val;
	val = std::max( std::max( col.red, col.green ), col.blue );
	val /= 255;
	m->m_vBrightness = val;

	if( m->m_pcSlider ) {
		m->m_pcSlider->SetValue( val, false );
	}

	Control::SetValue( cNewValue, bUpdate );
}

bool ColorWheel::Invoked( Message* pcMessage )
{
	pcMessage->AddColor32( "color", m->m_cRGB );
	return Control::Invoked( pcMessage );
}

bool ColorWheel::SetPosition( const Point& cPosition, bool bUpdate )
{
	uint32 col;

	if( !_CalcWheelColor( int(cPosition.x), int(cPosition.y), &col, false ) )
    	return false;

	m->m_cRGB = col;
	m->m_cKnobPos = _CalcKnobPos();

	_CalcWheelHSB( int(cPosition.x), int(cPosition.y), &col );

	Control::SetValue( Color32_s( col ), bUpdate );
	
	return true;
}

Point ColorWheel::GetPosition() const
{
	return m->m_cKnobPos;
}

void ColorWheel::SetForceCircular( bool bCircular )
{
	m->m_bCircular = bCircular;
	FrameSized( Point() );
	Invalidate();
}

bool ColorWheel::GetForceCircular() const
{
	return m->m_bCircular;
}

void ColorWheel::Paint( const Rect& cUpdateRect )
{
	if( m->m_pcWheelBuffer != NULL )
	{
		DrawBitmap( m->m_pcWheelBuffer, cUpdateRect, cUpdateRect );
		_RenderKnob();
	}
}

void ColorWheel::FrameSized( const Point & cDelta )
{
	Rect cBounds( GetBounds() );
	m->m_cWheelRadius.x = m->m_cWheelCenter.x = cBounds.Width()/2;
	m->m_cWheelRadius.y = m->m_cWheelCenter.y = cBounds.Height()/2;
	if( m->m_bCircular ) {
		m->m_cWheelRadius.x = std::min( m->m_cWheelRadius.x, m->m_cWheelRadius.y );
		m->m_cWheelRadius.y = m->m_cWheelRadius.x;
	}
	m->m_cKnobPos = _CalcKnobPos();
	_MakeWheelBuffer();
}

void ColorWheel::MouseDown( const Point& cPosition, uint32 nButtons )
{
	if( nButtons == 1 ) {
		MakeFocus();
		if( SetPosition( cPosition ) )
			m->m_bFollowMouse = true;
	} else {
		Control::MouseDown( cPosition, nButtons );
	}
}

void ColorWheel::MouseUp( const Point& cPosition, uint32 nButtons, Message * pcData )
{
	if( nButtons == 1 && m->m_bFollowMouse ) {
		SetPosition( cPosition );
		m->m_bFollowMouse = false;
	} else {
		Control::MouseUp( cPosition, nButtons, pcData );
	}
}

void ColorWheel::MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData )
{
	if( nButtons == 1 && m->m_bFollowMouse ) {
		SetPosition( cPosition );
	} else {
		Control::MouseMove( cPosition, nCode, nButtons, pcData );
	}
}

void ColorWheel::EnableStatusChanged( bool bEnabled )
{
}

Point ColorWheel::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )  {
		return Point( COORD_MAX, COORD_MAX );
	} else {
		return Point( 50, 50 );
	}
}

void ColorWheel::_MakeWheelBuffer( )
{
	Sync();

	Rect cBounds( GetNormalizedBounds() );
	
	if( cBounds.IsValid() )
	{
		if( m->m_pcWheelBuffer ) {
			Rect cBitmapBounds( m->m_pcWheelBuffer->GetBounds() );
			
			if( cBounds.Width() > cBitmapBounds.Width() ||	cBounds.Height() > cBitmapBounds.Height() ) {
				// Bitmap is too small - we must re-allocate
				delete m->m_pcWheelBuffer;
				m->m_pcWheelBuffer = NULL;
			}
		}

		if( !m->m_pcWheelBuffer ) {
			// Allocate 25 extra pixels, so we don't have to re-allocate every time, while resizing
			m->m_pcWheelBuffer = new Bitmap( int ( cBounds.Width() ) + 1 + 25, int ( cBounds.Height(  ) ) + 1 + 25, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );
		}
		
		if( m->m_pcWheelBuffer ) {
			_RenderWheel();
			m->m_pcWheelBuffer->Sync();
			Invalidate();
		}
	} else {
		m->m_pcWheelBuffer = NULL;
	}
}

//#define CW_PI 			3.14159265358979
#define CW_PI 			3.1415926535898

/****************************************************************************/

bool ColorWheel::_CalcWheelHSB( int32 x, int32 y, uint32* col )
{
    double r, rx, ry, l, h, s;
	double cx, xrad;
	double cy, yrad;

	cx = m->m_cWheelCenter.x;
	cy = m->m_cWheelCenter.y;
	xrad = m->m_cWheelRadius.x;
	yrad = m->m_cWheelRadius.y;
  
    rx = (double) cx - x;
//    ry = ((double) y - cy) * cx / cy;
    ry = ((double) y - cy) * xrad / yrad;

    r = sqrt (SQR(rx) + SQR(ry));
   
    if (r != 0.0)
        h = atan2 (rx / r, ry / r);
    else
        h = 0.0;

	xrad -= 4;

    l = sqrt (SQR(( xrad * cos (h + 0.5 * CW_PI))) + SQR(( xrad * sin (h + 0.5 * CW_PI))));
    
    s = r / l;
    h = (h + CW_PI) / (2.0 * CW_PI);
    
    if (s == 0.0)
        s = 0.00001;
    else if (s > 1.0)
        s = 1.0;

	double R, G, B, f, w, q, t;
	int32 i;
   
//	if (h == 1.0) h = 0.0;
	h = h * 6.0;
	i = (int32)h;
	f = h - i;
	w = m->m_vBrightness * ( 1.0 - s);
	q = m->m_vBrightness * ( 1.0 - (s * f));
	t = m->m_vBrightness * ( 1.0 - (s * (1.0 - f)));

	R = G = B = m->m_vBrightness;

	switch (i)
	{
		case 0:
			G = t;
			B = w;
			break;
		
		case 1:
			R = q;
			B = w;
			break;
		
		case 2:
			R = w;
			B = t;
			break;
		
		case 3:
			R = w;
			G = q;
			break;
		
		case 4:
			R = t;
			G = w;
			break;
		
		case 5:
			G = w;
			B = q;
			break;
	}

	*col = ( uint8(R * 0xFF + 0.5 ) << 16 |
			uint8(G * 0xFF + 0.5 ) << 8 |
			uint8(B * 0xFF + 0.5 ) |
			0xFF << 24 );
	
	return true;
}

bool ColorWheel::_CalcWheelColor( int32 x, int32 y, uint32* col, bool bMargin )
{
    double r, rx, ry, l, h, s;
	double cx, xrad;
	double cy, yrad;

	cx = m->m_cWheelCenter.x;
	cy = m->m_cWheelCenter.y;
	xrad = m->m_cWheelRadius.x;
	yrad = m->m_cWheelRadius.y;
  
    rx = (double) cx - x;
//    ry = ((double) y - cy) * cx / cy;
    ry = ((double) y - cy) * xrad / yrad;

    r = sqrt (SQR(rx) + SQR(ry));

	if( bMargin ) {
		if( r > ( xrad - 4 ) ) {
			if( r < xrad ) {
	    		*col = 0xFF000000;
		    	return true;
		    }
	    	return false;
	    }
	}
    
    if (r != 0.0)
        h = atan2 (rx / r, ry / r);
    else
        h = 0.0;

	xrad -= 4;

    l = sqrt (SQR(( xrad * cos (h + 0.5 * CW_PI))) + SQR(( xrad * sin (h + 0.5 * CW_PI))));
    
    s = r / l;
    h = (h + CW_PI) / (2.0 * CW_PI);
    
    if (s == 0.0)
        s = 0.00001;
    else if (s > 1.0)
        s = 1.0;

	double R, G, B, f, w, q, t;
	int32 i;
   
//	if (h == 1.0) h = 0.0;
	h = h * 6.0;
	i = (int32)h;
	f = h - i;
	w = (1.0 - s);
	q = (1.0 - (s * f));
	t = (1.0 - (s * (1.0 - f)));

	R = G = B = 1.0;

	switch (i)
	{
		case 0:
			G = t;
			B = w;
			break;
		
		case 1:
			R = q;
			B = w;
			break;
		
		case 2:
			R = w;
			B = t;
			break;
		
		case 3:
			R = w;
			G = q;
			break;
		
		case 4:
			R = t;
			G = w;
			break;
		
		case 5:
			G = w;
			B = q;
			break;
	}

	*col = ( uint8(R * 0xFF + 0.5 ) << 16 |
			uint8(G * 0xFF + 0.5 ) << 8 |
			uint8(B * 0xFF + 0.5 ) |
			0xFF << 24 );

	return true;
}

/***************************************************************************************************/

Point ColorWheel::_CalcKnobPos()
{
	double alpha;
	double R, G, B, H = 0.0, S, max, min, delta;

	R = (double) m->m_cRGB.red / (double) 0xFF;
	G = (double) m->m_cRGB.green / (double) 0xFF;
	B = (double) m->m_cRGB.blue / (double) 0xFF;

	max = std::max( std::max( R, G ), B );
	min = std::min( std::min( R, G ), B );
       
	if (max != 0.0)
	{
		S = (max - min) / max;
	}
	else
	{
		S = 0.0;
	}
    
	if (S != 0.0)
	{
		delta = max - min;

		if (R == max)
		{
			H = (G - B) / delta;
		}
		else if (G == max)
		{
			H = 2.0 + (B - R) / delta;
		}
		else if (B == max)
		{
			H = 4.0 + (R - G) / delta;
		}
	
		H = H * 60.0;
		
		if (H < 0.0)
		{
			H = H + 360;
		}
		
		H = H / 360.0;
	}
   
	alpha = H;
	alpha *= CW_PI * 2.0;
	alpha -= CW_PI / 2.0;

	return Point( m->m_cWheelCenter.x + ((double)(m->m_cWheelRadius.x-4) * S * cos(alpha)),
		  m->m_cWheelCenter.y + ((double)(m->m_cWheelRadius.y-4) * S * sin(alpha)) );
}
                                                                         
/***************************************************************************************************/

void ColorWheel::_RenderWheel()
{
	int x, y;
	int height, width;
//	uint32 hue, sat;
	uint32 col;

//	printf( "wb %p\n", m->m_pcWheelBuffer );

	if( !m->m_pcWheelBuffer )
		return;
	
	height = (int)GetBounds().Height() + 1;
	width = (int)GetBounds().Width() + 1;

	uint32	*p, *p2;
	uint32	bgcolor = GetBgColor().red << 16 | GetBgColor().blue | GetBgColor().green << 8 | 0xFF000000;

	p = (uint32*)m->m_pcWheelBuffer->LockRaster();

	if( !p )
		return;

	for(y = 0; y < height; y++)
	{
		p = (uint32*)( m->m_pcWheelBuffer->LockRaster() + m->m_pcWheelBuffer->GetBytesPerRow() * y );
		p2 = (uint32*)( m->m_pcWheelBuffer->LockRaster() + m->m_pcWheelBuffer->GetBytesPerRow() * y + width * 4 );
		for(x = 0; x < width/2 + 1 ; x++)
		{
			if( _CalcWheelColor( x, y, &col, true ) )
			{
		    	*p++ = col;
		    	
		    	// Swap Green and Blue
				*--p2 = ( ( col & 0xFF00 ) >> 8 ) | ( ( col & 0x00FF ) << 8) | ( col & 0xFFFF0000 );
			} else {
				*p++ = bgcolor;
				*--p2 = bgcolor;
			}
		}
/*		if( width & 1 ) {
				p = (uint32*)( m->m_pcWheelBuffer->LockRaster() + m->m_pcWheelBuffer->GetBytesPerRow() * y + width * 4 - 4 );
		    	*p = bgcolor;
		}*/
	}
}

void ColorWheel::_RenderKnob()
{
	const int xsize = 5, ysize = 5;
	
	int x, y;
	
	x = int( m->m_cKnobPos.x );
	y = int( m->m_cKnobPos.y );

	SetFgColor( 0xFFFFFFFF );
	MovePenTo( Point( x - xsize, y ) );
	DrawLine( Point( x, y - ysize ) );
	DrawLine( Point( x + xsize, y ) );
	DrawLine( Point( x, y + ysize) );
	DrawLine( Point( x - xsize, y ) );

	SetFgColor( 0xFF000000 );
	x++;
	MovePenTo( Point( x - xsize, y ) );
	DrawLine( Point( x, y - ysize ) );
	DrawLine( Point( x + xsize, y ) );
	DrawLine( Point( x, y + ysize) );
	DrawLine( Point( x - xsize, y ) );

}

/***************************************************************************************************/

void ColorWheel::_reserved1() {}
void ColorWheel::_reserved2() {}
void ColorWheel::_reserved3() {}
void ColorWheel::_reserved4() {}
void ColorWheel::_reserved5() {}



