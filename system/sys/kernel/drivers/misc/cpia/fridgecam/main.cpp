// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
//#include <assert.h>
//#include <stdio.h>
//#include <memory.h>
//#include <unistd.h>
//-------------------------------------
//#include <atheos/threads.h>
//#include <atheos/time.h>
//#include <gui/checkbox.h>
//#include <gui/bitmap.h>
//#include <gui/dropdownmenu.h>
//#include <gui/message.h>
//#include <gui/spinner.h>
//#include <gui/tableview.h>
//#include <gui/window.h>
//-------------------------------------
#include "fcapp.h"
//#include "fcwin.h"
//#include "bitmapview.h"
//-----------------------------------------------------------------------------

#if 0
class ExposureWindow : public os::Window
{
public:
    ExposureWindow( os::IPoint pos, const char *title );

//    void MessageReceived( os::Message *message );
    void DispatchMessage( os::Message* pcMessage, os::Handler* pcHandler );
private:
    void UpdateFromCamera();

    os::TableView	*fRootView;
    os::CheckBox	*fAutoExposure;
    os::DropdownMenu	*fFramerate;
    os::Spinner		*fExposure;
    os::Spinner		*fGain;
};
#endif

//-------------------------------------


//-------------------------------------


//-------------------------------------


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

#if 0
#define EXPWIN_WIDTH 128
#define EXPWIN_HEIGHT 64

ExposureWindow::ExposureWindow( os::IPoint pos, const char *title ) :
	os::Window( os::Rect(pos.x,pos.y,pos.x+EXPWIN_WIDTH,pos.y+EXPWIN_HEIGHT), "ExposureWindow", title,
		      /*os::WND_NO_CLOSE_BUT|*/os::WND_NO_ZOOM_BUT/*|os::WND_NOT_RESIZABLE*/ )
{
    fRootView = new os::TableView( GetBounds(), "root", NULL, 2, 2, os::CF_FOLLOW_ALL );
    fRootView->SetCellBorders( 2, 2, 2, 2 );

    fAutoExposure = new os::CheckBox( os::Rect(0,0,0,0), "autoexp", "Auto exposure", new os::Message('AEXP') );
    fAutoExposure->SetValue( 1 );
    fRootView->SetChild( fAutoExposure, 0, 0 );

    fFramerate = new os::DropdownMenu( os::Rect(0,0,0,0), "Framerate", true, os::CF_FOLLOW_NONE );
    fFramerate->SetTarget( NULL, this );
    fFramerate->SetSelectionMessage( new os::Message('FRMR') );
    fFramerate->SetReadOnly();
    fFramerate->AppendItem( "30 fps" );
    fFramerate->AppendItem( "15 fps" );
    fFramerate->AppendItem( "7.5 fps" );
    fFramerate->AppendItem( "3.75 fps" );
    fFramerate->AppendItem( "25 fps" );
    fFramerate->AppendItem( "12.5 fps" );
    fFramerate->AppendItem( "6.25 fps" );
    fFramerate->AppendItem( "3.125 fps" );
    fFramerate->SetSelection( 0, true );
    fRootView->SetChild( fFramerate, 1, 0 );

    fExposure = new os::Spinner( os::Rect(0,0,0,0), "exposure", 0.0f, new os::Message('EXPO') );
    fExposure->SetTarget( NULL, this );
    fExposure->SetMinMax( 0.0f, 100.0f );
    fExposure->SetStep( 0.01f );
    fExposure->SetScale( 1.0f );
    fExposure->SetFormat( "%.2f" );
    fRootView->SetChild( fExposure, 0, 1 );

    fGain = new os::Spinner( os::Rect(0,0,0,0), "gain", 0.0f, new os::Message('GAIN') );
    fGain->SetTarget( NULL, this );
    fGain->SetMinMax( 0.0f, 3.0f );
    fGain->SetStep( 1.0f );
    fGain->SetScale( 1.0f );
    fGain->SetFormat( "%.0f" );
    fRootView->SetChild( fGain, 1, 1 );

    UpdateFromCamera();
	
//    new BMessageRunner( BMessenger(this), new BMessage('TCK_'), 1000000/2 );

    AddChild( fRootView );
    Show();
}

//void ExposureWindow::MessageReceived( os::Message *message )
void ExposureWindow::DispatchMessage( os::Message *message, os::Handler *handler )
{
    switch( message->GetCode() )
    {
	case 'AEXP':
	{
	    gCam->Lock();
	    bool autoexp = fAutoExposure->GetValue();
//	    fExposure->SetEnable( !autoexp );
//	    fGain->SetEnable( !autoexp );
	    gCam->EnableAutoExposure( autoexp );
	    gCam->Unlock();
	}
	break;
			
	case 'EXPO':
	case 'GAIN':
	    printf( "%f %f\n", fExposure->GetValue(), fGain->GetValue() );
	    gCam->Lock();
	    gCam->SetExposure( float(fExposure->GetValue())/100.0f, fGain->GetValue() );
	    gCam->Unlock();
	    break;

	case 'FRMR':
	{
	    bool final = false;
	    if( message->FindBool("final",&final)<0 || final==false )
		break;

	    gCam->Lock();
	    int32 selection;
	    if( message->FindInt32("selection",&selection) >= 0 )
	    {
		switch( selection )
		{
		    case 0: gCam->SetFrameRate( CPiACam::FRATE_30, CPiACam::FDIV_1 ); break;
		    case 1: gCam->SetFrameRate( CPiACam::FRATE_30, CPiACam::FDIV_2 ); break;
		    case 2: gCam->SetFrameRate( CPiACam::FRATE_30, CPiACam::FDIV_4 ); break;
		    case 3: gCam->SetFrameRate( CPiACam::FRATE_30, CPiACam::FDIV_8 ); break;
		    case 4: gCam->SetFrameRate( CPiACam::FRATE_25, CPiACam::FDIV_1 ); break;
		    case 5: gCam->SetFrameRate( CPiACam::FRATE_25, CPiACam::FDIV_2 ); break;
		    case 6: gCam->SetFrameRate( CPiACam::FRATE_25, CPiACam::FDIV_4 ); break;
		    case 7: gCam->SetFrameRate( CPiACam::FRATE_25, CPiACam::FDIV_8 ); break;
		}
	    }
	    gCam->Unlock();
	}
	break;

	case 'TCK_':
	    UpdateFromCamera();

//	    while( (message=MessageQueue()->FindMessage('TCK_',0)) != NULL )
//	      	MessageQueue()->RemoveMessage( message );
	    break;

	default:
	    os::Window::DispatchMessage( message, handler );
    }
}

void ExposureWindow::UpdateFromCamera()
{
    if( fAutoExposure->GetValue() )
    {
	float exp;
	int gain;
			
	gCam->Lock();
	gCam->GetExposure( &exp, &gain );
	gCam->Unlock();

	  //		printf( "%f %d\n", exp, gain );

	fExposure->SetValue( exp*1000000.0f );
	fGain->SetValue( gain );
    }
}
#endif

//-----------------------------------------------------------------------------

int main()
{
    FCApp app;
    app.Run();

    return 0;
}
	
//-----------------------------------------------------------------------------

