//-----------------------------------------------------------------------------
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
//-------------------------------------
#include <gui/application.h>
#include <gui/checkbox.h>
#include <gui/desktop.h>
#include <gui/dropdownmenu.h>
#include <gui/frameview.h>
#include <gui/layoutview.h>
#include <gui/message.h>
#include <gui/slider.h>
#include <gui/stringview.h>
#include <gui/tabview.h>
//-------------------------------------
#include "cpiacam.h"
//-------------------------------------
#include "fcwin.h"
#include "fcapp.h"
#include "bitmapview.h"
//-----------------------------------------------------------------------------

enum messages
{
    MSG_FRAMERATE = 'FRTE',
    MSG_ENABLE_MANUAL_EXPOSURE = 'MEXP',
    MSG_EXPOSURE = 'EXPO',
    MSG_GAIN = 'GAIN',
    MSG_COMPRESSION = 'CMPR',
    MSG_DECIMATE = 'DECM',
    MSG_AUTOTARGET = 'AUTA',
    MSG_AUTOQUALITY = 'AUQU',
    MSG_AUTOFRAMERATE = 'AUFR',
    MSG_MANUALYQUALITY = 'MAYQ',
    MSG_MANUALUVQUALITY = 'MAUV',
    MSG_BRIGHTNESS = 'CPBR',
    MSG_CONTRAST = 'CPCO',
    MSG_SATURATION = 'CPSA',
    MSG_ENABLE_MANUAL_COLORBALANCE = 'COLA',
    MSG_COLORBALANCE = 'COLB',
};

#define CAMP_FRAMERATE 			0x00000001
#define CAMP_EXPOSURE_MODE		0x00000002
#define CAMP_EXPOSURE			0x00000004 // and gain
#define CAMP_COMPRESSION_MODE 		0x00000008
#define CAMP_AUTO_COMPRESSION_PARMS	0x00000010
#define CAMP_MANUAL_COMPRESSION_PARMS	0x00000020
#define CAMP_COLOR_PARAMS		0x00000040
#define CAMP_COLORBALANCE_MODE		0x00000080
#define CAMP_COLORBALANCE		0x00000100
#define CAMP_ALL       			0xffffffff

//-----------------------------------------------------------------------------

FCWin::FCWin( os::Rect rect, const char *title ) :
	os::Window( rect, "FridgeCam", title )
{
    LoadConfig();

    SetCameraParms( CAMP_ALL );

    os::LayoutNode *n;

    fRootView = new os::LayoutView( GetBounds(), "", NULL );
    os::HLayoutNode *h = new os::HLayoutNode( "" );
    {
	fBitmapView = new BitmapView( os::Point(352,288), os::CF_FOLLOW_NONE );
	n = new os::LayoutNode( "", 1.0f, h, fBitmapView );

	os::TabView *fSettingsTabView = new os::TabView( os::Rect(0,0,0,0), "" );
	new os::LayoutNode( "", 1.0f, h, fSettingsTabView );

	os::LayoutView *config_normal = new os::LayoutView( os::Rect(0,0,0,0), "", NULL );
	os::VLayoutNode *v = new os::VLayoutNode( "", 1.0f );
	{
	    GuiSensorFPS( v );
	    new os::LayoutSpacer( "", 1000.0f, v );
	    GuiExposure( v );
	    new os::LayoutSpacer( "", 1000.0f, v );
	    GuiCompress( v );
	}
	config_normal->SetRoot( v );
	fSettingsTabView->AppendTab( "normal", config_normal );

	os::LayoutView *config_advanced = new os::LayoutView( os::Rect(0,0,0,0), "", NULL );
	v = new os::VLayoutNode( "", 1.0f );
	{
	    new os::LayoutSpacer( "", 500.0f, v );

	    GuiColorParams( v );
	    new os::LayoutSpacer( "", 1000.0f, v );

	    GuiColorBalance( v );
	    new os::LayoutSpacer( "", 500.0f, v );
	}
	config_advanced->SetRoot( v );
	fSettingsTabView->AppendTab( "advanced", config_advanced );
    }
    fRootView->SetRoot( h );
    AddChild( fRootView );
//    fRootView->Layout();

    os::Point minsize = fRootView->GetPreferredSize( false );
    os::Point maxsize = fRootView->GetPreferredSize( true );
    SetSizeLimits( minsize, maxsize );
    ResizeTo( minsize );

    os::IPoint desktopsize = os::Desktop().GetResolution();
    MoveTo( (desktopsize.x-minsize.x)/2.0f, (desktopsize.y-minsize.y)/2.0f+30 );

    
#if 0
    fRootView->SetChild( fBitmapView, 0, 0 );

    fControlView = new os::TableView( GetBounds(), "", NULL, 1, 1 );
    fControlView->SetCellBorders( 2, 2, 2, 2 );
    fRootView->SetChild( fControlView, 1, 0 );

    os::FrameView *fv = new os::FrameView( os::Rect(0,0,0,0), "", "Test" );
    fControlView->SetChild( fv, 0, 0 );
#endif
    Show();
}

