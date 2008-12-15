// Mouse (C)opyright 2008 Jonas Jarvoll
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

#include <appserver/protocol.h>
#include <util/application.h>
#include <util/application.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/slider.h>
#include <gui/button.h>
#include <gui/checkbox.h>
#include <gui/image.h>
#include <util/resources.h>

#include "appwindow.h"
#include "resources/mouse.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
//
// P R I V A T E
//
///////////////////////////////////////////////////////////////////////////////

class AppWindow :: _Private
{
public:
	_Private()
	{
	}

	void _SetMouseConfig( float speed, float accel, int32 doubleclick, bool swapbuttons )
	{
		Message cReq( DR_SET_MOUSE_CFG );

		cReq.AddFloat( "speed", speed );
		cReq.AddFloat( "acceleration", accel );
		cReq.AddInt32( "doubleclick", doubleclick );
		cReq.AddBool( "swapbuttons", swapbuttons );
		Messenger( Application::GetInstance()->GetServerPort() ).SendMessage( &cReq );
	}

	void _GetMouseConfig( float& speed, float& accel, int32& doubleclick, bool& swapbuttons )
	{
		Message cReq( DR_GET_MOUSE_CFG );
		Message cReply;

		Messenger( Application::GetInstance()->GetServerPort() ).SendMessage( &cReq, &cReply );

		cReply.FindFloat( "speed", &speed );
		cReply.FindFloat( "acceleration", &accel );
		cReply.FindInt32( "doubleclick", &doubleclick );
		cReply.FindBool( "swapbuttons", &swapbuttons );
	}

	void _SetSliders( float speed, float accel, int32 doubleclick, bool swapbuttons )
	{
		SpeedSlider->SetValue( speed / 3.0f );
		AccelSlider->SetValue( accel / 2.0f );
		ClickSlider->SetValue( ((float)doubleclick) / 1000000 );
		SwapCheckbox->SetValue( swapbuttons );
	}

	enum AppWindowMessage { MSG_APPLY, MSG_DEFAULT, MSG_UNDO };

	float MouseSpeed;
	float MouseAcceleration;
	int32 MouseDoubleclick;
	bool  MouseSwapButtons;

	Slider* SpeedSlider;
	Slider* AccelSlider;
	Slider* ClickSlider;
	CheckBox* SwapCheckbox;
};

