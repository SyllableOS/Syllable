#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/variant.h>
#include <gui/image.h>

#include <math.h>

#include "gradientview.h"

#define SQR(x) 			((x) * (x))

using namespace os;

class GradientView::Private
{
	public:
	Private() {
		m_pcBuffer = new BitmapImage(  );
	}
	~Private() {
		if( m_pcBuffer ) {
			delete m_pcBuffer;
		}
	}
	public:
	BitmapImage*	m_pcBuffer;
	Color32_s		m_cColor1;
	Color32_s		m_cColor2;
	uint32			m_nOrientation;
};

GradientView::GradientView( const Rect& cRect, const String& cName, Color32_s cColor1, Color32_s cColor2, uint32 nOrientation, uint32 nResizeMask, uint32 nFlags )
:View( cRect, cName, nResizeMask, nFlags )
{
	m = new Private;
	
	m->m_cColor1 = cColor1;
	m->m_cColor2 = cColor2;
	m->m_nOrientation = nOrientation;

	if( cRect.IsValid() ) {
		FrameSized( Point( 0, 0 ) );
		_MakeBuffer();
	}
}

GradientView::~GradientView()
{
	delete m;
}

void GradientView::AttachedToWindow()
{
	SetBgColor( GetParent()->GetBgColor(  ) );
	Sync();
	_MakeBuffer();
	Invalidate();
	Flush();
	View::AttachedToWindow();
}

void GradientView::SetColors( Color32 cColor1, Color32 cColor2 )
{
	m->m_cColor1 = cColor1;
	m->m_cColor2 = cColor2;
	Sync();
	_MakeBuffer();
	Invalidate();
	Flush();	
}

void GradientView::HandleMessage( Message *pcMsg )
{
	switch( pcMsg->GetCode() ) {
		case M_VALUE_CHANGED:
			{
				Variant cVal;
				if( pcMsg->FindVariant( "value", &cVal  ) == 0 )
				{
//					printf( "g %s\n", cVal.AsString().c_str() );
					SetColors( cVal.AsColor32(), 0xFF000000 );
				}
			}
			break;
		default:
			View::HandleMessage( pcMsg );
	}
}

void GradientView::Paint( const Rect& cUpdateRect )
{
	if( m->m_pcBuffer != NULL )
	{
		m->m_pcBuffer->Draw( cUpdateRect, cUpdateRect, this );
	}
}

void GradientView::FrameSized( const Point & cDelta )
{
	Rect cBounds( GetBounds() );
	_MakeBuffer();
}

Point GradientView::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )  {
		if( m->m_nOrientation == VERTICAL ) {
			return Point( 20, COORD_MAX );
		} else {
			return Point( COORD_MAX, 20 );
		}
	} else {
		return Point( 10, 10 );
	}
}

void GradientView::_MakeBuffer( )
{
	Sync();

	Rect cBounds( GetNormalizedBounds() );
	
	if( cBounds.IsValid() )
	{
		m->m_pcBuffer->ResizeCanvas( Point( cBounds.Width()+1, cBounds.Height()+1 ) );
		_Render();
		m->m_pcBuffer->Sync();
		Invalidate();
	}
}
/***************************************************************************************************/

void GradientView::_Render()
{
	int x, y;
	int height, width;
	float vRD, vGD, vBD;
	float vR, vG, vB;

	if( !m->m_pcBuffer )
		return;
	
	height = (int)GetBounds().Height() + 1;
	width = (int)GetBounds().Width() + 1;

	float w = ( m->m_nOrientation == VERTICAL ) ? height : width;

	vRD = ( (float)m->m_cColor2.red - (float)m->m_cColor1.red ) / w;
	vGD = ( (float)m->m_cColor2.green - (float)m->m_cColor1.green ) / w;
	vBD = ( (float)m->m_cColor2.blue - (float)m->m_cColor1.blue ) / w;

	vR = ( (float)m->m_cColor1.red );
	vG = ( (float)m->m_cColor1.green );
	vB = ( (float)m->m_cColor1.blue );

	uint8	*p;

	if( m->m_nOrientation == VERTICAL ) {
		for(y = 0; y < height; y++)
		{
			p = (uint8*)(*m->m_pcBuffer)[ y ];
			for(x = 0; x < width ; x++)
			{
				*p++ = (uint8)(vB);
				*p++ = (uint8)(vG);
				*p++ = (uint8)(vR);
				*p++ = 0xFF;
			}
			vR += vRD;
			vG += vGD;
			vB += vBD;
		}
	} else {
		for(x = 0; x < width; x++)
		{
			for(y = 0; y < height ; y++)
			{
				p = ((uint8*)(*m->m_pcBuffer)[ y ]) + x*4;
				*p++ = (uint8)(vB);
				*p++ = (uint8)(vG);
				*p++ = (uint8)(vR);
				*p = 0xFF;
			}
			vR += vRD;
			vG += vGD;
			vB += vBD;
		}
	}
}

/***************************************************************************************************/

void GradientView::_reserved1() {}
void GradientView::_reserved2() {}
void GradientView::_reserved3() {}
void GradientView::_reserved4() {}
void GradientView::_reserved5() {}