void FCWin::GuiSensorFPS( os::LayoutNode *parent )
{
    os::HLayoutNode *h = new os::HLayoutNode( "", 1.0f, parent );
    {
	os::StringView *sv = new os::StringView(
	    os::Rect(0,0,0,0), "", "Sensor fps" );
	new os::LayoutNode( "", 1.0f, h, sv );

	os::DropdownMenu *x = new os::DropdownMenu( os::Rect(0,0,0,0), "" );
	x->SetSelectionMessage( new os::Message(MSG_FRAMERATE) );
	x->SetTarget( this );
	x->AppendItem( "30" );
	x->AppendItem( "25" );
	x->AppendItem( "15" );
	x->AppendItem( "12.5" );
	x->AppendItem( "7.5" );
	x->AppendItem( "6.25" );
	x->AppendItem( "3.75" );
	x->AppendItem( "3.125" );
	x->SetReadOnly( true );
	x->SetSelection( Framerate_Rate2Index(fFramerate), false );
	new os::LayoutNode( "", 1.0f, h, x );
    }
}

void FCWin::GuiExposure( os::LayoutNode *parent )
{
    fExposureCheckBox = new os::CheckBox(
	os::Rect(0,0,0,0),"","Manual exposure", new os::Message(MSG_ENABLE_MANUAL_EXPOSURE) );
    fExposureCheckBox->SetValue( fManualExposure );
    new os::LayoutNode( "", 1.0f, parent, fExposureCheckBox );

    fExposureSlider = new os::Slider(
	os::Rect(0,0,0,0), "", new os::Message(MSG_EXPOSURE), os::Slider::TICKS_BELOW, 11,
	os::Slider::KNOB_SQUARE, HORIZONTAL );
    fExposureSlider->SetMinMax( 0, 100 );
    fExposureSlider->SetProgStrFormat( "Exposure (%.1f%%)" );
//  fExposureSlider->SetValue( 0 );
    fExposureSlider->SetCurValue( fExposure, false );
    fExposureSlider->SetTarget( this );
    new os::LayoutNode( "", 1.0f, parent, fExposureSlider );

    fGainSlider = new os::Slider(
	os::Rect(0,0,0,0), "", new os::Message(MSG_GAIN), os::Slider::TICKS_BELOW, 4,
	os::Slider::KNOB_SQUARE, HORIZONTAL );
    fGainSlider->SetMinMax( 1, 4 );
    fGainSlider->SetStepCount( 4 );
    fGainSlider->SetProgStrFormat( "Gain (%.0fx)" );
    fGainSlider->SetCurValue( fGain, false );
    fGainSlider->SetTarget( this );
    new os::LayoutNode( "", 1.0f, parent, fGainSlider );
}