///////////////////////////////////////////////////////////////////////////////
//
// T H E   C L A S S
//
///////////////////////////////////////////////////////////////////////////////
AppWindow :: AppWindow( const Rect& cFrame ) : Window( cFrame, "main_window", ID_MSG_TITLE )
{	
	// Create the private class
	m = new _Private();
	
	/* Initialise the parameters */
	m->MouseSpeed = 1.0f;
	m->MouseAcceleration = 0.0f;
	m->MouseDoubleclick = 500000;
	m->MouseSwapButtons = false;
	
	

	// Set an application icon for Dock
	BitmapImage* pcImage = new BitmapImage();
	Resources cRes( get_image_id() );
	pcImage->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon( pcImage->LockBitmap() );

	// Create root view
	LayoutView *root = new LayoutView( GetBounds(), "mouse_prefs_root" );
	AddChild( root );

	// Create vertical node
	VLayoutNode* layout = new os::VLayoutNode( "mouse_prefs_vlayout" );
	layout->SetBorders( os::Rect( 10, 5, 10, 5 ) );

	// Create speed settings
	FrameView* speed_frame = new FrameView( Rect(), "", ID_MSG_MOUSE_SPEED );
	VLayoutNode* frame_layout = new os::VLayoutNode( "mouse_prefs_vlayout" );
	frame_layout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	speed_frame->SetRoot( frame_layout );
	layout->AddChild( speed_frame, 1.0f );

	m->SpeedSlider = new Slider( Rect(), "speed_slider", NULL );
	m->SpeedSlider->SetMinMax( 0.1f, 1.0f );
	m->SpeedSlider->SetTabOrder();
	frame_layout->AddChild( m->SpeedSlider, 1.0f);

	// Create acceleration settings
	FrameView* accel_frame = new FrameView( Rect(), "", ID_MSG_MOUSE_ACCELERATION );
	VLayoutNode* frame_accel_layout = new os::VLayoutNode( "mouse_prefs_vlayout" );
	frame_accel_layout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	accel_frame->SetRoot( frame_accel_layout );
	layout->AddChild( accel_frame, 1.0f );

	m->AccelSlider = new Slider( Rect(), "accel_slider", NULL );
	m->AccelSlider->SetTabOrder();
	frame_accel_layout->AddChild( m->AccelSlider, 1.0f);

	// Create double click settings
	FrameView* click_frame = new FrameView( Rect(), "", ID_MSG_DOUBLECLICK );
	VLayoutNode* frame_click_layout = new os::VLayoutNode( "mouse_prefs_vlayout" );
	frame_click_layout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	click_frame->SetRoot( frame_click_layout );
	layout->AddChild( click_frame, 1.0f );

	m->ClickSlider = new Slider( Rect(), "click_slider", NULL );
	m->ClickSlider->SetTabOrder();
	frame_click_layout->AddChild( m->ClickSlider, 1.0f);

	/* Create checkbox */
	HLayoutNode* checkbox_layout = new os::HLayoutNode( "mouse_prefs_checkbox" );
	checkbox_layout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	layout->AddChild( checkbox_layout );

	m->SwapCheckbox = new CheckBox( os::Rect(), "swap_buttons_checkbox", ID_MSG_SWAPBUTTONS, NULL );
	m->SwapCheckbox->SetEnable( true );
	m->SwapCheckbox->SetTabOrder();
	checkbox_layout->AddChild( m->SwapCheckbox, 1.0f );

	// Create buttons
	HLayoutNode* button_layout = new os::HLayoutNode( "mouse_prefs_buttons" );
	button_layout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	layout->AddChild( button_layout );
	
	Button* pcApply;
	Button* pcUndo;
	Button* pcDefault;
	button_layout->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f, NULL, 1.0f ) );
	pcApply = new Button( os::Rect(), "", ID_MSG_APPLY, new Message( _Private::MSG_APPLY ) );
	pcApply->SetTabOrder();
	button_layout->AddChild( pcApply, 1.0f );
	button_layout->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f, NULL, 1.0f ) );
	pcUndo = new Button( os::Rect(), "", ID_MSG_UNDO, new Message( _Private::MSG_UNDO ) );
	pcUndo->SetTabOrder();
	button_layout->AddChild( pcUndo, 1.0f );
	button_layout->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f, NULL, 1.0f ) );
	pcDefault = new Button( os::Rect(), "", ID_MSG_DEFAULT, new Message( _Private::MSG_DEFAULT ) );
	pcDefault->SetTabOrder();
	button_layout->AddChild( pcDefault, 1.0f );

	// Set up the layout root
	root->SetRoot( layout );
	root->InvalidateLayout();
	
	Point cSize = root->GetPreferredSize( false );
	if( cSize.x < 300 ) cSize.x = 300;
	ResizeTo( cSize );
	CenterInScreen();
	pcApply->MakeFocus();

	// Set up current mouse values and store them
	m->_GetMouseConfig( m->MouseSpeed, m->MouseAcceleration, m->MouseDoubleclick, m->MouseSwapButtons );
	m->_SetSliders( m->MouseSpeed, m->MouseAcceleration, m->MouseDoubleclick, m->MouseSwapButtons );
	
}

AppWindow :: ~AppWindow()
{
	delete m;
}

void AppWindow :: HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )	//Get the message code from the message
	{	
		case _Private::MSG_APPLY:
			m->_SetMouseConfig( m->SpeedSlider->GetValue().AsFloat() * 3.0f,  
							 	m->AccelSlider->GetValue().AsFloat() * 2.0f,
							 	(int32)(m->ClickSlider->GetValue().AsFloat() * 1000000.0f),
								m->SwapCheckbox->GetValue().AsBool());
			break;
		case _Private::MSG_UNDO:
			m->_SetSliders( m->MouseSpeed, m->MouseAcceleration, m->MouseDoubleclick, m->MouseSwapButtons );
			m->_SetMouseConfig( m->MouseSpeed, m->MouseAcceleration, m->MouseDoubleclick, m->MouseSwapButtons );
			break;
		case _Private::MSG_DEFAULT:
			m->_SetSliders( 1.0f, 0.0f, 500000, false );
			m->_SetMouseConfig( 1.0f, 0.0f, 500000, false );
			break;
		default:
		{
			Window::HandleMessage( pcMessage );
			break;
		}
	}
}

bool AppWindow :: OkToQuit( void )
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;	
}
