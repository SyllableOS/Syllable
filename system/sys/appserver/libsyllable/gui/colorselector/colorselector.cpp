#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <gui/tabview.h>
#include <gui/layoutview.h>
#include <util/message.h>

#include <math.h>

#include <gui/colorselector.h>

#include "colorwheel.h"
#include "gradientview.h"
#include "colorview.h"

using namespace os;

enum {
	MSG_COLORWHEEL = 100,
	MSG_REDSLIDER,
	MSG_GREENSLIDER,
	MSG_BLUESLIDER
};

class ColorSelector::Private
{
	public:
	Private() {
	}
	~Private() {
	}

	void SetColor( Color32 col ) {
		m_pcColorViewRGB->SetColor( col );
		m_pcGreenGradientSlider->SetValue( col.green / 255.0, false );
		m_pcRedGradientSlider->SetValue( col.red / 255.0, false );
		m_pcBlueGradientSlider->SetValue( col.blue / 255.0, false );
		m_pcColorViewHSV->SetColor( col );
	}

	void SetColorWheel( Color32 col ) {
		m_pcColorWheel->SetValue( col, false );
	}

	public:
	TabView*		m_pcTabView;

	/* HSV Tab */
	ColorWheel*		m_pcColorWheel;
	Slider*			m_pcGradientSlider;
	GradientView*	m_pcGradient;
	ColorView*		m_pcColorViewHSV;
	
	/* RGB Tab */
	Slider*			m_pcRedGradientSlider;
	GradientView*	m_pcRedGradient;
	Slider*			m_pcGreenGradientSlider;
	GradientView*	m_pcGreenGradient;
	Slider*			m_pcBlueGradientSlider;
	GradientView*	m_pcBlueGradient;
	ColorView*		m_pcColorViewRGB;
};

ColorSelector::ColorSelector( const Rect& cRect, const String& cName, Message* pcMsg, Color32_s cColor, uint32 nResizeMask, uint32 nFlags )
:Control( cRect, cName, cName, pcMsg, nResizeMask, nFlags )
{
	m = new Private;

	LayoutNode*	pcRootNode = new HLayoutNode( "" );	
	m->m_pcTabView = new TabView( GetBounds(), "m_pcTabView" );
	pcRootNode->AddChild( m->m_pcTabView );
	LayoutView*	pcRootView = new LayoutView( GetBounds(), "pcRootView" );
	pcRootView->SetRoot( pcRootNode );

	/* Palette Tab */
/*	LayoutNode*	pcPaletteRootNode = new HLayoutNode( "" );

	LayoutView*	pcPaletteRootView = new LayoutView( GetBounds(), "pcPaletteRootView" );
	pcPaletteRootView->SetRoot( pcPaletteRootNode );
	pcPaletteRootNode->SetBorders( Rect( 5, 5, 5, 5 ) );*/

	/* HSV Tab */
	LayoutNode*	pcHSVRootNode = new HLayoutNode( "" );

	m->m_pcColorWheel = new ColorWheel( GetBounds(), "m_pcColorWheel", new Message( MSG_COLORWHEEL ), 0x000000, CF_FOLLOW_ALL );
	m->m_pcColorWheel->SetForceCircular();
	pcHSVRootNode->AddChild( m->m_pcColorWheel );

	m->m_pcGradientSlider = new Slider( GetBounds(), "m_pcGradientSlider", NULL, Slider::TICKS_RIGHT, 10, Slider::KNOB_TRIANGLE, VERTICAL );
	m->m_pcGradientSlider->SetMinMax( 0, 1 );
	pcHSVRootNode->AddChild( m->m_pcGradientSlider );

	m->m_pcGradient = new GradientView( Rect(), "m_pcGradient" );
	pcHSVRootNode->AddChild( m->m_pcGradient );

	m->m_pcColorViewHSV = new ColorView( Rect(), "m_pcColorViewHSV", 0 );
	pcHSVRootNode->AddChild( m->m_pcColorViewHSV, 0.25f );
	pcHSVRootNode->SetBorders( Rect( 6, 6, 6, 6 ), "m_pcColorViewHSV", NULL );

	LayoutView*	pcHSVRootView = new LayoutView( GetBounds(), "pcRootView" );
	pcHSVRootView->SetRoot( pcHSVRootNode );
	pcHSVRootNode->SetBorders( Rect( 5, 5, 5, 5 ) );
	pcHSVRootNode->SetBorders( Rect( 1, 6, 1, 6 ), "m_pcGradient", NULL );

	/* RGB Tab */
	LayoutNode*	pcRGBRootNode = new HLayoutNode( "" );
	LayoutNode*	pcRGBSliderNode = new VLayoutNode( "" );

	m->m_pcRedGradientSlider = new Slider( GetBounds(), "m_pcRedGradientSlider", new Message( MSG_REDSLIDER ), Slider::TICKS_BELOW, 10, Slider::KNOB_TRIANGLE, HORIZONTAL );
	m->m_pcRedGradientSlider->SetMinMax( 0, 1 );
	pcRGBSliderNode->AddChild( m->m_pcRedGradientSlider );

	m->m_pcRedGradient = new GradientView( Rect(), "m_pcRedGradient", 0xFF000000, 0xFFFF0000, HORIZONTAL );
	pcRGBSliderNode->AddChild( m->m_pcRedGradient );

	m->m_pcGreenGradientSlider = new Slider( GetBounds(), "m_pcGreenGradientSlider", new Message( MSG_GREENSLIDER ), Slider::TICKS_BELOW, 10, Slider::KNOB_TRIANGLE, HORIZONTAL );
	m->m_pcGreenGradientSlider->SetMinMax( 0, 1 );
	pcRGBSliderNode->AddChild( m->m_pcGreenGradientSlider );

	m->m_pcGreenGradient = new GradientView( Rect(), "m_pcGreenGradient", 0xFF000000, 0xFF00FF00, HORIZONTAL );
	pcRGBSliderNode->AddChild( m->m_pcGreenGradient );

	m->m_pcBlueGradientSlider = new Slider( GetBounds(), "m_pcBlueGradientSlider", new Message( MSG_BLUESLIDER ), Slider::TICKS_BELOW, 10, Slider::KNOB_TRIANGLE, HORIZONTAL );
	m->m_pcBlueGradientSlider->SetMinMax( 0, 1 );
	pcRGBSliderNode->AddChild( m->m_pcBlueGradientSlider );

	m->m_pcBlueGradient = new GradientView( Rect(), "m_pcBlueGradient", 0xFF000000, 0xFF0000FF, HORIZONTAL );
	pcRGBSliderNode->AddChild( m->m_pcBlueGradient );

	pcRGBSliderNode->SetBorders( Rect( 6, 1, 6, 1 ), "m_pcRedGradient", NULL );
	pcRGBSliderNode->SetBorders( Rect( 6, 1, 6, 1 ), "m_pcGreenGradient", NULL );
	pcRGBSliderNode->SetBorders( Rect( 6, 1, 6, 1 ), "m_pcBlueGradient", NULL );
	pcRGBRootNode->AddChild( pcRGBSliderNode );

	m->m_pcColorViewRGB = new ColorView( Rect(), "m_pcColorViewRGB", 0 );
	pcRGBRootNode->AddChild( m->m_pcColorViewRGB, 0.25f );
	pcRGBRootNode->SetBorders( Rect( 6, 6, 6, 6 ), "m_pcColorViewRGB", NULL );

	LayoutView*	pcRGBRootView = new LayoutView( GetBounds(), "pcRGBRootView" );
	pcRGBRootView->SetRoot( pcRGBRootNode );
	pcRGBRootNode->SetBorders( Rect( 5, 5, 5, 5 ) );

	AddChild( pcRootView );

//	m->m_pcTabView->AppendTab( "Palette", pcPaletteRootView );
	m->m_pcTabView->AppendTab( "HSV", pcHSVRootView );
	m->m_pcTabView->AppendTab( "RGB", pcRGBRootView );
}