void FCWin::GuiCompress( os::LayoutNode *parent )
{
    os::LayoutNode *n;

    os::StringView *sv = new os::StringView(
	os::Rect(0,0,0,0), "", "Compression" );
    new os::LayoutNode( "", 1.0f, parent, sv );

    fCompressionTabView = new os::TabView(
	os::Rect(0,0,0,0), "test" );
    fCompressionTabView->SetMessage( new os::Message(MSG_COMPRESSION) );
    new os::LayoutNode( "", 1.0f, parent, fCompressionTabView );

    os::LayoutView *compress_none = new os::LayoutView( os::Rect(0,0,0,0), "", NULL );
    os::VLayoutNode *v = new os::VLayoutNode( "", 1.0f );
    {
	fCompNoneDecimateCheckBox = new os::CheckBox(
	    os::Rect(0,0,0,0),"","Decimate", new os::Message(MSG_DECIMATE) );
	fCompNoneDecimateCheckBox->SetValue( fNoneDecimate );
	new os::LayoutNode( "", 1.0f, v, fCompNoneDecimateCheckBox );
    }
    compress_none->SetRoot( v );
    fCompressionTabView->AppendTab( "none", compress_none );

    os::LayoutView *compress_auto = new os::LayoutView( os::Rect(0,0,0,0), "", NULL );
    v = new os::VLayoutNode( "", 1.0f );
    v->SetHAlignment( ALIGN_LEFT );
    {
	fCompAutoDecimateCheckBox = new os::CheckBox(
	    os::Rect(0,0,0,0),"decimate","Auto decimate", new os::Message(MSG_DECIMATE) );
	fCompAutoDecimateCheckBox->SetValue( fAutoDecimate );
	n = new os::LayoutNode( "", 1.0f, v, fCompAutoDecimateCheckBox );
//	n->SetHAlignment( ALIGN_LEFT );

	fCompAutoTargetCheckBox = new os::CheckBox(
	    os::Rect(0,0,0,0),"target","Target quality", new os::Message(MSG_AUTOTARGET) );
	fCompAutoTargetCheckBox->SetValue( fAutoTargetQuality );
	n = new os::LayoutNode( "", 1.0f, v, fCompAutoTargetCheckBox );
//	n->SetHAlignment( ALIGN_LEFT );

//	v->SameWidth( "decimate", "target" );
	
	fCompAutoQualitySlider = new os::Slider(
	    os::Rect(0,0,0,0), "", new os::Message(MSG_AUTOQUALITY),
	    os::Slider::TICKS_BELOW, 11, os::Slider::KNOB_SQUARE, HORIZONTAL );
	fCompAutoQualitySlider->SetMinMax( 0, 100 );
	fCompAutoQualitySlider->SetProgStrFormat( "Quality (%.1f)" );
	fCompAutoQualitySlider->SetCurValue( fAutoQuality, false );
	fCompAutoQualitySlider->SetTarget( this );
	new os::LayoutNode( "", 1.0f, v, fCompAutoQualitySlider );

	fCompAutoFramerateSlider = new os::Slider(
	    os::Rect(0,0,0,0), "", new os::Message(MSG_AUTOFRAMERATE),
	    os::Slider::TICKS_BELOW, 31, os::Slider::KNOB_SQUARE, HORIZONTAL );
	fCompAutoFramerateSlider->SetMinMax( 0, 30 );
	fCompAutoFramerateSlider->SetStepCount( 31 );
	fCompAutoFramerateSlider->SetProgStrFormat( "Framerate (%.0f)" );
	fCompAutoFramerateSlider->SetCurValue( fAutoFramerate, false );
	fCompAutoFramerateSlider->SetTarget( this );
	new os::LayoutNode( "", 1.0f, v, fCompAutoFramerateSlider );
    }
    compress_auto->SetRoot( v );
    fCompressionTabView->AppendTab( "auto", compress_auto );
    
    os::LayoutView *compress_manual = new os::LayoutView( os::Rect(0,0,0,0), "", NULL );
    v = new os::VLayoutNode( "", 1.0f );
    {
	fCompManualDecimateCheckBox = new os::CheckBox(
	    os::Rect(0,0,0,0),"","Decimate", new os::Message(MSG_DECIMATE) );
	fCompManualDecimateCheckBox->SetValue( fManualDecimate );
	new os::LayoutNode( "", 1.0f, v, fCompManualDecimateCheckBox );

	fCompManualYQualitySlider = new os::Slider(
	    os::Rect(0,0,0,0), "", new os::Message(MSG_MANUALYQUALITY),
	    os::Slider::TICKS_BELOW, 11, os::Slider::KNOB_SQUARE, HORIZONTAL );
	fCompManualYQualitySlider->SetMinMax( 0, 100 );
	fCompManualYQualitySlider->SetProgStrFormat( "Y Quality (%.1f)" );
	fCompManualYQualitySlider->SetCurValue( fManualYQuality, false );
	fCompManualYQualitySlider->SetTarget( this );
	new os::LayoutNode( "", 1.0f, v, fCompManualYQualitySlider );

	fCompManualUVQualitySlider = new os::Slider(
	    os::Rect(0,0,0,0), "", new os::Message(MSG_MANUALUVQUALITY),
	    os::Slider::TICKS_BELOW, 11, os::Slider::KNOB_SQUARE, HORIZONTAL );
	fCompManualUVQualitySlider->SetMinMax( 0, 100 );
	fCompManualUVQualitySlider->SetProgStrFormat( "UV Quality (%.1f)" );
	fCompManualUVQualitySlider->SetCurValue( fManualUVQuality, false );
	fCompManualUVQualitySlider->SetTarget( this );
	new os::LayoutNode( "", 1.0f, v, fCompManualUVQualitySlider );
    }
    compress_manual->SetRoot( v );
    fCompressionTabView->AppendTab( "manual", compress_manual );
    fCompressionTabView->SetSelection( fCompressionMode );
}

