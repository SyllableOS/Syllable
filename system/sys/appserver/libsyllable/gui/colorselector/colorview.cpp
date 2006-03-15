#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <gui/tabview.h>
#include <gui/layoutview.h>
#include <util/message.h>

#include <math.h>

#include "colorview.h"

using namespace os;

class ColorView::Private
{
	public:
	Private() {
	}
	~Private() {
	}
	public:
	Color32_s	m_cColor;
	String		m_cText;
	Point		m_cTextSize;
};

ColorView::ColorView( const Rect& cRect, const String& cName, Color32_s cColor, uint32 nResizeMask, uint32 nFlags )
:View( cRect, cName, nResizeMask, nFlags )
{
	m = new Private;

	SetColor( cColor );
}

ColorView::~ColorView()
{
	delete m;
}

void ColorView::SetColor( const Color32_s &cColor )
{
	m->m_cColor = cColor;
	String s;
	m->m_cText = s.Format("R: %d\nG: %d\nB: %d\n\n#%02X%02X%02X",
		cColor.red, cColor.green, cColor.blue,
		cColor.red, cColor.green, cColor.blue );
	m->m_cTextSize = GetTextExtent( m->m_cText );
	Invalidate();
}

Point ColorView::GetPreferredSize( bool bLargest ) const
{
	if( bLargest ) {
		return Point( COORD_MAX, COORD_MAX );
	} else {
		String s1, s2;
		s2 = s1.Format("R: 000\nG: 000\nB: 000\n\n#000000" );
		return GetTextExtent( s2 ) + Point( 5, 15 );
	}
}

void ColorView::Paint( const Rect& cUpdateRect )
{
	Rect cBounds = GetBounds();

	Rect cTextBox = cBounds;
	Point cTextSize = GetTextExtent( m->m_cText );
	cTextBox.bottom = cTextSize.y + 5;
	Rect cColorBox = cBounds;
	cColorBox.top = cTextBox.bottom;

	if( cColorBox.top < cColorBox.bottom - 10 ) {
		SetFgColor( get_default_color( COL_NORMAL ) );
		SetBgColor( get_default_color( COL_NORMAL ) );
		FillRect( cTextBox );
		SetFgColor( 0 );
		DrawText( cTextBox, m->m_cText, DTF_UNDERLINES );
	} else {
		cColorBox = cBounds;
	}

	DrawFrame( cColorBox, FRAME_RECESSED | FRAME_TRANSPARENT );

	cColorBox.Resize( 2, 2, -2, -2 );

	SetFgColor( m->m_cColor );

	FillRect( cColorBox );

}

/***************************************************************************************************/

void ColorView::_reserved1() {}
void ColorView::_reserved2() {}
void ColorView::_reserved3() {}
void ColorView::_reserved4() {}
void ColorView::_reserved5() {}