ColorSelector::~ColorSelector()
{
	delete m;
}

void ColorSelector::AttachedToWindow()
{
	Control::AttachedToWindow();

	m->m_pcColorWheel->SetBrightnessSlider( m->m_pcGradientSlider );
	m->m_pcColorWheel->SetGradientView( m->m_pcGradient );
	m->m_pcColorWheel->SetTarget( this );
	m->m_pcRedGradientSlider->SetTarget( this );
	m->m_pcGreenGradientSlider->SetTarget( this );
	m->m_pcBlueGradientSlider->SetTarget( this );
}

void ColorSelector::HandleMessage( Message *pcMsg )
{
	Color32_s	col = GetValue().AsColor32();

	switch( pcMsg->GetCode() ) {
		case MSG_COLORWHEEL:
			{
				col = m->m_pcColorWheel->GetValue().AsColor32();
				m->SetColor( col );
				Control::SetValue( col );
			}
			break;
		case MSG_REDSLIDER:
			{
				col.red = (uint8)( m->m_pcRedGradientSlider->GetValue().AsFloat() * 255 );
				m->SetColor( col );
				m->SetColorWheel( col );
				Control::SetValue( col );
			}
			break;
		case MSG_GREENSLIDER:
			{
				col.green = (uint8)( m->m_pcGreenGradientSlider->GetValue().AsFloat() * 255 );
				m->SetColor( col );
				m->SetColorWheel( col );
				Control::SetValue( col );
			}
			break;
		case MSG_BLUESLIDER:
			{
				col.blue = (uint8)( m->m_pcBlueGradientSlider->GetValue().AsFloat() * 255 );
				m->SetColor( col );
				m->SetColorWheel( col );
				Control::SetValue( col );
			}
			break;
		default:
			Control::HandleMessage( pcMsg );
	}
}

void ColorSelector::SetValue( Variant cNewValue, bool bInvoke )
{
	m->SetColor( cNewValue.AsColor32() );
	m->SetColorWheel( cNewValue.AsColor32() );
	Control::SetValue( cNewValue, bInvoke );
}

void ColorSelector::PostValueChange( const Variant& cNewValue )
{
}

void ColorSelector::EnableStatusChanged( bool bEnabled )
{
}

/***************************************************************************************************/

void ColorSelector::_reserved1() {}
void ColorSelector::_reserved2() {}
void ColorSelector::_reserved3() {}
void ColorSelector::_reserved4() {}
void ColorSelector::_reserved5() {}