//-----------------------------------------------------------------------------

void FCWin::GuiColorParams( os::LayoutNode *parent )
{
    fBrightnessSlider = new os::Slider(
	os::Rect(0,0,0,0), "", new os::Message(MSG_BRIGHTNESS), os::Slider::TICKS_BELOW, 11,
	os::Slider::KNOB_SQUARE, HORIZONTAL );
    fBrightnessSlider->SetMinMax( 0, 100 );
    fBrightnessSlider->SetProgStrFormat( "Brightness (%.1f%%)" );
    fBrightnessSlider->SetCurValue( fBrightness, false );
    fBrightnessSlider->SetTarget( this );
    new os::LayoutNode( "", 1.0f, parent, fBrightnessSlider );

    fContrastSlider = new os::Slider(
	os::Rect(0,0,0,0), "", new os::Message(MSG_CONTRAST), os::Slider::TICKS_BELOW, 11,
	os::Slider::KNOB_SQUARE, HORIZONTAL );
    fContrastSlider->SetMinMax( 0, 100 );
    fContrastSlider->SetProgStrFormat( "Contrast (%.1f%%)" );
    fContrastSlider->SetCurValue( fContrast, false );
    fContrastSlider->SetTarget( this );
    new os::LayoutNode( "", 1.0f, parent, fContrastSlider );

    fSaturationSlider = new os::Slider(
	os::Rect(0,0,0,0), "", new os::Message(MSG_SATURATION), os::Slider::TICKS_BELOW, 11,
	os::Slider::KNOB_SQUARE, HORIZONTAL );
    fSaturationSlider->SetMinMax( 0, 100 );
    fSaturationSlider->SetProgStrFormat( "Saturation (%.1f%%)" );
    fSaturationSlider->SetCurValue( fSaturation, false );
    fSaturationSlider->SetTarget( this );
    new os::LayoutNode( "", 1.0f, parent, fSaturationSlider );
}

void FCWin::GuiColorBalance( os::LayoutNode *parent )
{
    fColorBalanceCheckBox = new os::CheckBox(
	    os::Rect(0,0,0,0),"","Manual balance", new os::Message(MSG_ENABLE_MANUAL_COLORBALANCE) );
    fColorBalanceCheckBox->SetValue( fManualColorBalance );
    new os::LayoutNode( "", 1.0f, parent, fColorBalanceCheckBox );

    static char *label[3] = {"Red (%.1f%%)","Green (%.1f%%)","Blue (%.1f%%)"};
    for( int i=0; i<3; i++ )
    {
	fColorBalanceSlider[i] = new os::Slider(
	    os::Rect(0,0,0,0), "", new os::Message(MSG_COLORBALANCE), os::Slider::TICKS_BELOW, 11,
	    os::Slider::KNOB_SQUARE, HORIZONTAL );
	fColorBalanceSlider[i]->SetMinMax( 0, 100 );
	fColorBalanceSlider[i]->SetProgStrFormat( label[i] );
	fColorBalanceSlider[i]->SetCurValue( fColorbalance[i], false );
	fColorBalanceSlider[i]->SetTarget( this );
	new os::LayoutNode( "", 1.0f, parent, fColorBalanceSlider[i] );
    }
}

//-----------------------------------------------------------------------------

