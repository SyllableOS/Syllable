#include "coloredit.h"

#include <gui/spinner.h>
#include <gui/stringview.h>
#include <util/message.h>

enum { COLOR_CHANGED };

static Point get_max_width( Point* pcFirst, ... )
{
  Point cSize = *pcFirst;
  va_list hList;

  va_start( hList, pcFirst );

  Point* pcTmp;
  while( (pcTmp = va_arg( hList, Point* )) != NULL ) {
    if ( pcTmp->x > cSize.x ) {
      cSize = *pcTmp;
    }
  }
  va_end( hList );
  return( cSize );
}

ColorEdit::ColorEdit( const Rect& cFrame, const char* pzName, const Color32_s& sColor ) : View( cFrame, pzName )
{
  m_sColor = sColor;

  m_pcRedSpinner   = new Spinner( Rect(0,0,0,0), "red_spinner", sColor.red, new Message( COLOR_CHANGED ) );
  m_pcGreenSpinner = new Spinner( Rect(0,0,0,0), "green_spinner", sColor.green, new Message( COLOR_CHANGED ) );
  m_pcBlueSpinner  = new Spinner( Rect(0,0,0,0), "blue_spinner", sColor.blue, new Message( COLOR_CHANGED ) );

  m_pcRedStr   = new StringView( Rect(0,0,0,0), "red_txt", "Red", ALIGN_CENTER );
  m_pcGreenStr = new StringView( Rect(0,0,0,0), "green_txt", "Green", ALIGN_CENTER );
  m_pcBlueStr  = new StringView( Rect(0,0,0,0), "blue_txt", "Blue", ALIGN_CENTER );

  m_pcRedSpinner->SetMinPreferredSize( 3 );
  
  AddChild( m_pcRedSpinner );
  AddChild( m_pcGreenSpinner );
  AddChild( m_pcBlueSpinner );

  m_pcRedSpinner->SetFormat( "%.0f" );
  m_pcRedSpinner->SetMinMax( 0.0f, 255.0f );
  m_pcRedSpinner->SetStep( 1.0f );
  m_pcRedSpinner->SetScale( 1.0f );

  m_pcGreenSpinner->SetFormat( "%.0f" );
  m_pcGreenSpinner->SetMinMax( 0.0f, 255.0f );
  m_pcGreenSpinner->SetStep( 1.0f );
  m_pcGreenSpinner->SetScale( 1.0f );

  m_pcBlueSpinner->SetFormat( "%.0f" );
  m_pcBlueSpinner->SetMinMax( 0.0f, 255.0f );
  m_pcBlueSpinner->SetStep( 1.0f );
  m_pcBlueSpinner->SetScale( 1.0f );
  
  AddChild( m_pcRedStr );
  AddChild( m_pcGreenStr );
  AddChild( m_pcBlueStr );

  FrameSized( Point(0,0) );
}

void ColorEdit::HandleMessage( Message* pcMessage )
{
  switch( pcMessage->GetCode() )
  {
    case COLOR_CHANGED:
      SetValue( Color32_s( m_pcRedSpinner->GetValue(), m_pcGreenSpinner->GetValue(), m_pcBlueSpinner->GetValue(), 255 ) );
      break;
    default:
      View::HandleMessage( pcMessage );
      break;
  }
}

void ColorEdit::SetValue( const Color32_s& sColor )
{
  m_sColor = sColor;

  m_pcRedSpinner->SetValue( float( sColor.red ) );
  m_pcGreenSpinner->SetValue( float( sColor.green ) );
  m_pcBlueSpinner->SetValue( float( sColor.blue ) );
  
  Color32_s sOldCol = GetEraseColor();
  SetEraseColor( m_sColor );
  DrawFrame( m_cTestRect, FRAME_RECESSED );
  SetEraseColor( sOldCol );
}

const Color32_s& ColorEdit::GetValue() const
{
  return( m_sColor );
}

void ColorEdit::AllAttached()
{
  m_pcRedSpinner->SetTarget( this );
  m_pcGreenSpinner->SetTarget( this );
  m_pcBlueSpinner->SetTarget( this );
}

Point ColorEdit::GetPreferredSize( bool bLargest ) const
{
    Point cSize = m_pcRedSpinner->GetPreferredSize( false );
    cSize.x *= 3.0f;
    cSize.x += 8.0f;
    cSize.y *= 5.0f;
    return( cSize );
}

void ColorEdit::FrameSized( const Point& cDelta )
{
  Rect cBounds = GetBounds();

  Point cSpinnerSize = m_pcRedSpinner->GetPreferredSize(false);
  cSpinnerSize.x = 16 + m_pcRedSpinner->GetStringWidth( "8888" );

  Point cRStrSize = m_pcRedStr->GetPreferredSize(false);
  Point cGStrSize = m_pcGreenStr->GetPreferredSize(false);
  Point cBStrSize = m_pcBlueStr->GetPreferredSize(false);
  Point cStrSize = get_max_width( &cRStrSize, &cGStrSize, &cBStrSize, NULL );

  Rect cStrRect( 0, 0, cStrSize.x - 1, cStrSize.y - 1 );
  Rect cSpinnerRect( 0, 0, cSpinnerSize.x - 1, cSpinnerSize.y - 1 );

  if ( cStrSize.y < cSpinnerSize.y ) {
    cStrSize += Point( 0, (cSpinnerSize.y - cStrSize.y) / 2  );
  } else {
    cSpinnerSize += Point( 0, (cStrSize.y - cSpinnerSize.y) / 2 );
  }

  m_pcRedSpinner->SetFrame( cSpinnerRect + Point( 4, cStrSize.y + 8 ) );
  m_pcGreenSpinner->SetFrame( cSpinnerRect + Point( (cBounds.Width()+1.0f) / 2 - cSpinnerSize.x / 2, cStrSize.y + 8 ) );
  m_pcBlueSpinner->SetFrame( cSpinnerRect + Point( (cBounds.Width()+1.0f) - cSpinnerSize.x - 4, cStrSize.y + 8 ) );
  
  m_pcRedStr->SetFrame( cStrRect + Point( 4, 4 ) );
  m_pcGreenStr->SetFrame( cStrRect + Point( (cBounds.Width()+1.0f) / 2 - cStrSize.x / 2, 4 ));
  m_pcBlueStr->SetFrame( cStrRect + Point( cBounds.Width() + 1.0f - cStrSize.x - 4, 4 ) );

  m_cTestRect = Rect( 10, cSpinnerRect.right + 14, cBounds.right - 10, cBounds.bottom - 10 );
}

void ColorEdit::Paint( const Rect& cUpdateRect )
{
  SetEraseColor( get_default_color( COL_NORMAL ) );
  DrawFrame( GetBounds(), FRAME_RECESSED );

  SetEraseColor( m_sColor );
  DrawFrame( m_cTestRect, FRAME_RECESSED );
}

