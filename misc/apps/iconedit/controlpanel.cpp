#include "controlpanel.h"
#include "controlid.h"
#include "bitmapscale.h"

#include <gui/window.h>
#include <gui/radiobutton.h>

#include <util/message.h>


ControlPanel::ControlPanel( const Rect &cFrame ) : View( cFrame, "control_panel", CF_FOLLOW_ALL )
{
  Rect cRadioFrame( 8, 8, 80, 23 );

  m_nFilter = filter_filter;
  
  m_apcFilterButtons[0] = new RadioButton( cRadioFrame, "r1", "Default", new Message( ID_FILTER_DEFAULT ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;
  m_apcFilterButtons[1] = new RadioButton( cRadioFrame, "r2", "Box", new Message( ID_FILTER_BOX ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;
  m_apcFilterButtons[2] = new RadioButton( cRadioFrame, "r3", "Triangle", new Message( ID_FILTER_TRIANGLE ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;
  m_apcFilterButtons[3] = new RadioButton( cRadioFrame, "r4", "Bell", new Message( ID_FILTER_BELL ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;
  m_apcFilterButtons[4] = new RadioButton( cRadioFrame, "r5", "BSpline", new Message( ID_FILTER_BSPLINE ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;
  m_apcFilterButtons[5] = new RadioButton( cRadioFrame, "r6", "Lanczos3", new Message( ID_FILTER_LANCZOS3 ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;
  m_apcFilterButtons[6] = new RadioButton( cRadioFrame, "r7", "Mitchell", new Message( ID_FILTER_MITCHELL ) );
  cRadioFrame.top += 16; cRadioFrame.bottom += 16;

  m_apcFilterButtons[0]->SetValue( true );
  for ( int i = 0 ; i < NUM_FILTERS ; ++i )
  {
    AddChild( m_apcFilterButtons[i] );
  }
}

void ControlPanel::AttachedToWindow()
{
  for ( int i = 0 ; i < NUM_FILTERS ; ++i ) {
    m_apcFilterButtons[i]->SetTarget( this );
  }
}

void ControlPanel::HandleMessage( Message* pcMessage )
{
  int32 nValue;
  pcMessage->FindInt32( "value", &nValue );

  bitmapscale_filtertype nOldFilter = m_nFilter;
  
  switch( pcMessage->GetCode() )
  {
    case ID_FILTER_DEFAULT:	if ( nValue ) m_nFilter = filter_filter;	break;
    case ID_FILTER_BOX:		if ( nValue ) m_nFilter = filter_box;		break;
    case ID_FILTER_TRIANGLE:	if ( nValue ) m_nFilter = filter_triangle;	break;
    case ID_FILTER_BELL:	if ( nValue ) m_nFilter = filter_bell;		break;
    case ID_FILTER_BSPLINE:	if ( nValue ) m_nFilter = filter_bspline;	break;
    case ID_FILTER_LANCZOS3:	if ( nValue ) m_nFilter = filter_lanczos3;	break;
    case ID_FILTER_MITCHELL:	if ( nValue ) m_nFilter = filter_mitchell;	break;
    default:
      View::HandleMessage( pcMessage );
  }

  if ( m_nFilter != nOldFilter ) {
    Window* pcWnd = GetWindow();
    if ( pcWnd != NULL ) {
      pcWnd->PostMessage( ID_FILTER_CHANGED, pcWnd );
    }
  }

}