void FCWin::LoadConfig()
{
    os::Message m;

    char *home = getenv( "HOME" );
    if( home )
    {
	char configfile[PATH_MAX];
	sprintf( configfile, "%s/config/fridgecam.cfg", home );
	int hf = open( configfile, O_RDONLY );
	if( hf >= 0 )
	{
	    struct stat s;
	    fstat( hf, &s );
	    void *buffer = malloc( s.st_size );
	    if( read(hf, buffer, s.st_size) == s.st_size )
		m.Unflatten( (uint8*)buffer ); // FIXME: Unflatten should take void*
	    free( buffer );
	    close( hf );
	}
    }

    float f;
    bool b;
    int32 i;
    fFramerate = m.FindFloat("framerate",&f)==0 ? f : 30.0f;
    printf( "%f\n", fFramerate );
    fManualExposure = m.FindBool("exp_manual",&b)==0 ? b : false;
    fExposure = m.FindFloat("exp_exposure",&f)==0 ? f : 50.0f;
    fGain = m.FindInt32("exp_gain",&i)==0 ? i : 1;

    fCompressionMode = m.FindInt32("comp_mode",&i)==0 ? i : 1;
    fNoneDecimate = m.FindBool("comp_none_decomate",&b)==0 ? b : false;
    fAutoDecimate = m.FindBool("comp_auto_decomate",&b)==0 ? b : false;
    fAutoTargetQuality = m.FindBool("comp_auto_targetquality",&b)==0 ? b : false;
    fAutoQuality = m.FindFloat("comp_auto_quality",&f)==0 ? f : 100.0f;
    fAutoFramerate = m.FindInt32("comp_auto_framerate",&i)==0 ? i : 15;

    fManualDecimate = m.FindBool("comp_man_decomate",&b)==0 ? b : false;
    fManualYQuality = m.FindFloat("comp_man_y",&f)==0 ? f : 20.0f;
    fManualUVQuality = m.FindFloat("comp_man_uv",&f)==0 ? f : 20.0f;

    fBrightness = m.FindFloat( "color_brightness",&f)==0 ? f : 50.0f;
    fContrast = m.FindFloat( "color_contrast",&f)==0 ? f : 50.0f;
    fSaturation = m.FindFloat( "color_saturation",&f)==0 ? f : 50.0f;

    fManualColorBalance = m.FindBool( "colorbalance_manual",&b)==0 ? b : false;
    fColorbalance[0] = m.FindFloat( "colorbalance_red",&f)==0 ? f : 50.0f;
    fColorbalance[1] = m.FindFloat( "colorbalance_green",&f)==0 ? f : 50.0f;
    fColorbalance[2] = m.FindFloat( "colorbalance_blue",&f)==0 ? f : 50.0f;
}

void FCWin::SaveConfig() const
{
    os::Message m;

    m.AddFloat( "framerate", fFramerate );
    m.AddBool( "exp_manual", fManualExposure );
    m.AddFloat( "exp_exposure", fExposure );
    m.AddInt32( "exp_gain", fGain );

    m.AddInt32( "comp_mode", fCompressionMode );
    m.AddBool( "comp_none_decomate", fNoneDecimate );
    m.AddBool( "comp_auto_decomate", fAutoDecimate );
    m.AddBool( "comp_auto_targetquality", fAutoTargetQuality );
    m.AddFloat( "comp_auto_quality", fAutoQuality );
    m.AddInt32( "comp_auto_framerate", fAutoFramerate );

    m.AddBool( "comp_man_decomate", fManualDecimate );
    m.AddFloat( "comp_man_y", fManualYQuality );
    m.AddFloat( "comp_man_uv", fManualUVQuality );

    m.AddFloat( "color_brightness", fBrightness );
    m.AddFloat( "color_contrast", fContrast );
    m.AddFloat( "color_saturation", fSaturation );

    m.AddBool( "colorbalance_manual", fManualColorBalance );
    m.AddFloat( "colorbalance_red", fColorbalance[0] );
    m.AddFloat( "colorbalance_green", fColorbalance[1] );
    m.AddFloat( "colorbalance_blue", fColorbalance[2] );
    
    char *home = getenv( "HOME" );
    if( home )
    {
	char configfile[PATH_MAX];
	sprintf( configfile, "%s/config/fridgecam.cfg", home );
	int hf = open( configfile, O_WRONLY|O_CREAT|O_TRUNC );
	size_t flattensize = m.GetFlattenedSize();
	void *buffer = malloc( flattensize );
	m.Flatten( (uint8*)buffer, flattensize ); // FIXME: should take void*
	write( hf, buffer, flattensize );
	free( buffer );
	close( hf );
    }
}

//-----------------------------------------------------------------------------


