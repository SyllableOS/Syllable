#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gui/window.h>
#include <gui/font.h>

#include "athgui.h"


NumView::NumView( const char* pzName, int nValue ) : StringView( Rect( 0, 0, 1, 1 ), pzName, "", ALIGN_RIGHT )
{
    SetMinPreferredSize( 3 );
    SetMaxPreferredSize( 3 );
    SetNumber( nValue );
}

/*Point NumView::GetPreferredSize( bool bLargest ) const
{
  return( m_cPrefSize );
}*/

void	NumView::SetNumber( int nValue )
{
  char	zStr[ 16 ];

  sprintf( zStr, "%d", nValue );
  SetString( zStr );
}

void NumView::AttachedToWindow( void )
{
    font_height	sHeight;

    GetFontHeight( &sHeight );

    m_cPrefSize.y = sHeight.ascender + sHeight.descender - 1.0f;

    for ( int i = 0 ; i < 10 ; ++i )
    {
	char	zStr[2] = { '0' + i, 0 };
	float	vStrLen = GetStringWidth( zStr );

	if ( vStrLen > m_cPrefSize.x ) {
	    m_cPrefSize.x = vStrLen;
	}
    }
    m_cPrefSize.x *= 3;
    StringView::AttachedToWindow();
}



RollupWidget::RollupWidget( const Rect& cFrame, const char* pzName, View* pcChild, uint32 nResizeMask )
			 : View( cFrame, pzName, nResizeMask, WID_WILL_DRAW )
{
    m_bTrack	= false;
    m_pcWidget	= pcChild;

    if ( NULL != pcChild ) {
	AddChild( pcChild );
    }
}

RollupWidget::~RollupWidget()
{
}

void	RollupWidget::AllAttached( void )
{
  if ( NULL != m_pcWidget ) {
    m_pcPrefSize = m_pcWidget->GetPreferredSize( false );
    m_pcWidget->ResizeTo( m_pcPrefSize );
  }
}

Point RollupWidget::GetPreferredSize( bool bLargest ) const
{
	return( m_pcPrefSize );
}

void RollupWidget::Paint( const Rect& cUpdateRect )
{
  Window*	pcWindow = GetWindow();

  if ( NULL != pcWindow )
  {
//    SetFgColor( pcWindow->GetDefColor( PEN_WINCLIENT ) );
    SetFgColor( get_default_color( COL_NORMAL ) );
    FillRect( GetBounds() );
  }
}


void RollupWidget::MouseDown( const Point& cPosition, uint32 nButtons )
{
  MakeFocus( true );
  m_bTrack	=	true;
  m_cHitPos	=	cPosition;
}

void	RollupWidget::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
  m_bTrack	=	false;
}

void RollupWidget::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    if ( m_bTrack )
    {
	float vDist = (cNewPos.y - m_cHitPos.y);
	m_pcWidget->MoveBy( 0, vDist );
	Flush();
	m_cHitPos = cNewPos;
    }
}