bool FCWin::OkToQuit()
{
    os::Application::GetInstance()->PostMessage( M_QUIT );
    return false;
}

void FCWin::HandleMessage( os::Message *message )
{
    int32 tmp32;

      // a lot of view's set this, so extract it here...
    bool final = false;
    message->FindBool( "final" , &final );

    int v = message->GetCode();
    
    switch( v )
    {
	case MSG_FRAMERATE:
	    if( !final ) break;

	    if( message->FindInt32("selection",&tmp32) >= 0 )
	    {
		fFramerate = Framerate_Index2Rate( tmp32 );
		SetCameraParms( CAMP_FRAMERATE );
	    }
	    break;

	case MSG_ENABLE_MANUAL_EXPOSURE:
	    fManualExposure = fExposureCheckBox->GetValue();
	    SetCameraParms( CAMP_EXPOSURE_MODE );
	    break;

	case MSG_EXPOSURE:
	case MSG_GAIN:
	    if( !fManualExposure )
	    {
		fManualExposure = true;
		fExposureCheckBox->SetValue( fManualExposure );
		SetCameraParms( CAMP_EXPOSURE_MODE );
	    }
	    fExposure = fExposureSlider->GetCurValue();
	    fGain = int(rint(fGainSlider->GetCurValue()));
	    SetCameraParms( CAMP_EXPOSURE );
	    break;

	case MSG_COMPRESSION:
	case MSG_DECIMATE:
	    fCompressionMode = fCompressionTabView->GetSelection();
	    fNoneDecimate = fCompNoneDecimateCheckBox->GetValue();
	    fAutoDecimate = fCompAutoDecimateCheckBox->GetValue();
	    fManualDecimate = fCompManualDecimateCheckBox->GetValue();
	    SetCameraParms( CAMP_COMPRESSION_MODE );
	    break;

	case MSG_AUTOTARGET:
	case MSG_AUTOQUALITY:
	case MSG_AUTOFRAMERATE:
	    fAutoTargetQuality = fCompAutoTargetCheckBox->GetValue();
	    fAutoFramerate = fCompAutoFramerateSlider->GetCurValue();
	    fAutoQuality = fCompAutoQualitySlider->GetCurValue();
	    SetCameraParms( CAMP_AUTO_COMPRESSION_PARMS );
	    break;

	case MSG_MANUALYQUALITY:
	case MSG_MANUALUVQUALITY:
	    fManualYQuality = fCompManualYQualitySlider->GetCurValue();
	    fManualUVQuality = fCompManualUVQualitySlider->GetCurValue();
	    SetCameraParms( CAMP_MANUAL_COMPRESSION_PARMS );
	    break;

	case MSG_BRIGHTNESS:
	case MSG_CONTRAST:
	case MSG_SATURATION:
	    fBrightness = fBrightnessSlider->GetCurValue();
	    fContrast = fContrastSlider->GetCurValue();
	    fSaturation = fSaturationSlider->GetCurValue();
	    SetCameraParms( CAMP_COLOR_PARAMS );
	    break;

	case MSG_ENABLE_MANUAL_COLORBALANCE:
	    fManualColorBalance = fColorBalanceCheckBox->GetValue();
	    SetCameraParms( CAMP_COLORBALANCE_MODE );
	    break;

	case MSG_COLORBALANCE:
	    if( !fManualColorBalance )
	    {
		fManualColorBalance = true;
		fColorBalanceCheckBox->SetValue( fManualColorBalance );
		SetCameraParms( CAMP_COLORBALANCE_MODE );
	    }
	    for( int i=0; i<3; i++ )
		fColorbalance[i] = fColorBalanceSlider[i]->GetCurValue();
	    SetCameraParms( CAMP_COLORBALANCE );
	    break;
	    
	default:
	    printf( "MESSAGE: %c%c%c%c\n", v>>24, v>>16, v>>8, v );
	    os::Window::HandleMessage( message );
    }
}

void FCWin::DrawBitmap( const os::Bitmap *bitmap )
{
    fBitmapView->DrawNewBitmap( bitmap );
}

void FCWin::SetExposure( float exposure, int gain )
{
    if( !fManualExposure )
    {
	fExposure = exposure*100.0f;
	fGain = gain+1;
	fExposureSlider->SetCurValue( fExposure, false );
	fGainSlider->SetCurValue( fGain, false );
    }
}

void FCWin::SetColorBalance( float red, float green, float blue )
{
    if( !fManualColorBalance )
    {
	fColorbalance[0] = red*100.0f;
	fColorbalance[1] = green*100.0f;
	fColorbalance[2] = blue*100.0f;
	for( int i=0; i<3; i++ )
	    fColorBalanceSlider[i]->SetCurValue( fColorbalance[i], false );
    }
}

//-----------------------------------------------------------------------------

int FCWin::Framerate_Rate2Index( float rate )
{
    if( rate==30.0f ) return 0;
    else if( rate==25.0f ) return 1;
    else if( rate==30.0f/2.0f ) return 2;
    else if( rate==25.0f/2.0f ) return 3;
    else if( rate==30.0f/4.0f ) return 4;
    else if( rate==25.0f/4.0f ) return 5;
    else if( rate==30.0f/8.0f ) return 6;
    else if( rate==25.0f/8.0f ) return 7;
    assert( !"tresspasser!" );
    return 0;
}

float FCWin::Framerate_Index2Rate( int index )
{
    if( index==0 ) return 30.0f;
    else if( index==1 ) return 25.0f;
    else if( index==2 ) return 30.0f/2.0f;
    else if( index==3 ) return 25.0f/2.0f;
    else if( index==4 ) return 30.0f/4.0f;
    else if( index==5 ) return 25.0f/4.0f;
    else if( index==6 ) return 30.0f/8.0f;
    else if( index==7 ) return 25.0f/8.0f;
    assert( !"tresspasser!" );
    return 30.0f;
}

//-----------------------------------------------------------------------------

void FCWin::SetCameraParms( uint32 flags )
{
    FCApp *app = (FCApp*)os::Application::GetInstance();

    os::Message msg( FCApp::MSG_SETCAMERAPARMS );

    if( flags & CAMP_FRAMERATE )
	msg.AddInt32( "framerate", fFramerate );

    if( flags & CAMP_EXPOSURE_MODE )
	msg.AddBool( "exp:auto", !fManualExposure );
    if( ((flags&CAMP_EXPOSURE_MODE) && fManualExposure) || (flags&CAMP_EXPOSURE) )
    {
	msg.AddFloat( "exp:exposure", fExposure/100.0f );
	msg.AddInt32( "exp:gain", fGain-1 );
    }

    if( flags & CAMP_COMPRESSION_MODE )
    {
	msg.AddInt32( "comp:mode", 
		      fCompressionMode==0 ? CPiACam::COMPRESSION_DISABLED :
		      fCompressionMode==1 ? CPiACam::COMPRESSION_AUTO :
		      CPiACam::COMPRESSION_MANUAL );
	msg.AddInt32( "comp:decimation",
		      fCompressionMode==0 ? fNoneDecimate :
		      fCompressionMode==1 ? fAutoDecimate :
		      fManualDecimate );
    }

    if( flags & CAMP_AUTO_COMPRESSION_PARMS )
    {
	msg.AddBool( "comp:auto:targetquality", fAutoTargetQuality );
	msg.AddFloat( "comp:auto:quality", fAutoQuality/100.0f );
	msg.AddInt32( "comp:auto:framerate", fAutoFramerate );
    }

    if( flags & CAMP_MANUAL_COMPRESSION_PARMS )
    {
	msg.AddFloat( "comp:manual:y", fManualYQuality/100.0f );
	msg.AddFloat( "comp:manual:uv", fManualUVQuality/100.0f );
    }

    if( flags & CAMP_COLOR_PARAMS )
    {
	msg.AddFloat( "color:brightness", fBrightness/100.0f );
	msg.AddFloat( "color:contrast", fContrast/100.0f );
	msg.AddFloat( "color:saturation", fSaturation/100.0f );
    }

    if( flags & CAMP_COLORBALANCE_MODE )
    {
	msg.AddBool( "colorbalance:auto", !fManualColorBalance );
    }
    
    if( ((flags&CAMP_COLORBALANCE_MODE) && fManualColorBalance) || (flags&CAMP_COLORBALANCE) )
    {
	msg.AddFloat( "colorbalance:red", fColorbalance[0]/100.0f );
	msg.AddFloat( "colorbalance:green", fColorbalance[1]/100.0f );
	msg.AddFloat( "colorbalance:blue", fColorbalance[2]/100.0f );
    }

    
    SaveConfig();
    
    app->PostMessage( &msg );
}

